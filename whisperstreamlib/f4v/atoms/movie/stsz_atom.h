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

#ifndef __MEDIA_F4V_ATOMS_MOVIE_STSZ_ATOM_H__
#define __MEDIA_F4V_ATOMS_MOVIE_STSZ_ATOM_H__

#include <string>
#include <whisperlib/common/io/buffer/io_memory_stream.h>
#include <whisperstreamlib/f4v/atoms/multi_record_versioned_atom.h>

namespace streaming {
namespace f4v {

struct StszRecord {
  // sample size
  uint32 entry_size_;
  static const uint32 kEncodingSize = 4;

  StszRecord();
  StszRecord(uint32 entry_size);
  StszRecord(const StszRecord& other);
  ~StszRecord();

  StszRecord* Clone() const;

  bool Decode(io::MemoryStream& in);
  void Encode(io::MemoryStream& out) const;

  string ToString() const;
};

class StszAtom : public MultiRecordVersionedAtom<StszRecord> {
public:
 static const AtomType kType = ATOM_STSZ;

public:
 StszAtom();
 StszAtom(const StszAtom& other);
 virtual ~StszAtom();

 uint32 uniform_sample_size() const;

 ///////////////////////////////////////////////////////////////////////////
 // Methods from MultiRecordVersionedAtom
 virtual BaseAtom* Clone() const;
 virtual TagDecodeStatus DecodeVersionedBody(uint64 size, io::MemoryStream& in,
                                              Decoder& decoder);
 virtual void EncodeVersionedBody(io::MemoryStream& out, Encoder& encoder) const;
 virtual uint64 MeasureVersionedBodySize() const;
 virtual string ToStringVersionedBody(uint32 indent) const;

private:
 // If != 0, this is the size for all samples (used in audio traks were all
 //   frames have the same size)
 // If == 0, this attribute is useless. And we have a table of various sizes
 //   following.
 uint32 uniform_sample_size_;
};
}
}

#endif // __MEDIA_F4V_ATOMS_MOVIE_STSZ_ATOM_H__
