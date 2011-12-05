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

#ifndef __MEDIA_FLV_FLV_CONSTS_H__
#define __MEDIA_FLV_FLV_CONSTS_H__

#include <whisperlib/common/base/types.h>

namespace streaming {

// Constants found in FLV files / streams.
// Tag Types
enum FlvFrameType {
  // Video data
  FLV_FRAMETYPE_VIDEO = 0x09,      // == rtmp::EVENT_VIDEO_DATA,
  // Audio data
  FLV_FRAMETYPE_AUDIO = 0x08,      // == rtmp::EVENT_AUDIO_DATA,
  // Metadata
  FLV_FRAMETYPE_METADATA = 0x12,   // == rtmp::EVENT_STREAM_METAEVENT,
};
const char* FlvFrameTypeName(FlvFrameType frame_type);

//////////////////////////////////////////////////////////////////////

// Audio Flags in the first data byte of Audio Tags

// Masks for extracting sound types
static const int kFlvMaskSoundType   = 0x01;
static const int kFlvMaskSoundSize   = 0x02;
static const int kFlvMaskSoundRate   = 0x0C;
static const int kFlvMaskSoundFormat = 0xF0;

enum FlvFlagSoundType {
  FLV_FLAG_SOUND_TYPE_MONO = 0x00,
  FLV_FLAG_SOUND_TYPE_STEREO = 0x01,
};
const char* FlvFlagSoundTypeName(FlvFlagSoundType sound_type);

enum FlvFlagSoundSize {
  FLV_FLAG_SOUND_SIZE_8_BIT = 0x00,
  FLV_FLAG_SOUND_SIZE_16_BIT = 0x01,
};
const char* FlvFlagSoundSizeName(FlvFlagSoundSize sound_size);


// Possible rates: 5500, 11250, 22000 and 44000
// TODO(cpopescu): actually 44100 and divisors..
enum FlvFlagSoundRate {
  FLV_FLAG_SOUND_RATE_5_5_KHZ = 0x00,
  FLV_FLAG_SOUND_RATE_11_KHZ  = 0x01,
  FLV_FLAG_SOUND_RATE_22_KHZ  = 0x02,
  FLV_FLAG_SOUND_RATE_44_KHZ  = 0x03,
};
const char* FlvFlagSoundRateName(FlvFlagSoundRate soundrate);
uint32 FlvFlagSoundRateNumber(FlvFlagSoundRate soundrate);

enum FlvFlagSoundFormat {
  FLV_FLAG_SOUND_FORMAT_RAW               = 0x00,
  FLV_FLAG_SOUND_FORMAT_ADPCM             = 0x01,
  FLV_FLAG_SOUND_FORMAT_MP3               = 0x02,
  FLV_FLAG_SOUND_FORMAT_NELLYMOSER_8_KHZ  = 0x05,
  FLV_FLAG_SOUND_FORMAT_NELLYMOSER        = 0x06,

  FLV_FLAG_SOUND_FORMAT_AAC               = 0x0A,
};
const char* FlvFlagSoundFormatName(FlvFlagSoundFormat sound_format);


//////////////////////////////////////////////////////////////////////
//
//        Video Flags in the first data byte of Video Tags

static const int kFlvMaskVideoCodec = 0x0F;
static const int kFlvMaskVideoFrameType = 0xF0;

enum FlvFlagVideoCodec {
  FLV_FLAG_VIDEO_CODEC_JPEG             = 0x01,
  FLV_FLAG_VIDEO_CODEC_H263             = 0x02,
  FLV_FLAG_VIDEO_CODEC_SCREEN           = 0x03,
  FLV_FLAG_VIDEO_CODEC_VP6              = 0x04,
  FLV_FLAG_VIDEO_CODEC_VP6_WITH_ALPHA   = 0x05,
  FLV_FLAG_VIDEO_CODEC_SCREEN_VIDEO_V2  = 0x06,
  FLV_FLAG_VIDEO_CODEC_AVC              = 0x07,
};
const char* FlvFlagVideoCodecName(FlvFlagVideoCodec flag);

enum FlvFlagVideoFrameType {
  FLV_FLAG_VIDEO_FRAMETYPE_KEYFRAME   = 0x01,
  FLV_FLAG_VIDEO_FRAMETYPE_INTERFRAME = 0x02,
  FLV_FLAG_VIDEO_FRAMETYPE_DISPOSABLE = 0x03,
  FLV_FLAG_VIDEO_FRAMETYPE_GENERATED  = 0x04,
  FLV_FLAG_VIDEO_FRAMETYPE_COMMAND    = 0x05,
};
const char* FlvFlagVideoFrameTypeName(FlvFlagVideoFrameType flag);

enum AvcPacketType {
  AVC_SEQUENCE_HEADER = 0x00,
  AVC_NALU            = 0x01,
  AVC_END_OF_SEQUENCE = 0x02,
};
const char* AvcPacketTypeName(AvcPacketType flag);

static const int kFlvHeaderSize = 9;
static const int kFlvTagHeaderSize = 15;

//////////////////////////////////////////////////////////////////////

static const char kOnMetaData[] = "onMetaData";
static const char kOnCuePoint[] = "onCuePoint";

//////////////////////////////////////////////////////////////////////

// A special value that denotes a header when present in "previous_tag_size"
static const uint32 kFlvHeaderMark  = 0x464c5600;
static const uint32 kFlvHeaderMask  = 0xFFFFFF00;
}

#endif  // __NET_RTMP_FLV_FLV_CONSTS_H__
