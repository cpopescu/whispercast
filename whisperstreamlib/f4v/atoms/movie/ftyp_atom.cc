#include <whisperlib/common/base/strutil.h>
#include "f4v/atoms/movie/ftyp_atom.h"

namespace streaming {
namespace f4v {

FtypAtom::FtypAtom()
  : BaseAtom(kType),
    major_brand_(0),
    minor_version_(0),
    compatible_brands_() {
}
FtypAtom::FtypAtom(const FtypAtom& other)
  : BaseAtom(other),
    major_brand_(other.major_brand()),
    minor_version_(other.minor_version()),
    compatible_brands_(other.compatible_brands()) {
}
FtypAtom::~FtypAtom() {
}

uint32 FtypAtom::major_brand() const {
  return major_brand_;
}
uint32 FtypAtom::minor_version() const {
  return minor_version_;
}
const vector<uint32>& FtypAtom::compatible_brands() const {
  return compatible_brands_;
}
vector<uint32>& FtypAtom::mutable_compatible_brands() {
  return compatible_brands_;
}

void FtypAtom::set_major_brand(uint32 major_brand) {
  major_brand_ = major_brand;
}
void FtypAtom::set_minor_version(uint32 minor_version) {
  minor_version_ = minor_version;
}

void FtypAtom::GetSubatoms(vector<const BaseAtom*>& subatoms) const {
}
BaseAtom* FtypAtom::Clone() const {
  return new FtypAtom(*this);
}
TagDecodeStatus FtypAtom::DecodeBody(uint64 size, io::MemoryStream& in,
                                      Decoder& decoder) {
  if ( size < kBodyMinSize || size % 4 ) {
    EATOMLOG << "Funny size: " << size;
    return TAG_DECODE_ERROR;
  }
  if ( in.Size() < size ) {
    DATOMLOG << "Not enough data in stream: " << in.Size()
             << " is less than expected: " << size;
    return TAG_DECODE_NO_DATA;
  }
  major_brand_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  minor_version_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  for ( uint32 i = kBodyMinSize; i < size; i += 4 ) {
    uint32 brand = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
    compatible_brands_.push_back(brand);
  }
  return TAG_DECODE_SUCCESS;
}
void FtypAtom::EncodeBody(io::MemoryStream& out, Encoder& encoder) const {
  io::NumStreamer::WriteUInt32(&out, major_brand_, common::BIGENDIAN);
  io::NumStreamer::WriteUInt32(&out, minor_version_, common::BIGENDIAN);
  for ( uint32 i = 0; i < compatible_brands_.size(); i++ ) {
    uint32 brand = compatible_brands_[i];
    io::NumStreamer::WriteUInt32(&out, brand, common::BIGENDIAN);
  }
}
uint64 FtypAtom::MeasureBodySize() const {
  return kBodyMinSize + 4 * compatible_brands_.size();
}
string FtypAtom::ToStringBody(uint32 indent) const {
  ostringstream oss;
  oss << "major_brand_: " << major_brand_
      << ", minor_version_: " << minor_version_
      << ", compatible_brands_: "
      << strutil::ToString(compatible_brands_);
  return oss.str();
}

}
}
