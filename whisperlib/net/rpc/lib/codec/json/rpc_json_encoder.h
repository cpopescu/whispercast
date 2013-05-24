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

#ifndef __NET_RPC_LIB_CODEC_JSON_RPC_JSON_ENCODER_H__
#define __NET_RPC_LIB_CODEC_JSON_RPC_JSON_ENCODER_H__

#include <string>
#include <whisperlib/net/rpc/lib/codec/rpc_encoder.h>

namespace rpc {

class JsonEncoder : public Encoder {
 public:
  JsonEncoder() : Encoder(kCodecIdJson), encoding_map_key_(false) {}
  virtual ~JsonEncoder() {}

  template<class C>
  static void EncodeToString(const C& obj, string* out) {
    io::MemoryStream tmp;
    rpc::JsonEncoder encoder;
    encoder.Encode(obj, &tmp);
    tmp.ReadString(out);
  }

  template<class C>
  static string EncodeObject(const C& obj) {
    string s;
    EncodeToString(obj, &s);
    return s;
  }

  //////////////////////////////////////////////////////////////////////
  //
  //                   rpc::Encoder interface methods
  //
  void EncodeStructStart(uint32, io::MemoryStream* out) { out->Write("{"); }
  void EncodeStructContinue(io::MemoryStream* out)      { out->Write(", "); }
  void EncodeStructEnd(io::MemoryStream* out)           { out->Write("}"); }
  void EncodeStructAttribStart(io::MemoryStream* out)   {  }
  void EncodeStructAttribMiddle(io::MemoryStream* out)  { out->Write(": "); }
  void EncodeStructAttribEnd(io::MemoryStream* out)     {  }
  void EncodeArrayStart(uint32, io::MemoryStream* out)  { out->Write("["); }
  void EncodeArrayContinue(io::MemoryStream* out)       { out->Write(", "); }
  void EncodeArrayEnd(io::MemoryStream* out)            { out->Write("]"); }
  void EncodeMapStart(uint32, io::MemoryStream* out)    { out->Write("{"); }
  void EncodeMapContinue(io::MemoryStream* out)         { out->Write(", "); }
  void EncodeMapEnd(io::MemoryStream* out)              { out->Write("}"); }
  void EncodeMapPairStart(io::MemoryStream* out)        { encoding_map_key_ = true; }
  void EncodeMapPairMiddle(io::MemoryStream* out)       { out->Write(": ");
                                                          encoding_map_key_ = false; }
  void EncodeMapPairEnd(io::MemoryStream* out)          {  }

#define PrintBody(format, object, out_ms)                               \
  do {                                                                  \
      if ( !encoding_map_key_ ) {                                       \
        out_ms->Write(strutil::StringPrintf(                            \
                        format" ", object));                            \
      } else {                                                          \
        out_ms->Write(strutil::StringPrintf(                            \
                        "\""format"\"", object));                       \
      }                                                                 \
    } while(false)

  virtual void Encode(const rpc::Void&, io::MemoryStream* out) {
    out->Write("null");
  }
  virtual void Encode(const bool v, io::MemoryStream* out) {
    out->Write(v ? "true" : "false");
  }
  virtual void Encode(const int32 v, io::MemoryStream* out) {
    PrintBody("%d", v, out);
  }
  virtual void Encode(const uint32 v, io::MemoryStream* out) {
    PrintBody("%u", v, out);
  }
  virtual void Encode(const int64 v, io::MemoryStream* out) {
    PrintBody("%"PRId64, v, out);
  }
  virtual void Encode(const uint64 v, io::MemoryStream* out) {
    PrintBody("%"PRIu64, v, out);
  }
  virtual void Encode(const double v, io::MemoryStream* out) {
    PrintBody("%.60e", v, out);
  }
  virtual void Encode(const string& str, io::MemoryStream* out) {
    out->Write("\"");
    out->Write(strutil::JsonStrEscape(str));
    out->Write("\"");
  }

  // these methods are public in the base class, but because of some compiler
  // issue they need to be re-declared here.
  template <typename T>
  void Encode(const vector<T>& v, io::MemoryStream* out)    { Encoder::Encode(v, out); }
  template <typename K, typename V>
  void Encode(const map<K,V>& m, io::MemoryStream* out)     { Encoder::Encode(m, out); }
  void Encode(const rpc::Custom& c, io::MemoryStream* out)  { Encoder::Encode(c, out); }
  void Encode(const rpc::Message& m, io::MemoryStream* out) { Encoder::Encode(m, out); }

  virtual void WriteRawObject(const io::MemoryStream& obj, io::MemoryStream* out) {
    out->AppendStreamNonDestructive(&obj);
  }

 private:
  // if true we're encoding the key of a map.
  // useful in encoding map<int, ... > as map<string, ... >
  // because javascript does not support map<int..>
  bool encoding_map_key_;
  DISALLOW_EVIL_CONSTRUCTORS(JsonEncoder);
};
}
#endif  //  __NET_RPC_LIB_CODEC_JSON_RPC_JSON_CODEC_H__
