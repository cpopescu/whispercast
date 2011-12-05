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
#ifndef __MEDIA_F4V_ATOMS_VERSIONED_ATOM_H__
#define __MEDIA_F4V_ATOMS_VERSIONED_ATOM_H__

#include <string>
#include <whisperstreamlib/f4v/f4v_encoder.h>
#include <whisperstreamlib/f4v/f4v_decoder.h>
#include <whisperstreamlib/f4v/atoms/base_atom.h>

// A simple atom with with version and flags.
// Also known as "full atom".

namespace streaming {
namespace f4v {
class VersionedAtom : public BaseAtom {
 public:
  VersionedAtom(AtomType type);
  VersionedAtom(const VersionedAtom& other);
  virtual ~VersionedAtom();

  uint8 version() const;
  const uint8* flags() const;

  void set_version(uint8 version);
  void set_flags(uint8 f0, uint8 f1, uint8 f2);
  void set_flags(const uint8* flags);

  // because we want the base info to include version numbers
  virtual string base_info() const;

  ////////////////////////////////////////////////////////////
  // These methods have to be implemented in upper "Atom" class.
  // The DecodeBody, EncodeBody and ToStringBody call these methods
  //  to communicate with the upper "Atom" class.
  //
 protected:
  // 'in' contains atom body without the 'version' header
  virtual TagDecodeStatus DecodeVersionedBody(uint64 size,
                                               io::MemoryStream& in,
                                               Decoder& decoder) = 0;
  // write atom body without the 'version' header
  virtual void EncodeVersionedBody(io::MemoryStream& out,
                                         Encoder& encoder) const = 0;
  // returns the size of the atom body, without the 'version' header
  virtual uint64 MeasureVersionedBodySize() const = 0;
  // returns a description of the atom, without the version header
  virtual string ToStringVersionedBody(uint32 indent) const = 0;

  /////////////////////////////////////////////////////////////
  // AtomBase methods
  //
 private:
  virtual BaseAtom* Clone() const = 0;
  virtual TagDecodeStatus DecodeBody(uint64 size,
                                      io::MemoryStream& in,
                                      Decoder& decoder);
  virtual void EncodeBody(io::MemoryStream& out, Encoder& encoder) const;
  virtual uint64 MeasureBodySize() const;
  virtual string ToStringBody(uint32 indent) const;

 private:
  uint8 version_;
  uint8 flags_[3];
};
}
}

#endif // __MEDIA_F4V_ATOMS_VERSIONED_ATOM_H__
