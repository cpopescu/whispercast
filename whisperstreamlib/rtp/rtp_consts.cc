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

#include <whisperstreamlib/rtp/rtp_consts.h>

namespace streaming {
namespace rtp {

const char* AvpPayloadTypeName(uint8 avp_payload_type) {
  switch(avp_payload_type) {
    case 0: return "PCMU";
    case 1:
    case 2: return "reserved";
    case 3: return "GSM";
    case 4: return "G723";
    case 5:
    case 6: return "DVI4";
    case 7: return "LPC";
    case 8: return "PCMA";
    case 9: return "G722";
    case 10:
    case 11: return "L16";
    case 12: return "QCELP";
    case 13: return "CN";
    case 14: return "MPA";
    case 15: return "G728";
    case 16:
    case 17: return "DVI4";
    case 18: return "G729";
    case 19: return "reserved";
    case 20:
    case 21:
    case 22:
    case 23:
    case 24: return "unassigned";
    case 25: return "CelB";
    case 26: return "JPEG";
    case 27: return "unassigned";
    case 28: return "nv";
    case 29:
    case 30: return "unassigned";
    case 31: return "H261";
    case 32: return "MPV";
    case 33: return "MP2T";
    case 34: return "H263";
  }
  if ( (avp_payload_type >= 35 && avp_payload_type <= 71) ||
       (avp_payload_type >= 77 && avp_payload_type <= 95) ) {
    return "unassigned";
  }
  if ( avp_payload_type >= 72 && avp_payload_type <= 76 ) {
    return "reserved";
  }
  if ( avp_payload_type >= 96 && avp_payload_type <= 127 ) {
    return "dynamic";
  }
  return "unknown";
}

} // namespace rtp
} // namespace streaming

