
#include "f4v/atoms/movie/vmhd_atom.h"

namespace streaming {
namespace f4v {

VmhdAtom::VmhdAtom()
  : VersionedAtom(kType),
    graphics_mode_(0) {
  ::memset(opcolor_, 0, sizeof(opcolor_));
}
VmhdAtom::VmhdAtom(const VmhdAtom& other)
  : VersionedAtom(other),
    graphics_mode_(other.graphics_mode()) {
  ::memcpy(opcolor_, other.opcolor_, sizeof(opcolor_));
}
VmhdAtom::~VmhdAtom() {
}
uint16 VmhdAtom::graphics_mode() const {
  return graphics_mode_;
}
const uint16* VmhdAtom::opcolor() const {
  return opcolor_;
}
void VmhdAtom::set_graphics_mode(uint16 graphics_mode) {
  graphics_mode_ = graphics_mode;
}
void VmhdAtom::set_opcolor(const uint16* opcolor, uint32 size) {
  CHECK_EQ(size, sizeof(opcolor_));
  ::memcpy(opcolor_, opcolor, sizeof(opcolor_));
}
void VmhdAtom::set_opcolor(uint16 red, uint16 green, uint16 blue) {
  opcolor_[0] = red;
  opcolor_[1] = green;
  opcolor_[2] = blue;
}
bool VmhdAtom::EqualsVersionedBody(const VersionedAtom& other) const {
  const VmhdAtom& a = static_cast<const VmhdAtom&>(other);
  return graphics_mode_ == a.graphics_mode_ &&
         ::memcmp(opcolor_, a.opcolor_, sizeof(opcolor_)) == 0;
}
void VmhdAtom::GetSubatoms(vector<const BaseAtom*>& subatoms) const {
}
BaseAtom* VmhdAtom::Clone() const {
  return new VmhdAtom(*this);
}
TagDecodeStatus VmhdAtom::DecodeVersionedBody(uint64 size, io::MemoryStream& in,
                                              Decoder& decoder) {
  if ( size != kVersionedBodySize ) {
    EATOMLOG << "Cannot decode VmhdAtom from " << size << " bytes"
                ", expected " << kVersionedBodySize << " bytes";
    return TAG_DECODE_ERROR;
  }
  if ( in.Size() < size ) {
    DATOMLOG << "Not enough data in stream: " << in.Size()
             << " is less than expected: " << size;
    return TAG_DECODE_NO_DATA;
  }
  graphics_mode_ = io::NumStreamer::ReadUInt16(&in, common::BIGENDIAN);
  in.Read(opcolor_, sizeof(opcolor_));
  return TAG_DECODE_SUCCESS;
}
void VmhdAtom::EncodeVersionedBody(io::MemoryStream& out, Encoder& encoder) const {
  io::NumStreamer::WriteUInt16(&out, graphics_mode_, common::BIGENDIAN);
  out.Write(opcolor_, sizeof(opcolor_));
}
uint64 VmhdAtom::MeasureVersionedBodySize() const {
  return kVersionedBodySize;
}
string VmhdAtom::ToStringVersionedBody(uint32 indent) const {
  ostringstream oss;
  oss << "graphics_mode_: " << graphics_mode_
      << ", opcolor_: " << opcolor_[0] << ","
                        << opcolor_[1] << ","
                        << opcolor_[2];
  return oss.str();
}
}
}
