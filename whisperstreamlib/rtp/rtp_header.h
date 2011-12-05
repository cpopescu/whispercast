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

#ifndef __MEDIA_RTP_HEADER_H__
#define __MEDIA_RTP_HEADER_H__

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/io/buffer/memory_stream.h>

namespace streaming {
namespace rtp {

// Implements the RTP header.
class Header {
public:
  Header();
  virtual ~Header();

  void set_flag_version(uint8 two_bit_version);
  void set_flag_padding(uint8 one_bit_padding);
  void set_flag_extension(uint8 one_bit_extension);
  void set_flag_cssid(uint8 four_bit_cssid);
  void set_payload_type_marker(uint8 one_bit_marker);
  void set_payload_type (uint8 seven_bit_payload_type);
  void set_sequence_number(uint16 sequence_number);
  void set_timestamp(uint32 timestamp);
  void set_ssrc(uint32 ssrc);

  void Encode(io::MemoryStream* out);

private:
  uint8 flags_;
  uint8 payload_type_;
  uint16 sequence_number_;
  uint32 timestamp_;
  uint32 ssrc_;
};

} // namespace rtp
} // namespace streaming

#endif // __MEDIA_RTP_HEADER_H__
