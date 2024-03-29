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

#ifndef __MEDIA_F4V_ATOMS_MOVIE_STBL_ATOM_H__
#define __MEDIA_F4V_ATOMS_MOVIE_STBL_ATOM_H__

#include <string>
#include <whisperlib/common/io/buffer/io_memory_stream.h>
#include <whisperstreamlib/f4v/atoms/container_atom.h>

namespace streaming {
namespace f4v {

class StsdAtom;
class SttsAtom;
class StssAtom;
class StscAtom;
class StszAtom;
class StcoAtom;
class Co64Atom;
class CttsAtom;

class StblAtom : public ContainerAtom {
 public:
  static const AtomType kType = ATOM_STBL;

 public:
  StblAtom();
  StblAtom(const StblAtom& other);
  virtual ~StblAtom();

  StsdAtom* stsd();
  const StsdAtom* stsd() const;
  SttsAtom* stts();
  const SttsAtom* stts() const;
  StssAtom* stss();
  const StssAtom* stss() const;
  StscAtom* stsc();
  const StscAtom* stsc() const;
  StszAtom* stsz();
  const StszAtom* stsz() const;
  StcoAtom* stco();
  const StcoAtom* stco() const;
  CttsAtom* ctts();
  const CttsAtom* ctts() const;
  Co64Atom* co64();
  const Co64Atom* co64() const;

  ///////////////////////////////////////////////////////////////////////////
  // Methods from ContainerAtom
  virtual void SubatomFound(BaseAtom* atom);
  virtual BaseAtom* Clone() const;

 private:
  StsdAtom* stsd_;
  SttsAtom* stts_;
  StssAtom* stss_;
  StscAtom* stsc_;
  StszAtom* stsz_;
  StcoAtom* stco_;
  CttsAtom* ctts_;
  Co64Atom* co64_;
};
}
}

#endif // __MEDIA_F4V_ATOMS_MOVIE_STBL_ATOM_H__
