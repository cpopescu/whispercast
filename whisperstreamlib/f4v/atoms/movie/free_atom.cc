
#include "f4v/atoms/movie/free_atom.h"

namespace streaming {
namespace f4v {

FreeAtom::FreeAtom()
  : BaseAtom(kType),
    body_size_(0) {
}
FreeAtom::FreeAtom(uint32 body_size)
  : BaseAtom(kType),
    body_size_(body_size) {
}
FreeAtom::FreeAtom(const FreeAtom& other)
  : BaseAtom(other),
    body_size_(other.body_size_) {
}
FreeAtom::~FreeAtom() {
}
bool FreeAtom::EqualsBody(const BaseAtom& other) const {
  const FreeAtom& a = static_cast<const FreeAtom&>(other);
  return body_size_ == a.body_size_;
}
void FreeAtom::GetSubatoms(vector<const BaseAtom*>& subatoms) const {
}
BaseAtom* FreeAtom::Clone() const {
  return new FreeAtom(*this);
}
TagDecodeStatus FreeAtom::DecodeBody(uint64 size, io::MemoryStream& in,
                                      Decoder& decoder) {
  if ( in.Size() < size ) {
    DATOMLOG << "Not enough data in stream: " << in.Size()
             << " is less than expected: " << size;
    return TAG_DECODE_NO_DATA;
  }
  body_size_ = size;
  in.Skip(size);
  return TAG_DECODE_SUCCESS;
}
void FreeAtom::EncodeBody(io::MemoryStream& out, Encoder& encoder) const {
  // the Free atom has an irrelevant body, we're just going to write zeros
  static const uint8 kZeros[64] =
    {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
  for ( uint32 i = 0; i < body_size_; ) {
    size_t w = min((size_t)(body_size_ - i), sizeof(kZeros));
    out.Write(kZeros, w);
    i += w;
  }
}
uint64 FreeAtom::MeasureBodySize() const {
  return body_size_;
}
string FreeAtom::ToStringBody(uint32 indent) const {
  ostringstream oss;
  oss << "body_size_: " << body_size_;
  return oss.str();
}

}
}
