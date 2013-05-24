
#include "f4v/atoms/movie/mvhd_atom.h"

namespace streaming {
namespace f4v {

const uint32 MvhdAtom::kVersionedBodySize_0 = 96;
const uint32 MvhdAtom::kVersionedBodySize_1 = 108;

MvhdAtom::MvhdAtom()
  : VersionedAtom(kType),
    creation_time_(0),
    modification_time_(0),
    time_scale_(0),
    duration_(0),
    preferred_rate_(0),
    preferred_volume_(0),
    preview_time_(0),
    preview_duration_(0),
    poster_time_(0),
    selection_time_(0),
    selection_duration_(0),
    current_time_(0),
    next_track_id_(0) {
  ::memset(reserved_, 0, sizeof(reserved_));
  ::memset(matrix_structure_, 0, sizeof(matrix_structure_));
}
MvhdAtom::MvhdAtom(const MvhdAtom& other)
  : VersionedAtom(other),
    creation_time_(other.creation_time_),
    modification_time_(other.modification_time_),
    time_scale_(other.time_scale_),
    duration_(other.duration_),
    preferred_rate_(other.preferred_rate_),
    preferred_volume_(other.preferred_volume_),
    preview_time_(other.preview_time_),
    preview_duration_(other.preview_duration_),
    poster_time_(other.poster_time_),
    selection_time_(other.selection_time_),
    selection_duration_(other.selection_duration_),
    current_time_(other.current_time_),
    next_track_id_(other.next_track_id_) {
  ::memcpy(reserved_, other.reserved_, sizeof(reserved_));
  ::memcpy(matrix_structure_, other.matrix_structure_,
           sizeof(matrix_structure_));
}
MvhdAtom::~MvhdAtom() {
}

uint64 MvhdAtom::creation_time() const {
  return creation_time_;
}
uint64 MvhdAtom::modification_time() const {
  return modification_time_;
}
uint32 MvhdAtom::time_scale() const {
  return time_scale_;
}
uint64 MvhdAtom::duration() const {
  return duration_;
}
uint32 MvhdAtom::preferred_rate() const {
  return preferred_rate_;
}
uint16 MvhdAtom::preferred_volume() const {
  return preferred_volume_;
}
uint32 MvhdAtom::preview_time() const {
  return preview_time_;
}
uint32 MvhdAtom::preview_duration() const {
  return preview_duration_;
}
uint32 MvhdAtom::poster_time() const {
  return poster_time_;
}
uint32 MvhdAtom::selection_time() const {
  return selection_time_;
}
uint32 MvhdAtom::selection_duration() const {
  return selection_duration_;
}
uint32 MvhdAtom::current_time() const {
  return current_time_;
}
uint32 MvhdAtom::next_track_id() const {
  return next_track_id_;
}

void MvhdAtom::set_creation_time(uint64 creation_time) {
  creation_time_ = creation_time;
}
void MvhdAtom::set_modification_time(uint64 modification_time) {
  modification_time_ = modification_time;
}
void MvhdAtom::set_time_scale(uint32 time_scale) {
  time_scale_ = time_scale;
}
void MvhdAtom::set_duration(uint64 duration) {
  duration_ = duration;
}
void MvhdAtom::set_preferred_rate(uint32 preferred_rate) {
  preferred_rate_ = preferred_rate;
}
void MvhdAtom::set_preferred_volume(uint16 preferred_volume) {
  preferred_volume_ = preferred_volume;
}
void MvhdAtom::set_reserved(const uint8* reserved, uint32 size) {
  CHECK_EQ(size, sizeof(reserved_));
  ::memcpy(reserved_, reserved, sizeof(reserved_));
}
void MvhdAtom::set_matrix_structure(const uint8* matrix_structure, uint32 size) {
  CHECK_EQ(size, sizeof(matrix_structure_));
  ::memcpy(matrix_structure_, matrix_structure, sizeof(matrix_structure_));
}
void MvhdAtom::set_preview_time(uint32 preview_time) {
  preview_time_ = preview_time;
}
void MvhdAtom::set_preview_duration(uint32 preview_duration) {
  preview_duration_ = preview_duration;
}
void MvhdAtom::set_poster_time(uint32 poster_time) {
  poster_time_ = poster_time;
}
void MvhdAtom::set_selection_time(uint32 selection_time) {
  selection_time_ = selection_time;
}
void MvhdAtom::set_selection_duration(uint32 selection_duration) {
  selection_duration_ = selection_duration;
}
void MvhdAtom::set_current_time(uint32 current_time) {
  current_time_ = current_time;
}
void MvhdAtom::set_next_track_id(uint32 next_track_id) {
  next_track_id_ = next_track_id;
}

bool MvhdAtom::EqualsVersionedBody(const VersionedAtom& other) const {
  const MvhdAtom& a = static_cast<const MvhdAtom&>(other);
  return creation_time_ == a.creation_time_ &&
         modification_time_ == a.modification_time_ &&
         time_scale_ == a.time_scale_ &&
         duration_ == a.duration_ &&
         preferred_rate_ == a.preferred_rate_ &&
         preferred_volume_ == a.preferred_volume_ &&
         ::memcmp(matrix_structure_, a.matrix_structure_,
             sizeof(matrix_structure_)) == 0 &&
         preview_time_ == a.preview_time_ &&
         preview_duration_ == a.preview_duration_ &&
         poster_time_ == a.poster_time_ &&
         selection_time_ == a.selection_time_ &&
         selection_duration_ == a.selection_duration_ &&
         current_time_ == a.current_time_ &&
         next_track_id_ == a.next_track_id_;
}
void MvhdAtom::GetSubatoms(vector<const BaseAtom*>& subatoms) const {
}
BaseAtom* MvhdAtom::Clone() const {
  return new MvhdAtom(*this);
}
TagDecodeStatus MvhdAtom::DecodeVersionedBody(uint64 size, io::MemoryStream& in,
                                               Decoder& decoder) {
  if ( version() != 0 && version() != 1 ) {
    EATOMLOG << "Unknown version: " << version();
    return TAG_DECODE_ERROR;
  }
  const uint32 kMvhdVersionedAtomBodySize =
    (version() == 0 ? kVersionedBodySize_0 : kVersionedBodySize_1);
  if ( size != kMvhdVersionedAtomBodySize ) {
    EATOMLOG << "Cannot decode MvhdAtom from " << size << " bytes"
                ", expected " << kMvhdVersionedAtomBodySize << " bytes";
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
  time_scale_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  if ( version() == 0 ) {
    duration_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  } else {
    CHECK_EQ(version(), 1);
    duration_ = io::NumStreamer::ReadUInt64(&in, common::BIGENDIAN);
  }
  preferred_rate_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  preferred_volume_ = io::NumStreamer::ReadUInt16(&in, common::BIGENDIAN);
  in.Read(reserved_, sizeof(reserved_));
  in.Read(matrix_structure_, sizeof(matrix_structure_));
  preview_time_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  preview_duration_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  poster_time_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  selection_time_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  selection_duration_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  current_time_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  next_track_id_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  return TAG_DECODE_SUCCESS;
}
void MvhdAtom::EncodeVersionedBody(io::MemoryStream& out, Encoder& encoder) const {
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
  io::NumStreamer::WriteUInt32(&out, time_scale_, common::BIGENDIAN);
  if ( version() == 0 ) {
    io::NumStreamer::WriteUInt32(&out, duration_, common::BIGENDIAN);
  } else {
    CHECK_EQ(version(), 1);
    io::NumStreamer::WriteUInt64(&out, duration_, common::BIGENDIAN);
  }
  io::NumStreamer::WriteUInt32(&out, preferred_rate_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt16(&out, preferred_volume_, common::BIGENDIAN);
  out.Write(reserved_, sizeof(reserved_));
  out.Write(matrix_structure_, sizeof(matrix_structure_));
  io::NumStreamer::WriteUInt32(&out, preview_time_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt32(&out, preview_duration_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt32(&out, poster_time_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt32(&out, selection_time_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt32(&out, selection_duration_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt32(&out, current_time_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt32(&out, next_track_id_, common::BIGENDIAN);

}
uint64 MvhdAtom::MeasureVersionedBodySize() const {
  return version() == 0 ? kVersionedBodySize_0 : kVersionedBodySize_1;
}
string MvhdAtom::ToStringVersionedBody(uint32 indent) const {
  return strutil::StringPrintf(
      "creation_time_: %"PRIu64"(%s) "
      ", modification_time_: %"PRIu64"(%s)"
      ", time_scale_: %u"
      ", duration_: %"PRIu64"(%"PRIu64" seconds)"
      ", preferred_rate_: %u"
      ", preferred_volume_: %u"
      ", preview_time_: %u"
      ", preview_duration_: %u"
      ", poster_time_: %u"
      ", selection_time_: %u"
      ", selection_duration_: %u"
      ", current_time_: %u"
      ", next_track_id_: %u",
      (creation_time_),
      MacDate(creation_time_).ToString().c_str(),
      (modification_time_),
      MacDate(modification_time_).ToString().c_str(),
      time_scale_,
      (duration_),
      (duration_ / time_scale_),
      preferred_rate_,
      preferred_volume_,
      preview_time_,
      preview_duration_,
      poster_time_,
      selection_time_,
      selection_duration_,
      current_time_,
      next_track_id_);
}

}
}
