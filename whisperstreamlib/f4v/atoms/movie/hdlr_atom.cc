
#include "f4v/atoms/movie/hdlr_atom.h"

namespace streaming {
namespace f4v {

HdlrAtom::HdlrAtom()
  : VersionedAtom(kType),
    pre_defined_(0),
    handler_type_(0),
    name_() {
  ::memset(reserved_, 0, sizeof(reserved_));
}
HdlrAtom::HdlrAtom(const HdlrAtom& other)
  : VersionedAtom(other),
    pre_defined_(other.pre_defined_),
    handler_type_(other.handler_type_),
    name_(other.name_) {
  ::memcpy(reserved_, other.reserved_, sizeof(reserved_));
}
HdlrAtom::~HdlrAtom() {
}

uint32 HdlrAtom::pre_defined() const {
  return pre_defined_;
}
uint32 HdlrAtom::handler_type() const {
  return handler_type_;
}
const string& HdlrAtom::name() const {
  return name_;
}

void HdlrAtom::set_pre_defined(uint32 pre_defined) {
  pre_defined_ = pre_defined;
}
void HdlrAtom::set_handler_type(uint32 handler_type) {
  handler_type_ = handler_type;
}
void HdlrAtom::set_reserved(const uint32* reserved, uint32 size) {
  CHECK_EQ(size, sizeof(reserved_));
  ::memcpy(reserved_, reserved, sizeof(reserved_));
}
void HdlrAtom::set_name(string name) {
  name_ = name;
}

void HdlrAtom::GetSubatoms(vector<const BaseAtom*>& subatoms) const {
}
BaseAtom* HdlrAtom::Clone() const {
  return new HdlrAtom(*this);
}
TagDecodeStatus HdlrAtom::DecodeVersionedBody(uint64 size,
                                              io::MemoryStream& in,
                                              Decoder& decoder) {
  if ( in.Size() < size ) {
    DATOMLOG << "Not enough data in stream: " << in.Size()
             << " is less than expected: " << size;
    return TAG_DECODE_NO_DATA;
  }
  if ( size < kVersionedBodyMinSize ) {
    EATOMLOG << "Cannot decode HdlrAtom from " << size << " bytes"
                ", expected at least " << kVersionedBodyMinSize
             << " bytes";
    return TAG_DECODE_ERROR;
  }
  pre_defined_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);;
  handler_type_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);;
  in.Read(reserved_, sizeof(reserved_));
  uint32 remaining = size - kVersionedBodyMinSize;

  // TODO(cosmin): HACK! If first byte is not a printable char => must be
  //               string size. No official reference found. Find a better way
  //               of handling this situation.
  uint8 name_size = 0;
  if ( remaining > 0 && in.Peek(&name_size, 1) == 1 && name_size < ' ' ) {
    in.Skip(1);
    remaining--;
  }
  in.ReadString(&name_, remaining);

  return TAG_DECODE_SUCCESS;
}
void HdlrAtom::EncodeVersionedBody(io::MemoryStream& out,
                                   Encoder& encoder) const {
  io::NumStreamer::WriteUInt32(&out, pre_defined_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt32(&out, handler_type_, common::BIGENDIAN);
  out.Write(reserved_, sizeof(reserved_));
  out.Write(name_.c_str(), name_.size());
}
uint64 HdlrAtom::MeasureVersionedBodySize() const {
  return kVersionedBodyMinSize + name_.size();
}
string HdlrAtom::ToStringVersionedBody(uint32 indent) const {
  return strutil::StringPrintf("pre_defined_: %u, "
                               "handler_type_: %u(%c%c%c%c), "
                               "reserved_: [%u, %u, %u], "
                               "name_: %s",
                               pre_defined_,
                               handler_type_,
                               static_cast<char>((handler_type_ >> 24) & 0xff),
                               static_cast<char>((handler_type_ >> 16) & 0xff),
                               static_cast<char>((handler_type_ >>  8) & 0xff),
                               static_cast<char>((handler_type_ >>  0) & 0xff),
                               reserved_[0], reserved_[1], reserved_[2],
                               name_.c_str());
}
}
}
