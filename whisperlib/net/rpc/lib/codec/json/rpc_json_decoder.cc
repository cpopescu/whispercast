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
//

#include <deque>
#include <unicode/utf8.h>      // ICU/source/common/unicode/utf8.h

#include "common/base/log.h"
#include "common/base/common.h"
#include "net/rpc/lib/codec/json/rpc_json_decoder.h"

namespace rpc {

DECODE_RESULT JsonDecoder::DecodeElementContinue(bool& more_attribs,
                                                 const char* separator) {
  string s;
  in_.MarkerSet();
  const io::TokenReadError res = in_.ReadNextAsciiToken(&s);
  if ( res == io::TOKEN_NO_DATA ) {
    in_.MarkerRestore();
    return DECODE_RESULT_NOT_ENOUGH_DATA;
  }
  if ( res == io::TOKEN_SEP_OK ) {
    if ( s == separator ) {
      more_attribs = false;
      in_.MarkerClear();
      return DECODE_RESULT_SUCCESS;
    }
    if ( s == "," ) {
      more_attribs = true;
      in_.MarkerClear();
      return DECODE_RESULT_SUCCESS;
    }
  }
  if ( !is_first_item_ ) {
    in_.MarkerRestore();
    DLOG_ERROR << "Json Bad Struct continues: " << s;
    return DECODE_RESULT_ERROR;
  }
  in_.MarkerRestore();
  is_first_item_ = false;
  more_attribs = true;
  return DECODE_RESULT_SUCCESS;
}

DECODE_RESULT JsonDecoder::DecodeStructStart(uint32& num_attribs) {
  is_first_item_ = true;
  num_attribs = kMaxUInt32;
  return ReadExpectedSeparator('{');
}

DECODE_RESULT JsonDecoder::DecodeStructContinue(bool& more_attribs) {
  return DecodeElementContinue(more_attribs, "}");
}

DECODE_RESULT JsonDecoder::DecodeStructAttribStart() {
  return DECODE_RESULT_SUCCESS;
}
DECODE_RESULT JsonDecoder::DecodeStructAttribMiddle() {
  return ReadExpectedSeparator(':');
}
DECODE_RESULT JsonDecoder::DecodeStructAttribEnd() {
  return DECODE_RESULT_SUCCESS;
}
DECODE_RESULT JsonDecoder::DecodeArrayStart(uint32& num_elements) {
  is_first_item_ = true;
  num_elements = kMaxUInt32;
  return ReadExpectedSeparator('[');
}
DECODE_RESULT JsonDecoder::DecodeArrayContinue(bool& more_elements) {
  return DecodeElementContinue(more_elements, "]");
}

DECODE_RESULT JsonDecoder::DecodeMapStart(uint32& num_pairs) {
  is_first_item_ = true;
  num_pairs = kMaxUInt32;
  return ReadExpectedSeparator('{');
}

DECODE_RESULT JsonDecoder::DecodeMapContinue(bool& more_pairs) {
  return DecodeElementContinue(more_pairs, "}");
}

DECODE_RESULT JsonDecoder::DecodeMapPairStart() {
  decoding_map_key_ = true;
  return DECODE_RESULT_SUCCESS;
}
DECODE_RESULT JsonDecoder::DecodeMapPairMiddle() {
  decoding_map_key_ = false;
  return ReadExpectedSeparator(':');
}
DECODE_RESULT JsonDecoder::DecodeMapPairEnd() {
  return DECODE_RESULT_SUCCESS;
}

DECODE_RESULT JsonDecoder::DecodeBody(rpc::Void& out) {
  string s;
  const io::TokenReadError res = in_.ReadNextAsciiToken(&s);
  if ( res == io::TOKEN_NO_DATA ) {
    return DECODE_RESULT_NOT_ENOUGH_DATA;
  }
  if ( res == io::TOKEN_OK && s == "null" ) {
    return DECODE_RESULT_SUCCESS;
  }
  DLOG_ERROR << "bad void: " << res << " Got: " << s;
  return DECODE_RESULT_ERROR;
}

DECODE_RESULT JsonDecoder::DecodeBody(bool& out) {
  string s;
  const io::TokenReadError res = in_.ReadNextAsciiToken(&s);
  if ( res == io::TOKEN_NO_DATA ) {
    return DECODE_RESULT_NOT_ENOUGH_DATA;
  }
  if ( res == io::TOKEN_OK ) {
    if ( s == "true" ) {
      out = true;
      return DECODE_RESULT_SUCCESS;
    } else if ( s == "false" ) {
      out = false;
      return DECODE_RESULT_SUCCESS;
    }
  }
  DLOG_ERROR << "bad bool: " << res << " Got: " << s;
  return DECODE_RESULT_ERROR;
}

DECODE_RESULT JsonDecoder::DecodeBody(int32& out)  {
  return ReadNumber(out, false, decoding_map_key_);
}
DECODE_RESULT JsonDecoder::DecodeBody(uint32& out) {
  return ReadNumber(out, false, decoding_map_key_);
}
DECODE_RESULT JsonDecoder::DecodeBody(int64& out)  {
  return ReadNumber(out, false, decoding_map_key_);
}
DECODE_RESULT JsonDecoder::DecodeBody(uint64& out) {
  return ReadNumber(out, false, decoding_map_key_);
}
DECODE_RESULT JsonDecoder::DecodeBody(double& out) {
  return ReadNumber(out, true , decoding_map_key_);
}

DECODE_RESULT JsonDecoder::DecodeBody(string& out) {
  string s;
  const io::TokenReadError res = in_.ReadNextAsciiToken(&s);
  if ( res == io::TOKEN_NO_DATA ) {
    return DECODE_RESULT_NOT_ENOUGH_DATA;
  }
  if ( res == io::TOKEN_QUOTED_OK ) {
    DCHECK_GE(s.size(), 2);
      out = strutil::JsonStrUnescape(s.substr(1, s.length() - 2));
      return DECODE_RESULT_SUCCESS;
  }
  DLOG_ERROR << "Json Decoder bad string: " << res << " Got: " << s;
  return DECODE_RESULT_ERROR;
}

void JsonDecoder::Reset() {
  is_first_item_ = true;
  decoding_map_key_ = false;
}

DECODE_RESULT JsonDecoder::DecodeRaw(io::MemoryStream* out) {
  deque<char> paranthesis;
  in_.MarkerSet();
  do {
    string s;
    const io::TokenReadError res = in_.ReadNextAsciiToken(&s);
    switch ( res ) {
      case io::TOKEN_OK:
      case io::TOKEN_QUOTED_OK:
        out->Write(s);
        break;
      case io::TOKEN_SEP_OK:
        out->Write(s);
        if ( s == "{" ) {
          paranthesis.push_back('}');
        } else if ( s == "[" ) {
          paranthesis.push_back(']');
        } else if ( s == "}" || s == "]" ) {
          if ( paranthesis.empty() ) {
            DLOG_DEBUG << "Json Decoder: Badly started paranthesis";
            in_.MarkerClear();
            return DECODE_RESULT_ERROR;
          }
          const char p = paranthesis.back();
          paranthesis.pop_back();
          if ( p != s[0] ) {
            DLOG_DEBUG << "Json Decoder: Badly closed paranthesis: "
                       << "expecting: " << p;
            in_.MarkerClear();
            return DECODE_RESULT_ERROR;
          }
        }
        break;
      case io::TOKEN_ERROR_CHAR:
        in_.MarkerClear();
        return DECODE_RESULT_ERROR;
      case io::TOKEN_NO_DATA:
        in_.MarkerRestore();
        return DECODE_RESULT_NOT_ENOUGH_DATA;
    }
  } while ( !paranthesis.empty() );
  in_.MarkerClear();
  out->Write(" ");    // append always a separator at the end in this cases..
  return DECODE_RESULT_SUCCESS;
}

#define DECODE_EXPECTED_FIELD(expected)                                 \
  do {                                                                  \
    string out;                                                         \
    DECODE_RESULT res = DecodeBody(out);                                \
    if ( res != DECODE_RESULT_SUCCESS ) {                               \
      return res;                                                       \
    }                                                                   \
    if ( expected != out ) {                                            \
      DLOG_DEBUG << " JsonDecoder Expecting " << expected << " got: "   \
                 << res << ": " << out;                                 \
      return DECODE_RESULT_ERROR;                                       \
    }                                                                   \
  } while (false)                                                       \

#define DECODE_EXPECTED_STRUCT_CONTINUE()                               \
  do {                                                                  \
    bool more_attribs = false;                                          \
    DECODE_VERIFY(DecodeStructContinue(more_attribs));                  \
    if ( !more_attribs ) {                                              \
      DLOG_ERROR << "Expected more attributes, but found struct end. in_"; \
      return DECODE_RESULT_ERROR;                                       \
    }                                                                   \
  } while ( false )

#define DECODE_EXPECTED_STRUCT_END()                                    \
  do {                                                                  \
    bool more_attribs = false;                                          \
    DECODE_VERIFY(DecodeStructContinue(more_attribs));                  \
    if ( more_attribs ) {                                               \
      DLOG_ERROR << "Expected structure end, but found more attributes. "; \
      return DECODE_RESULT_ERROR;                                       \
    }                                                                   \
  } while ( false )

#define DECODE_STRING_ATTRIB(name, val)                         \
  do {                                                          \
    DECODE_VERIFY(DecodeStructAttribStart());                   \
    DECODE_EXPECTED_FIELD(name);                                \
    DECODE_VERIFY(DecodeStructAttribMiddle());                  \
    DECODE_VERIFY(DecodeBody((val)));                           \
    DECODE_VERIFY(DecodeStructAttribEnd());                     \
  } while ( false )

#define DECODE_STREAM_ATTRIB(name, val)                         \
  do {                                                          \
    DECODE_VERIFY(DecodeStructAttribStart());                   \
    DECODE_EXPECTED_FIELD(name);                                \
    DECODE_VERIFY(DecodeStructAttribMiddle());                  \
    (val).Clear();                                              \
    DECODE_VERIFY(DecodeRaw(&(val)));                           \
    DECODE_VERIFY(DecodeStructAttribEnd());                     \
  } while ( false )

DECODE_RESULT JsonDecoder::DecodePacket(rpc::Message& out) {
  string field;
  uint32 nAttr;
  DECODE_VERIFY(DecodeStructStart(nAttr));
  {
    DECODE_EXPECTED_FIELD("header");
    DECODE_VERIFY(DecodeStructAttribMiddle());
    {
      rpc::Message::Header& header = out.header_;
      DECODE_VERIFY(DecodeStructStart(nAttr));
      {
        DECODE_STRING_ATTRIB("xid", header.xid_);
        DECODE_EXPECTED_STRUCT_CONTINUE();
        DECODE_STRING_ATTRIB("msgType", header.msgType_);
      }
      DECODE_EXPECTED_STRUCT_END();
    }
    DECODE_VERIFY(DecodeStructAttribEnd());

    DECODE_EXPECTED_STRUCT_CONTINUE();

    DECODE_VERIFY(DecodeStructAttribStart());
    if ( out.header_.msgType_ == RPC_CALL ) {
      rpc::Message::CallBody& cbody = out.cbody_;
      DECODE_EXPECTED_FIELD("cbody");
      DECODE_VERIFY(DecodeStructAttribMiddle());
      {
        DECODE_VERIFY(DecodeStructStart(nAttr));
        DECODE_STRING_ATTRIB("service", cbody.service_);
        DECODE_EXPECTED_STRUCT_CONTINUE();
        DECODE_STRING_ATTRIB("method", cbody.method_);
        DECODE_EXPECTED_STRUCT_CONTINUE();
        DECODE_STREAM_ATTRIB("params", cbody.params_);
        DECODE_EXPECTED_STRUCT_END();
      }
    } else if ( out.header_.msgType_ == RPC_REPLY ) {
      rpc::Message::ReplyBody& rbody = out.rbody_;
      DECODE_EXPECTED_FIELD("rbody");
      DECODE_VERIFY(DecodeStructAttribMiddle());
      {
        DECODE_VERIFY(DecodeStructStart(nAttr));
        DECODE_STRING_ATTRIB("replyStatus", rbody.replyStatus_);
        DECODE_EXPECTED_STRUCT_CONTINUE();
        DECODE_STREAM_ATTRIB("result", rbody.result_);
        DECODE_EXPECTED_STRUCT_END();
      }
    } else {
      LOG_ERROR << "Bad rpc::Message.header.msgType = " << out.header_.msgType_;
      return DECODE_RESULT_ERROR;
    }
    DECODE_VERIFY(DecodeStructAttribEnd());
  }
  DECODE_EXPECTED_STRUCT_END();
  return DECODE_RESULT_SUCCESS;
}

DECODE_RESULT JsonDecoder::ReadExpectedSeparator(char expected) {
  string s;
  const io::TokenReadError res = in_.ReadNextAsciiToken(&s);
  if ( res == io::TOKEN_NO_DATA ) {
    return DECODE_RESULT_NOT_ENOUGH_DATA;
  }
  if ( s[0] == expected ) {
    return DECODE_RESULT_SUCCESS;      // read the expected char
  }
  if ( res != io::TOKEN_SEP_OK ) {
    LOG_ERROR << "Expected '" << expected << "', but found: " << s
              << " - erros: " << res;
    return DECODE_RESULT_ERROR;
  }
  return DECODE_RESULT_ERROR;
}

}
