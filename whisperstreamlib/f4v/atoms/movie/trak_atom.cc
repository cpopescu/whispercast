
#include "f4v/atoms/movie/trak_atom.h"
#include "f4v/atoms/movie/tkhd_atom.h"
#include "f4v/atoms/movie/mdia_atom.h"
#include "f4v/atoms/movie/auto/udta_atom.h"

namespace streaming {
namespace f4v {

TrakAtom::TrakAtom()
  : ContainerAtom(kType),
    tkhd_(NULL),
    mdia_(NULL),
    udta_(NULL) {
}
TrakAtom::TrakAtom(const TrakAtom& other)
  : ContainerAtom(other),
    tkhd_(NULL),
    mdia_(NULL),
    udta_(NULL) {
  DeliverSubatomsToUpperClass();
}
TrakAtom::~TrakAtom() {
}

TkhdAtom* TrakAtom::tkhd() {
  return tkhd_;
}
const TkhdAtom* TrakAtom::tkhd() const {
  return tkhd_;
}
MdiaAtom* TrakAtom::mdia() {
  return mdia_;
}
const MdiaAtom* TrakAtom::mdia() const {
  return mdia_;
}
UdtaAtom* TrakAtom::udta() {
  return udta_;
}
const UdtaAtom* TrakAtom::udta() const {
  return udta_;
}

void TrakAtom::SubatomFound(BaseAtom* atom) {
  switch( atom->type() ) {
    case ATOM_TKHD:
      tkhd_ = static_cast<TkhdAtom*>(atom);
      break;
    case ATOM_MDIA:
      mdia_ = static_cast<MdiaAtom*>(atom);
      break;
    case ATOM_UDTA:
      udta_ = static_cast<UdtaAtom*>(atom);
      break;
    default:
      DATOMLOG << "Unexpected subatom: " << atom->type_name();
      return;
  }
  return;
}
BaseAtom* TrakAtom::Clone() const {
  return new TrakAtom(*this);
}

}
}
