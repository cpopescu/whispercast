
#include "f4v/atoms/movie/stts_atom.h"

namespace streaming {
namespace f4v {

SttsRecord::SttsRecord()
  : sample_count_(0),
    sample_delta_(0) {
}
SttsRecord::SttsRecord(uint32 sample_count, uint32 sample_delta)
  : sample_count_(sample_count),
    sample_delta_(sample_delta) {
}
SttsRecord::SttsRecord(const SttsRecord& other)
  : sample_count_(other.sample_count_),
    sample_delta_(other.sample_delta_) {
}
SttsRecord::~SttsRecord() {
}
bool SttsRecord::Equals(const SttsRecord& other) const {
  return sample_count_ == other.sample_count_ &&
         sample_delta_ == other.sample_delta_;
}
SttsRecord* SttsRecord::Clone() const {
  return new SttsRecord(*this);
}
bool SttsRecord::Decode(io::MemoryStream& in) {
  if ( in.Size() < 8 ) {
    return false;
  }
  sample_count_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  sample_delta_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  return true;
}
void SttsRecord::Encode(io::MemoryStream& out) const {
  io::NumStreamer::WriteUInt32(&out, sample_count_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt32(&out, sample_delta_, common::BIGENDIAN);
}
string SttsRecord::ToString() const {
  return strutil::StringPrintf("sample_count_: %d, "
                               "sample_delta_: %d",
                               sample_count_,
                               sample_delta_);
}


SttsAtom::SttsAtom()
  : MultiRecordVersionedAtom<SttsRecord>(kType) {
}
SttsAtom::SttsAtom(const SttsAtom& other)
  : MultiRecordVersionedAtom<SttsRecord>(other) {
}
SttsAtom::~SttsAtom() {
}
BaseAtom* SttsAtom::Clone() const {
  return new SttsAtom(*this);
}

}
}
