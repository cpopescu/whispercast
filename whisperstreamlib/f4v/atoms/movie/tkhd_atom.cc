
#include "f4v/atoms/movie/tkhd_atom.h"

namespace streaming {
namespace f4v {

const uint32 TkhdAtom::kVersionedBodySize_0 = 80;
const uint32 TkhdAtom::kVersionedBodySize_1 = 92;

TkhdAtom::TkhdAtom()
  : VersionedAtom(kType),
    creation_time_(0),
    modification_time_(0),
    id_(0),
    duration_(0),
    layer_(0),
    alternate_group_(0),
    volume_(0),
    width_(0),
    height_(0) {
  ::memset(reserved1_, 0, sizeof(reserved1_));
  ::memset(reserved2_, 0, sizeof(reserved2_));
  ::memset(reserved3_, 0, sizeof(reserved3_));
  ::memset(matrix_structure_, 0, sizeof(matrix_structure_));
}
TkhdAtom::TkhdAtom(const TkhdAtom& other)
  : VersionedAtom(other),
    creation_time_(other.creation_time()),
    modification_time_(other.modification_time()),
    id_(other.id()),
    duration_(other.duration()),
    layer_(other.layer()),
    alternate_group_(other.alternate_group()),
    volume_(other.volume()),
    width_(other.width()),
    height_(other.height()) {
  ::memcpy(reserved1_, other.reserved1_, sizeof(reserved1_));
  ::memcpy(reserved2_, other.reserved2_, sizeof(reserved2_));
  ::memcpy(reserved3_, other.reserved3_, sizeof(reserved3_));
  ::memcpy(matrix_structure_, other.matrix_structure_,
           sizeof(matrix_structure_));
}
TkhdAtom::~TkhdAtom() {
}
uint64 TkhdAtom::creation_time() const {
  return creation_time_;
}
uint64 TkhdAtom::modification_time() const {
  return modification_time_;
}
uint32 TkhdAtom::id() const {
  return id_;
}
uint64 TkhdAtom::duration() const {
  return duration_;
}
uint16 TkhdAtom::layer() const {
  return layer_;
}
uint16 TkhdAtom::alternate_group() const {
  return alternate_group_;
}
uint16 TkhdAtom::volume() const {
  return volume_;
}
FPU16U16 TkhdAtom::width() const {
  return width_;
}
FPU16U16 TkhdAtom::height() const {
  return height_;
}

void TkhdAtom::set_creation_time(uint64 creation_time) {
  creation_time_ = creation_time;
}
void TkhdAtom::set_modification_time(uint64 modification_time) {
  modification_time_ = modification_time;
}
void TkhdAtom::set_id(uint32 id) {
  id_ = id;
}
void TkhdAtom::set_reserved1(const uint8* reserved1, uint32 size) {
  CHECK_EQ(size, sizeof(reserved1_));
  ::memcpy(reserved1_, reserved1, sizeof(reserved1_));
}
void TkhdAtom::set_duration(uint64 duration) {
  duration_ = duration;
}
void TkhdAtom::set_reserved2(const uint8* reserved2, uint32 size) {
  CHECK_EQ(size, sizeof(reserved2_));
  ::memcpy(reserved2_, reserved2, sizeof(reserved2_));
}
void TkhdAtom::set_layer(uint16 layer) {
  layer_ = layer;
}
void TkhdAtom::set_alternate_group(uint16 alternate_group) {
  alternate_group_ = alternate_group;
}
void TkhdAtom::set_volume(uint16 volume) {
  volume_ = volume;
}
void TkhdAtom::set_reserved3(const uint8* reserved3, uint32 size) {
  CHECK_EQ(size, sizeof(reserved3_));
  ::memcpy(reserved3_, reserved3, sizeof(reserved3_));
}
void TkhdAtom::set_matrix_structure(const uint8* matrix_structure, uint32 size) {
  CHECK_EQ(size, sizeof(matrix_structure_));
  ::memcpy(matrix_structure_, matrix_structure, sizeof(matrix_structure_));
}
void TkhdAtom::set_width(FPU16U16 width) {
  width_ = width;
}
void TkhdAtom::set_height(FPU16U16 height) {
  height_ = height;
}

bool TkhdAtom::EqualsVersionedBody(const VersionedAtom& other) const {
  const TkhdAtom& a = static_cast<const TkhdAtom&>(other);
  return creation_time_ == a.creation_time_ &&
         modification_time_ == a.modification_time_ &&
         id_ == a.id_ &&
         duration_ == a.duration_ &&
         layer_ == a.layer_ &&
         alternate_group_ == a.alternate_group_ &&
         volume_ == a.volume_ &&
         ::memcmp(matrix_structure_, a.matrix_structure_,
             sizeof(matrix_structure_)) == 0 &&
         width_ == a.width_ &&
         height_ == a.height_;
}
void TkhdAtom::GetSubatoms(vector<const BaseAtom*>& subatoms) const {
}
BaseAtom* TkhdAtom::Clone() const {
  return new TkhdAtom(*this);
}
TagDecodeStatus TkhdAtom::DecodeVersionedBody(uint64 size, io::MemoryStream& in,
                                               Decoder& decoder) {
  if ( version() != 0 && version() != 1 ) {
    EATOMLOG << "Unknown version: " << version();
    return TAG_DECODE_ERROR;
  }
  const uint32 kTkhdVersionedAtomBodySize =
    (version() == 0 ? kVersionedBodySize_0 : kVersionedBodySize_1);
  if ( size != kTkhdVersionedAtomBodySize ) {
    EATOMLOG << "Cannot decode TkhdAtom from " << size << " bytes"
                ", expected " << kTkhdVersionedAtomBodySize << " bytes";
    return TAG_DECODE_ERROR;
  }
  if ( in.Size() < size ) {
    DATOMLOG << "Not enough data in stream: " << in.Size()
             << " is less than expected: " << size;
    return TAG_DECODE_NO_DATA;
  }
  if ( version() == 0 ) {
    creation_time_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  } else {
    CHECK_EQ(version(), 1);
    creation_time_ = io::NumStreamer::ReadUInt64(&in, common::BIGENDIAN);
  }
  if ( version() == 0 ) {
    modification_time_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  } else {
    CHECK_EQ(version(), 1);
    modification_time_ = io::NumStreamer::ReadUInt64(&in, common::BIGENDIAN);
  }
  id_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  in.Read(reserved1_, sizeof(reserved1_));
  if ( version() == 0 ) {
    duration_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  } else {
    CHECK_EQ(version(), 1);
    duration_ = io::NumStreamer::ReadUInt64(&in, common::BIGENDIAN);
  }
  in.Read(reserved2_, sizeof(reserved2_));
  layer_ = io::NumStreamer::ReadUInt16(&in, common::BIGENDIAN);
  alternate_group_ = io::NumStreamer::ReadUInt16(&in, common::BIGENDIAN);
  volume_ = io::NumStreamer::ReadUInt16(&in, common::BIGENDIAN);
  in.Read(reserved3_, sizeof(reserved3_));
  in.Read(matrix_structure_, sizeof(matrix_structure_));
  width_.integer_ = io::NumStreamer::ReadUInt16(&in, common::BIGENDIAN);
  width_.fraction_ = io::NumStreamer::ReadUInt16(&in, common::BIGENDIAN);
  height_.integer_ = io::NumStreamer::ReadUInt16(&in, common::BIGENDIAN);
  height_.fraction_ = io::NumStreamer::ReadUInt16(&in, common::BIGENDIAN);
  return TAG_DECODE_SUCCESS;
}
void TkhdAtom::EncodeVersionedBody(io::MemoryStream& out, Encoder& encoder) const {
  CHECK(version() == 0 || version() == 1) << "Illegal version: " << version();
  if ( version() == 0 ) {
    io::NumStreamer::WriteUInt32(&out, creation_time_, common::BIGENDIAN);
  } else {
    CHECK_EQ(version(), 1);
    io::NumStreamer::WriteUInt64(&out, creation_time_, common::BIGENDIAN);
  }
  if ( version() == 0 ) {
    io::NumStreamer::WriteUInt32(&out, modification_time_, common::BIGENDIAN);
  } else {
    CHECK_EQ(version(), 1);
    io::NumStreamer::WriteUInt64(&out, modification_time_, common::BIGENDIAN);
  }
  io::NumStreamer::WriteUInt32(&out, id_, common::BIGENDIAN);
  out.Write(reserved1_, sizeof(reserved1_));
  if ( version() == 0 ) {
    io::NumStreamer::WriteUInt32(&out, duration_, common::BIGENDIAN);
  } else {
    CHECK_EQ(version(), 1);
    io::NumStreamer::WriteUInt64(&out, duration_, common::BIGENDIAN);
  }
  out.Write(reserved2_, sizeof(reserved2_));
  io::NumStreamer::WriteUInt16(&out, layer_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt16(&out, alternate_group_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt16(&out, volume_, common::BIGENDIAN);
  out.Write(reserved3_, sizeof(reserved3_));
  out.Write(matrix_structure_, sizeof(matrix_structure_));
  io::NumStreamer::WriteUInt16(&out, width_.integer_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt16(&out, width_.fraction_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt16(&out, height_.integer_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt16(&out, height_.fraction_, common::BIGENDIAN);
}
uint64 TkhdAtom::MeasureVersionedBodySize() const {
  return version() == 0 ? kVersionedBodySize_0 : kVersionedBodySize_1;
}
string TkhdAtom::ToStringVersionedBody(uint32 indent) const {
  return strutil
    ::StringPrintf("creation_time_: %"PRIu64"(%s)"
		   ", modification_time_: %"PRIu64"(%s)"
		   ", id_: %u"
		   ", duration_: %"PRIu64""
		   ", layer_: %u"
		   ", alternate_group_: %u"
		   ", volume_: %u"
		   ", width_: %u.%u"
		   ", height_: %u.%u",
		   (creation_time_),
		   MacDate(creation_time_).ToString().c_str(),
		   (modification_time_),
		   MacDate(modification_time_).ToString().c_str(),
		   id_,
		   (duration_),
		   layer_,
		   alternate_group_,
		   volume_,
		   width_.integer_, width_.fraction_,
		   height_.integer_, height_.fraction_);
}

}
}
