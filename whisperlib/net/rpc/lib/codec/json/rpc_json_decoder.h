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
// Author: Cosmin Tudorache & Catalin Popescu

#ifndef __NET_RPC_LIB_CODEC_JSON_RPC_JSON_DECODER_H__
#define __NET_RPC_LIB_CODEC_JSON_RPC_JSON_DECODER_H__

#include <string>
#include <whisperlib/net/rpc/lib/codec/rpc_decoder.h>

namespace rpc {

class JsonDecoder : public rpc::Decoder {
 public:
  JsonDecoder()
      : Decoder(kCodecIdJson),
        decoding_map_key_(false),
        is_first_item_(true) { }
  virtual ~JsonDecoder() { }

  //////////////////////////////////////////////////////////////////////
  //
  //                   rpc::Decoder interface methods
  //
  virtual DECODE_RESULT DecodeStructStart(io::MemoryStream& in);
  virtual DECODE_RESULT DecodeStructContinue(io::MemoryStream& in, bool* more_attribs);
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
  virtual DECODE_RESULT Decode(io::MemoryStream& in, int32* out);
  virtual DECODE_RESULT Decode(io::MemoryStream& in, uint32* out);
  virtual DECODE_RESULT Decode(io::MemoryStream& in, int64* out);
  virtual DECODE_RESULT Decode(io::MemoryStream& in, uint64* out);
  virtual DECODE_RESULT Decode(io::MemoryStream& in, double* out);
  virtual DECODE_RESULT Decode(io::MemoryStream& in, string* out);

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

  void Reset1() {
    is_first_item_ = true;
    decoding_map_key_ = false;
  }

  // Reads raw data of the next value. You don't have to know the type.
  // e.g. if <in> contains: "{a:2}{b:3" it reads & appends "{a:2}" to out,
  //      returns DECODE_RESULT_SUCCESS
  // e.g. if <in> contains: "\"abcdef\" , \"" it reads & appends "\"abcdef\""
  //      to out, returns DECODE_RESULT_SUCCESS
  // e.g. if <in> contains: "23, 3" it reads & appends "23" to out,
  //      returns DECODE_RESULT_SUCCESS
  // e.g. if <in> contains: "{a:2" it returns DECODE_RESULT_NOT_ENOUGH_DATA
  //      (out may contain junk)
  // e.g. if <in> contains: "[1,2," it returns DECODE_RESULT_NOT_ENOUGH_DATA
  //      (out may contain junk)
  // e.g. if <in> contains: ", 14" it returns DECODE_RESULT_ERROR,
  //      input is not a value (out may contain junk)
  virtual DECODE_RESULT ReadRawObject(io::MemoryStream& in, io::MemoryStream* out);

  template<class C>
  static bool DecodeObject(const string& s, C* obj) {
    io::MemoryStream tmp;
    tmp.Write(s.c_str(), s.size());
    rpc::JsonDecoder decoder;
    const rpc::DECODE_RESULT result = decoder.Decode(tmp, obj);
    if ( result != DECODE_RESULT_SUCCESS ) {
      LOG_ERROR << "DecodeObject failed for string: [" << s << "]"
                   ", err: " << DecodeResultName(result);
      return false;
    }
    return true;
  }
 private:
  DECODE_RESULT DecodeElementContinue(io::MemoryStream& in,
                                      bool* more_attribs,
                                      const char* separator);

  // read next stuff and expect it to be a separator
  DECODE_RESULT ReadExpectedSeparator(io::MemoryStream& in,
                                      char expected);

  template <typename T>
  DECODE_RESULT ReadNumber(io::MemoryStream& in,
                           T* out,
                           bool is_floating_point = false,
                           bool has_quotes = false) {
    string s;
    const io::TokenReadError res = in.ReadNextAsciiToken(&s);
    switch ( res ) {
      case io::TOKEN_NO_DATA:
        return DECODE_RESULT_NOT_ENOUGH_DATA;
      case io::TOKEN_OK:
        if ( has_quotes ) {
          DLOG_ERROR << "Json: Expecting quotes, got a no quotes token: "
                     << s;
          return DECODE_RESULT_ERROR;
        }
        break;
      case io::TOKEN_QUOTED_OK:
        if ( !has_quotes ) {
          DLOG_ERROR << "Json: Expecting non-quoted, got a no quoted token: "
                     << s;
          return DECODE_RESULT_ERROR;
        }
        s = strutil::JsonStrUnescape(s.substr(1, s.length() - 2));
        break;
      default:
        DLOG_ERROR << "Json: Error decoding expected number";
        return DECODE_RESULT_ERROR;
    }
    char *endp;
    errno = 0;
    if ( !is_floating_point ) {
      long long int l = strtoll(s.c_str(), &endp, 0);
      if ( !errno && s.c_str() != endp && *endp == '\0' ) {
        *out = l;
        return DECODE_RESULT_SUCCESS;
      } else {
        DLOG_ERROR << "Json: Got wrong data for int: " << s;
      }
    } else {
      double d = strtod(s.c_str(), &endp);
      if ( !errno && s.c_str() != endp && *endp == '\0') {
        *out = d;
        return DECODE_RESULT_SUCCESS;
      }
      DLOG_ERROR << "Json: Got wrong data for double: " << s;
    }
    return DECODE_RESULT_ERROR;
  }

 private:
  // if true we're decoding the key of a map
  // usefull in decoding map<int, ... > as map<string, ... >
  // because javascript does not support map<int..>
  bool decoding_map_key_;
  bool is_first_item_;
};
}
#endif   // __NET_RPC_LIB_CODEC_JSON_RPC_JSON_DECODER_H__
