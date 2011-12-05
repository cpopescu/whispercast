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

#include <string>
#include <whisperlib/common/base/types.h>
#include <whisperlib/net/rpc/lib/codec/rpc_encoder.h>
#include <whisperlib/net/rpc/lib/codec/rpc_decoder.h>
#include <whisperlib/net/rpc/lib/codec/rpc_codec_id.h>

namespace rpc {

class Codec {
 public:
  Codec() {
  }
  virtual ~Codec() {
  }

  // Allocate an encoder which will write serialized data to stream "out".
  // When no longer needed, delete the encoder.
  virtual rpc::Encoder* CreateEncoder(io::MemoryStream& out) = 0;

  // Allocate a decoder which reads serialized data from the stream "in".
  // When no longer needed, delete the decoder.
  virtual rpc::Decoder* CreateDecoder(io::MemoryStream& in) = 0;

  // Allocate a new identical codec.
  virtual rpc::Codec* Clone() const = 0;

  // Returns the codec id for this factory.
  // Used in RPC handshake to estabilish a common codec to use.
  virtual rpc::CODEC_ID GetCodecID() const = 0;

  // Returns codec name, based on codec id.
  const char* GetCodecName() const;

  // Easy encode rpc::Object "obj" to stream "out".
  template <typename T>
  void Encode(io::MemoryStream& out, const T& obj) {
    rpc::Encoder* const encoder = CreateEncoder(out);
    encoder->Encode(obj);
    delete encoder;
  }

  // Easy decode expected "obj" from stream "in".
  // Returns the return value of rpc::Decoder::Decode(obj) :
  //   DECODE_RESULT_SUCCESS: if the object was successfully read.
  //   DECODE_RESULT_NOT_ENOUGH_DATA: if there is not enough data in the input
  //                                  buffer. The read data is not restored.
  //   DECODE_RESULT_ERROR: if the data does not look like the expected object.
  //                        The read data is not restored.
  template <typename T>
  DECODE_RESULT Decode(io::MemoryStream& in, T& obj) {
    rpc::Decoder* const decoder = CreateDecoder(in);
    const DECODE_RESULT result = decoder->Decode(obj);
    delete decoder;
    return result;
  }

  // Easy encode a single RPC packet.
  void EncodePacket(io::MemoryStream& out, const rpc::Message& p);

  //  Easy decode a single RPC packet.
  // Returns the same value as rpc::Decoder::DecodePacket(p) :
  //   DECODE_RESULT_SUCCESS: if a rpc::Message was successfully read.
  //   DECODE_RESULT_NOT_ENOUGH_DATA: if there is not enough data in the input
  //                                  buffer. The read data is not restored.
  //   DECODE_RESULT_ERROR: if the data does not look like a rpc::Message.
  //                        The read data is not restored.
  // NOTE: in case of error or not-enough-data, the read data is not restored.
  DECODE_RESULT DecodePacket(io::MemoryStream& in, rpc::Message& p);

  virtual string ToString() const = 0;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(Codec);
};
}

#endif   // __NET_RPC_LIB_CODEC_RPC_CODEC_H__
