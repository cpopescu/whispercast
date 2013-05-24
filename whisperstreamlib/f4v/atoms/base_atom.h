// Copyright (c) 2009, Whispersoft s.r.l.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
// * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
// * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
// * Neither the name of Whispersoft s.r.l. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//

#ifndef __MEDIA_F4V_ATOMS_ATOM_BASE_H__
#define __MEDIA_F4V_ATOMS_ATOM_BASE_H__

#include <string>
#include <whisperlib/common/base/types.h>
#include <whisperstreamlib/f4v/f4v_encoder.h>
#include <whisperstreamlib/f4v/f4v_decoder.h>
#include <whisperstreamlib/base/tag.h>

#define DATOMLOG F4V_LOG_DEBUG << this->base_info() << ": "
#define IATOMLOG F4V_LOG_INFO << this->base_info() << ": "
#define WATOMLOG F4V_LOG_WARNING << this->base_info() << ": "
#define EATOMLOG F4V_LOG_ERROR << this->base_info() << ": "
#define FATOMLOG F4V_LOG_FATAL << this->base_info() << ": "

namespace streaming {
namespace f4v {

enum AtomType {
  ATOM_AVC1 = FourCC<'a', 'v', 'c', '1'>::value,
  ATOM_AVCC = FourCC<'a', 'v', 'c', 'C'>::value,
  ATOM_BLNK = FourCC<'b', 'l', 'n', 'k'>::value,
  ATOM_CHPL = FourCC<'c', 'h', 'p', 'l'>::value,
  ATOM_CO64 = FourCC<'c', 'o', '6', '4'>::value,
  ATOM_CTTS = FourCC<'c', 't', 't', 's'>::value,
  ATOM_DINF = FourCC<'d', 'i', 'n', 'f'>::value,
  ATOM_DLAY = FourCC<'d', 'l', 'a', 'y'>::value,
  ATOM_DRPO = FourCC<'d', 'r', 'p', 'o'>::value,
  ATOM_DRPT = FourCC<'d', 'r', 'p', 't'>::value,
  ATOM_EDTS = FourCC<'e', 'd', 't', 's'>::value,
  ATOM_ELTS = FourCC<'e', 'l', 't', 's'>::value,
  ATOM_ESDS = FourCC<'e', 's', 'd', 's'>::value,
  ATOM_FREE = FourCC<'f', 'r', 'e', 'e'>::value,
  ATOM_FTYP = FourCC<'f', 't', 'y', 'p'>::value,
  ATOM_HCLR = FourCC<'h', 'c', 'l', 'r'>::value,
  ATOM_HDLR = FourCC<'h', 'd', 'l', 'r'>::value,
  ATOM_HLIT = FourCC<'h', 'l', 'i', 't'>::value,
  ATOM_HREF = FourCC<'h', 'r', 'e', 'f'>::value,
  ATOM_ILST = FourCC<'i', 'l', 's', 't'>::value,
  ATOM_KROK = FourCC<'k', 'r', 'o', 'k'>::value,
  ATOM_LOAD = FourCC<'l', 'o', 'a', 'd'>::value,
  ATOM_MDAT = FourCC<'m', 'd', 'a', 't'>::value,
  ATOM_MDHD = FourCC<'m', 'd', 'h', 'd'>::value,
  ATOM_MDIA = FourCC<'m', 'd', 'i', 'a'>::value,
  ATOM_META = FourCC<'m', 'e', 't', 'a'>::value,
  ATOM_MINF = FourCC<'m', 'i', 'n', 'f'>::value,
  ATOM_MOOV = FourCC<'m', 'o', 'o', 'v'>::value,
  ATOM_MP4A = FourCC<'m', 'p', '4', 'a'>::value,
  ATOM_MVHD = FourCC<'m', 'v', 'h', 'd'>::value,
  ATOM_NULL = FourCC< 0,   0 ,  0 ,  0 >::value, // aka: Terminator Atom
  ATOM_PDIN = FourCC<'p', 'd', 'i', 'n'>::value,
  ATOM_SDTP = FourCC<'s', 'd', 't', 'p'>::value,
  ATOM_SMHD = FourCC<'s', 'm', 'h', 'd'>::value,
  ATOM_STBL = FourCC<'s', 't', 'b', 'l'>::value,
  ATOM_STCO = FourCC<'s', 't', 'c', 'o'>::value,
  ATOM_STSC = FourCC<'s', 't', 's', 'c'>::value,
  ATOM_STSD = FourCC<'s', 't', 's', 'd'>::value,
  ATOM_STSS = FourCC<'s', 't', 's', 's'>::value,
  ATOM_STSZ = FourCC<'s', 't', 's', 'z'>::value,
  ATOM_STTS = FourCC<'s', 't', 't', 's'>::value,
  ATOM_STYL = FourCC<'s', 't', 'y', 'l'>::value,
  ATOM_TBOX = FourCC<'t', 'b', 'o', 'x'>::value,
  ATOM_TKHD = FourCC<'t', 'k', 'h', 'd'>::value,
  ATOM_TRAK = FourCC<'t', 'r', 'a', 'k'>::value,
  ATOM_TWRP = FourCC<'t', 'w', 'r', 'p'>::value,
  ATOM_UDTA = FourCC<'u', 'd', 't', 'a'>::value,
  ATOM_UUID = FourCC<'u', 'u', 'i', 'd'>::value,
  ATOM_VMHD = FourCC<'v', 'm', 'h', 'd'>::value,
  ATOM_WAVE = FourCC<'w', 'a', 'v', 'e'>::value,

