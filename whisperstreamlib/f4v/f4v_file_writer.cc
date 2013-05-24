#include <whisperstreamlib/f4v/f4v_file_writer.h>

namespace streaming {
namespace f4v {

FileWriter::FileWriter()
  : buf_(), file_(), encoder_(), written_tags_(0), written_bytes_(0) {
}
FileWriter::~FileWriter() {
  Close();
}

bool FileWriter::Open(const string& filename) {
  if ( file_.is_open() ) {
    F4V_LOG_FATAL << "Double open file: [" << filename << "]";
    return false;
  }
  if ( !file_.Open(filename.c_str(), io::File::GENERIC_READ_WRITE,
      io::File::CREATE_ALWAYS) ) {
    F4V_LOG_ERROR << "Failed to create file: [" << filename << "]";
    return false;
  }
  return true;
}
void FileWriter::Close() {
  if ( !file_.is_open() ) {
    return;
  }
  Flush();
  file_.Close();
}

bool FileWriter::Write(const Tag& tag) {
  CHECK(buf_.IsEmpty());
  encoder_.WriteTag(buf_, tag);
  written_tags_++;
  return Flush();
}
bool FileWriter::Write(const BaseAtom& atom) {
  CHECK(buf_.IsEmpty());
  encoder_.WriteAtom(buf_, atom);
  written_tags_++;
  return Flush();
}
bool FileWriter::Write(const Frame& frame) {
  CHECK(buf_.IsEmpty());
  encoder_.WriteFrame(buf_, frame);
  written_tags_++;
  return Flush();
}

bool FileWriter::Flush() {
  while ( !buf_.IsEmpty() ) {
    const char* tmp = NULL;
    int32 size = 0;
    if ( !buf_.ReadNext(&tmp, &size) ) {
      F4V_LOG_ERROR << "MemoryStream error: cannot read next";
      return false;
    }
    int32 w = file_.Write(tmp, size);
    if ( w != size ) {
      F4V_LOG_ERROR << "Failed to write complete buffer to file.";
      return false;
    }
    written_bytes_ += w;
  }
  return true;
}

uint64 FileWriter::Position() const {
  return file_.Position();
}

string FileWriter::StrStats() const {
  return strutil::StringPrintf(
      "\n  - %"PRIu64" tags"
      "\n  - %"PRIu64" bytes",
      written_tags_,
      written_bytes_);
}

} // namespace f4v
} // namespace streaming
