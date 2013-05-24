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

#include <whisperstreamlib/mts/mts_decoder.h>
#include <whisperstreamlib/mts/mts_consts.h>
#include <whisperstreamlib/mts/mts_tag.h>

DEFINE_int32(mts_log_level, LERROR,
             "Enables MPEG TS debug messages. Set to 4 for all messages.");

namespace streaming {
namespace mts {

DecodeStatus TSDecoder::Read(io::MemoryStream& in,
    scoped_ref<TSPacket>* out) {
  scoped_ref<TSPacket> p = new TSPacket();
  DecodeStatus status = p->Decode(&in);
  if ( status != DECODE_SUCCESS ) {
    return status;
  }
  *out = p;
  return DECODE_SUCCESS;
}

///////////////////////////////////////////////////////////////////

DecodeStatus PESDecoder::Read(io::MemoryStream& in,
    scoped_ref<PESPacket>* out) {
  scoped_ref<PESPacket> p = new PESPacket();
  DecodeStatus status = p->Decode(&in);
  if ( status != DECODE_SUCCESS ) {
    return status;
  }
  *out = p;
  return DECODE_SUCCESS;
}

///////////////////////////////////////////////////////////////////

Splitter::Splitter(const string& name)
  : streaming::TagSplitter(MFORMAT_MTS, name),
    ts_decoder_(),
    pes_buffer_(),
    pes_decoder_() {
}
Splitter::~Splitter() {
}

DecodeStatus Splitter::FetchTSPacket(io::MemoryStream* in) {
  scoped_ref<TSPacket> p;
  DecodeStatus status = ts_decoder_.Read(*in, &p);
  if ( status != DECODE_SUCCESS ) { return status; }
  LOG_WARNING << "# " << p.ToString();
  if ( p->payload() != NULL ) {
    pes_buffer_.AppendStreamNonDestructive(p->payload());
  }
  return DECODE_SUCCESS;
}
DecodeStatus Splitter::ReadPESPacket(scoped_ref<PESPacket>* out) {
  DecodeStatus status = pes_decoder_.Read(pes_buffer_, out);
  if ( status != DECODE_SUCCESS ) { return status; }
  LOG_WARNING << "# " << out->ToString();
  return DECODE_SUCCESS;
}

streaming::TagReadStatus Splitter::GetNextTagInternal(
      io::MemoryStream* in,
      scoped_ref<Tag>* tag,
      int64* timestamp_ms,
      bool is_at_eos) {
  scoped_ref<PESPacket> p;
  while ( true ) {
    DecodeStatus status = ReadPESPacket(&p);
    if ( status == DECODE_SUCCESS ) { break; }
    if ( status == DECODE_ERROR ) { return streaming::READ_CORRUPTED; }
    CHECK(status == DECODE_NO_DATA) << "Illegal DecodeStatus: " << (int)status;

    status = FetchTSPacket(in);
    if ( status == DECODE_ERROR ) { return streaming::READ_CORRUPTED; }
    if ( status == DECODE_NO_DATA ) { return is_at_eos ? streaming::READ_EOF : streaming::READ_NO_DATA; }
    CHECK(status == DECODE_SUCCESS) << "Illegal DecodeStatus: " << (int)status;
    continue;
  }

  //*tag = new MtsTag();
  //*timestamp_ms = 0;
  //return streaming::READ_OK;

  return streaming::READ_SKIP;
}

}
}