  // type for RawAtom: an unknown atom with the regular
  //                   format: <size><type><data..>
  ATOM_RAW  = FourCC<'x', 'x', 'x', 'x'>::value,
};
const string& AtomTypeName(AtomType atom_type);

// The atom type is an unsigned containing 4 bytes.
// These bytes are usually ASCII codes for readable chars.
// Returns: the 4 readable chars in a string.
// e.g. 0x6d6f6f76 = 'm','o','o','v' => return "moov"
//      0x00000063 => return "???c"
string Printable4(uint32 t);

template <typename T>
bool AllEquals(vector<T>& a, const vector<T>& b) {
  if ( a.size() != b.size() ) { return false; }
  for ( uint32 i = 0; i < a.size(); i++ ) {
    if ( !a[i].Equals(b[i]) ) {
      return false;
    }
  }
  return true;
}
template <typename T>
bool AllEqualsP(const vector<T*>& a, const vector<T*>& b) {
  if ( a.size() != b.size() ) { return false; }
  for ( uint32 i = 0; i < a.size(); i++ ) {
    if ( !a[i]->Equals(*b[i]) ) {
      return false;
    }
  }
  return true;
}

class Encoder;
class Decoder;

// A simple Atom
// The generic F4V atom structure:
//
//  <4 bytes : size> <4 bytes : type> <'size' - 8 bytes : body>
//

class BaseAtom {
 public:
  // returns true if the 'body_size' needs 64bit storage
  static bool IsExtendedSize(uint64 body_size);
  // returns 8 (normal) or 16 (extended)
  static uint32 HeaderSize(bool is_extended);
 public:
  BaseAtom(AtomType type);
  BaseAtom(const BaseAtom& other);
  virtual ~BaseAtom();

  f4v::AtomType type() const;
  const string& type_name() const;
  uint64 body_size() const;
  uint64 position() const;
  // Returns a simple description of the atom, generally useful for log & debug.
  // For a more detailed description, use ToString().
  virtual string base_info() const;

  void set_body_size(uint64 body_size);
  void set_force_extended_body_size(bool value);

  // is the atom's size extended (i.e. needs 64 bit storage)
  bool is_extended_size() const;
  // atom's header size
  uint32 header_size() const;
  // atom's full size (body size + header size)
  uint64 size() const;

  // Read F4V atom body from input stream.
  // - stream_position: stream index where the atom starts.
  // - body_size: atom's body size
  //              (the value advertised in header 'size' field - header size)
  // - in: contains just the atom body
  //       (the 'size' and 'type' fields have been already cut)
  // - decoder: keeps the decoding state for stream 'in'.
  //            He is the one that called us. Useful for reading subatoms.
  TagDecodeStatus Decode(uint64 stream_position, uint64 body_size,
                         io::MemoryStream& in, Decoder& decoder);
  // Write complete F4V atom in output stream. Writes 'size' and 'type'
  //  fields too.
  // encoder: keeps the encoding state for stream 'out'.
  //          He is the one that called us. Useful for writing subatoms.
  void Encode(io::MemoryStream& out, Encoder& encoder) const;

  // equality test, especially useful in testing.
  bool Equals(const BaseAtom& other) const;

  // Measures it's own size. Useful if you modified the internal records and
  // you want to update the atom size.
  uint64 MeasureSize() const;

  // Recursively measures & updates it's own size and that of it's subatoms.
  // TODO(cosmin): try to get rid of this; it's easily forgettable to update
  //               the size after each atom modification. The size should
  //               automatically update.
  void UpdateSize();

  // Returns a description of the atom.
  // Uses the given indent to print subatoms nicely.
  string ToString(uint32 indent) const;

  // Returns a description of the atom.
  string ToString() const;

  /////////////////////////////////////////////////////////////////////
  // These methods have to be implemented in upper "Atom" class.
  // The Decode, Encode and ToString call these methods to communicate
  //  with the upper "Atom" class.
 public:
  // Equality test; the type()==other.type() has already been checked
  virtual bool EqualsBody(const BaseAtom& other) const = 0;
  // append contained atoms to "subatoms" vector.
  virtual void GetSubatoms(vector<const BaseAtom*>& subatoms) const = 0;
  // self explanatory
  virtual BaseAtom* Clone() const = 0;
  // returns the measured encoding size of the body (in mp4 file format)
  virtual uint64 MeasureBodySize() const = 0;
 protected:
  // similar to Decode
  virtual TagDecodeStatus DecodeBody(uint64 size,
                                     io::MemoryStream& in,
                                     Decoder& decoder) = 0;
  // similar to Encode
  virtual void EncodeBody(io::MemoryStream& out, Encoder& encoder) const = 0;
  // returns a description of the atom body
  virtual string ToStringBody(uint32 indent) const = 0;

 private:
  const f4v::AtomType type_;

  // TODO(cosmin): try to get rid of this; The size should
  //               not be separated from the internal structure.
  // the atom's body size, in bytes (without the header
  // which is 8 or 16 depending on body_size_ < kMaxUInt32).
  uint64 body_size_;

  // false = use extended size according to body_size_
  // true = use extended size always (no matter what the body_size_ is)
  bool force_extended_size_;

  // The index in stream where this atom was found.
  // For debug only.
  uint64 position_;
};
}
}

inline ostream& operator<<(ostream& os, const streaming::f4v::BaseAtom& atom) {
  return os << atom.ToString();
}
inline ostream& operator<<(ostream& os, streaming::f4v::AtomType atom_type) {
  return os << strutil::StringPrintf("0x%08x", atom_type);
}

#endif // __MEDIA_F4V_ATOMS_ATOM_BASE_H__
