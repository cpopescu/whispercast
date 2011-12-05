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

#include <whisperstreamlib/f4v/f4v_encoder.h>
#include <whisperstreamlib/f4v/f4v_tag.h>
#include <whisperstreamlib/f4v/f4v_tag_reorder.h>
#include <whisperstreamlib/f4v/atoms/base_atom.h>
#include <whisperstreamlib/f4v/frames/frame.h>

namespace streaming {
namespace f4v {

void Encoder::WriteTag(io::MemoryStream& out, const Tag& in) {
  if ( in.is_atom() ) {
    WriteAtom(out, *in.atom());
    return;
  }
  CHECK(in.is_frame());
  WriteFrame(out, *in.frame());
}
void Encoder::WriteAtom(io::MemoryStream& out, const BaseAtom& in) {
  in.Encode(out, *this);
}
void Encoder::WriteFrame(io::MemoryStream& out, const Frame& in) {
  in.Encode(out, *this);
}

////////////////////////////////////////////////////////////////////////

Serializer::Serializer()
  : streaming::TagSerializer(),
    encoder_(),
    reorder_(false) {
}
Serializer::~Serializer() {
}

bool Serializer::Serialize(const f4v::Tag& f4v_tag, io::MemoryStream* out) {
  reorder_.Push(new F4vTag(f4v_tag));

  while ( true ) {
    const F4vTag* f4v_ordered_tag = reorder_.Pop();
    if ( f4v_ordered_tag == NULL ) {
      break;
    }
    encoder_.WriteTag(*out, *f4v_ordered_tag);
    delete f4v_ordered_tag;
  }

  return true;
}
void Serializer::Initialize(io::MemoryStream* out) {
}
void Serializer::Finalize(io::MemoryStream* out) {
}
bool Serializer::SerializeInternal(const streaming::Tag* tag,
                                   int64 base_timestamp_ms,
                                   io::MemoryStream* out) {
  if ( tag->type() != Tag::TYPE_F4V ) {
    return false;
  }
  return Serialize(static_cast<const F4vTag&>(*tag), out);
}
}
}
