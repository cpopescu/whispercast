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

#ifndef __NET_RPC_LIB_CODEC_JSON_RPC_JSON_CODEC_H__
#define __NET_RPC_LIB_CODEC_JSON_RPC_JSON_CODEC_H__

#include <string>
#include <whisperlib/net/rpc/lib/codec/rpc_codec.h>
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_encoder.h>
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_decoder.h>

namespace rpc {

class JsonCodec : public rpc::Codec {
 public:
  static const rpc::CODEC_ID CODEC_ID = rpc::CID_JSON;

 public:
  JsonCodec() : rpc::Codec() {
  }
  virtual ~JsonCodec() {
  }

  inline static const char* Name() {
    return rpc::CodecName(CODEC_ID);
  }

  //////////////////////////////////////////////////////////////////////
  //
  //                rpc::Codec interface methods
  //

  // Allocate a JSON encoder that writes data to stream "out".
  rpc::Encoder* CreateEncoder(io::MemoryStream& out) {
    return new rpc::JsonEncoder(out);
  }
  // Allocate a JSON decoder that reads data from stream "in".
  rpc::Decoder* CreateDecoder(io::MemoryStream& in) {
    return new rpc::JsonDecoder(in);
  }
  // Allocate a new JSON codec.
  rpc::Codec* Clone() const {
    return new rpc::JsonCodec();
  }
  // Return codec id.
  rpc::CODEC_ID GetCodecID() const {
    return CODEC_ID;
  }

  //////////////////////////////////////////////////////////////////////
  //
  //                      rpc::Loggable interface
  //
  string ToString() const {
    return string("rpc::JsonCodec");
  }
};
}

#endif  //  __NET_RPC_LIB_CODEC_JSON_RPC_JSON_CODEC_H__
