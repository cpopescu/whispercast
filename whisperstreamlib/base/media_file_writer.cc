#include <whisperstreamlib/base/media_file_writer.h>
#include <whisperstreamlib/base/media_file_reader.h>

namespace streaming {

MediaFileWriter::MediaFileWriter()
  : serializer_(NULL),
    buf_(),
    file_(),
    filename_(),
    format_(),
    media_info_(),
    async_finalize_(NULL) {
}
MediaFileWriter::~MediaFileWriter() {
  if ( async_finalize_ != NULL ) {
    async_finalize_->Kill();
    delete async_finalize_;
    async_finalize_ = NULL;
  }
  CancelFile();
  CHECK_NULL(serializer_);
  CHECK(buf_.IsEmpty());
  CHECK(!file_.is_open());
  CHECK_NULL(async_finalize_);
}

bool MediaFileWriter::IsOpen() const {
  return file_.is_open();
}

bool MediaFileWriter::Open(const string& filename, MediaFormat format) {
  CHECK_NULL(serializer_) << "Already opened!";
  filename_ = filename;
  format_ = format;
  media_info_ = MediaInfo();

  serializer_ = CreateSerializer(kPartFileFormat);
  CHECK_NOT_NULL(serializer_) << "Unsupported serializer: "
                              << MediaFormatName(kPartFileFormat);
  if ( !file_.Open(PartFilename(), io::File::GENERIC_READ_WRITE,
                   io::File::CREATE_ALWAYS) ) {
    LOG_ERROR << "Failed to create file: [" << PartFilename() << "], err: "
              << GetLastSystemErrorDescription();
    CancelFile();
    return false;
  }
  // write the file header (does nothing if the format has no header)
  serializer_->Initialize(&buf_);
  Flush(false);
  return true;
}

bool MediaFileWriter::Write(const Tag* tag, int64 ts) {
  CHECK(IsOpen());
  if ( tag->type() == Tag::TYPE_MEDIA_INFO ) {
    if ( media_info_.has_audio() || media_info_.has_video() ) {
      LOG_ERROR << "Ignoring duplicate media info"
                   ", first: " << media_info_.ToString()
                << ", now: " << tag->ToString();
      return true;
    }
    media_info_ = static_cast<const MediaInfoTag*>(tag)->info();
    media_info_.mutable_frames()->clear();
    return true;
  }
  if ( !tag->is_audio_tag() && !tag->is_video_tag() ) {
    LOG_INFO << "Ignoring tag: " << tag->ToString();
    return true;
  }
  media_info_.mutable_frames()->push_back(MediaInfo::Frame(tag->is_audio_tag(),
      tag->size(), ts, 0, tag->is_video_tag() && tag->can_resync()));
  if ( !serializer_->Serialize(tag, ts, &buf_) ) {
    LOG_ERROR << "Failed to serialize tag: " << tag->ToString();
    return false;
  }
  Flush(false);
  return true;
}

bool MediaFileWriter::Finalize(net::Selector* selector,
                                   Callback1<bool>* completion) {
  if ( selector == NULL || completion == NULL ) {
    return FinalizeFile();
  }
  CHECK_NULL(async_finalize_);
  async_finalize_ = new thread::Thread(NewCallback(this,
      &MediaFileWriter::AsyncFinalize, selector, completion));
  async_finalize_->SetJoinable();
  async_finalize_->SetStackSize(1<<20);
  async_finalize_->Start();
  return true;
}

void MediaFileWriter::CancelFile() {
  buf_.Clear();
  CloseFile();
}
void MediaFileWriter::CloseFile() {
  if ( IsOpen() ) { Flush(true); }
  delete serializer_;
  serializer_ = NULL;
  file_.Close();
  buf_.Clear();
}
void MediaFileWriter::AsyncFinalize(net::Selector* selector,
    Callback1<bool>* completion) {
  bool success = FinalizeFile();
  selector->RunInSelectLoop(NewCallback(this,
      &MediaFileWriter::AsyncFinalizeComleted, completion, success));
}
void MediaFileWriter::AsyncFinalizeComleted(Callback1<bool>* completion,
    bool success) {
  completion->Run(success);
  delete async_finalize_;
  async_finalize_ = NULL;
}

bool MediaFileWriter::FinalizeFile() {
  CHECK(IsOpen()) << "Nothing to finalize";
  CloseFile();

  LOG_INFO << "Part file complete: " << PartFilename();
  LOG_INFO << "Finalizing the output file..";

  serializer_ = CreateSerializer(format_);
  if ( serializer_ == NULL ) {
    LOG_ERROR << "Unsupported format: " << MediaFormatName(format_);
    return false;
  }

  // reading the .part file
  MediaFileReader reader;
  if ( !reader.Open(PartFilename(), kPartFileFormat) ) {
    LOG_ERROR << "Failed to open file: [" << PartFilename() << "], err: "
              << GetLastSystemErrorDescription();
    return false;
  }
  // writing the complete & correct file, using a .tmp file output
  LOG_INFO << "Writing to: " << TmpFilename();
  if ( !file_.Open(TmpFilename(), io::File::GENERIC_READ_WRITE,
                                  io::File::CREATE_ALWAYS) ) {
    LOG_ERROR << "Failed to create file: [" << TmpFilename() << "], err: "
              << GetLastSystemErrorDescription();
    return false;
  }

  // initialize the serializer
  serializer_->Initialize(&buf_);
  Flush(false);

  // serialize media info
  LOG_INFO << "Using media info: " << media_info_.ToString();
  if ( !serializer_->Serialize(scoped_ref<Tag>(new MediaInfoTag(
      0, kDefaultFlavourMask, media_info_)).get(), 0, &buf_) ) {
    LOG_ERROR << "Failed to serialize media info: " << media_info_.ToString();
    return false;
  }
  Flush(false);

  // read the .part file again and output the correct file
  while ( true ) {
    scoped_ref<Tag> tag;
    int64 ts = 0;
    TagReadStatus status = reader.Read(&tag, &ts);
    if ( status == READ_EOF ) {
      break;
    }
    if ( status == READ_SKIP ) {
      continue;
    }
    // ignore the empty media info (the .part file has no metadata)
    if ( tag->type() == Tag::TYPE_MEDIA_INFO ) {
      continue;
    }
    if ( !serializer_->Serialize(tag.get(), ts, &buf_) ){
      LOG_ERROR << "Failed to serialize tag: " << tag->ToString();
      return false;
    }
    Flush(false);
  }

  // finish the correct file
  serializer_->Finalize(&buf_);

  // close the tmp file
  CloseFile();

  // and rename it to it's final place
  io::Rename(TmpFilename(), filename_, true);

  // removing .part file
  io::Rm(PartFilename());

  return true;
}
void MediaFileWriter::Flush(bool force) {
  if ( buf_.Size() > kFlushSize || force ) {
    file_.Write(buf_);
  }
}

}
