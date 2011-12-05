#include <whisperstreamlib/flv/flv_file_writer.h>

namespace streaming {

FlvFileWriter::FlvFileWriter(bool has_video, bool has_audio)
  : buf_(), file_(), encoder_(true, has_video, has_audio),
    written_tags_(0), written_bytes_(0) {
}
FlvFileWriter::~FlvFileWriter() {
  Close();
}

void FlvFileWriter::SetFlags(bool has_video, bool has_audio) {
  encoder_.set_has_audio(has_audio);
  encoder_.set_has_video(has_video);
}

bool FlvFileWriter::Open(const string& filename) {
  if ( file_.is_open() ) {
    LOG_FATAL << "Double open file: [" << filename << "]";
    return false;
  }
  if ( !file_.Open(filename.c_str(), io::File::GENERIC_READ_WRITE,
      io::File::CREATE_ALWAYS) ) {
    LOG_ERROR << "Failed to create file: [" << filename << "]";
    return false;
  }
  written_tags_ = 0;
  written_bytes_ = 0;
  // writes the FlvHeader
  encoder_.Initialize(&buf_);
  Flush();
  return true;
}

void FlvFileWriter::Close() {
  if ( !file_.is_open() ) {
    return;
  }
  encoder_.Finalize(&buf_);
  Flush();
  file_.Close();
}

bool FlvFileWriter::IsOpen() const {
  return file_.is_open();
}

bool FlvFileWriter::Write(const FlvTag& tag) {
  encoder_.SerializeFlvTag(&tag, 0, &buf_);
  written_tags_++;
  return Flush();
}
bool FlvFileWriter::Write(scoped_ref<FlvTag>& tag) {
  return Write(*tag.get());
}
bool FlvFileWriter::Write(scoped_ref<const FlvTag>& tag) {
  return Write(*tag.get());
}
bool FlvFileWriter::Flush() {
  while ( !buf_.IsEmpty() ) {
    const char* tmp = NULL;
    int32 size = 0;
    if ( !buf_.ReadNext(&tmp, &size) ) {
      LOG_ERROR << "MemoryStream error: cannot read next";
      return false;
    }
    int32 w = file_.Write(tmp, size);
    if ( w != size ) {
      LOG_ERROR << "Failed to write complete buffer to file.";
      return false;
    }
    written_bytes_ += w;
  }
  return true;
}

uint64 FlvFileWriter::Position() const {
  return file_.Position() + buf_.Size();
}

string FlvFileWriter::StrStats() const {
  return strutil::StringPrintf(
      "\n  - %"PRIu64" tags"
      "\n  - %"PRIu64" bytes",
      written_tags_,
      written_bytes_);
}

} // namespace streaming
