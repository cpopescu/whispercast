
#include "f4v/atoms/movie/stsz_atom.h"

namespace streaming {
namespace f4v {

StszRecord::StszRecord()
  : entry_size_(0) {
}
StszRecord::StszRecord(uint32 entry_size)
  : entry_size_(entry_size) {
}
StszRecord::StszRecord(const StszRecord& other)
  : entry_size_(other.entry_size_) {
}
StszRecord::~StszRecord() {
}
StszRecord* StszRecord::Clone() const {
  return new StszRecord(*this);
}
bool StszRecord::Decode(io::MemoryStream& in) {
  if ( in.Size() < 4 ) {
    return false;
  }
  entry_size_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  return true;
}
void StszRecord::Encode(io::MemoryStream& out) const {
  io::NumStreamer::WriteUInt32(&out, entry_size_, common::BIGENDIAN);
}
string StszRecord::ToString() const {
  return strutil::StringPrintf("entry_size_: %d", entry_size_);
}


StszAtom::StszAtom()
  : MultiRecordVersionedAtom<StszRecord>(kType),
    uniform_sample_size_(0) {
}
StszAtom::StszAtom(const StszAtom& other)
  : MultiRecordVersionedAtom<StszRecord>(other),
    uniform_sample_size_(other.uniform_sample_size()) {
}
StszAtom::~StszAtom() {
}
uint32 StszAtom::uniform_sample_size() const {
  return uniform_sample_size_;
}
BaseAtom* StszAtom::Clone() const {
  return new StszAtom(*this);
}
TagDecodeStatus StszAtom::DecodeVersionedBody(uint64 size, io::MemoryStream& in,
                                              Decoder& decoder) {
  if ( in.Size() < 8 ) {
    DATOMLOG << "Not enough data in stream: " << in.Size()
             << " is less than expected: 8";
    return TAG_DECODE_NO_DATA;
  }
  uniform_sample_size_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  if ( uniform_sample_size_ != 0 ) {
    // All records have the same size: uniform_sample_size_.
    // Happens on audio tracks where all samples have the same size.
    if ( size != 8 ) {
      EATOMLOG << "Illegal size: " << size
               << ", expected: 8"
               << ", for uniform_sample_size_: " << uniform_sample_size_;
      return TAG_DECODE_ERROR;
    }
    uint32 sample_count = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
    for ( uint32 i = 0; i < sample_count; i++ ) {
      AddRecord(new StszRecord(uniform_sample_size_));
    }
    return TAG_DECODE_SUCCESS;
  }
  // Various size records, decode each of them separately.
  return MultiRecordVersionedAtom<StszRecord>::DecodeVersionedBody(
      size - 4, in, decoder);
}
void StszAtom::EncodeVersionedBody(io::MemoryStream& out,
                                   Encoder& encoder) const {
  io::NumStreamer::WriteUInt32(&out, uniform_sample_size_, common::BIGENDIAN);
  MultiRecordVersionedAtom<StszRecord>::EncodeVersionedBody(out, encoder);
}
uint64 StszAtom::MeasureVersionedBodySize() const {
  return 4 + MultiRecordVersionedAtom<StszRecord>::MeasureVersionedBodySize();
}
string StszAtom::ToStringVersionedBody(uint32 indent) const {
  return strutil::StringPrintf("uniform_sample_size_: %d, ",
      uniform_sample_size_) +
      MultiRecordVersionedAtom<StszRecord>::ToStringVersionedBody(indent);
}

}
}
