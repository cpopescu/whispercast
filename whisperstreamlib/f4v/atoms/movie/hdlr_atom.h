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
#ifndef __MEDIA_F4V_ATOMS_MOVIE_HDLR_ATOM_H__
#define __MEDIA_F4V_ATOMS_MOVIE_HDLR_ATOM_H__

#include <string>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperstreamlib/f4v/atoms/versioned_atom.h>

namespace streaming {
namespace f4v {

class HdlrAtom : public VersionedAtom {
 public:
  static const AtomType kType = ATOM_HDLR;
  static const uint32 kVersionedBodyMinSize = 20;
 public:
  HdlrAtom();
  HdlrAtom(const HdlrAtom& other);
  virtual ~HdlrAtom();

  uint32 pre_defined() const;
  uint32 handler_type() const;
  const string& name() const;

  void set_pre_defined(uint32 pre_defined);
  void set_handler_type(uint32 handler_type);
  void set_reserved(const uint32* reserved, uint32 size);
  void set_name(string name);

  ///////////////////////////////////////////////////////////////////////////
  // Methods from VersionedAtom
  virtual bool EqualsVersionedBody(const VersionedAtom& other) const;
  virtual void GetSubatoms(vector<const BaseAtom*>& subatoms) const;
  virtual BaseAtom* Clone() const;
  virtual TagDecodeStatus DecodeVersionedBody(uint64 size, io::MemoryStream& in,
                                              Decoder& decoder);
  virtual void EncodeVersionedBody(io::MemoryStream& out, Encoder& encoder) const;
  virtual uint64 MeasureVersionedBodySize() const;
  virtual string ToStringVersionedBody(uint32 indent) const;

 private:
  uint32 pre_defined_;
  uint32 handler_type_;
  uint32 reserved_[3];
  string name_;
};
}
}

#endif // __MEDIA_F4V_ATOMS_MOVIE_HDLR_ATOM_H__
