
#include "f4v/atoms/movie/mdhd_atom.h"

namespace streaming {
namespace f4v {

const uint32 MdhdAtom::kVersionedBodySize_0 = 20;
const uint32 MdhdAtom::kVersionedBodySize_1 = 32;

MdhdAtom::MdhdAtom()
  : VersionedAtom(kType),
    creation_time_(0),
    modification_time_(0),
    time_scale_(0),
    duration_(0),
    language_(0),
    reserved_(0) {
}
MdhdAtom::MdhdAtom(const MdhdAtom& other)
  : VersionedAtom(other),
    creation_time_(other.creation_time()),
    modification_time_(other.modification_time()),
    time_scale_(other.time_scale()),
    duration_(other.duration()),
    language_(other.language()),
    reserved_(other.reserved_) {
}
MdhdAtom::~MdhdAtom() {
}
uint64 MdhdAtom::creation_time() const {
  return creation_time_;
}
uint64 MdhdAtom::modification_time() const {
  return modification_time_;
}
uint32 MdhdAtom::time_scale() const {
  return time_scale_;
}
uint64 MdhdAtom::duration() const {
  return duration_;
}
uint16 MdhdAtom::language() const {
  return language_;
}
string MdhdAtom::language_name() const {
  return MacLanguageName(language_);
}

void MdhdAtom::set_creation_time(uint64 creation_time) {
  creation_time_ = creation_time;
}
void MdhdAtom::set_modification_time(uint64 modification_time) {
  modification_time_ = modification_time;
}
void MdhdAtom::set_time_scale(uint32 time_scale) {
  time_scale_ = time_scale;
}
void MdhdAtom::set_duration(uint64 duration) {
  duration_ = duration;
}
void MdhdAtom::set_language(uint16 language) {
  language_ = language;
}
void MdhdAtom::set_reserved(uint16 reserved) {
  reserved_ = reserved;
}

bool MdhdAtom::EqualsVersionedBody(const VersionedAtom& other) const {
  const MdhdAtom& a = static_cast<const MdhdAtom&>(other);
  return creation_time_ == a.creation_time_ &&
         modification_time_ == a.modification_time_ &&
         time_scale_ == a.time_scale_ &&
         duration_ == a.duration_ &&
         language_ == a.language_ &&
         reserved_ == a.reserved_;
}
void MdhdAtom::GetSubatoms(vector<const BaseAtom*>& subatoms) const {
}
BaseAtom* MdhdAtom::Clone() const {
  return new MdhdAtom(*this);
}
TagDecodeStatus MdhdAtom::DecodeVersionedBody(uint64 size, io::MemoryStream& in,
                                               Decoder& decoder) {
  if ( in.Size() < size ) {
    DATOMLOG << "Not enough data in stream: " << in.Size()
             << " is less than expected: " << size;
    return TAG_DECODE_NO_DATA;
  }
  if ( version() == 0 ) {
    if ( size != kVersionedBodySize_0 ) {
      EATOMLOG << "Cannot decode MdhdAtom from " << size << " bytes"
                  ", expected " << kVersionedBodySize_0 << " bytes";
      return TAG_DECODE_ERROR;
    }
    creation_time_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
    modification_time_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
    time_scale_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
    duration_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
    language_ = io::NumStreamer::ReadUInt16(&in, common::BIGENDIAN);
    in.Read(&reserved_, sizeof(reserved_));
    return TAG_DECODE_SUCCESS;
  }
  if ( version() == 1 ) {
    if ( size != kVersionedBodySize_1 ) {
      EATOMLOG << "Cannot decode MdhdAtom from " << size << " bytes"
                  ", expected " << kVersionedBodySize_1 << " bytes";
      return TAG_DECODE_ERROR;
    }
    creation_time_ = io::NumStreamer::ReadUInt64(&in, common::BIGENDIAN);
    modification_time_ = io::NumStreamer::ReadUInt64(&in, common::BIGENDIAN);
    time_scale_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
    duration_ = io::NumStreamer::ReadUInt64(&in, common::BIGENDIAN);
    language_ = io::NumStreamer::ReadUInt16(&in, common::BIGENDIAN);
    in.Read(&reserved_, sizeof(reserved_));
    return TAG_DECODE_SUCCESS;
  }
  EATOMLOG << "Unknown version: " << version();
  return TAG_DECODE_ERROR;
}
void MdhdAtom::EncodeVersionedBody(io::MemoryStream& out, Encoder& encoder) const {
  if ( version() == 0 ) {
    io::NumStreamer::WriteUInt32(&out, creation_time_, common::BIGENDIAN);
    io::NumStreamer::WriteUInt32(&out, modification_time_, common::BIGENDIAN);
    io::NumStreamer::WriteUInt32(&out, time_scale_, common::BIGENDIAN);
    io::NumStreamer::WriteUInt32(&out, duration_, common::BIGENDIAN);
    io::NumStreamer::WriteUInt16(&out, language_, common::BIGENDIAN);
    out.Write(&reserved_, sizeof(reserved_));
    return;
  }
  if ( version() == 1 ) {
    io::NumStreamer::WriteUInt64(&out, creation_time_, common::BIGENDIAN);
    io::NumStreamer::WriteUInt64(&out, modification_time_, common::BIGENDIAN);
    io::NumStreamer::WriteUInt32(&out, time_scale_, common::BIGENDIAN);
    io::NumStreamer::WriteUInt64(&out, duration_, common::BIGENDIAN);
    io::NumStreamer::WriteUInt16(&out, language_, common::BIGENDIAN);
    out.Write(&reserved_, sizeof(reserved_));
    return;
  }
  FATOMLOG << "Illegal version: " << version();
}
uint64 MdhdAtom::MeasureVersionedBodySize() const {
  return version() == 0 ? kVersionedBodySize_0 : kVersionedBodySize_1;
}
string MdhdAtom::ToStringVersionedBody(uint32 indent) const {
  return strutil::StringPrintf(
      "creation_time_: %"PRIu64"(%s)"
      ", modification_time_: %"PRIu64"(%s)"
      ", time_scale_: %u"
      ", duration_: %"PRIu64"(%"PRIu64" seconds)"
      ", language_: %u(%s)",
      (creation_time_),
      MacDate(creation_time_).ToString().c_str(),
      (modification_time_),
      MacDate(modification_time_).ToString().c_str(),
      time_scale_,
      (duration_),
      (duration_ / time_scale_),
      language_, language_name().c_str());
}

}
}
