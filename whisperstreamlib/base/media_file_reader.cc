#include <whisperlib/common/base/strutil.h>
#include <whisperstreamlib/base/media_file_reader.h>

#include <whisperstreamlib/flv/flv_tag_splitter.h>
#include <whisperstreamlib/f4v/f4v_tag_splitter.h>
#include <whisperstreamlib/mp3/mp3_tag_splitter.h>
#include <whisperstreamlib/aac/aac_tag_splitter.h>
#include <whisperstreamlib/raw/raw_tag_splitter.h>
#include <whisperstreamlib/internal/internal_tag_splitter.h>

namespace streaming {

MediaFileReader::MediaFileReader()
  : buf_(),
    file_(),
    file_size_(-1),
    file_eof_reached_(false),
    splitter_(NULL) {
}
MediaFileReader::~MediaFileReader() {
  delete splitter_;
  splitter_ = NULL;
}

TagSplitter* MediaFileReader::splitter() {
  return splitter_;
}

bool MediaFileReader::Open(const string& filename, TagSplitter::Type ts_type) {
  CHECK_NULL(splitter_);
  if ( !file_.Open(filename.c_str(),
                   io::File::GENERIC_READ,
                   io::File::OPEN_EXISTING) ) {
    LOG_ERROR << "Failed to open file: [" << filename << "]"
                 ", error: " << GetLastSystemErrorDescription();
    return false;
  }
  file_size_ = io::GetFileSize(filename);
  if ( file_size_ == -1 ) {
    LOG_ERROR << "Failed to check file size, file: [" << filename << "]"
                 ", error: " << GetLastSystemErrorDescription();
    return false;
  }

  // maybe estabilish file type
  if ( ts_type == -1 ) {
    const string ext = strutil::Extension(filename);
    if ( ext == "flv" ) {
      ts_type = TagSplitter::TS_FLV;
    } else if ( ext == "f4v" || ext == "mp4" || ext == "mov" ||
                ext == "m4v" || ext == "m4a" ) {
      ts_type = TagSplitter::TS_F4V;
    } else if ( ext == "mp3" ) {
      ts_type = TagSplitter::TS_MP3;
    } else if ( ext == "aac" ) {
      ts_type = TagSplitter::TS_AAC;
    } else {
      LOG_ERROR << "Unknown file type: [" << filename << "]"
                   ", cannot find a suitable tag splitter.";
      return false;
    }
  }

  // create splitter
  switch ( ts_type ) {
    case TagSplitter::TS_FLV:
      LOG_INFO << "Creating a FlvTagSplitter for file: [" << filename << "]";
      splitter_ = new FlvTagSplitter("flv_tag_splitter");
      break;
    case TagSplitter::TS_F4V:
      LOG_INFO << "Creating a F4vTagSplitter for file: [" << filename << "]";
      splitter_ = new F4vTagSplitter("f4v_tag_splitter");
      break;
    case TagSplitter::TS_MP3:
      LOG_INFO << "Creating a Mp3TagSplitter for file: [" << filename << "]";
      splitter_ = new Mp3TagSplitter("mp3_tag_splitter");
      break;
    case TagSplitter::TS_AAC:
      LOG_INFO << "Creating an AacTagSplitter for file: [" << filename << "]";
      splitter_ = new AacTagSplitter("aac_tag_splitter");
      break;
    case TagSplitter::TS_RAW:
      LOG_INFO << "Creating a RawTagSplitter for file: [" << filename << "]";
      splitter_ = new RawTagSplitter("raw_tag_splitter", 1 << 20);
      break;
    case TagSplitter::TS_INTERNAL:
      LOG_INFO << "Creating an InternalTagSplitter for file: [" << filename << "]";
      splitter_ = new InternalTagSplitter("internal_tag_splitter");
      break;
  }

  return true;
}
TagReadStatus MediaFileReader::Read(scoped_ref<Tag>* out, int64* timestamp_ms) {
  while ( true ) {
    TagReadStatus result = splitter_->GetNextTag(
        &buf_, out, timestamp_ms, file_eof_reached_);
    //LOG_WARNING << "GetNextTag => " << TagReadStatusName(result)
    //            << ", *timestamp_ms: " << *timestamp_ms
    //            << ", *out=" << out->ToString();
    if ( result == READ_NO_DATA ) {
      CHECK_NULL(out->get());
      // EOF already reached? => fast return, don't attempt another read
      if ( file_eof_reached_ ) {
        return READ_EOF;
      }
      // read file -> buf_
      char tmp[512];
      int32 read = file_.Read(tmp, sizeof(tmp));
      if ( read < 0 ) {
        LOG_ERROR << "file_.Read failed: " << GetLastSystemErrorDescription();
        return READ_EOF;
      }
      if ( read < sizeof(tmp) ) {
        file_eof_reached_ = true;
      }
      buf_.Write(tmp, read);
      // attempt decode again
      continue;
    }
    if ( result == READ_OK ) {
      CHECK_NOT_NULL(out->get());
    } else {
      CHECK_NULL(out->get());
    }
    return result;
  }
}
TagReadStatus MediaFileReader::Read(scoped_ref<Tag>* out,
                                    io::MemoryStream* out_data) {
  buf_.MarkerSet();
  int64 timestamp_ms;
  TagReadStatus result = Read(out, &timestamp_ms);
  const int32 end_size = buf_.Size();
  buf_.MarkerRestore();
  const int32 start_size = buf_.Size();
  CHECK_GT(start_size, end_size);
  out_data->AppendStream(&buf_, start_size - end_size);
  return result;
}

bool MediaFileReader::IsEof() const {
  return file_eof_reached_;
}

uint64 MediaFileReader::Position() const {
  return file_.Position() - buf_.Size();
}

uint64 MediaFileReader::Remaining() const {
  return file_size_ - Position();
}

}
