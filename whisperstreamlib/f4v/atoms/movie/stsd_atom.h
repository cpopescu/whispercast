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

#ifndef __MEDIA_F4V_ATOMS_MOVIE_STSD_ATOM_H__
#define __MEDIA_F4V_ATOMS_MOVIE_STSD_ATOM_H__

#include <string>
#include <whisperlib/common/io/buffer/io_memory_stream.h>
#include <whisperstreamlib/f4v/atoms/container_versioned_atom.h>

namespace streaming {
namespace f4v {

class Avc1Atom;
class Mp4aAtom;

class StsdAtom : public ContainerVersionedAtom {
 public:
  static const AtomType kType = ATOM_STSD;
  static const uint32 kDataSize = 4;
 public:
  StsdAtom();
  StsdAtom(const StsdAtom& other);
  virtual ~StsdAtom();

  const Avc1Atom* avc1() const;
  const Mp4aAtom* mp4a() const;

  ///////////////////////////////////////////////////////////////////////////
  // Methods from VersionedAtom
 public:
  virtual void SubatomFound(BaseAtom* atom);
  virtual BaseAtom* Clone() const;
  virtual TagDecodeStatus DecodeData(uint64 size, io::MemoryStream& in,
                                     Decoder& decoder);
  virtual void EncodeData(io::MemoryStream& out, Encoder& encoder) const;
  virtual uint64 MeasureDataSize() const;
  virtual bool EqualsData(const ContainerVersionedAtom& other) const;
  virtual string ToStringData(uint32 indent) const;

 private:
  uint32 count_;
  Avc1Atom* avc1_;
  Mp4aAtom* mp4a_;
};
}
}

#endif // __MEDIA_F4V_ATOMS_MOVIE_STSD_ATOM_H__
