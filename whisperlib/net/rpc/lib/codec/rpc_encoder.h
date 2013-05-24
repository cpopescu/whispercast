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

#ifndef __NET_RPC_LIB_CODEC_RPC_ENCODER_H__
#define __NET_RPC_LIB_CODEC_RPC_ENCODER_H__

#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperlib/common/io/output_stream.h>
#include <whisperlib/common/io/num_streaming.h>
#include <whisperlib/net/rpc/lib/rpc_constants.h>

namespace rpc {

class Void;
class Custom;
class Message;

class Encoder {
 public:
  Encoder(CodecId type) : type_(type) {
  }
  virtual ~Encoder() {
  }

  CodecId type() const { return type_; }
  const string& type_name() const { return CodecName(type()); }

  // Methods for serializing complex types: structure, array, map.
  virtual void EncodeStructStart(uint32 nAttribs, io::MemoryStream* out) = 0;
  virtual void EncodeStructContinue(io::MemoryStream* out) = 0;
  virtual void EncodeStructEnd(io::MemoryStream* out) = 0;
  virtual void EncodeStructAttribStart(io::MemoryStream* out) = 0;
  virtual void EncodeStructAttribMiddle(io::MemoryStream* out) = 0;
  virtual void EncodeStructAttribEnd(io::MemoryStream* out) = 0;
  virtual void EncodeArrayStart(uint32 nElements, io::MemoryStream* out) = 0;
  virtual void EncodeArrayContinue(io::MemoryStream* out) = 0;
  virtual void EncodeArrayEnd(io::MemoryStream* out) = 0;
  virtual void EncodeMapStart(uint32 nPairs, io::MemoryStream* out) = 0;
  virtual void EncodeMapContinue(io::MemoryStream* out) = 0;
  virtual void EncodeMapEnd(io::MemoryStream* out) = 0;
  virtual void EncodeMapPairStart(io::MemoryStream* out) = 0;
  virtual void EncodeMapPairMiddle(io::MemoryStream* out) = 0;
  virtual void EncodeMapPairEnd(io::MemoryStream* out) = 0;

  template <typename T>
  void EncodeStructAttrib(const string& name, T value, io::MemoryStream* out) {
    EncodeStructAttribStart(out);
    Encode(name, out);
    EncodeStructAttribMiddle(out);
    Encode(value, out);
    EncodeStructAttribEnd(out);
  }

  // Encode the value of a base type object in the output stream.
  // The reverse of these methods is rpc::Decoder::Decode(..).
  virtual void Encode(const rpc::Void& obj, io::MemoryStream* out) = 0;
  virtual void Encode(const bool v, io::MemoryStream* out) = 0;
  virtual void Encode(const int32 v, io::MemoryStream* out) = 0;
  virtual void Encode(const uint32 v, io::MemoryStream* out) = 0;
  virtual void Encode(const int64 v, io::MemoryStream* out) = 0;
  virtual void Encode(const uint64 v, io::MemoryStream* out) = 0;
  virtual void Encode(const double v, io::MemoryStream* out) = 0;
  virtual void Encode(const string& str, io::MemoryStream* out) = 0;

  // Writes the given data directly to the output stream, without encoding.
  virtual void WriteRawObject(const io::MemoryStream& obj, io::MemoryStream* out) = 0;

  // NOTE! without this function, const char* is silently converted to bool !!
  void Encode(const char* v, io::MemoryStream* out) { Encode(string(v), out); }

  template<typename T>
  void Encode(const vector<T>& obj, io::MemoryStream* out) {
    uint32 count = obj.size();
    EncodeArrayStart(count, out);           // encode the items count
    for ( uint32 i = 0; i < count; i++ ) {
      if ( i > 0 ) {
        EncodeArrayContinue(out);
      }
      Encode(obj[i], out);                  // encode the items themselves
    }
    EncodeArrayEnd(out);
  }

  template<typename K, typename V>
  void Encode(const map<K, V>& obj, io::MemoryStream* out) {
    uint32 count = obj.size();             // count
    EncodeMapStart(count, out);            // encode the items count
    uint32 i = 0;
    for ( typename map<K, V>::const_iterator it = obj.begin();
          it != obj.end(); ++it, ++i ) {
      EncodeMapPairStart(out);
      Encode(it->first, out);              // encode the key for an item
      EncodeMapPairMiddle(out);
      Encode(it->second, out);             // encode the value for an item
      EncodeMapPairEnd(out);
      if ( i + 1 < count ) {
        EncodeMapContinue(out);
      }
    }
    EncodeMapEnd(out);
  }

  // Encode a custom type object.
  void Encode(const rpc::Custom& obj, io::MemoryStream* out);

  // Encode an rpc::Message.
  void Encode(const rpc::Message& p, io::MemoryStream* out);

 protected:
  CodecId type_;
  DISALLOW_EVIL_CONSTRUCTORS(Encoder);
};
}
#endif   // __NET_RPC_LIB_CODEC_RPC_ENCODER_H__
