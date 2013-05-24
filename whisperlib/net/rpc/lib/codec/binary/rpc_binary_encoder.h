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

#ifndef __NET_RPC_LIB_CODEC_BINARY_RPC_BINARY_ENCODER_H__
#define __NET_RPC_LIB_CODEC_BINARY_RPC_BINARY_ENCODER_H__

#include <whisperlib/net/rpc/lib/codec/rpc_encoder.h>
#include <whisperlib/net/rpc/lib/codec/binary/rpc_binary_common.h>

namespace rpc {

class BinaryEncoder : public rpc::Encoder {
 public:
  BinaryEncoder() : Encoder(kCodecIdBinary) {}
  virtual ~BinaryEncoder() {}

  //////////////////////////////////////////////////////////////////////
  //
  //                   rpc::Encoder interface methods
  //
  virtual void EncodeStructStart(uint32 nAttribs, io::MemoryStream* out) { Encode(nAttribs, out); }
  virtual void EncodeStructContinue(io::MemoryStream* out)               { }
  virtual void EncodeStructEnd(io::MemoryStream* out)                    { }
  virtual void EncodeStructAttribStart(io::MemoryStream* out)            { }
  virtual void EncodeStructAttribMiddle(io::MemoryStream* out)           { }
  virtual void EncodeStructAttribEnd(io::MemoryStream* out)              { }
  virtual void EncodeArrayStart(uint32 nElements, io::MemoryStream* out) { Encode(nElements, out); }
  virtual void EncodeArrayContinue(io::MemoryStream* out)                { }
  virtual void EncodeArrayEnd(io::MemoryStream* out)                     { }
  virtual void EncodeMapStart(uint32 nPairs, io::MemoryStream* out)      { Encode(nPairs, out); }
  virtual void EncodeMapContinue(io::MemoryStream* out)                  { }
  virtual void EncodeMapEnd(io::MemoryStream* out)                       { }
  virtual void EncodeMapPairStart(io::MemoryStream* out)                 { }
  virtual void EncodeMapPairMiddle(io::MemoryStream* out)                { }
  virtual void EncodeMapPairEnd(io::MemoryStream* out)                   { }

  virtual void Encode(const rpc::Void&, io::MemoryStream* out)    {
    io::NumStreamer::WriteByte(out, 0xff);
  }
  virtual void Encode(const bool v, io::MemoryStream* out)    {
    io::NumStreamer::WriteByte(out, v ? 1 : 0);
  }
  virtual void Encode(const int32 v, io::MemoryStream* out)  { EncodeNumeric(v, out); }
  virtual void Encode(const uint32 v, io::MemoryStream* out) { EncodeNumeric(v, out); }
  virtual void Encode(const int64 v, io::MemoryStream* out)  { EncodeNumeric(v, out); }
  virtual void Encode(const uint64 v, io::MemoryStream* out) { EncodeNumeric(v, out); }
  virtual void Encode(const double v, io::MemoryStream* out) { EncodeNumeric(v, out); }

  virtual void Encode(const string& str, io::MemoryStream* out)  {
    io::NumStreamer::WriteInt32(out, str.size(), rpc::kBinaryByteOrder);
    out->Write(str.data(), str.size());
  }
  virtual void WriteRawObject(const io::MemoryStream& obj, io::MemoryStream* out) {
    uint32 size = obj.Size();
    EncodeNumeric(size, out);
    out->AppendStreamNonDestructive(&obj);
  }

  // these methods are public in the base class, but because of some compiler
  // issue they need to be re-declared here.
  template <typename T>
  void Encode(const vector<T>& v, io::MemoryStream* out)    { Encoder::Encode(v, out); }
  template <typename K, typename V>
  void Encode(const map<K,V>& m, io::MemoryStream* out)     { Encoder::Encode(m, out); }
  void Encode(const rpc::Custom& c, io::MemoryStream* out)  { Encoder::Encode(c, out); }
  void Encode(const rpc::Message& m, io::MemoryStream* out) { Encoder::Encode(m, out); }


 private:
  // helper function for encoding numeric types
  template <typename T>
  void EncodeNumeric(const T& val, io::MemoryStream* out) {
    io::NumStreamer::WriteNumber<T>(out, val, rpc::kBinaryByteOrder);
  }

 private:
  DISALLOW_EVIL_CONSTRUCTORS(BinaryEncoder);
};
}
#endif   // __NET_RPC_LIB_CODEC_BINARY_RPC_BINARY_ENCODER_H__
