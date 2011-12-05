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
// Author: Cosmin Tudorache

#ifndef __MEDIA_F4V_ATOMS_ATOM_FTYP_H__
#define __MEDIA_F4V_ATOMS_ATOM_FTYP_H__

#include <string>
#include <whisperstreamlib/f4v/atoms/base_atom.h>

namespace streaming {
namespace f4v {

class FtypAtom : public BaseAtom {
 public:
  static const AtomType kType = ATOM_FTYP;
  static const uint32 kBodyMinSize = 8;
 public:
  FtypAtom();
  FtypAtom(const FtypAtom& other);
  virtual ~FtypAtom();

  uint32 major_brand() const;
  uint32 minor_version() const;
  const vector<uint32>& compatible_brands() const;
  vector<uint32>& mutable_compatible_brands();

  void set_major_brand(uint32 major_brand);
  void set_minor_version(uint32 minor_version);

  ///////////////////////////////////////////////////////////////////////////
  // Methods from AtomBase
  virtual void GetSubatoms(vector<const BaseAtom*>& subatoms) const;
  virtual BaseAtom* Clone() const;
  virtual TagDecodeStatus DecodeBody(uint64 size, io::MemoryStream& in,
                                     Decoder& decoder);
  virtual void EncodeBody(io::MemoryStream& out, Encoder& encoder) const;
  virtual uint64 MeasureBodySize() const;
  virtual string ToStringBody(uint32 indent) const;

 private:
  uint32 major_brand_;   // main type (or "brand" as they prefer to call it)
  uint32 minor_version_; // the version of the brand
  vector<uint32> compatible_brands_;
};
}
}

#endif // __MEDIA_F4V_ATOMS_ATOM_FTYP_H__
