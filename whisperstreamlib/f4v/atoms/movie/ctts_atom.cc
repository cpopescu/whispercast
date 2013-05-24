
#include "f4v/atoms/movie/ctts_atom.h"

namespace streaming {
namespace f4v {

CttsRecord::CttsRecord()
  : sample_count_(0),
    sample_offset_(0) {
}
CttsRecord::CttsRecord(uint32 sample_count, int32 sample_offset)
  : sample_count_(sample_count),
    sample_offset_(sample_offset) {
}
CttsRecord::CttsRecord(const CttsRecord& other)
  : sample_count_(other.sample_count_),
    sample_offset_(other.sample_offset_) {
}
CttsRecord::~CttsRecord() {
}
bool CttsRecord::Equals(const CttsRecord& other) const {
  return sample_count_ == other.sample_count_ &&
         sample_offset_ == other.sample_offset_;
}
CttsRecord* CttsRecord::Clone() const {
  return new CttsRecord(*this);
}
bool CttsRecord::Decode(io::MemoryStream& in) {
  if ( in.Size() < (sizeof(sample_count_) +
                        sizeof(sample_offset_)) ) {
    return false;
  }
  sample_count_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  sample_offset_ = io::NumStreamer::ReadInt32(&in, common::BIGENDIAN);
  return true;
}
void CttsRecord::Encode(io::MemoryStream& out) const {
  io::NumStreamer::WriteUInt32(&out, sample_count_, common::BIGENDIAN);
  io::NumStreamer::WriteInt32(&out, sample_offset_, common::BIGENDIAN);
}
string CttsRecord::ToString() const {
  return strutil::StringPrintf("sample_count_: %d, sample_offset_: %d",
                               sample_count_, sample_offset_);
}


CttsAtom::CttsAtom()
  : MultiRecordVersionedAtom<CttsRecord>(kType) {
}
CttsAtom::CttsAtom(const CttsAtom& other)
  : MultiRecordVersionedAtom<CttsRecord>(other) {
}
CttsAtom::~CttsAtom() {
}
BaseAtom* CttsAtom::Clone() const {
  return new CttsAtom(*this);
}

}
}
