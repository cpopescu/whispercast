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

#define MAKE_ATOM_TYPE(a,b,c,d) \
  (((int)(a)) << 24) | (((int)(b)) << 16) | (((int)(c)) << 8) | ((int)(d))

enum AtomType {
  ATOM_AVC1 = MAKE_ATOM_TYPE('a', 'v', 'c', '1'),
  ATOM_AVCC = MAKE_ATOM_TYPE('a', 'v', 'c', 'C'),
  ATOM_BLNK = MAKE_ATOM_TYPE('b', 'l', 'n', 'k'),
  ATOM_CHPL = MAKE_ATOM_TYPE('c', 'h', 'p', 'l'),
  ATOM_CO64 = MAKE_ATOM_TYPE('c', 'o', '6', '4'),
  ATOM_CTTS = MAKE_ATOM_TYPE('c', 't', 't', 's'),
  ATOM_DINF = MAKE_ATOM_TYPE('d', 'i', 'n', 'f'),
  ATOM_DLAY = MAKE_ATOM_TYPE('d', 'l', 'a', 'y'),
  ATOM_DRPO = MAKE_ATOM_TYPE('d', 'r', 'p', 'o'),
  ATOM_DRPT = MAKE_ATOM_TYPE('d', 'r', 'p', 't'),
  ATOM_EDTS = MAKE_ATOM_TYPE('e', 'd', 't', 's'),
  ATOM_ELTS = MAKE_ATOM_TYPE('e', 'l', 't', 's'),
  ATOM_ESDS = MAKE_ATOM_TYPE('e', 's', 'd', 's'),
  ATOM_FREE = MAKE_ATOM_TYPE('f', 'r', 'e', 'e'),
  ATOM_FTYP = MAKE_ATOM_TYPE('f', 't', 'y', 'p'),
  ATOM_HCLR = MAKE_ATOM_TYPE('h', 'c', 'l', 'r'),
  ATOM_HDLR = MAKE_ATOM_TYPE('h', 'd', 'l', 'r'),
  ATOM_HLIT = MAKE_ATOM_TYPE('h', 'l', 'i', 't'),
  ATOM_HREF = MAKE_ATOM_TYPE('h', 'r', 'e', 'f'),
  ATOM_ILST = MAKE_ATOM_TYPE('i', 'l', 's', 't'),
  ATOM_KROK = MAKE_ATOM_TYPE('k', 'r', 'o', 'k'),
  ATOM_LOAD = MAKE_ATOM_TYPE('l', 'o', 'a', 'd'),
  ATOM_MDAT = MAKE_ATOM_TYPE('m', 'd', 'a', 't'),
  ATOM_MDHD = MAKE_ATOM_TYPE('m', 'd', 'h', 'd'),
  ATOM_MDIA = MAKE_ATOM_TYPE('m', 'd', 'i', 'a'),
  ATOM_META = MAKE_ATOM_TYPE('m', 'e', 't', 'a'),
  ATOM_MINF = MAKE_ATOM_TYPE('m', 'i', 'n', 'f'),
  ATOM_MOOV = MAKE_ATOM_TYPE('m', 'o', 'o', 'v'),
  ATOM_MP4A = MAKE_ATOM_TYPE('m', 'p', '4', 'a'),
  ATOM_MVHD = MAKE_ATOM_TYPE('m', 'v', 'h', 'd'),
  ATOM_NULL = MAKE_ATOM_TYPE( 0 ,  0 ,  0 ,  0 ), // aka: Terminator Atom
  ATOM_PDIN = MAKE_ATOM_TYPE('p', 'd', 'i', 'n'),
  ATOM_SDTP = MAKE_ATOM_TYPE('s', 'd', 't', 'p'),
  ATOM_SMHD = MAKE_ATOM_TYPE('s', 'm', 'h', 'd'),
  ATOM_STBL = MAKE_ATOM_TYPE('s', 't', 'b', 'l'),
  ATOM_STCO = MAKE_ATOM_TYPE('s', 't', 'c', 'o'),
  ATOM_STSC = MAKE_ATOM_TYPE('s', 't', 's', 'c'),
  ATOM_STSD = MAKE_ATOM_TYPE('s', 't', 's', 'd'),
  ATOM_STSS = MAKE_ATOM_TYPE('s', 't', 's', 's'),
  ATOM_STSZ = MAKE_ATOM_TYPE('s', 't', 's', 'z'),
  ATOM_STTS = MAKE_ATOM_TYPE('s', 't', 't', 's'),
  ATOM_STYL = MAKE_ATOM_TYPE('s', 't', 'y', 'l'),
  ATOM_TBOX = MAKE_ATOM_TYPE('t', 'b', 'o', 'x'),
  ATOM_TKHD = MAKE_ATOM_TYPE('t', 'k', 'h', 'd'),
  ATOM_TRAK = MAKE_ATOM_TYPE('t', 'r', 'a', 'k'),
  ATOM_TWRP = MAKE_ATOM_TYPE('t', 'w', 'r', 'p'),
  ATOM_UDTA = MAKE_ATOM_TYPE('u', 'd', 't', 'a'),
  ATOM_UUID = MAKE_ATOM_TYPE('u', 'u', 'i', 'd'),
  ATOM_VMHD = MAKE_ATOM_TYPE('v', 'm', 'h', 'd'),
  ATOM_WAVE = MAKE_ATOM_TYPE('w', 'a', 'v', 'e'),

