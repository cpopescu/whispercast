
#include "f4v/atoms/raw_atom.h"

namespace streaming {
namespace f4v {

RawAtom::RawAtom(uint32 atom_type)
  : BaseAtom(kType),
    atom_type_(atom_type),
    raw_data_() {
}
RawAtom::RawAtom(const RawAtom& other)
  : BaseAtom(other),
    atom_type_(other.atom_type()),
    raw_data_() {
  raw_data_.AppendStreamNonDestructive(&other.raw_data_);
}
RawAtom::~RawAtom() {
}

uint32 RawAtom::atom_type() const {
  return atom_type_;
}
bool RawAtom::EqualsBody(const BaseAtom& other) const {
  const RawAtom& a = static_cast<const RawAtom&>(other);
  return atom_type_ == a.atom_type_ &&
         raw_data_.Equals(a.raw_data_);
}
void RawAtom::GetSubatoms(vector<const BaseAtom*>& subatoms) const {
}
BaseAtom* RawAtom::Clone() const {
  return new RawAtom(*this);
}
TagDecodeStatus RawAtom::DecodeBody(uint64 size, io::MemoryStream& in,
                                    Decoder& decoder) {
  if ( in.Size() < size ) {
    DATOMLOG << "Not enough data in stream: " << in.Size()
             << " is less than expected: " << size;
    return TAG_DECODE_NO_DATA;
  }
  raw_data_.AppendStream(&in, size);
  return TAG_DECODE_SUCCESS;
}
void RawAtom::EncodeBody(io::MemoryStream& out, Encoder& encoder) const {
  out.AppendStreamNonDestructive(&raw_data_);
}
uint64 RawAtom::MeasureBodySize() const {
  return raw_data_.Size();
}
string RawAtom::ToStringBody(uint32 indent) const {
  ostringstream oss;
  oss << "atom_type: " << strutil::StringPrintf("0x%08x", atom_type_)
      << "(" << Printable4(atom_type_) << ")"
         ", raw_data_: " << raw_data_.Size() << " bytes";
                         //<< raw_data_.DumpContent();
  return oss.str();
}

}
}
