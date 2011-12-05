
#include "f4v/atoms/movie/mdat_atom.h"

namespace streaming {
namespace f4v {

MdatAtom::MdatAtom()
  : BaseAtom(kType) {
}
MdatAtom::MdatAtom(const MdatAtom& other)
  : BaseAtom(other) {
}
MdatAtom::~MdatAtom() {
}
void MdatAtom::GetSubatoms(vector<const BaseAtom*>& subatoms) const {
}
BaseAtom* MdatAtom::Clone() const {
  return new MdatAtom(*this);
}
TagDecodeStatus MdatAtom::DecodeBody(uint64 size, io::MemoryStream& in,
                                     Decoder& decoder) {
  // We don't check for in.Readable() size because we're not going to
  // read the body.
  // Instead, the Decoder will acknowledge the start of MDAT atom and will
  // start reading frames from the MDAT body.
  return TAG_DECODE_SUCCESS;
}
void MdatAtom::EncodeBody(io::MemoryStream& out, Encoder& encoder) const {
  // Similar to DecodeBody, we have nothing to encode here.
  // The Encoder will acknowledge the start of MDAT atom and will start
  // encoding frames in MDAT body.
}
uint64 MdatAtom::MeasureBodySize() const {
  return 0;
}
string MdatAtom::ToStringBody(uint32 indent) const {
  ostringstream oss;
  oss << "<frames not included>";
  return oss.str();
}

}
}
