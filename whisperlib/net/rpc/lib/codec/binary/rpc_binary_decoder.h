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
  BinaryDecoder() : Decoder(kCodecIdBinary) {}
  virtual ~BinaryDecoder() {}

 private:
  // decode a uint32 value that represents item counts in following struct,
  // array, or map.
  DECODE_RESULT DecodeItemCount(io::MemoryStream& in);

 public:
  //////////////////////////////////////////////////////////////////////
  //
  //                   rpc::Decoder interface methods
  //
  virtual DECODE_RESULT DecodeStructStart(io::MemoryStream& in);
  virtual DECODE_RESULT DecodeStructContinue(io::MemoryStream& in, bool* more_attributes);
  virtual DECODE_RESULT DecodeStructAttribStart(io::MemoryStream& in);
  virtual DECODE_RESULT DecodeStructAttribMiddle(io::MemoryStream& in);
  virtual DECODE_RESULT DecodeStructAttribEnd(io::MemoryStream& in);
  virtual DECODE_RESULT DecodeArrayStart(io::MemoryStream& in);
  virtual DECODE_RESULT DecodeArrayContinue(io::MemoryStream& in, bool* more_elements);
  virtual DECODE_RESULT DecodeMapStart(io::MemoryStream& in);
  virtual DECODE_RESULT DecodeMapContinue(io::MemoryStream& in, bool* more_pairs);
  virtual DECODE_RESULT DecodeMapPairStart(io::MemoryStream& in);
  virtual DECODE_RESULT DecodeMapPairMiddle(io::MemoryStream& in);
  virtual DECODE_RESULT DecodeMapPairEnd(io::MemoryStream& in);

  virtual DECODE_RESULT Decode(io::MemoryStream& in, rpc::Void* out);
  virtual DECODE_RESULT Decode(io::MemoryStream& in, bool* out);
  virtual DECODE_RESULT Decode(io::MemoryStream& in, int32* out)   { return DecodeNumeric(in, out); }
  virtual DECODE_RESULT Decode(io::MemoryStream& in, uint32* out)  { return DecodeNumeric(in, out); }
  virtual DECODE_RESULT Decode(io::MemoryStream& in, int64* out)   { return DecodeNumeric(in, out); }
  virtual DECODE_RESULT Decode(io::MemoryStream& in, uint64* out)  { return DecodeNumeric(in, out); }
  virtual DECODE_RESULT Decode(io::MemoryStream& in, double* out)  { return DecodeNumeric(in, out); }
  virtual DECODE_RESULT Decode(io::MemoryStream& in, string* out);

  virtual DECODE_RESULT ReadRawObject(io::MemoryStream& in, io::MemoryStream* out);

  // these methods are public in the Decoder. But because of some compiler
  // issue they need to be re-declared here.
  template <typename T>
  DECODE_RESULT Decode(io::MemoryStream& in, vector<T>* out) {
    return Decoder::Decode(in, out);
  }
  template <typename K, typename V>
  DECODE_RESULT Decode(io::MemoryStream& in, map<K, V>* out) {
    return Decoder::Decode(in, out);
  }
  DECODE_RESULT Decode(io::MemoryStream& in, rpc::Custom* out) {
    return Decoder::Decode(in, out);
  }
  DECODE_RESULT Decode(io::MemoryStream& in, rpc::Message* out) {
    return Decoder::Decode(in, out);
  }


 private:
  template <typename T>
  DECODE_RESULT DecodeNumeric(io::MemoryStream& in, T* out)  {
    if ( in.Size() < sizeof(T) ) {
      return DECODE_RESULT_NOT_ENOUGH_DATA;
    }
    *out = io::NumStreamer::ReadNumber<T>(&in, rpc::kBinaryByteOrder);
    return DECODE_RESULT_SUCCESS;
  }

  void Reset1() {
    items_stack_.clear();
  }

 private:
  // Helps with nested structures decoding.
  // When decoding a struct:
  //  - push here the number of attributes
  //  - for every attribute read: decrement this number (last one in stack)
  //  - if it reaches 0 => no more attributes, and pop the number from stack
  //  - if an attribute is another struct, then push
  //    the new struct attr's number on top
  //     .. continue reading the inner structure, until completed, and it's
  //        attr count is removed from stack
  //    Continue with lower struct
  // Similar for Array, Map.
  vector<uint32> items_stack_;

  DISALLOW_EVIL_CONSTRUCTORS(BinaryDecoder);
};
}

#endif   // __NET_RPC_LIB_CODEC_BINARY_RPC_BINARY_DECODER_H__
