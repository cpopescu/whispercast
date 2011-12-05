
#include "f4v/atoms/movie/stco_atom.h"

namespace streaming {
namespace f4v {

StcoRecord::StcoRecord()
  : chunk_offset_(0) {
}
StcoRecord::StcoRecord(uint32 chunk_offset)
  : chunk_offset_(chunk_offset) {
}
StcoRecord::StcoRecord(const StcoRecord& other)
  : chunk_offset_(other.chunk_offset_) {
}
StcoRecord::~StcoRecord() {
}
StcoRecord* StcoRecord::Clone() const {
  return new StcoRecord(*this);
}
bool StcoRecord::Decode(io::MemoryStream& in) {
  if ( in.Size() < 4 ) {
    return false;
  }
  chunk_offset_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  return true;
}
void StcoRecord::Encode(io::MemoryStream& out) const {
  io::NumStreamer::WriteUInt32(&out, chunk_offset_, common::BIGENDIAN);
}
string StcoRecord::ToString() const {
  return strutil::StringPrintf("%d", chunk_offset_);
}


StcoAtom::StcoAtom()
  : MultiRecordVersionedAtom<StcoRecord>(kType) {
}
StcoAtom::StcoAtom(const StcoAtom& other)
  : MultiRecordVersionedAtom<StcoRecord>(other) {
}
StcoAtom::~StcoAtom() {
}
BaseAtom* StcoAtom::Clone() const {
  return new StcoAtom(*this);
}

}
}
