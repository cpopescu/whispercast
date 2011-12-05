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

#ifndef __MEDIA_F4V_ATOMS_MOVIE_STCO_ATOM_H__
#define __MEDIA_F4V_ATOMS_MOVIE_STCO_ATOM_H__

#include <string>
#include <whisperlib/common/io/buffer/io_memory_stream.h>
#include <whisperstreamlib/f4v/atoms/multi_record_versioned_atom.h>

namespace streaming {
namespace f4v {

struct StcoRecord {
  // the offset of the chunk start
  uint32 chunk_offset_;
  static const uint32 kEncodingSize = 4;

  StcoRecord();
  StcoRecord(uint32 chunk_offset);
  StcoRecord(const StcoRecord& other);
  ~StcoRecord();

  StcoRecord* Clone() const;

  bool Decode(io::MemoryStream& in);
  void Encode(io::MemoryStream& out) const;

  string ToString() const;
};

class StcoAtom : public MultiRecordVersionedAtom<StcoRecord> {
public:
 static const AtomType kType = ATOM_STCO;

public:
 StcoAtom();
 StcoAtom(const StcoAtom& other);
 virtual ~StcoAtom();

 ///////////////////////////////////////////////////////////////////////////
 // Methods from MultiRecordVersionedAtom
 virtual BaseAtom* Clone() const;
};
}
}

#endif // __MEDIA_F4V_ATOMS_MOVIE_STCO_ATOM_H__
