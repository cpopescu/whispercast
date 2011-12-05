
#include "f4v/atoms/movie/moov_atom.h"
#include "f4v/atoms/movie/mvhd_atom.h"
#include "f4v/atoms/movie/trak_atom.h"
#include "f4v/atoms/movie/auto/udta_atom.h"
#include "f4v/atoms/movie/auto/meta_atom.h"
#include "f4v/atoms/movie/tkhd_atom.h"

namespace streaming {
namespace f4v {

MoovAtom::MoovAtom()
  : ContainerAtom(kType),
    mvhd_(NULL),
    traks_(),
    udta_(NULL),
    meta_(NULL) {
}
MoovAtom::MoovAtom(const MoovAtom& other)
  : ContainerAtom(other),
    mvhd_(NULL),
    traks_(),
    udta_(NULL),
    meta_(NULL) {
  DeliverSubatomsToUpperClass();
}
MoovAtom::~MoovAtom() {
}

MvhdAtom* MoovAtom::mvhd() {
  return mvhd_;
}
const MvhdAtom* MoovAtom::mvhd() const {
  return mvhd_;
}
vector<TrakAtom*>& MoovAtom::traks() {
  return traks_;
}
const vector<TrakAtom*>& MoovAtom::traks() const {
  return traks_;
}
TrakAtom* MoovAtom::trak(bool audio) {
  for ( vector<TrakAtom*>::iterator it = traks_.begin();
        it != traks_.end(); ++it ) {
    TrakAtom* trak = *it;
    if ( trak->tkhd() == NULL ) {
      continue;
    }
    const TkhdAtom& tkhd = *trak->tkhd();
    if ( ( audio && tkhd.volume() != 0 ) ||
         (!audio && tkhd.volume() == 0 ) ) {
      return trak;
    }
  }
  return NULL;
}
const TrakAtom* MoovAtom::trak(bool audio) const {
  for ( vector<TrakAtom*>::const_iterator it = traks_.begin();
        it != traks_.end(); ++it ) {
    const TrakAtom* trak = *it;
    if ( trak->tkhd() == NULL ) {
      continue;
    }
    const TkhdAtom& tkhd = *trak->tkhd();
    if ( ( audio && tkhd.volume() != 0 ) ||
         (!audio && tkhd.volume() == 0 ) ) {
      return trak;
    }
  }
  return NULL;
}
UdtaAtom* MoovAtom::udta() {
  return udta_;
}
const UdtaAtom* MoovAtom::udta() const {
  return udta_;
}
MetaAtom* MoovAtom::meta() {
  return meta_;
}
const MetaAtom* MoovAtom::meta() const {
  return meta_;
}

void MoovAtom::SubatomFound(BaseAtom* atom) {
  switch ( atom->type() ) {
    case ATOM_MVHD:
      mvhd_ = static_cast<MvhdAtom*>(atom);
      break;
    case ATOM_TRAK:
      traks_.push_back(static_cast<TrakAtom*>(atom));
      break;
    case ATOM_UDTA:
      udta_ = static_cast<UdtaAtom*>(atom);
      break;
    default:
      WATOMLOG << "Unknown subatom: " << AtomTypeName(atom->type());
      return;
  }
  return;
}
BaseAtom* MoovAtom::Clone() const {
  return new MoovAtom(*this);
}

}
}
