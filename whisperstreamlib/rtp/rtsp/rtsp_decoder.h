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

#ifndef __MEDIA_RTP_RTSP_DECODER_H__
#define __MEDIA_RTP_RTSP_DECODER_H__

#include <vector>

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/io/input_stream.h>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperlib/net/base/address.h>

#include <whisperstreamlib/rtp/rtsp/rtsp_common.h>
#include <whisperstreamlib/rtp/rtsp/types/rtsp_request.h>
#include <whisperstreamlib/rtp/rtsp/types/rtsp_response.h>
#include <whisperstreamlib/rtp/rtsp/types/rtsp_interleaved_frame.h>

namespace streaming {
namespace rtsp {

enum ParserState {
  PARSING_IDLE,
  PARSING_HEADER,
  PARSING_BODY,
};
const char* ParserStateName(ParserState state);

// Message (Request / Response) Decoder
// - keeps a partially decoded message which is gradually constructed by
//   multiple calls to Decode(). This is why you have to decode 1 message
//   at a time.
class Decoder {
public:
  static bool ParseString(char*& in, char sep);
  static bool ParseInteger(char*& in, uint32* out);
  static bool ParseExpected(char*& in, const char* expected);
  static void ParseBlanks(char*& in);
  static bool ParseRequestLine(const string& line, REQUEST_METHOD* out_method,
      URL** out_url, uint32* out_ver_hi, uint32* out_ver_lo);
  static bool ParseResponseLine(const string& line,
      STATUS_CODE* out_status_code, string* out_description,
      uint32* out_ver_hi, uint32* out_ver_lo);

public:
  Decoder();
  virtual ~Decoder();

  void set_remote_address(const string& remote_address);

  //  Reads from 'in' and gradually fills in the local 'msg_'.
  //  This function does partial decoding.
  //  If the 'msg_' gets completed, then it's returned in 'out'.
  // returns:
  //   - DECODE_ERROR: corrupted data in 'in'. Abort parsing.
  //   - DECODE_NO_DATA: message partially decoded, call Decode() again
  //                     when more data is available in 'in'. Nothing in 'out'.
  //   - DECODE_SUCCESS: success. Message returned in 'out'. You can reuse
  //                     this decoder for reading the next response.
  DecodeStatus Decode(io::MemoryStream& in, Message** out);

private:
  // Fill in msg_ which is a Request*.
  DecodeStatus DecodeRequest(io::MemoryStream& in);
  // Fill in msg_ which is a Response*.
  DecodeStatus DecodeResponse(io::MemoryStream& in);
  // Fill in msg_ which is a InterleavedFrame*.
  DecodeStatus DecodeInterleavedFrame(io::MemoryStream& in);
  // This is a common method for both Request and Response.
  DecodeStatus DecodeHeaderAndBody(io::MemoryStream& in, BaseHeader* out_header,
      io::MemoryStream* out_body);
  // Reads from 'in' and gradually fills in 'out'.
  // This could be static, but we leave it a member to use Info() log.
  DecodeStatus DecodeHeader(io::MemoryStream& in, BaseHeader* out);

  string Info() const;

private:
  // just for logging
  string remote_addr_;

  // the current message being parsed (Request or Response)
  Message* msg_;

  // length of the body for the current response
  uint32 content_length_;

  // parser state
  ParserState state_;
};

} // namespace rtsp
} // namespace streaming

#endif // __MEDIA_RTP_RTSP_DECODER_H__
