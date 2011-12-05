#include "f4v/atoms/movie/avcc_atom.h"

namespace streaming {
namespace f4v {

AvccParameter::AvccParameter()
  : raw_data_() {
}
AvccParameter::AvccParameter(const AvccParameter& other)
  : raw_data_() {
  raw_data_.AppendStreamNonDestructive(&other.raw_data_);
}
AvccParameter::~AvccParameter() {
}

uint8 AvccAtom::configuration_version() const {
  return configuration_version_;
}
uint8 AvccAtom::profile() const {
  return profile_;
}
uint8 AvccAtom::profile_compatibility() const {
  return profile_compatibility_;
}
uint8 AvccAtom::level() const {
  return level_;
}
uint8 AvccAtom::nalu_length_size() const {
  return nalu_length_size_;
}
const vector<AvccParameter*>& AvccAtom::seq_parameters() const {
  return seq_parameters_;
}
const vector<AvccParameter*>& AvccAtom::pic_parameters() const {
  return pic_parameters_;
}

AvccParameter* AvccParameter::Clone() const {
  return new AvccParameter(*this);
}

bool AvccParameter::Decode(io::MemoryStream& in, uint64 in_size) {
  if ( in_size < 2 ) {
    F4V_LOG_ERROR << "Cannot decode AvccParameter, stream size: " << in_size
                  << " too small";
    return false;
  }
  uint16 len = io::NumStreamer::ReadUInt16(&in, common::BIGENDIAN);
  if ( len > in_size-2 ) {
    F4V_LOG_ERROR << "Cannot decode AvccParameter, len: " << len
                  << " exceeds stream size: " << (in_size-2);
    return false;
  }
  raw_data_.Clear();
  raw_data_.AppendStream(&in, len);
  return true;
}
void AvccParameter::Encode(io::MemoryStream& out) const {
  io::NumStreamer::WriteUInt16(&out, raw_data_.Size(), common::BIGENDIAN);
  out.AppendStreamNonDestructive(&raw_data_);
}
uint64 AvccParameter::MeasureSize() const {
  return 2 + raw_data_.Size();
}
string AvccParameter::ToString() const {
  return strutil::StringPrintf("raw_data_: %u bytes", raw_data_.Size());
}

AvccAtom::AvccAtom()
  : BaseAtom(kType),
    configuration_version_(0),
    profile_(0),
    profile_compatibility_(0),
    level_(0),
    original_nalu_length_size_(0),
    nalu_length_size_(0),
    original_seq_count_(0),
    seq_parameters_(),
    pic_parameters_() {
}
AvccAtom::AvccAtom(const AvccAtom& other)
  : BaseAtom(other),
    configuration_version_(other.configuration_version_),
    profile_(other.profile_),
    profile_compatibility_(other.profile_compatibility_),
    level_(other.level_),
    original_nalu_length_size_(other.original_nalu_length_size_),
    nalu_length_size_(other.nalu_length_size_),
    original_seq_count_(other.original_seq_count_),
    seq_parameters_(),
    pic_parameters_() {
  for ( uint32 i = 0; i < other.seq_parameters_.size(); i++ ) {
    seq_parameters_.push_back(other.seq_parameters_[i]->Clone());
  }
  for ( uint32 i = 0; i < other.pic_parameters_.size(); i++ ) {
    pic_parameters_.push_back(other.pic_parameters_[i]->Clone());
  }
}
AvccAtom::~AvccAtom() {
  for ( uint32 i = 0; i < seq_parameters_.size(); i++ ) {
    delete seq_parameters_[i];
  }
  seq_parameters_.clear();
  for ( uint32 i = 0; i < pic_parameters_.size(); i++ ) {
    delete pic_parameters_[i];
  }
  pic_parameters_.clear();
}

void AvccAtom::GetSubatoms(vector<const BaseAtom*>& subatoms) const {
}
BaseAtom* AvccAtom::Clone() const {
  return new AvccAtom(*this);
}
TagDecodeStatus AvccAtom::DecodeBody(uint64 size,
                                     io::MemoryStream& in,
                                     Decoder& decoder) {
  if ( in.Size() < size ) {
    DATOMLOG << "Not enough data in stream: " << in.Size()
             << " is less than expected: " << size;
    return TAG_DECODE_NO_DATA;
  }
  if ( size < kBodyMinSize ) {
    EATOMLOG << "Cannot decode from " << size << " bytes"
                ", expected at least " << kBodyMinSize << " bytes";
    return TAG_DECODE_NO_DATA;
  }
  configuration_version_ = io::NumStreamer::ReadByte(&in);
  profile_ = io::NumStreamer::ReadByte(&in);
  profile_compatibility_ = io::NumStreamer::ReadByte(&in);
  level_ = io::NumStreamer::ReadByte(&in);
  original_nalu_length_size_ = io::NumStreamer::ReadByte(&in);
  // Hack from "rtmpd" project, file: atomavcc.cpp:96
  nalu_length_size_ = 1 + (original_nalu_length_size_ & 0x03);
  DATOMLOG << "original_nalu_length_size_: "
           << static_cast<uint32>(original_nalu_length_size_)
           << " => adjusted nalu_length_size_: "
           << static_cast<uint32>(nalu_length_size_);

  original_seq_count_ = io::NumStreamer::ReadByte(&in);
  // Hack from "rtmpd" project, file: atomavcc.cpp:103
  uint8 seq_count = original_seq_count_ & 0x1f;
  DATOMLOG << "original_seq_count_: "
           << static_cast<uint32>(original_seq_count_)
           << " => adjusted seq_count: " << static_cast<uint32>(seq_count);

  uint32 decoded_bytes = 6;
  for ( uint8 i = 0; i < seq_count; i++ ) {
    AvccParameter* p = new AvccParameter();
    CHECK_LE(decoded_bytes, size);
    if ( !p->Decode(in, size - decoded_bytes) ) {
      EATOMLOG << "Failed to decode AvccParameter, index: "
               << (int)i << "/" << (int)seq_count;
      delete p;
      return TAG_DECODE_NO_DATA;
    }
    decoded_bytes += p->MeasureSize();
    seq_parameters_.push_back(p);
  }

  if ( in.Size() < 1 ) {
    EATOMLOG << "Cannot decode from " << in.Size() << " bytes"
                ", expected at least 1 bytes";
    return TAG_DECODE_NO_DATA;
  }
  uint8 pic_count = io::NumStreamer::ReadByte(&in);
  decoded_bytes++;
  DATOMLOG << "pic_count: " << static_cast<uint32>(pic_count)
           << " => no adjustments";

  for ( uint8 i = 0; i < pic_count; i++ ) {
    AvccParameter* p = new AvccParameter();
    CHECK_LE(decoded_bytes, size);
    if ( !p->Decode(in, size - decoded_bytes) ) {
      EATOMLOG << "Failed to decode AvccParameter, index: "
               << (int)i << "/" << (int)pic_count;
      delete p;
      return TAG_DECODE_NO_DATA;
    }
    decoded_bytes += p->MeasureSize();
    pic_parameters_.push_back(p);
  }

  if ( decoded_bytes != size ) {
    CHECK_LT(decoded_bytes, size);
    EATOMLOG << "Useless data at atom end:  " << (size - decoded_bytes)
             << " bytes";
    return TAG_DECODE_ERROR;
  }
  return TAG_DECODE_SUCCESS;
}
void AvccAtom::EncodeBody(io::MemoryStream& out, Encoder& encoder) const {
  io::NumStreamer::WriteByte(&out, configuration_version_);
  io::NumStreamer::WriteByte(&out, profile_);
  io::NumStreamer::WriteByte(&out, profile_compatibility_);
  io::NumStreamer::WriteByte(&out, level_);
  io::NumStreamer::WriteByte(&out, original_nalu_length_size_);
  io::NumStreamer::WriteByte(&out, original_seq_count_);
  for ( uint32 i = 0; i < seq_parameters_.size(); i++ ) {
    seq_parameters_[i]->Encode(out);
  }
  io::NumStreamer::WriteByte(&out, pic_parameters_.size());
  for ( uint32 i = 0; i < pic_parameters_.size(); i++ ) {
    pic_parameters_[i]->Encode(out);
  }
}
uint64 AvccAtom::MeasureBodySize() const {
  uint64 size = kBodyMinSize;
  for ( uint32 i = 0; i < seq_parameters_.size(); i++ ) {
    size += seq_parameters_[i]->MeasureSize();
  }
  for ( uint32 i = 0; i < pic_parameters_.size(); i++ ) {
    size += pic_parameters_[i]->MeasureSize();
  }
  return size;
}
namespace {
template <typename T>
string ToStringP(const vector<T*>& p) {
  ostringstream oss;
  oss << "Array #" << p.size() << "{";
  for ( uint32 i = 0; i < p.size(); ) {
    oss << p[i]->ToString();
    i++;
    if ( i < p.size() ) {
      oss << ", ";
    }
  }
  oss << "}";
  return oss.str();
}
}
string AvccAtom::ToStringBody(uint32 indent) const {
  return strutil::StringPrintf("configuration_version_: %u, "
                               "profile_: %u, "
                               "profile_compatibility_: %u, "
                               "level_: %u, "
                               "original_nalu_length_size_: %u, "
                               "nalu_length_size_: %u, "
                               "original_seq_count_: %u, "
                               "seq_parameters_: %s, "
                               "pic_parameters_: %s",
                               configuration_version_,
                               profile_,
                               profile_compatibility_,
                               level_,
                               original_nalu_length_size_,
                               nalu_length_size_,
                               original_seq_count_,
                               ToStringP(seq_parameters_).c_str(),
                               ToStringP(pic_parameters_).c_str());
}
}
}