  // type for RawAtom: an unknown atom with the regular
  //                   format: <size><type><data..>
  ATOM_RAW  = MAKE_ATOM_TYPE('x', 'x', 'x', 'x'),
};
const string& AtomTypeName(AtomType atom_type);

// The atom type is an unsigned containing 4 bytes.
// These bytes are usually ASCII codes for readable chars.
// Returns: the 4 readable chars in a string.
// e.g. 0x6d6f6f76 = 'm','o','o','v' => return "moov"
//      0x00000063 => return "???c"
string Printable4(uint32 t);

class Encoder;
class Decoder;

// A simple Atom
// The generic F4V atom structure:
//
//  <4 bytes : size> <4 bytes : type> <'size' - 8 bytes : body>
//

class BaseAtom {
 public:
  BaseAtom(AtomType type);
  BaseAtom(const BaseAtom& other);
  virtual ~BaseAtom();

  f4v::AtomType type() const;
  const string& type_name() const;
  uint64 size() const;
  uint64 position() const;
  bool is_extended_size() const;
  // Returns a simple description of the atom, generally useful for log & debug.
  // For a more detailed description, use ToString().
  virtual string base_info() const;

  void set_size(uint64 size);
  void set_extended_size(bool extended_size);

  // Read F4V atom body from input stream.
  // - stream_position: stream index where the atom starts.
  // - atom_size: full atom size (the value advertised in header 'size' field)
  // - extended_header: true for a header with 64 bit 'size' field
  //                    false for a header with 32 bit 'size' field
  // - in: contains just the atom body
  //       (the 'size' and 'type' fields have been cut already)
  // - decoder: keeps the decoding state for stream 'in'.
  //            He is the one that called us. Useful for reading subatoms.
  TagDecodeStatus Decode(uint64 stream_position,
                          uint64 atom_size, bool extended_header,
                          io::MemoryStream& in, Decoder& decoder);
  // Write complete F4V atom in output stream. Writes 'size' and 'type'
  //  fields too.
  // encoder: keeps the encoding state for stream 'out'.
  //          He is the one that called us. Useful for writing subatoms.
  void Encode(io::MemoryStream& out, Encoder& encoder) const;

  // Measures it's own size. Useful if you modified the internal records and
  // you want to update the atom size.
  uint64 MeasureSize() const;

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
  // append contained atoms to "subatoms" vector.
  virtual void GetSubatoms(vector<const BaseAtom*>& subatoms) const = 0;
  // self explanatory
  virtual BaseAtom* Clone() const = 0;
 protected:
  // similar to Decode
  virtual TagDecodeStatus DecodeBody(uint64 size,
                                     io::MemoryStream& in,
                                     Decoder& decoder) = 0;
  // similar to Encode
  virtual void EncodeBody(io::MemoryStream& out, Encoder& encoder) const = 0;
  // returns the measured size of the body
  virtual uint64 MeasureBodySize() const = 0;
  // returns a description of the atom body
  virtual string ToStringBody(uint32 indent) const = 0;

 private:
  const f4v::AtomType type_;
  uint64 size_; // the complete atom size, in bytes
  bool is_extended_size_; // the 'size' field is extended

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
