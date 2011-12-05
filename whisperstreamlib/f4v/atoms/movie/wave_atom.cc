
#include "f4v/atoms/movie/wave_atom.h"
#include "f4v/atoms/movie/esds_atom.h"
#include "f4v/atoms/movie/mp4a_atom.h"

namespace streaming {
namespace f4v {

WaveAtom::WaveAtom()
  : ContainerAtom(kType),
    esds_(NULL),
    mp4a_(NULL) {
}
WaveAtom::WaveAtom(const WaveAtom& other)
  : ContainerAtom(other),
    esds_(NULL),
    mp4a_(NULL) {
  DeliverSubatomsToUpperClass();
}
WaveAtom::~WaveAtom() {
}

EsdsAtom* WaveAtom::esds() {
  return esds_;
}
const EsdsAtom* WaveAtom::esds() const {
  return esds_;
}
Mp4aAtom* WaveAtom::mp4a() {
  return mp4a_;
}
const Mp4aAtom* WaveAtom::mp4a() const {
  return mp4a_;
}

void WaveAtom::SubatomFound(BaseAtom* atom) {
  switch( atom->type() ) {
    case ATOM_MP4A:
      mp4a_ = static_cast<Mp4aAtom*>(atom);
      break;
    case ATOM_ESDS:
      esds_ = static_cast<EsdsAtom*>(atom);
      break;
    default:
      WATOMLOG << "Unexpected subatom: " << atom->type_name();
      return;
  }
  return;
}
BaseAtom* WaveAtom::Clone() const {
  return new WaveAtom(*this);
}

}
}
