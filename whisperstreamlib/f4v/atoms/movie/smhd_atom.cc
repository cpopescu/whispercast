
#include "f4v/atoms/movie/smhd_atom.h"

namespace streaming {
namespace f4v {

SmhdAtom::SmhdAtom()
  : VersionedAtom(kType),
    balance_(0),
    reserved_(0) {
}
SmhdAtom::SmhdAtom(const SmhdAtom& other)
  : VersionedAtom(other),
    balance_(other.balance()),
    reserved_(other.reserved_) {
}
SmhdAtom::~SmhdAtom() {
}
uint16 SmhdAtom::balance() const {
  return balance_;
}
void SmhdAtom::GetSubatoms(vector<const BaseAtom*>& subatoms) const {
}
BaseAtom* SmhdAtom::Clone() const {
  return new SmhdAtom(*this);
}
TagDecodeStatus SmhdAtom::DecodeVersionedBody(uint64 size, io::MemoryStream& in,
                                               Decoder& decoder) {
  if ( size != kVersionedBodySize ) {
    EATOMLOG << "Cannot decode SmhdAtom from " << size << " bytes"
                ", expected " << kVersionedBodySize << " bytes";
    return TAG_DECODE_ERROR;
  }
  if ( in.Size() < size ) {
    DATOMLOG << "Not enough data in stream: " << in.Size()
             << " is less than expected: " << size;
    return TAG_DECODE_NO_DATA;
  }
  balance_ = io::NumStreamer::ReadUInt16(&in, common::BIGENDIAN);
  in.Read(&reserved_, sizeof(reserved_));
  return TAG_DECODE_SUCCESS;
}
void SmhdAtom::EncodeVersionedBody(io::MemoryStream& out, Encoder& encoder) const {
  io::NumStreamer::WriteUInt16(&out, balance_, common::BIGENDIAN);
  out.Write(&reserved_, sizeof(reserved_));
}
uint64 SmhdAtom::MeasureVersionedBodySize() const {
  return kVersionedBodySize;
}
string SmhdAtom::ToStringVersionedBody(uint32 indent) const {
  return strutil::StringPrintf("balance: %d", static_cast<int>(balance_));
}
}
}
