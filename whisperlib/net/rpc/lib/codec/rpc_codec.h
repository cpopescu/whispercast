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

#ifndef __NET_RPC_LIB_CODEC_RPC_CODEC_H__
#define __NET_RPC_LIB_CODEC_RPC_CODEC_H__

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/scoped_ptr.h>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperlib/net/rpc/lib/codec/rpc_encoder.h>
#include <whisperlib/net/rpc/lib/codec/rpc_decoder.h>

#include <whisperlib/net/rpc/lib/codec/json/rpc_json_encoder.h>
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_decoder.h>
#include <whisperlib/net/rpc/lib/codec/binary/rpc_binary_encoder.h>
#include <whisperlib/net/rpc/lib/codec/binary/rpc_binary_decoder.h>

namespace rpc {

Encoder* CreateEncoder(CodecId codec);
Decoder* CreateDecoder(CodecId codec);

// Helper methods for easy encoding/decoding when only the codec Type is known.

template <typename T>
void EncodeBy(CodecId codec, const T& in, io::MemoryStream* out) {
  switch ( codec ) {
    case kCodecIdBinary: BinaryEncoder().Encode(in,out); return;
    case kCodecIdJson: JsonEncoder().Encode(in,out); return;
  }
  LOG_FATAL << "Illegal codec id: " << codec;
}
template <typename T>
DECODE_RESULT DecodeBy(CodecId codec, io::MemoryStream& in, T* out) {
  switch ( codec ) {
    case kCodecIdBinary: return BinaryDecoder().Decode(in, out);
    case kCodecIdJson: return JsonDecoder().Decode(in, out);
  }
  LOG_FATAL << "Illegal codec id: " << codec;
  return DECODE_RESULT_ERROR;
}

}

#endif   // __NET_RPC_LIB_CODEC_RPC_CODEC_H__
