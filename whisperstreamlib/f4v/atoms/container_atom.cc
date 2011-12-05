#include "f4v/atoms/container_atom.h"
#include "f4v/f4v_util.h"

namespace streaming {
namespace f4v {

ContainerAtom::ContainerAtom(AtomType type)
  : BaseAtom(type),
    subatoms_(),
    debug_subatoms_delivered_(true) {
}
ContainerAtom::ContainerAtom(const ContainerAtom& other)
  : BaseAtom(other),
    subatoms_(),
    debug_subatoms_delivered_(false) {
  for ( vector<BaseAtom*>::const_iterator it = other.subatoms().begin();
        it != other.subatoms().end(); ++it ) {
    const BaseAtom* other_atom = *it;
    BaseAtom* atom = other_atom->Clone();
    subatoms_.push_back(atom);
    // IMPORTANT NOTE: We cannot call virtual method SubatomFound(..) because
    //                 we are the base class. Instead the upper class's
    //                 copy constructor should call
    //                 DeliverSubatomsToUpperClass(..).
    //SubatomFound(atom);
  }
}
ContainerAtom::~ContainerAtom() {
  for ( vector<BaseAtom*>::iterator it = subatoms_.begin();
        it != subatoms_.end(); ++it ) {
    BaseAtom* atom = *it;
    delete atom;
  }
  subatoms_.clear();
  CHECK(debug_subatoms_delivered_) << " for atom type_name: " << type_name();
}
const vector<BaseAtom*>& ContainerAtom::subatoms() const {
  return subatoms_;
}

void ContainerAtom::AddSubatom(BaseAtom* subatom) {
  subatoms_.push_back(subatom);
  SubatomFound(subatom);
}

void ContainerAtom::DeliverSubatomsToUpperClass() {
  for ( vector<BaseAtom*>::iterator it = subatoms_.begin();
        it != subatoms_.end(); ++it ) {
    BaseAtom* atom = *it;
    SubatomFound(atom);
  }
  debug_subatoms_delivered_ = true;
}
void ContainerAtom::GetSubatoms(vector<const BaseAtom*>& subatoms) const {
  subatoms.insert(subatoms.end(), subatoms_.begin(), subatoms_.end());
}
TagDecodeStatus ContainerAtom::DecodeBody(uint64 size,
                                          io::MemoryStream& in,
                                          Decoder& decoder) {
  if ( in.Size() < size ) {
    return TAG_DECODE_NO_DATA;
  }
  uint64 read = 0;
  while ( read < size ) {
    BaseAtom* atom;
    const int64 in_size_before = in.Size();
    TagDecodeStatus status = decoder.TryReadAtom(in, &atom);
    const int64 in_size_after = in.Size();
    const int64 r = in_size_before - in_size_after;
    if ( status != TAG_DECODE_SUCCESS ) {
      // failed
      EATOMLOG << "Failed to decode next atom, error: "
               << TagDecodeStatusName(status);
      return TAG_DECODE_ERROR;
    }
    read += r;
    AddSubatom(atom);
  }
  CHECK(read == size) << "Subatoms size error, stream size: " << size
                      << ", decoded so far: " << read
                      << ", this: " << this->ToString();
  return TAG_DECODE_SUCCESS;
}
void ContainerAtom::EncodeBody(io::MemoryStream& out, Encoder& encoder) const {
  for ( vector<BaseAtom*>::const_iterator it = subatoms_.begin();
        it != subatoms_.end(); ++it ) {
    const BaseAtom& atom = **it;
    encoder.WriteAtom(out, atom);
  }
}
uint64 ContainerAtom::MeasureBodySize() const {
  uint64 size = 0;
  for ( uint32 i = 0; i < subatoms_.size(); i++ ) {
    size += subatoms_[i]->MeasureSize();
  }
  return size;
}
string ContainerAtom::ToStringBody(uint32 indent) const {
  ostringstream oss;
  oss << subatoms_.size() << " subatoms: " << endl;
  for ( vector<BaseAtom*>::const_iterator it = subatoms_.begin();
        it != subatoms_.end(); ) {
    const BaseAtom& atom = **it;
    oss << util::AtomIndent(indent) << " - subatom: " << atom.ToString(indent + 1);
    ++it;
    if ( it != subatoms_.end() ) {
      oss << endl;
    }
  }
  return oss.str();
}

}
}
