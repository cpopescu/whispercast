
#include "f4v/atoms/movie/minf_atom.h"
#include "f4v/atoms/movie/vmhd_atom.h"
#include "f4v/atoms/movie/smhd_atom.h"
#include "f4v/atoms/movie/auto/dinf_atom.h"
#include "f4v/atoms/movie/stbl_atom.h"

namespace streaming {
namespace f4v {

MinfAtom::MinfAtom()
  : ContainerAtom(kType),
    vmhd_(NULL),
    smhd_(NULL),
    dinf_(NULL),
    stbl_(NULL) {
}
MinfAtom::MinfAtom(const MinfAtom& other)
  : ContainerAtom(other),
    vmhd_(NULL),
    smhd_(NULL),
    dinf_(NULL),
    stbl_(NULL) {
  DeliverSubatomsToUpperClass();
}
MinfAtom::~MinfAtom() {
}

VmhdAtom* MinfAtom::vmhd() {
  return vmhd_;
}
const VmhdAtom* MinfAtom::vmhd() const {
  return vmhd_;
}
SmhdAtom* MinfAtom::smhd() {
  return smhd_;
}
const SmhdAtom* MinfAtom::smhd() const {
  return smhd_;
}
DinfAtom* MinfAtom::dinf() {
  return dinf_;
}
const DinfAtom* MinfAtom::dinf() const {
  return dinf_;
}
StblAtom* MinfAtom::stbl() {
  return stbl_;
}
const StblAtom* MinfAtom::stbl() const {
  return stbl_;
}

void MinfAtom::SubatomFound(BaseAtom* atom) {
  switch ( atom->type() ) {
    case ATOM_VMHD:
      if ( vmhd_ != NULL ) {
        EATOMLOG << "Multiple VMHD subatoms.";
        break;
      }
      vmhd_ = static_cast<VmhdAtom*>(atom);
      break;
    case ATOM_SMHD:
      if ( smhd_ != NULL ) {
        EATOMLOG << "Multiple SMHD subatoms.";
        break;
      }
      smhd_ = static_cast<SmhdAtom*>(atom);
      break;
    case ATOM_DINF:
      if ( dinf_ != NULL ) {
        EATOMLOG << "Multiple DINF subatoms.";
        break;
      }
      dinf_ = static_cast<DinfAtom*>(atom);
      break;
    case ATOM_STBL:
      if ( stbl_ != NULL ) {
        EATOMLOG << "Multiple STBL subatoms.";
        break;
      }
      stbl_ = static_cast<StblAtom*>(atom);
      break;
    default:
      break;
  }
}
BaseAtom* MinfAtom::Clone() const {
  return new MinfAtom(*this);
}

}
}
