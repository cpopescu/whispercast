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

#ifndef __MEDIA_RTP_RTP_CONSTS_H__
#define __MEDIA_RTP_RTP_CONSTS_H__

#include <whisperlib/common/base/types.h>

namespace streaming {
namespace rtp {

static const uint32 kRtpMtu = 1386;

enum {
  AVP_PAYLOAD_PCMU = 0,
  AVP_PAYLOAD_GSM = 3,
  AVP_PAYLOAD_G723 = 4,
  AVP_PAYLOAD_DVI4 = 6,
  AVP_PAYLOAD_LPC = 7,
  AVP_PAYLOAD_PCMA = 8,
  AVP_PAYLOAD_G722 = 9,
  AVP_PAYLOAD_L16 = 11,
  AVP_PAYLOAD_QCELP = 12,
  AVP_PAYLOAD_CN = 13,
  AVP_PAYLOAD_MPA = 14,
  AVP_PAYLOAD_G728 = 15,
  AVP_PAYLOAD_DVI4_2 = 17,
  AVP_PAYLOAD_G729 = 18,
  AVP_PAYLOAD_CelB = 25,
  AVP_PAYLOAD_JPEG = 26,
  AVP_PAYLOAD_H261 = 31,
  AVP_PAYLOAD_MPV = 32,
  AVP_PAYLOAD_MP2T = 33,
  AVP_PAYLOAD_H263 = 34,
  AVP_PAYLOAD_DYNAMIC_AAC = 96,
  AVP_PAYLOAD_DYNAMIC_H264 = 97,
};

// AVP payload type, see RFC 3551
const char* AvpPayloadTypeName(uint8 avp_payload_type);


} // namespace rtp
} // namespace streaming

#endif // __MEDIA_RTP_RTP_CONSTS_H__
