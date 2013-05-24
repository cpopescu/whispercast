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

#ifndef __NET_RPC_LIB_CODEC_RPC_DECODER_H__
#define __NET_RPC_LIB_CODEC_RPC_DECODER_H__

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperlib/net/rpc/lib/rpc_constants.h>

namespace rpc {

class Void;
class Custom;
class Message;

#define DECODE_VERIFY(dec_op)                           \
  do {                                                  \
    const rpc::DECODE_RESULT result = dec_op;           \
    if ( result != rpc::DECODE_RESULT_SUCCESS ) {       \
      LOG_ERROR << "" #dec_op " failed";                \
      return result;                                    \
    }                                                   \
  } while ( false )

enum DECODE_RESULT {
  DECODE_RESULT_SUCCESS,
  DECODE_RESULT_NOT_ENOUGH_DATA,
  DECODE_RESULT_ERROR,
};
const char* DecodeResultName(DECODE_RESULT result);

class Decoder {
 public:
  // Construct a decoder that reads data from the given input stream.
  Decoder(CodecId type) : type_(type) {}
  virtual ~Decoder() {}

  CodecId type() const { return type_; }
  const string& type_name() const { return CodecName(type()); }

  //  Methods for unwinding complex types: structure, array, map.
  // returns:
  //  success status.
  // NOTE: in case of error or not-enough-data, the read data is not restored.
  virtual DECODE_RESULT DecodeStructStart(io::MemoryStream& in) = 0;
  virtual DECODE_RESULT DecodeStructContinue(io::MemoryStream& in, bool* bMoreAttribs) = 0;
  virtual DECODE_RESULT DecodeStructAttribStart(io::MemoryStream& in) = 0;
  virtual DECODE_RESULT DecodeStructAttribMiddle(io::MemoryStream& in) = 0;
  virtual DECODE_RESULT DecodeStructAttribEnd(io::MemoryStream& in) = 0;
  virtual DECODE_RESULT DecodeArrayStart(io::MemoryStream& in) = 0;
  virtual DECODE_RESULT DecodeArrayContinue(io::MemoryStream& in, bool* bMoreElements) = 0;
  virtual DECODE_RESULT DecodeMapStart(io::MemoryStream& in) = 0;
  virtual DECODE_RESULT DecodeMapContinue(io::MemoryStream& in, bool* bMorePairs) = 0;
  virtual DECODE_RESULT DecodeMapPairStart(io::MemoryStream& in) = 0;
  virtual DECODE_RESULT DecodeMapPairMiddle(io::MemoryStream& in) = 0;
  virtual DECODE_RESULT DecodeMapPairEnd(io::MemoryStream& in) = 0;

  //  Decode a basic type value from the input stream. Usually the object type
  //  is not stored in stream, so the decoder cannot check data correctness.
  //  You must know somehow the type of data you're expecting.
  //  These methods are the opposite of rpc::Encoder::Encode(..).
  // NOTE: in case of error or not-enough-data, the read data is not restored.
  virtual DECODE_RESULT Decode(io::MemoryStream& in, rpc::Void* out) = 0;
  virtual DECODE_RESULT Decode(io::MemoryStream& in, bool* out) = 0;
  virtual DECODE_RESULT Decode(io::MemoryStream& in, int32* out) = 0;
  virtual DECODE_RESULT Decode(io::MemoryStream& in, uint32* out) = 0;
  virtual DECODE_RESULT Decode(io::MemoryStream& in, int64* out) = 0;
  virtual DECODE_RESULT Decode(io::MemoryStream& in, uint64* out) = 0;
  virtual DECODE_RESULT Decode(io::MemoryStream& in, double* out) = 0;
  virtual DECODE_RESULT Decode(io::MemoryStream& in, string* out) = 0;

  // Copies the next object or value into 'out'. The data is copied raw,
  // without decoding.
  virtual DECODE_RESULT ReadRawObject(io::MemoryStream& in, io::MemoryStream* out) = 0;

  //  Reset internal state. Should be called before Decode.
  //  If a previous try to Decode failed for insufficient data,
  //  the internal state may be mangled.
  virtual void Reset1() = 0;

  template <typename T>
  DECODE_RESULT Decode(io::MemoryStream& in, vector<T>* out) {
    DECODE_VERIFY(DecodeArrayStart(in));

    // decode elements in a temporary list
    while ( true ) {
      bool has_more = false;
      DECODE_VERIFY(DecodeArrayContinue(in, &has_more));
      if ( !has_more ) {
        break;
      }
      T item;
      DECODE_VERIFY(Decode(in, &item));
      out->push_back(item);
    }
    return DECODE_RESULT_SUCCESS;
  }

  template <typename K, typename V>
  DECODE_RESULT Decode(io::MemoryStream& in, map<K, V>* out) {
    out->clear();
    DECODE_VERIFY(DecodeMapStart(in));

    // decode [key, value] pairs
    //
    while ( true ) {
      bool has_more = true;
      DECODE_VERIFY(DecodeMapContinue(in, &has_more));
      if ( !has_more ) {
        break;
      }
      DECODE_VERIFY(DecodeMapPairStart(in));
      K key;
      DECODE_VERIFY(Decode(in, &key));
      DECODE_VERIFY(DecodeMapPairMiddle(in));
      V value;
      DECODE_VERIFY(Decode(in, &value));
      DECODE_VERIFY(DecodeMapPairEnd(in));
      (*out)[key] = value;
    }

    return DECODE_RESULT_SUCCESS;
  }

  //  Decode a custom type object.
  DECODE_RESULT Decode(io::MemoryStream& in, rpc::Custom* out);

  // Decodes data from the "input" stream trying to recompose an rpc::Message
  //  which is stored in "out".
  // returns:
  //   DECODE_RESULT_SUCCESS: if a rpc::Message was successfully read.
  //   DECODE_RESULT_NOT_ENOUGH_DATA: if there is not enough data
  //                                  in the input buffer.
  //                                  The read data is not restored.
  //   DECODE_RESULT_ERROR: if the data does not look like an rpc::Message.
  //                        The read data is not restored.
  // NOTE: in case of error or not-enough-data, the read data is not restored.
  DECODE_RESULT Decode(io::MemoryStream& in, rpc::Message* out);


 protected:
  const CodecId type_;
  DISALLOW_EVIL_CONSTRUCTORS(Decoder);
};
}

#endif   // __NET_RPC_LIB_CODEC_RPC_DECODER_H__
