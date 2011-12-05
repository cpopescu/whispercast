
#include "f4v/atoms/movie/stbl_atom.h"
#include "f4v/atoms/movie/stsd_atom.h"
#include "f4v/atoms/movie/stts_atom.h"
#include "f4v/atoms/movie/stss_atom.h"
#include "f4v/atoms/movie/stsc_atom.h"
#include "f4v/atoms/movie/stsz_atom.h"
#include "f4v/atoms/movie/stco_atom.h"
#include "f4v/atoms/movie/ctts_atom.h"
#include "f4v/atoms/movie/co64_atom.h"

namespace streaming {
namespace f4v {

StblAtom::StblAtom()
  : ContainerAtom(kType),
    stsd_(NULL),
    stts_(NULL),
    stss_(NULL),
    stsc_(NULL),
    stsz_(NULL),
    stco_(NULL),
    ctts_(NULL),
    co64_(NULL) {
}
StblAtom::StblAtom(const StblAtom& other)
  : ContainerAtom(other),
    stsd_(NULL),
    stts_(NULL),
    stss_(NULL),
    stsc_(NULL),
    stsz_(NULL),
    stco_(NULL),
    ctts_(NULL),
    co64_(NULL) {
  DeliverSubatomsToUpperClass();
}
StblAtom::~StblAtom() {
}

StsdAtom* StblAtom::stsd() {
  return stsd_;
}
const StsdAtom* StblAtom::stsd() const {
  return stsd_;
}
SttsAtom* StblAtom::stts() {
  return stts_;
}
const SttsAtom* StblAtom::stts() const {
  return stts_;
}
StssAtom* StblAtom::stss() {
  return stss_;
}
const StssAtom* StblAtom::stss() const {
  return stss_;
}
StscAtom* StblAtom::stsc() {
  return stsc_;
}
const StscAtom* StblAtom::stsc() const {
  return stsc_;
}
StszAtom* StblAtom::stsz() {
  return stsz_;
}
const StszAtom* StblAtom::stsz() const {
  return stsz_;
}
StcoAtom* StblAtom::stco() {
  return stco_;
}
const StcoAtom* StblAtom::stco() const {
  return stco_;
}
CttsAtom* StblAtom::ctts() {
  return ctts_;
}
const CttsAtom* StblAtom::ctts() const {
  return ctts_;
}
Co64Atom* StblAtom::co64() {
  return co64_;
}
const Co64Atom* StblAtom::co64() const {
  return co64_;
}

void StblAtom::SubatomFound(BaseAtom* atom) {
  switch ( atom->type() ) {
    case ATOM_STSD:
      stsd_ = static_cast<StsdAtom*>(atom);
      break;
    case ATOM_STTS:
      stts_ = static_cast<SttsAtom*>(atom);
      break;
    case ATOM_STSS:
      stss_ = static_cast<StssAtom*>(atom);
      break;
    case ATOM_STSC:
      stsc_ = static_cast<StscAtom*>(atom);
      break;
    case ATOM_STSZ:
      stsz_ = static_cast<StszAtom*>(atom);
      break;
    case ATOM_STCO:
      stco_ = static_cast<StcoAtom*>(atom);
      break;
    case ATOM_CTTS:
      ctts_ = static_cast<CttsAtom*>(atom);
      break;
    case ATOM_CO64:
      co64_ = static_cast<Co64Atom*>(atom);
      break;
    default:
      break;
  }
}
BaseAtom* StblAtom::Clone() const {
  return new StblAtom(*this);
}

}
}
