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

#include "flv/flv_consts.h"

namespace streaming {
const char* FlvFrameTypeName(FlvFrameType frame_type) {
  switch ( frame_type ) {
    CONSIDER(FLV_FRAMETYPE_VIDEO);
    CONSIDER(FLV_FRAMETYPE_AUDIO);
    CONSIDER(FLV_FRAMETYPE_METADATA);
  }
  return "FLV_FRAMETYPE_UNKNOWN";
}
const char* FlvFlagSoundTypeName(FlvFlagSoundType flag) {
  switch ( flag ) {
    CONSIDER(FLV_FLAG_SOUND_TYPE_MONO);
    CONSIDER(FLV_FLAG_SOUND_TYPE_STEREO);
  }
  return "FLV_FLAG_SOUND_TYPE_UNKNOWN";
}

const char* FlvFlagSoundSizeName(FlvFlagSoundSize flag) {
  switch ( flag ) {
    CONSIDER(FLV_FLAG_SOUND_SIZE_8_BIT);
    CONSIDER(FLV_FLAG_SOUND_SIZE_16_BIT);
  }
  return "FLV_FLAG_SOUND_SIZE_UNKNOWN";
}

const char* FlvFlagSoundRateName(FlvFlagSoundRate flag) {
  switch ( flag ) {
    CONSIDER(FLV_FLAG_SOUND_RATE_5_5_KHZ);
    CONSIDER(FLV_FLAG_SOUND_RATE_11_KHZ);
    CONSIDER(FLV_FLAG_SOUND_RATE_22_KHZ);
    CONSIDER(FLV_FLAG_SOUND_RATE_44_KHZ);
  }
  return "FLV_FLAG_SOUND_RATE_UNKNOWN";
}
uint32 FlvFlagSoundRateNumber(FlvFlagSoundRate flag) {
  switch ( flag ) {
    case FLV_FLAG_SOUND_RATE_5_5_KHZ: return 8000;
    case FLV_FLAG_SOUND_RATE_11_KHZ: return 11025;
    case FLV_FLAG_SOUND_RATE_22_KHZ: return 22050;
    case FLV_FLAG_SOUND_RATE_44_KHZ: return 44100;
  }
  return 0;
}

const char* FlvFlagSoundFormatName(FlvFlagSoundFormat flag) {
  switch ( flag ) {
    CONSIDER(FLV_FLAG_SOUND_FORMAT_RAW);
    CONSIDER(FLV_FLAG_SOUND_FORMAT_ADPCM);
    CONSIDER(FLV_FLAG_SOUND_FORMAT_MP3);
    CONSIDER(FLV_FLAG_SOUND_FORMAT_NELLYMOSER_8_KHZ);
    CONSIDER(FLV_FLAG_SOUND_FORMAT_NELLYMOSER);
    CONSIDER(FLV_FLAG_SOUND_FORMAT_AAC);
  }
  return "FLV_FLAG_SOUND_FORMAT_UNKNOWN";
}

const char* FlvFlagVideoCodecName(FlvFlagVideoCodec flag) {
  switch ( flag ) {
    CONSIDER(FLV_FLAG_VIDEO_CODEC_JPEG);
    CONSIDER(FLV_FLAG_VIDEO_CODEC_H263);
    CONSIDER(FLV_FLAG_VIDEO_CODEC_SCREEN);
    CONSIDER(FLV_FLAG_VIDEO_CODEC_VP6);
    CONSIDER(FLV_FLAG_VIDEO_CODEC_VP6_WITH_ALPHA);
    CONSIDER(FLV_FLAG_VIDEO_CODEC_SCREEN_VIDEO_V2);
    CONSIDER(FLV_FLAG_VIDEO_CODEC_AVC);
  }
  return "FLV_FLAG_VIDEO_CODEC_UNKNOWN";
}

const char* FlvFlagVideoFrameTypeName(FlvFlagVideoFrameType flag) {
  switch ( flag ) {
    CONSIDER(FLV_FLAG_VIDEO_FRAMETYPE_KEYFRAME);
    CONSIDER(FLV_FLAG_VIDEO_FRAMETYPE_INTERFRAME);
    CONSIDER(FLV_FLAG_VIDEO_FRAMETYPE_DISPOSABLE);
    CONSIDER(FLV_FLAG_VIDEO_FRAMETYPE_GENERATED);
    CONSIDER(FLV_FLAG_VIDEO_FRAMETYPE_COMMAND);
  }
  return "FLV_FLAG_VIDEO_FRAMETYPE_UNKNOWN";
}

const char* AvcPacketTypeName(AvcPacketType flag) {
  switch ( flag ) {
    CONSIDER(AVC_SEQUENCE_HEADER);
    CONSIDER(AVC_NALU);
    CONSIDER(AVC_END_OF_SEQUENCE);
  }
  return "AVC_UNKNOWN";
}
}
