
#include "f4v/atoms/movie/stss_atom.h"

namespace streaming {
namespace f4v {

StssRecord::StssRecord()
  : sample_number_(0) {
}
StssRecord::StssRecord(uint32 sample_number)
  : sample_number_(sample_number) {
}
StssRecord::StssRecord(const StssRecord& other)
  : sample_number_(other.sample_number_) {
}
StssRecord::~StssRecord() {
}
StssRecord* StssRecord::Clone() const {
  return new StssRecord(*this);
}
bool StssRecord::Decode(io::MemoryStream& in) {
  if ( in.Size() < 4 ) {
    return false;
  }
  sample_number_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  return true;
}
void StssRecord::Encode(io::MemoryStream& out) const {
  io::NumStreamer::WriteUInt32(&out, sample_number_, common::BIGENDIAN);
}
string StssRecord::ToString() const {
  return strutil::StringPrintf("sample_number_: %d",
                               sample_number_);
}


StssAtom::StssAtom()
  : MultiRecordVersionedAtom<StssRecord>(kType) {
}
StssAtom::StssAtom(const StssAtom& other)
  : MultiRecordVersionedAtom<StssRecord>(other) {
}
StssAtom::~StssAtom() {
}
BaseAtom* StssAtom::Clone() const {
  return new StssAtom(*this);
}

}
}
