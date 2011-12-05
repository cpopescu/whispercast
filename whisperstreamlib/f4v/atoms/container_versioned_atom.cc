#include "f4v/atoms/container_versioned_atom.h"
#include "f4v/atoms/container_atom.h"
#include "f4v/f4v_util.h"

namespace streaming {
namespace f4v {

ContainerVersionedAtom::ContainerVersionedAtom(AtomType type)
  : VersionedAtom(type),
    subatoms_(),
    debug_subatoms_delivered_(true) {
}
ContainerVersionedAtom::ContainerVersionedAtom(const ContainerVersionedAtom& other)
  : VersionedAtom(other),
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
ContainerVersionedAtom::~ContainerVersionedAtom() {
  for ( vector<BaseAtom*>::iterator it = subatoms_.begin();
        it != subatoms_.end(); ++it ) {
    BaseAtom* atom = *it;
    delete atom;
  }
  subatoms_.clear();
  CHECK(debug_subatoms_delivered_) << " for atom type_name: " << type_name();
}
const vector<BaseAtom*>& ContainerVersionedAtom::subatoms() const {
  return subatoms_;
}

void ContainerVersionedAtom::AddSubatom(BaseAtom* subatom) {
  subatoms_.push_back(subatom);
  SubatomFound(subatom);
}

void ContainerVersionedAtom::DeliverSubatomsToUpperClass() {
  for ( vector<BaseAtom*>::iterator it = subatoms_.begin();
        it != subatoms_.end(); ++it ) {
    BaseAtom* atom = *it;
    SubatomFound(atom);
  }
  debug_subatoms_delivered_ = true;
}
void ContainerVersionedAtom::GetSubatoms(vector<const BaseAtom*>& subatoms) const {
  subatoms.insert(subatoms.end(), subatoms_.begin(), subatoms_.end());
}
TagDecodeStatus ContainerVersionedAtom::DecodeVersionedBody(uint64 size,
                                                            io::MemoryStream& in,
                                                            Decoder& decoder) {
  if ( in.Size() < size ) {
    return TAG_DECODE_NO_DATA;
  }

  // Decode data between version - subatoms.
  //
  int64 in_size_before = in.Size();
  TagDecodeStatus status = DecodeData(size, in, decoder);
  int64 in_size_after = in.Size();
  if ( status != TAG_DECODE_SUCCESS ) {
    // failed
    EATOMLOG << "DecodeData failed, error: " << TagDecodeStatusName(status);
    return TAG_DECODE_ERROR;
  }
  CHECK_GE(in_size_before, in_size_after);
  uint64 read = in_size_before - in_size_after;
  CHECK_LE(read, size);
  uint64 subatoms_size = size - read;

  // Move subatoms data into a separate stream. To limit stream size to the
  // outer ContainerAtom. There can be atoms with size==0, which means that
  // the atom occupies the whole space until EOS.
  io::MemoryStream subatoms_data;
  subatoms_data.AppendStream(&in, subatoms_size);
  CHECK_EQ(subatoms_data.Size(), subatoms_size);
  //WATOMLOG << "Reading subatoms from: " << subatoms_data.DumpContentInline();

  // Decode subatoms.
  //
  while ( !subatoms_data.IsEmpty() ) {
    if ( subatoms_data.Size() == 4 ) {
      // This is not enough for an atom.
      // TODO(cosmin): HACK! HACK! HACK!
      //               In some files, the Avc1AtomAtom has these useless 4
      //               bytes at the end. We're just going to ignore them.
      F4V_LOG_WARNING << "Ignoring useless 4 bytes.";
      break;
    }
    BaseAtom* atom;
    TagDecodeStatus status = decoder.TryReadAtom(subatoms_data, &atom);
    if ( status != TAG_DECODE_SUCCESS ) {
      // failed
      EATOMLOG << "Failed to decode next atom, error: "
               << TagDecodeStatusName(status)
               << ", remaining stream size: " << subatoms_data.Size();
      return TAG_DECODE_ERROR;
    }
    AddSubatom(atom);
  }
  return TAG_DECODE_SUCCESS;
}
void ContainerVersionedAtom::EncodeVersionedBody(io::MemoryStream& out,
                                                 Encoder& encoder) const {
  EncodeData(out, encoder);
  for ( vector<BaseAtom*>::const_iterator it = subatoms_.begin();
        it != subatoms_.end(); ++it ) {
    const BaseAtom& atom = **it;
    encoder.WriteAtom(out, atom);
  }
}
uint64 ContainerVersionedAtom::MeasureVersionedBodySize() const {
  uint64 size = 0;
  for ( uint32 i = 0; i < subatoms_.size(); i++ ) {
    size += subatoms_[i]->MeasureSize();
  }
  return MeasureDataSize() + size;
}
string ContainerVersionedAtom::ToStringVersionedBody(uint32 indent) const {
  ostringstream oss;
  oss << ToStringData(indent) << ", " << subatoms_.size() << " subatoms: " << endl;
  for ( vector<BaseAtom*>::const_iterator it = subatoms_.begin();
        it != subatoms_.end(); ) {
    const BaseAtom& atom = **it;
    oss << util::AtomIndent(indent) << " - subatom: "
        << atom.ToString(indent + 1);
    ++it;
    if ( it != subatoms_.end() ) {
      oss << endl;
    }
  }
  return oss.str();
}

}
}
