// Copyright (c) 2012, Whispersoft s.r.l.
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

#include <whisperlib/common/base/types.h>
#include <whisperstreamlib/mts/mts_consts.h>

namespace streaming {
namespace mts {

const char* DecodeStatusName(DecodeStatus status) {
  switch ( status ) {
    CONSIDER(DECODE_SUCCESS);
    CONSIDER(DECODE_NO_DATA);
    CONSIDER(DECODE_ERROR);
  }
  LOG_FATAL << "Illegal DecodeStatus: " << (int)status;
  return "Unknown";
}

string PESStreamIdName(uint8 stream_id) {
  if ( stream_id == kPESStreamIdProgramMap ) { return "ProgramMap"; }
  if ( stream_id == kPESStreamIdPrivate1 ) { return "Private1"; }
  if ( stream_id == kPESStreamIdPrivate2 ) { return "Private2"; }
  if ( stream_id == kPESStreamIdPadding ) { return "Padding"; }
  if ( stream_id == kPESStreamIdECM ) { return "ECM"; }
  if ( stream_id == kPESStreamIdEMM ) { return "EMM"; }
  if ( stream_id == kPESStreamIdProgramDirectory ) { return "ProgramDirectory"; }
  if ( stream_id >= kPESStreamIdAudioStart &&
       stream_id <= kPESStreamIdAudioEnd ) {
    return strutil::StringPrintf("Audio(0x%02x)", stream_id);
  }
  if ( stream_id >= kPESStreamIdVideoStart &&
       stream_id <= kPESStreamIdVideoEnd ) {
    return strutil::StringPrintf("Video(0x%02x)", stream_id);
  }
  return strutil::StringPrintf("Unknown(0x%02x)", stream_id);
}

}
}
