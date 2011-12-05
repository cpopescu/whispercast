
#include "f4v/atoms/movie/co64_atom.h"

namespace streaming {
namespace f4v {

Co64Record::Co64Record()
  : chunk_offset_(0) {
}
Co64Record::Co64Record(const Co64Record& other)
  : chunk_offset_(other.chunk_offset_) {
}
Co64Record::~Co64Record() {
}
Co64Record* Co64Record::Clone() const {
  return new Co64Record(*this);
}
bool Co64Record::Decode(io::MemoryStream& in) {
  if ( in.Size() < sizeof(chunk_offset_) ) {
    return false;
  }
  chunk_offset_ = io::NumStreamer::ReadUInt64(&in, common::BIGENDIAN);
  return true;
}
void Co64Record::Encode(io::MemoryStream& out) const {
  io::NumStreamer::WriteUInt64(&out, chunk_offset_, common::BIGENDIAN);
}
string Co64Record::ToString() const {
  return strutil::StringPrintf(
      "chunk_offset_: %"PRId64"",
      chunk_offset_);
}


Co64Atom::Co64Atom()
  : MultiRecordVersionedAtom<Co64Record>(kType) {
}
Co64Atom::Co64Atom(const Co64Atom& other)
  : MultiRecordVersionedAtom<Co64Record>(other) {
}
Co64Atom::~Co64Atom() {
}
BaseAtom* Co64Atom::Clone() const {
  return new Co64Atom(*this);
}

}
}
