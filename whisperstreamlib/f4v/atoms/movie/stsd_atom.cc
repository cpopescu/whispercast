
#include "f4v/atoms/movie/stsd_atom.h"

#include "f4v/atoms/movie/avc1_atom.h"
#include "f4v/atoms/movie/mp4a_atom.h"

namespace streaming {
namespace f4v {

StsdAtom::StsdAtom()
  : ContainerVersionedAtom(kType),
    count_(0),
    avc1_(NULL),
    mp4a_(NULL) {
}
StsdAtom::StsdAtom(const StsdAtom& other)
  : ContainerVersionedAtom(other),
    count_(other.count_),
    avc1_(NULL),
    mp4a_(NULL) {
  DeliverSubatomsToUpperClass();
}
StsdAtom::~StsdAtom() {
}
const Avc1Atom* StsdAtom::avc1() const {
  return avc1_;
}
const Mp4aAtom* StsdAtom::mp4a() const {
  return mp4a_;
}
void StsdAtom::SubatomFound(BaseAtom* atom) {
  switch ( atom->type() ) {
    case ATOM_AVC1: avc1_ = static_cast<Avc1Atom*>(atom); break;
    case ATOM_MP4A: mp4a_ = static_cast<Mp4aAtom*>(atom); break;
    default:
      WATOMLOG << "Unknown subatom: " << *atom;
      break;
  };
}
BaseAtom* StsdAtom::Clone() const {
  return new StsdAtom(*this);
}
TagDecodeStatus StsdAtom::DecodeData(uint64 size, io::MemoryStream& in,
                                     Decoder& decoder) {
  if ( size < kDataSize ) {
    EATOMLOG << "Cannot decode from " << size << " bytes"
                ", expected at least " << kDataSize << " bytes";
    return TAG_DECODE_ERROR;
  }
  count_ = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  return TAG_DECODE_SUCCESS;
}
void StsdAtom::EncodeData(io::MemoryStream& out,
                          Encoder& encoder) const {
  io::NumStreamer::WriteUInt32(&out, count_, common::BIGENDIAN);
}
uint64 StsdAtom::MeasureDataSize() const {
  return kDataSize;
}
string StsdAtom::ToStringData(uint32 indent) const {
  ostringstream oss;
  oss << "count_: " << count_;
  return oss.str();
}

}
}
