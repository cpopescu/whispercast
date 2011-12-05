
#include "f4v/atoms/movie/stsc_atom.h"

namespace streaming {
namespace f4v {

StscRecord::StscRecord()
  : first_chunk_(0),
    samples_per_chunk_(0),
    sample_description_index_(0) {
}
StscRecord::StscRecord(uint32 first_chunk,
                       uint32 samples_per_chunk,
                       uint32 sample_description_index)
  : first_chunk_(first_chunk),
    samples_per_chunk_(samples_per_chunk),
    sample_description_index_(sample_description_index) {
}
StscRecord::StscRecord(const StscRecord& other)
  : first_chunk_(other.first_chunk_),
    samples_per_chunk_(other.samples_per_chunk_),
    sample_description_index_(other.sample_description_index_) {
}
StscRecord::~StscRecord() {
}
StscRecord* StscRecord::Clone() const {
  return new StscRecord(*this);
}
bool StscRecord::Decode(io::MemoryStream& in) {
  if ( in.Size() < 12 ) {
    return false;
  }
  first_chunk_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  samples_per_chunk_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  sample_description_index_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  return true;
}
void StscRecord::Encode(io::MemoryStream& out) const {
  io::NumStreamer::WriteUInt32(&out, first_chunk_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt32(&out, samples_per_chunk_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt32(&out, sample_description_index_, common::BIGENDIAN);
}

string StscRecord::ToString() const {
  return strutil::StringPrintf("first_chunk_: %d,"
                               "samples_per_chunk_: %d,"
                               "sample_description_index_: %d",
                               first_chunk_,
                               samples_per_chunk_,
                               sample_description_index_);
}


StscAtom::StscAtom()
  : MultiRecordVersionedAtom<StscRecord>(kType) {
}
StscAtom::StscAtom(const StscAtom& other)
  : MultiRecordVersionedAtom<StscRecord>(other) {
}
StscAtom::~StscAtom() {
}
BaseAtom* StscAtom::Clone() const {
  return new StscAtom(*this);
}

}
}
