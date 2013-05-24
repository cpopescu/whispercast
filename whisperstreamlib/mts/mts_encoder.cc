// Copyright (c) 2012, Whispersoft s.r.l.
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

#include <whisperstreamlib/mts/mts_encoder.h>
#include <whisperstreamlib/mts/mts_consts.h>

namespace streaming {
namespace mts {

Encoder::Encoder() {
}
Encoder::~Encoder() {
}

void Encoder::WriteTag(const streaming::Tag& tag, io::MemoryStream* out) {
  LOG_FATAL << "Not implemented";
}

Serializer::Serializer()
  : streaming::TagSerializer(MFORMAT_MTS),
    encoder_() {
}
Serializer::~Serializer() {
}

void Serializer::Initialize(io::MemoryStream* out) {
  LOG_FATAL << "Not implemented";
}
void Serializer::Finalize(io::MemoryStream* out) {
  LOG_FATAL << "Not implemented";
}
bool Serializer::SerializeInternal(const streaming::Tag* tag,
                                   int64 timestamp_ms,
                                   io::MemoryStream* out) {
  LOG_FATAL << "Not implemented";
  return false;
}

}
}
