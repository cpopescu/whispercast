
#include "f4v/atoms/movie/null_atom.h"

namespace streaming {
namespace f4v {

NullAtom::NullAtom()
  : BaseAtom(kType) {
}
NullAtom::NullAtom(const NullAtom& other)
  : BaseAtom(other) {
}
NullAtom::~NullAtom() {
}
void NullAtom::GetSubatoms(vector<const BaseAtom*>& subatoms) const {
}
BaseAtom* NullAtom::Clone() const {
  return new NullAtom(*this);
}
TagDecodeStatus NullAtom::DecodeBody(uint64 size, io::MemoryStream& in,
                                     Decoder& decoder) {
  if ( in.Size() < size ) {
    DATOMLOG << "Not enough data in stream: " << in.Size()
             << " is less than expected: " << size;
    return TAG_DECODE_NO_DATA;
  }
  if ( size != kBodySize ) {
    EATOMLOG << "Illegal body size: " << size << ", expected: " << kBodySize;
    return TAG_DECODE_ERROR;
  }
  return TAG_DECODE_SUCCESS;
}
void NullAtom::EncodeBody(io::MemoryStream& out, Encoder& encoder) const {
  // empty body, nothing to write
}
uint64 NullAtom::MeasureBodySize() const {
  return kBodySize;
}
string NullAtom::ToStringBody(uint32 indent) const {
  return "";
}

}
}
