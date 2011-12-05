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

#ifndef __NET_RPC_LIB_CODEC_BINARY_RPC_BINARY_DECODER_H__
#define __NET_RPC_LIB_CODEC_BINARY_RPC_BINARY_DECODER_H__

#include <vector>
#include <whisperlib/net/rpc/lib/codec/rpc_decoder.h>
#include <whisperlib/net/rpc/lib/codec/binary/rpc_binary_common.h>

namespace rpc {

class BinaryDecoder : public rpc::Decoder {
 public:
  explicit BinaryDecoder(io::MemoryStream& in)
      : rpc::Decoder(in) {
  }
  virtual ~BinaryDecoder() {
  }

 private:
  // decode a uint32 value that represents items counts in following array
  // or map.
  DECODE_RESULT DecodeItemsCount(uint32& count);

 public:
  //////////////////////////////////////////////////////////////////////
  //
  //                   rpc::Decoder interface methods
  //
  DECODE_RESULT DecodeStructStart(uint32& num_attributes);
  DECODE_RESULT DecodeStructContinue(bool& more_attributes);
  DECODE_RESULT DecodeStructAttribStart();
  DECODE_RESULT DecodeStructAttribMiddle();
  DECODE_RESULT DecodeStructAttribEnd();
  DECODE_RESULT DecodeArrayStart(uint32& num_elements);
  DECODE_RESULT DecodeArrayContinue(bool& more_elements);
  DECODE_RESULT DecodeMapStart(uint32& num_pairs);
  DECODE_RESULT DecodeMapContinue(bool& more_pairs);
  DECODE_RESULT DecodeMapPairStart();
  DECODE_RESULT DecodeMapPairMiddle();
  DECODE_RESULT DecodeMapPairEnd();

 private:
  template <typename T>
  DECODE_RESULT DecodeNumeric(T& out)  {
    if ( in_.Size() < sizeof(T) ) {
      return DECODE_RESULT_NOT_ENOUGH_DATA;
    }
    out = io::NumStreamer::ReadNumber<T>(
        &in_, rpc::kBinaryByteOrder);
    return DECODE_RESULT_SUCCESS;
  }

 protected:
  DECODE_RESULT DecodeBody(rpc::Void& out);
  DECODE_RESULT DecodeBody(bool& out);

  DECODE_RESULT DecodeBody(int32& out)      { return DecodeNumeric(out); }
  DECODE_RESULT DecodeBody(uint32& out)     { return DecodeNumeric(out); }
  DECODE_RESULT DecodeBody(int64& out)      { return DecodeNumeric(out); }
  DECODE_RESULT DecodeBody(uint64& out)     { return DecodeNumeric(out); }
  DECODE_RESULT DecodeBody(double& out)     { return DecodeNumeric(out); }

  DECODE_RESULT DecodeBody(string& out);

  void Reset() {
    items_stack_.clear();
  }
 protected:
  vector<uint32> items_stack_;

  DISALLOW_EVIL_CONSTRUCTORS(BinaryDecoder);
};
}

#endif   // __NET_RPC_LIB_CODEC_BINARY_RPC_BINARY_DECODER_H__
