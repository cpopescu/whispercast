
#include "f4v/atoms/base_atom.h"
#include "f4v/atoms/raw_atom.h"

namespace streaming {
namespace f4v {
#define CONSIDER_STR(type) case type: { static const string str(#type); return str; }
const string& AtomTypeName(AtomType atom_type) {
  switch ( atom_type ) {
    CONSIDER_STR(ATOM_AVC1);
    CONSIDER_STR(ATOM_AVCC);
    CONSIDER_STR(ATOM_BLNK);
    CONSIDER_STR(ATOM_CHPL);
    CONSIDER_STR(ATOM_CO64);
    CONSIDER_STR(ATOM_CTTS);
    CONSIDER_STR(ATOM_DINF);
    CONSIDER_STR(ATOM_DLAY);
    CONSIDER_STR(ATOM_DRPO);
    CONSIDER_STR(ATOM_DRPT);
    CONSIDER_STR(ATOM_EDTS);
    CONSIDER_STR(ATOM_ELTS);
    CONSIDER_STR(ATOM_ESDS);
    CONSIDER_STR(ATOM_FREE);
    CONSIDER_STR(ATOM_FTYP);
    CONSIDER_STR(ATOM_HCLR);
    CONSIDER_STR(ATOM_HDLR);
    CONSIDER_STR(ATOM_HLIT);
    CONSIDER_STR(ATOM_HREF);
    CONSIDER_STR(ATOM_ILST);
    CONSIDER_STR(ATOM_KROK);
    CONSIDER_STR(ATOM_LOAD);
    CONSIDER_STR(ATOM_MDAT);
    CONSIDER_STR(ATOM_MDHD);
    CONSIDER_STR(ATOM_MDIA);
    CONSIDER_STR(ATOM_META);
    CONSIDER_STR(ATOM_MINF);
    CONSIDER_STR(ATOM_MOOV);
    CONSIDER_STR(ATOM_MP4A);
    CONSIDER_STR(ATOM_MVHD);
    CONSIDER_STR(ATOM_NULL);
    CONSIDER_STR(ATOM_PDIN);
    CONSIDER_STR(ATOM_SDTP);
    CONSIDER_STR(ATOM_SMHD);
    CONSIDER_STR(ATOM_STBL);
    CONSIDER_STR(ATOM_STCO);
    CONSIDER_STR(ATOM_STSC);
    CONSIDER_STR(ATOM_STSD);
    CONSIDER_STR(ATOM_STSS);
    CONSIDER_STR(ATOM_STSZ);
    CONSIDER_STR(ATOM_STTS);
    CONSIDER_STR(ATOM_STYL);
    CONSIDER_STR(ATOM_TBOX);
    CONSIDER_STR(ATOM_TKHD);
    CONSIDER_STR(ATOM_TRAK);
    CONSIDER_STR(ATOM_TWRP);
    CONSIDER_STR(ATOM_UDTA);
    CONSIDER_STR(ATOM_UUID);
    CONSIDER_STR(ATOM_VMHD);
    CONSIDER_STR(ATOM_WAVE);

    CONSIDER_STR(ATOM_RAW);
  }
  LOG_FATAL << "Illegal atom_type: " << atom_type;
  static const string kUnknown("Unknown");
  return kUnknown;
}

namespace {
char PrintableChar(char c) {
  return c >= 32 && c <= 126 ? c : '?';
}
}
string Printable4(uint32 t) {
  return strutil::StringPrintf("%c%c%c%c",
    PrintableChar((char)((t >> 24) & 0xff)),
    PrintableChar((char)((t >> 16) & 0xff)),
    PrintableChar((char)((t >>  8) & 0xff)),
    PrintableChar((char)((t      ) & 0xff)));
}

BaseAtom::BaseAtom(f4v::AtomType type)
  : type_(type), size_(0), is_extended_size_(false), position_(0) {
}
BaseAtom::BaseAtom(const BaseAtom& other)
  : type_(other.type()),
    size_(other.size()),
    is_extended_size_(other.is_extended_size()),
    position_(other.position()) {
}
BaseAtom::~BaseAtom() {
}

f4v::AtomType BaseAtom::type() const {
  return type_;
}
const string& BaseAtom::type_name() const {
  return f4v::AtomTypeName(type());
}
uint64 BaseAtom::size() const {
  return size_;
}
uint64 BaseAtom::position() const {
  return position_;
}
bool BaseAtom::is_extended_size() const {
  return is_extended_size_;
}
string BaseAtom::base_info() const {
  ostringstream oss;
  oss << type_name() << " @" << position() << " "
      << size() << " bytes" << (is_extended_size() ? "(extended)" : "");
  return oss.str();
}
void BaseAtom::set_size(uint64 size) {
  size_ = size;
  is_extended_size_ = size > kMaxUInt32;
}
void BaseAtom::set_extended_size(bool extended_size) {
  CHECK(extended_size || size_ < kMaxUInt32) << "size_: " << size_
    << " is to big for non-extended size";
  is_extended_size_ = extended_size;
}

TagDecodeStatus BaseAtom::Decode(uint64 stream_position,
                                  uint64 atom_size, bool extended_header,
                                  io::MemoryStream& in, Decoder& decoder) {
  position_ = stream_position;
  size_ = atom_size;
  is_extended_size_ = extended_header;

  const uint64 header_size = extended_header ? 16 : 8;
  if ( atom_size < header_size ) {
    EATOMLOG << "Atom size too small: " << atom_size;
    return TAG_DECODE_ERROR;
  }
  const uint64 body_size = atom_size - header_size;
  // TODO(cosmin): Don't check in.Size() >= body_size,
  //               we don't want the whole MDAT in stream.

  if ( extended_header && atom_size < kMaxUInt32 ) {
    WATOMLOG << "Extended 'size' header with a low value: " << atom_size;
  }

  const int32 size_before_decode = in.Size();
  TagDecodeStatus result = DecodeBody(body_size, in, decoder);
  const int32 size_after_decode = in.Size();
  const int32 decoded_size = size_before_decode - size_after_decode;
  if ( decoded_size > body_size ) {
    FATOMLOG << "Too many bytes decoded, body_size: " << body_size
             << ", decoded_size: " << decoded_size;
  }

  return result;
}

void BaseAtom::Encode(io::MemoryStream& out, Encoder& encoder) const {
  uint32 atom_type = type();
  if ( atom_type == ATOM_RAW ) {
    atom_type = static_cast<const RawAtom*>(this)->atom_type();
  }
  uint32 atom_size = size();

  if ( is_extended_size() ) {
    io::NumStreamer::WriteUInt32(&out, 1, common::BIGENDIAN);
    io::NumStreamer::WriteUInt32(&out, atom_type, common::BIGENDIAN);
    io::NumStreamer::WriteUInt64(&out, atom_size, common::BIGENDIAN);
  } else {
    io::NumStreamer::WriteUInt32(&out, atom_size, common::BIGENDIAN);
    io::NumStreamer::WriteUInt32(&out, atom_type, common::BIGENDIAN);
  }
  EncodeBody(out, encoder);
}

uint64 BaseAtom::MeasureSize() const {
  return (is_extended_size() ? 16 : 8) + MeasureBodySize();
}
string BaseAtom::ToString(uint32 indent) const {
  ostringstream oss;
  oss << type_name()
      << " type: " << type() << "(" << type_name() << ")"
         ", size: " << size() << (is_extended_size() ? "(extended)" : "")
      << ", offset: " << position()
      << ", " << ToStringBody(indent);
  return oss.str();
}
string BaseAtom::ToString() const {
  return ToString(0);
}

}
}
