#include "f4v/atoms/movie/avc1_atom.h"
#include "f4v/atoms/movie/avcc_atom.h"

namespace streaming {
namespace f4v {

Avc1Atom::Avc1Atom()
  : ContainerVersionedAtom(kType),
    reserved_(0),
    reference_index_(0),
    qt_video_encoding_version_(0),
    qt_video_encoding_revision_level_(0),
    qt_v_encoding_vendor_(0),
    qt_video_temporal_quality_(0),
    qt_video_spatial_quality_(0),
    video_frame_pixel_size_(0),
    horizontal_dpi_(0),
    vertical_dpi_(0),
    qt_video_data_size_(0),
    video_frame_count_(0),
    video_encoder_name_(),
    video_pixel_depth_(0),
    qt_video_color_table_id_(0),
    avcc_(NULL) {
}
Avc1Atom::Avc1Atom(const Avc1Atom& other)
  : ContainerVersionedAtom(other),
    reserved_(other.reserved_),
    reference_index_(other.reference_index_),
    qt_video_encoding_version_(other.qt_video_encoding_version_),
    qt_video_encoding_revision_level_(other.qt_video_encoding_revision_level_),
    qt_v_encoding_vendor_(other.qt_v_encoding_vendor_),
    qt_video_temporal_quality_(other.qt_video_temporal_quality_),
    qt_video_spatial_quality_(other.qt_video_spatial_quality_),
    video_frame_pixel_size_(other.video_frame_pixel_size_),
    horizontal_dpi_(other.horizontal_dpi_),
    vertical_dpi_(other.vertical_dpi_),
    qt_video_data_size_(other.qt_video_data_size_),
    video_frame_count_(other.video_frame_count_),
    video_encoder_name_(other.video_encoder_name_),
    video_pixel_depth_(other.video_pixel_depth_),
    qt_video_color_table_id_(other.qt_video_color_table_id_),
    avcc_(NULL) {
  DeliverSubatomsToUpperClass();
}
Avc1Atom::~Avc1Atom() {
}
uint16 Avc1Atom::reference_index() const {
  return reference_index_;
}
uint16 Avc1Atom::qt_video_encoding_version() const {
  return qt_video_encoding_version_;
}
uint16 Avc1Atom::qt_video_encoding_revision_level() const {
  return qt_video_encoding_revision_level_;
}
uint32 Avc1Atom::qt_v_encoding_vendor() const {
  return qt_v_encoding_vendor_;
}
uint32 Avc1Atom::qt_video_temporal_quality() const {
  return qt_video_temporal_quality_;
}
uint32 Avc1Atom::qt_video_spatial_quality() const {
  return qt_video_spatial_quality_;
}
uint32 Avc1Atom::video_frame_pixel_size() const {
  return video_frame_pixel_size_;
}
uint32 Avc1Atom::horizontal_dpi() const {
  return horizontal_dpi_;
}
uint32 Avc1Atom::vertical_dpi() const {
  return vertical_dpi_;
}
uint32 Avc1Atom::qt_video_data_size() const {
  return qt_video_data_size_;
}
uint16 Avc1Atom::video_frame_count() const {
  return video_frame_count_;
}
const string& Avc1Atom::video_encoder_name() const {
  return video_encoder_name_;
}
uint16 Avc1Atom::video_pixel_depth() const {
  return video_pixel_depth_;
}
uint16 Avc1Atom::qt_video_color_table_id() const {
  return qt_video_color_table_id_;
}

void Avc1Atom::set_reserved(uint16 reserved) {
  reserved_ = reserved;
}
void Avc1Atom::set_reference_index(uint16 reference_index) {
  reference_index_ = reference_index;
}
void Avc1Atom::set_qt_video_encoding_version(uint16 qt_video_encoding_version) {
  qt_video_encoding_version_ = qt_video_encoding_version;
}
void Avc1Atom::set_qt_video_encoding_revision_level(uint16 qt_video_encoding_revision_level) {
  qt_video_encoding_revision_level_ = qt_video_encoding_revision_level;
}
void Avc1Atom::set_qt_v_encoding_vendor(uint32 qt_v_encoding_vendor) {
  qt_v_encoding_vendor_ = qt_v_encoding_vendor;
}
void Avc1Atom::set_qt_video_temporal_quality(uint32 qt_video_temporal_quality) {
  qt_video_temporal_quality_ = qt_video_temporal_quality;
}
void Avc1Atom::set_qt_video_spatial_quality(uint32 qt_video_spatial_quality) {
  qt_video_spatial_quality_ = qt_video_spatial_quality;
}
void Avc1Atom::set_video_frame_pixel_size(uint32 video_frame_pixel_size) {
  video_frame_pixel_size_ = video_frame_pixel_size;
}
void Avc1Atom::set_horizontal_dpi(uint32 horizontal_dpi) {
  horizontal_dpi_ = horizontal_dpi;
}
void Avc1Atom::set_vertical_dpi(uint32 vertical_dpi) {
  vertical_dpi_ = vertical_dpi;
}
void Avc1Atom::set_qt_video_data_size(uint32 qt_video_data_size) {
  qt_video_data_size_ = qt_video_data_size;
}
void Avc1Atom::set_video_frame_count(uint16 video_frame_count) {
  video_frame_count_ = video_frame_count;
}
void Avc1Atom::set_video_encoder_name(const string& video_encoder_name) {
  video_encoder_name_ = video_encoder_name;
}
void Avc1Atom::set_video_pixel_depth(uint16 video_pixel_depth) {
  video_pixel_depth_ = video_pixel_depth;
}
void Avc1Atom::set_qt_video_color_table_id(uint16 qt_video_color_table_id) {
  qt_video_color_table_id_ = qt_video_color_table_id;
}

const AvccAtom* Avc1Atom::avcc() const {
  return avcc_;
}

void Avc1Atom::SubatomFound(BaseAtom* atom) {
  switch ( atom->type() ) {
    case ATOM_AVCC: avcc_ = static_cast<AvccAtom*>(atom); break;
    default:
      WATOMLOG << "Unknown subatom: " << atom->type_name();
  }
}
BaseAtom* Avc1Atom::Clone() const {
  return new Avc1Atom(*this);
}
TagDecodeStatus Avc1Atom::DecodeData(uint64 size, io::MemoryStream& in,
                                     Decoder& decoder) {
  if ( in.Size() < size ) {
    DATOMLOG << "Not enough data in stream: " << in.Size()
             << " is less than expected: " << size;
    return TAG_DECODE_NO_DATA;
  }
  if ( size < kDataSize ) {
    EATOMLOG << "Cannot decode from " << size << " bytes"
                ", expected at least " << kDataSize << " bytes";
    return TAG_DECODE_NO_DATA;
  }

  const int32 in_start_size = in.Size();
  reserved_ = io::NumStreamer::ReadUInt16(&in, common::BIGENDIAN);
  reference_index_ = io::NumStreamer::ReadUInt16(&in, common::BIGENDIAN);
  qt_video_encoding_version_ = io::NumStreamer::ReadUInt16(&in, common::BIGENDIAN);
  qt_video_encoding_revision_level_ = io::NumStreamer::ReadUInt16(&in, common::BIGENDIAN);
  qt_v_encoding_vendor_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  qt_video_temporal_quality_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  qt_video_spatial_quality_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  video_frame_pixel_size_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  horizontal_dpi_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  vertical_dpi_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  qt_video_data_size_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  video_frame_count_ = io::NumStreamer::ReadUInt16(&in, common::BIGENDIAN);
  uint8 video_encoder_name_length = io::NumStreamer::ReadByte(&in);
  // found this '31' hack in project "rtmpd", file: atomavc1.cpp:146
  uint8 video_encoder_padding_size = 31 > video_encoder_name_length ?
                                     31 - video_encoder_name_length : 0;
  in.ReadString(&video_encoder_name_, video_encoder_name_length);
  CHECK_EQ(video_encoder_name_length, video_encoder_name_.size());
  in.Skip(video_encoder_padding_size);
  video_pixel_depth_ = io::NumStreamer::ReadUInt16(&in, common::BIGENDIAN);
  qt_video_color_table_id_ = io::NumStreamer::ReadUInt16(&in, common::BIGENDIAN);
  const int32 in_end_size = in.Size();

  DATOMLOG << "#" << (size - in_start_size + in_end_size) << " bytes remaining"
              ", to be read as subatoms";

  return TAG_DECODE_SUCCESS;
}
void Avc1Atom::EncodeData(io::MemoryStream& out, Encoder& encoder) const {
  io::NumStreamer::WriteUInt16(&out, reserved_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt16(&out, reference_index_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt16(&out, qt_video_encoding_version_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt16(&out, qt_video_encoding_revision_level_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt32(&out, qt_v_encoding_vendor_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt32(&out, qt_video_temporal_quality_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt32(&out, qt_video_spatial_quality_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt32(&out, video_frame_pixel_size_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt32(&out, horizontal_dpi_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt32(&out, vertical_dpi_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt32(&out, qt_video_data_size_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt16(&out, video_frame_count_, common::BIGENDIAN);
  io::NumStreamer::WriteByte(&out, video_encoder_name_.size());
  out.Write(video_encoder_name_);
  uint8 zero[31] = {0,};
  uint8 video_encoder_padding_size = 31 > video_encoder_name_.size() ?
                                     31 - video_encoder_name_.size() : 0;
  out.Write(zero, video_encoder_padding_size);
  io::NumStreamer::WriteUInt16(&out, video_pixel_depth_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt16(&out, qt_video_color_table_id_, common::BIGENDIAN);
}
uint64 Avc1Atom::MeasureDataSize() const {
  return kDataSize;
}
bool Avc1Atom::EqualsData(const ContainerVersionedAtom& other) const {
  const Avc1Atom& a = static_cast<const Avc1Atom&>(other);
  return reserved_ == a.reserved_ &&
         reference_index_ == a.reference_index_ &&
         qt_video_encoding_version_ == a.qt_video_encoding_version_ &&
         qt_video_encoding_revision_level_ == a.qt_video_encoding_revision_level_ &&
         qt_v_encoding_vendor_ == a.qt_v_encoding_vendor_ &&
         qt_video_temporal_quality_ == a.qt_video_temporal_quality_ &&
         qt_video_spatial_quality_ == a.qt_video_spatial_quality_ &&
         video_frame_pixel_size_ == a.video_frame_pixel_size_ &&
         horizontal_dpi_ == a.horizontal_dpi_ &&
         vertical_dpi_ == a.vertical_dpi_ &&
         qt_video_data_size_ == a.qt_video_data_size_ &&
         video_frame_count_ == a.video_frame_count_ &&
         video_encoder_name_ == a.video_encoder_name_ &&
         video_pixel_depth_ == a.video_pixel_depth_ &&
         qt_video_color_table_id_ == a.qt_video_color_table_id_;
}
string Avc1Atom::ToStringData(uint32 indent) const {
  ostringstream oss;
  oss << "reserved_: " << reserved_
      << ", reference_index_: " << reference_index_
      << ", qt_video_encoding_version_: " << qt_video_encoding_version_
      << ", qt_video_encoding_revision_level_: " << qt_video_encoding_revision_level_
      << ", qt_v_encoding_vendor_: " << qt_v_encoding_vendor_
      << ", qt_video_temporal_quality_: " << qt_video_temporal_quality_
      << ", qt_video_spatial_quality_: " << qt_video_spatial_quality_
      << ", video_frame_pixel_size_: " << video_frame_pixel_size_
      << ", horizontal_dpi_: " << horizontal_dpi_
      << ", vertical_dpi_: " << vertical_dpi_
      << ", qt_video_data_size_: " << qt_video_data_size_
      << ", video_frame_count_: " << video_frame_count_
      << ", video_encoder_name_: \"" << video_encoder_name_ << "\""
      << ", video_pixel_depth_: " << video_pixel_depth_
      << ", qt_video_color_table_id_: " << qt_video_color_table_id_;
  return oss.str();
}
}
}
