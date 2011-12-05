#include "f4v/atoms/versioned_atom.h"

namespace streaming {
namespace f4v {

VersionedAtom::VersionedAtom(AtomType type)
  : BaseAtom(type),
    version_(0) {
  ::memset(flags_, 0, sizeof(flags_));
}
VersionedAtom::VersionedAtom(const VersionedAtom& other)
  : BaseAtom(other),
    version_(other.version()) {
  ::memcpy(flags_, other.flags(), sizeof(flags_));
}
VersionedAtom::~VersionedAtom() {
}

uint8 VersionedAtom::version() const {
  return version_;
}
const uint8* VersionedAtom::flags() const {
  return flags_;
}

void VersionedAtom::set_version(uint8 version) {
  version_ = version;
}
void VersionedAtom::set_flags(uint8 f0, uint8 f1, uint8 f2) {
  flags_[0] = f0;
  flags_[1] = f1;
  flags_[2] = f2;
}
void VersionedAtom::set_flags(const uint8* flags) {
  set_flags(flags[0], flags[1], flags[2]);
}

string VersionedAtom::base_info() const {
  return BaseAtom::base_info() +
         strutil::StringPrintf(", version: %hhu(%hhu,%hhu,%hhu)",
             version_, flags_[0], flags_[1], flags_[2]);
}
TagDecodeStatus VersionedAtom::DecodeBody(uint64 size,
                                           io::MemoryStream& in,
                                           Decoder& decoder) {
  if ( size < 4 ) {
    EATOMLOG << "Illegal small size: " << size;
    return TAG_DECODE_ERROR;
  }
  if ( in.Size() < 4 ) {
    DATOMLOG << "Not enough data in stream: " << in.Size()
             << " is less than expected: 4";
    return TAG_DECODE_NO_DATA;
  }
  if ( in.Read(&version_, 1) != 1 ) {
    // should never happen, we've already checked Readable()
    EATOMLOG << "Stream reading error";
    return TAG_DECODE_ERROR;
  }
  if ( in.Read(flags_, 3) != 3 ) {
    // should never happen, we've already checked Readable()
    EATOMLOG << "Stream reading error";
    return TAG_DECODE_ERROR;
  }
  return DecodeVersionedBody(size - 4, in, decoder);
}
void VersionedAtom::EncodeBody(io::MemoryStream& out, Encoder& encoder) const {
  out.Write(&version_, 1);
  out.Write(flags_, 3);
  EncodeVersionedBody(out, encoder);
}
uint64 VersionedAtom::MeasureBodySize() const {
  return 4 + MeasureVersionedBodySize();
}
string VersionedAtom::ToStringBody(uint32 indent) const {
  ostringstream oss;
  oss << "version:" << static_cast<int>(version_)
      << ", flags:" << static_cast<int>(flags_[0]) << ","
                    << static_cast<int>(flags_[1]) << ","
                    << static_cast<int>(flags_[2])
      << ", " << ToStringVersionedBody(indent);
  return oss.str();
}

}
}
