#include <whisperlib/common/base/strutil.h>
#include <whisperstreamlib/base/media_file_reader.h>

namespace streaming {

MediaFileReader::MediaFileReader()
  : buf_(),
    file_(),
    file_size_(-1),
    file_eof_reached_(false),
    splitter_(NULL) {
}
MediaFileReader::~MediaFileReader() {
  Close();
}

TagSplitter* MediaFileReader::splitter() {
  return splitter_;
}

bool MediaFileReader::Open(const string& filename, MediaFormat media_format) {
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

  // maybe establish file type
  if ( media_format == -1 ) {
    const string ext = strutil::Extension(filename);
    if ( !MediaFormatFromExtension(ext, &media_format) ) {
      LOG_ERROR << "Unrecognized file extension: [" << filename << "]"
                   " defaulting to FLV splitter.";
      media_format = MFORMAT_FLV;
    }
  }

  // create splitter
  splitter_ = CreateSplitter(strutil::Basename(filename), media_format);
  if ( splitter_ == NULL ) {
    LOG_ERROR << "Cannot create a splitter for file: " << filename;
    return false;
  }

  return true;
}
void MediaFileReader::Close() {
  buf_.Clear();
  file_.Close();
  file_size_ = -1;
  file_eof_reached_ = false;
  delete splitter_;
  splitter_ = NULL;
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
    int64* timestamp_ms, io::MemoryStream* out_data) {
  buf_.MarkerSet();
  TagReadStatus result = Read(out, timestamp_ms);
  const int32 end_size = buf_.Size();
  buf_.MarkerRestore();
  const int32 start_size = buf_.Size();
  CHECK_GE(start_size, end_size);
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
