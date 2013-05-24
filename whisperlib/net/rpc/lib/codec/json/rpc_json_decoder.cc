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

DECODE_RESULT JsonDecoder::DecodeElementContinue(io::MemoryStream& in,
                                                 bool* more_attribs,
                                                 const char* separator) {
  string s;
  in.MarkerSet();
  const io::TokenReadError res = in.ReadNextAsciiToken(&s);
  if ( res == io::TOKEN_NO_DATA ) {
    in.MarkerRestore();
    return DECODE_RESULT_NOT_ENOUGH_DATA;
  }
  if ( res == io::TOKEN_SEP_OK ) {
    if ( s == separator ) {
      *more_attribs = false;
      in.MarkerClear();
      return DECODE_RESULT_SUCCESS;
    }
    if ( s == "," ) {
      *more_attribs = true;
      in.MarkerClear();
      return DECODE_RESULT_SUCCESS;
    }
  }
  if ( !is_first_item_ ) {
    in.MarkerRestore();
    DLOG_ERROR << "Json Bad Struct continues: " << s;
    return DECODE_RESULT_ERROR;
  }
  in.MarkerRestore();
  is_first_item_ = false;
  *more_attribs = true;
  return DECODE_RESULT_SUCCESS;
}

DECODE_RESULT JsonDecoder::DecodeStructStart(io::MemoryStream& in) {
  is_first_item_ = true;
  return ReadExpectedSeparator(in, '{');
}

DECODE_RESULT JsonDecoder::DecodeStructContinue(io::MemoryStream& in,
                                                bool* more_attribs) {
  return DecodeElementContinue(in, more_attribs, "}");
}

DECODE_RESULT JsonDecoder::DecodeStructAttribStart(io::MemoryStream& in) {
  return DECODE_RESULT_SUCCESS;
}
DECODE_RESULT JsonDecoder::DecodeStructAttribMiddle(io::MemoryStream& in) {
  return ReadExpectedSeparator(in, ':');
}
DECODE_RESULT JsonDecoder::DecodeStructAttribEnd(io::MemoryStream& in) {
  return DECODE_RESULT_SUCCESS;
}
DECODE_RESULT JsonDecoder::DecodeArrayStart(io::MemoryStream& in) {
  is_first_item_ = true;
  return ReadExpectedSeparator(in, '[');
}
DECODE_RESULT JsonDecoder::DecodeArrayContinue(io::MemoryStream& in,
                                               bool* more_elements) {
  return DecodeElementContinue(in, more_elements, "]");
}

DECODE_RESULT JsonDecoder::DecodeMapStart(io::MemoryStream& in) {
  is_first_item_ = true;
  return ReadExpectedSeparator(in, '{');
}

DECODE_RESULT JsonDecoder::DecodeMapContinue(io::MemoryStream& in,
                                             bool* more_pairs) {
  return DecodeElementContinue(in, more_pairs, "}");
}

DECODE_RESULT JsonDecoder::DecodeMapPairStart(io::MemoryStream& in) {
  decoding_map_key_ = true;
  return DECODE_RESULT_SUCCESS;
}
DECODE_RESULT JsonDecoder::DecodeMapPairMiddle(io::MemoryStream& in) {
  decoding_map_key_ = false;
  return ReadExpectedSeparator(in, ':');
}
DECODE_RESULT JsonDecoder::DecodeMapPairEnd(io::MemoryStream& in) {
  return DECODE_RESULT_SUCCESS;
}

DECODE_RESULT JsonDecoder::Decode(io::MemoryStream& in, rpc::Void* out) {
  string s;
  const io::TokenReadError res = in.ReadNextAsciiToken(&s);
  if ( res == io::TOKEN_NO_DATA ) {
    return DECODE_RESULT_NOT_ENOUGH_DATA;
  }
  if ( res == io::TOKEN_OK && s == "null" ) {
    return DECODE_RESULT_SUCCESS;
  }
  DLOG_ERROR << "bad void: " << res << " Got: " << s;
  return DECODE_RESULT_ERROR;
}

DECODE_RESULT JsonDecoder::Decode(io::MemoryStream& in, bool* out) {
  string s;
  const io::TokenReadError res = in.ReadNextAsciiToken(&s);
  if ( res == io::TOKEN_NO_DATA ) {
    return DECODE_RESULT_NOT_ENOUGH_DATA;
  }
  if ( res == io::TOKEN_OK ) {
    if ( s == "true" ) {
      *out = true;
      return DECODE_RESULT_SUCCESS;
    } else if ( s == "false" ) {
      *out = false;
      return DECODE_RESULT_SUCCESS;
    }
  }
  DLOG_ERROR << "bad bool: " << res << " Got: " << s;
  return DECODE_RESULT_ERROR;
}

DECODE_RESULT JsonDecoder::Decode(io::MemoryStream& in, int32* out)  {
  return ReadNumber(in, out, false, decoding_map_key_);
}
DECODE_RESULT JsonDecoder::Decode(io::MemoryStream& in, uint32* out) {
  return ReadNumber(in, out, false, decoding_map_key_);
}
DECODE_RESULT JsonDecoder::Decode(io::MemoryStream& in, int64* out)  {
  return ReadNumber(in, out, false, decoding_map_key_);
}
DECODE_RESULT JsonDecoder::Decode(io::MemoryStream& in, uint64* out) {
  return ReadNumber(in, out, false, decoding_map_key_);
}
DECODE_RESULT JsonDecoder::Decode(io::MemoryStream& in, double* out) {
  return ReadNumber(in, out, true , decoding_map_key_);
}

DECODE_RESULT JsonDecoder::Decode(io::MemoryStream& in, string* out) {
  string s;
  const io::TokenReadError res = in.ReadNextAsciiToken(&s);
  if ( res == io::TOKEN_NO_DATA ) {
    return DECODE_RESULT_NOT_ENOUGH_DATA;
  }
  if ( res == io::TOKEN_QUOTED_OK ) {
    DCHECK_GE(s.size(), 2);
      *out = strutil::JsonStrUnescape(s.substr(1, s.length() - 2));
      return DECODE_RESULT_SUCCESS;
  }
  DLOG_ERROR << "Json Decoder bad string: " << res << " Got: " << s;
  return DECODE_RESULT_ERROR;
}

DECODE_RESULT JsonDecoder::ReadRawObject(io::MemoryStream& in,
                                         io::MemoryStream* out) {
  deque<char> paranthesis;
  in.MarkerSet();
  do {
    string s;
    const io::TokenReadError res = in.ReadNextAsciiToken(&s);
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
            in.MarkerClear();
            return DECODE_RESULT_ERROR;
          }
          const char p = paranthesis.back();
          paranthesis.pop_back();
          if ( p != s[0] ) {
            DLOG_DEBUG << "Json Decoder: Badly closed paranthesis: "
                       << "expecting: " << p;
            in.MarkerClear();
            return DECODE_RESULT_ERROR;
          }
        }
        break;
      case io::TOKEN_ERROR_CHAR:
        in.MarkerClear();
        return DECODE_RESULT_ERROR;
      case io::TOKEN_NO_DATA:
        in.MarkerRestore();
        return DECODE_RESULT_NOT_ENOUGH_DATA;
    }
  } while ( !paranthesis.empty() );
  in.MarkerClear();
  out->Write(" ");    // append always a separator at the end in this cases..
  return DECODE_RESULT_SUCCESS;
}

DECODE_RESULT JsonDecoder::ReadExpectedSeparator(io::MemoryStream& in,
                                                 char expected) {
  string s;
  const io::TokenReadError res = in.ReadNextAsciiToken(&s);
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
