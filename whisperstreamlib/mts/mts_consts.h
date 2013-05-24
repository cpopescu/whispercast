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

#ifndef __MEDIA_MTS_MTS_CONSTS_H__
#define __MEDIA_MTS_MTS_CONSTS_H__

#include <whisperstreamlib/base/media_info.h>
#include <whisperstreamlib/base/tag.h>
#include <whisperstreamlib/base/tag_serializer.h>

namespace streaming {
namespace mts {

enum DecodeStatus {
  DECODE_SUCCESS, // tag read successfully
  DECODE_NO_DATA, // not enough data to read tag
  DECODE_ERROR,   // stream corrupted
};
const char* DecodeStatusName(DecodeStatus status);

// transport stream packets have a constant length
const uint32 kTSPacketSize = 188;

// transport stream packets start with this byte
const uint8 kTSSyncByte = 0x47;

// PES packets start with this marker (Network Byte Order == BIGENDIAN)
const uint32 kPESMarker = 0x000001;

const uint8 kPESStreamIdProgramMap = 0xBC;
const uint8 kPESStreamIdPrivate1 = 0xBD;
const uint8 kPESStreamIdPrivate2 = 0xBF;
const uint8 kPESStreamIdPadding = 0xBE;
const uint8 kPESStreamIdAudioStart = 0xC0;
const uint8 kPESStreamIdAudioEnd = 0xDF;
const uint8 kPESStreamIdVideoStart = 0xE0;
const uint8 kPESStreamIdVideoEnd = 0xEF;
const uint8 kPESStreamIdECM = 0xF0;
const uint8 kPESStreamIdEMM = 0xF1;
const uint8 kPESStreamIdProgramDirectory = 0xFF;
string PESStreamIdName(uint8 stream_id);

}
}

#endif // __MEDIA_MTS_MTS_CONSTS_H__
