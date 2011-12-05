
#include "f4v/atoms/movie/mdia_atom.h"
#include "f4v/atoms/movie/mdhd_atom.h"
#include "f4v/atoms/movie/hdlr_atom.h"
#include "f4v/atoms/movie/minf_atom.h"

namespace streaming {
namespace f4v {

MdiaAtom::MdiaAtom()
  : ContainerAtom(kType),
    mdhd_(NULL),
    hdlr_(NULL),
    minf_(NULL) {
}
MdiaAtom::MdiaAtom(const MdiaAtom& other)
  : ContainerAtom(other),
    mdhd_(NULL),
    hdlr_(NULL),
    minf_(NULL) {
  DeliverSubatomsToUpperClass();
}
MdiaAtom::~MdiaAtom() {
}

MdhdAtom* MdiaAtom::mdhd() {
  return mdhd_;
}
const MdhdAtom* MdiaAtom::mdhd() const {
  return mdhd_;
}
HdlrAtom* MdiaAtom::hdlr() {
  return hdlr_;
}
const HdlrAtom* MdiaAtom::hdlr() const {
  return hdlr_;
}
MinfAtom* MdiaAtom::minf() {
  return minf_;
}
const MinfAtom* MdiaAtom::minf() const {
  return minf_;
}

void MdiaAtom::SubatomFound(BaseAtom* atom) {
  switch ( atom->type() ) {
    case ATOM_MDHD:
      mdhd_ = static_cast<MdhdAtom*>(atom);
      break;
    case ATOM_HDLR:
      hdlr_ = static_cast<HdlrAtom*>(atom);
      break;
    case ATOM_MINF:
      minf_ = static_cast<MinfAtom*>(atom);
      break;
    default:
      WATOMLOG << "Unknown subatom: " << AtomTypeName(atom->type());
      return;
  }
  return;
}
BaseAtom* MdiaAtom::Clone() const {
  return new MdiaAtom(*this);
}

}
}
