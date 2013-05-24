#include "f4v/atoms/movie/mp4a_atom.h"
#include "f4v/atoms/movie/esds_atom.h"
#include "f4v/atoms/movie/wave_atom.h"

namespace streaming {
namespace f4v {

Mp4aAtom::Mp4aAtom()
  : ContainerVersionedAtom(kType),
    raw_data_(),
    unknown_(0),
    data_reference_index_(0),
    inner_version_(0),
    revision_level_(0),
    vendor_(0),
    number_of_channels_(0),
    sample_size_in_bits_(0),
    compression_id_(0),
    packet_size_(0),
    sample_rate_(0),
    samples_per_packet_(0),
    bytes_per_packet_(0),
    bytes_per_frame_(0),
    bytes_per_sample_(0),
    esds_(NULL) {
}
Mp4aAtom::Mp4aAtom(const Mp4aAtom& other)
  : ContainerVersionedAtom(other),
    raw_data_(),
    unknown_(other.unknown_),
    data_reference_index_(other.data_reference_index_),
    inner_version_(other.inner_version_),
    revision_level_(other.revision_level_),
    vendor_(other.vendor_),
    number_of_channels_(other.number_of_channels_),
    sample_size_in_bits_(other.sample_size_in_bits_),
    compression_id_(other.compression_id_),
    packet_size_(other.packet_size_),
    sample_rate_(other.sample_rate_),
    samples_per_packet_(other.samples_per_packet_),
    bytes_per_packet_(other.bytes_per_packet_),
    bytes_per_frame_(other.bytes_per_frame_),
    bytes_per_sample_(other.bytes_per_sample_),
    esds_(NULL) {
  raw_data_.AppendStreamNonDestructive(&other.raw_data_);
  DeliverSubatomsToUpperClass();
}
Mp4aAtom::~Mp4aAtom() {
}

uint16 Mp4aAtom::data_reference_index() const {
  return data_reference_index_;
}
uint16 Mp4aAtom::inner_version() const {
  return inner_version_;
}
uint16 Mp4aAtom::revision_level() const {
  return revision_level_;
}
uint32 Mp4aAtom::vendor() const {
  return vendor_;
}
uint16 Mp4aAtom::number_of_channels() const {
  return number_of_channels_;
}
uint16 Mp4aAtom::sample_size_in_bits() const {
  return sample_size_in_bits_;
}
int16 Mp4aAtom::compression_id() const {
  return compression_id_;
}
uint16 Mp4aAtom::packet_size() const {
  return packet_size_;
}
FPU16U16 Mp4aAtom::sample_rate() const {
  return sample_rate_;
}
uint32 Mp4aAtom::samples_per_packet() const {
  return samples_per_packet_;
}
uint32 Mp4aAtom::bytes_per_packet() const {
  return bytes_per_packet_;
}
uint32 Mp4aAtom::bytes_per_frame() const {
  return bytes_per_frame_;
}
uint32 Mp4aAtom::bytes_per_sample() const {
  return bytes_per_sample_;
}

void Mp4aAtom::set_unknown(uint16 unknown) {
  unknown_ = unknown;
}
void Mp4aAtom::set_data_reference_index(uint16 data_reference_index) {
  data_reference_index_ = data_reference_index;
}
void Mp4aAtom::set_inner_version(uint16 inner_version) {
  inner_version_ = inner_version;
}
void Mp4aAtom::set_revision_level(uint16 revision_level) {
  revision_level_ = revision_level;
}
void Mp4aAtom::set_vendor(uint32 vendor) {
  vendor_ = vendor;
}
void Mp4aAtom::set_number_of_channels(uint16 number_of_channels) {
  number_of_channels_ = number_of_channels;
}
void Mp4aAtom::set_sample_size_in_bits(uint16 sample_size_in_bits) {
  sample_size_in_bits_ = sample_size_in_bits;
}
void Mp4aAtom::set_compression_id(int16 compression_id) {
  compression_id_ = compression_id;
}
void Mp4aAtom::set_packet_size(uint16 packet_size) {
  packet_size_ = packet_size;
}
void Mp4aAtom::set_sample_rate(FPU16U16 sample_rate) {
  sample_rate_ = sample_rate;
}
void Mp4aAtom::set_samples_per_packet(uint32 samples_per_packet) {
  samples_per_packet_ = samples_per_packet;
}
void Mp4aAtom::set_bytes_per_packet(uint32 bytes_per_packet) {
  bytes_per_packet_ = bytes_per_packet;
}
void Mp4aAtom::set_bytes_per_frame(uint32 bytes_per_frame) {
  bytes_per_frame_ = bytes_per_frame;
}
void Mp4aAtom::set_bytes_per_sample(uint32 bytes_per_sample) {
  bytes_per_sample_ = bytes_per_sample;
}

const EsdsAtom* Mp4aAtom::esds() const {
  return esds_;
}
const WaveAtom* Mp4aAtom::wave() const {
  return wave_;
}

void Mp4aAtom::SubatomFound(BaseAtom* atom) {
  switch ( atom->type() ) {
    case ATOM_ESDS: esds_ = static_cast<EsdsAtom*>(atom); break;
    case ATOM_WAVE: wave_ = static_cast<WaveAtom*>(atom); break;
    default:
      WATOMLOG << "Unknown subatom: " << atom->type_name();
  }
}

BaseAtom* Mp4aAtom::Clone() const {
  return new Mp4aAtom(*this);
}
TagDecodeStatus Mp4aAtom::DecodeData(uint64 size, io::MemoryStream& in,
                                     Decoder& decoder) {
  if ( size == 0 ) {
    WATOMLOG << "Empty body, size: 0";
    return TAG_DECODE_SUCCESS;
  }
  if ( in.Size() < size ) {
    DATOMLOG << "Not enough data in stream: " << in.Size()
             << " is less than expected: " << size;
    return TAG_DECODE_NO_DATA;
  }
  if ( size == 12 ) {
    WATOMLOG << "Strange Mp4aAtom, size: " << size << ", skipping decode";
    raw_data_.AppendStream(&in, size);
    return TAG_DECODE_SUCCESS;
  }
  if ( size < 24 ) {
    EATOMLOG << "Cannot decode from " << size << " bytes"
                ", expected at least 24 bytes";
    return TAG_DECODE_NO_DATA;
  }
  const int32 in_start_size = in.Size();
  unknown_ = io::NumStreamer::ReadUInt16(&in, common::BIGENDIAN);
  data_reference_index_ = io::NumStreamer::ReadUInt16(&in, common::BIGENDIAN);
  inner_version_ = io::NumStreamer::ReadUInt16(&in, common::BIGENDIAN);
  revision_level_ = io::NumStreamer::ReadUInt16(&in, common::BIGENDIAN);
  vendor_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  number_of_channels_ = io::NumStreamer::ReadUInt16(&in, common::BIGENDIAN);
  sample_size_in_bits_ = io::NumStreamer::ReadUInt16(&in, common::BIGENDIAN);
  compression_id_ = io::NumStreamer::ReadInt16(&in, common::BIGENDIAN);
  packet_size_ = io::NumStreamer::ReadUInt16(&in, common::BIGENDIAN);
  sample_rate_.integer_ = io::NumStreamer::ReadUInt16(&in, common::BIGENDIAN);
  sample_rate_.fraction_ = io::NumStreamer::ReadUInt16(&in, common::BIGENDIAN);
  if ( inner_version_ == 0 ) {
    return TAG_DECODE_SUCCESS;
  }
  if ( size < 16 ) {
    EATOMLOG << "Cannot decode from " << size << " bytes"
                ", expected at least 16 bytes";
    return TAG_DECODE_NO_DATA;
  }
  samples_per_packet_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  bytes_per_packet_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  bytes_per_frame_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  bytes_per_sample_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  const int32 in_end_size = in.Size();

  //char tmp[128] = {0,};
  //int32 read = in.Peek(tmp, min(static_cast<size_t>(size), sizeof(tmp)));
  DATOMLOG << "#" << (size - in_start_size + in_end_size) << " bytes remaining"
              ", to be read as subatoms"; // << strutil::PrintableDataBuffer(tmp, read);

  return TAG_DECODE_SUCCESS;
}
void Mp4aAtom::EncodeData(io::MemoryStream& out, Encoder& encoder) const {
  if ( raw_data_.Size() > 0 ) {
    out.AppendStreamNonDestructive(&raw_data_);
    return;
  }
  io::NumStreamer::WriteUInt16(&out, unknown_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt16(&out, data_reference_index_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt16(&out, inner_version_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt16(&out, revision_level_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt32(&out, vendor_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt16(&out, number_of_channels_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt16(&out, sample_size_in_bits_, common::BIGENDIAN);
  io::NumStreamer::WriteInt16(&out, compression_id_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt16(&out, packet_size_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt16(&out, sample_rate_.integer_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt16(&out, sample_rate_.fraction_, common::BIGENDIAN);
  if ( inner_version_ == 0 ) {
    return;
  }
  io::NumStreamer::WriteUInt32(&out, samples_per_packet_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt32(&out, bytes_per_packet_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt32(&out, bytes_per_frame_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt32(&out, bytes_per_sample_, common::BIGENDIAN);
}
uint64 Mp4aAtom::MeasureDataSize() const {
  if ( !raw_data_.IsEmpty() ) {
    return raw_data_.Size();
  }
  return inner_version_ == 0 ? 24 : 24 + 16;
}
bool Mp4aAtom::EqualsData(const ContainerVersionedAtom& other) const {
  const Mp4aAtom& a = static_cast<const Mp4aAtom&>(other);
  if ( !raw_data_.IsEmpty() ) {
    return raw_data_.Equals(a.raw_data_);
  }
  return unknown_ == a.unknown_ &&
         data_reference_index_ == a.data_reference_index_ &&
         inner_version_ == a.inner_version_ &&
         revision_level_ == a.revision_level_ &&
         vendor_ == a.vendor_ &&
         number_of_channels_ == a.number_of_channels_ &&
         sample_size_in_bits_ == a.sample_size_in_bits_ &&
         compression_id_ == a.compression_id_ &&
         packet_size_ == a.packet_size_ &&
         sample_rate_ == a.sample_rate_ &&
         samples_per_packet_ == a.samples_per_packet_ &&
         bytes_per_packet_ == a.bytes_per_packet_ &&
         bytes_per_frame_ == a.bytes_per_frame_ &&
         bytes_per_sample_ == a.bytes_per_sample_;
}
string Mp4aAtom::ToStringData(uint32 indent) const {
  return strutil::StringPrintf("data_reference_index_: %u"
                               ", inner_version_: %u"
                               ", revision_level_: %u"
                               ", vendor_: %u"
                               ", number_of_channels_: %u"
                               ", sample_size_in_bits_: %u"
                               ", compression_id_: %d"
                               ", packet_size_: %u"
                               ", sample_rate_: %u.%u"
                               ", samples_per_packet_: %u"
                               ", bytes_per_packet_: %u"
                               ", bytes_per_frame_: %u"
                               ", bytes_per_sample_: %u",
                               data_reference_index_,
                               inner_version_,
                               revision_level_,
                               vendor_,
                               number_of_channels_,
                               sample_size_in_bits_,
                               compression_id_,
                               packet_size_,
                               sample_rate_.integer_,
                               sample_rate_.fraction_,
                               samples_per_packet_,
                               bytes_per_packet_,
                               bytes_per_frame_,
                               bytes_per_sample_);
}

}
}
