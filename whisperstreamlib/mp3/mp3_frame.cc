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
// Author: Catalin Popescu

#include <whisperlib/common/base/log.h>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include "mp3/mp3_frame.h"

namespace streaming {

const int Mp3FrameTag::kMp3HeaderLen = 4;
const Tag::Type Mp3FrameTag::kType = Tag::TYPE_MP3;

string Mp3FrameTag::ToStringBody() const {
  return strutil::StringPrintf(
      " MP3FRAME{%s - %s} %s [%d kbps @ %d Hz] len: %d [pad: %d, crc: %d]",
      VersionName(), LayerName(), ChannelModeName(),
      bitrate_kbps_, sampling_rate_hz_, frame_length_bytes_,
      padding_, has_crc_);
}

streaming::TagReadStatus Mp3FrameTag::Read(io::MemoryStream* in,
                                       bool do_synchronize) {
  if ( version_ == VERSION_UNKNOWN ) {
    uint8 buffer[kMp3HeaderLen];
    while ( do_synchronize ) {
      if ( 2 != in->Peek(buffer, 2) ) {
        return streaming::READ_NO_DATA;
      }
      if ( buffer[0] == 0xFF && (buffer[1] & 0xE0) == 0xE0 ) {
        break;
      }
      in->Skip(1);
    }
    if ( in->Size() < kMp3HeaderLen ) {
      return streaming::READ_NO_DATA;
    }
    in->MarkerSet();
    CHECK_EQ(in->Read(buffer, kMp3HeaderLen), kMp3HeaderLen);
    in->MarkerRestore();
    streaming::TagReadStatus err = ExtractHeader(buffer);
    if ( err != streaming::READ_OK ) {
      return err;
    }
  }
  if ( in->Size() < frame_length_bytes_ ) {
    return streaming::READ_NO_DATA;
  }
  data_.AppendStream(in, frame_length_bytes_);
  return streaming::READ_OK;
}

int64 Mp3FrameTag::TimeLengthInMs() const {
  DCHECK_NE(layer_, LAYER_UNKNOWN);
  if ( layer_ == MPEG_LAYER_I ) {
    return static_cast<int64>((384 * 1000.00) / sampling_rate_hz_);
  } else {
    return static_cast<int64>((1152 * 1000.00) / sampling_rate_hz_);
  }
}

//////////////////////////////////////////////////////////////////////

static const Mp3FrameTag::Version kVersionIndex[] = {
  Mp3FrameTag::MPEG_VERSION_2_5,
  Mp3FrameTag::VERSION_UNKNOWN,
  Mp3FrameTag::MPEG_VERSION_2,
  Mp3FrameTag::MPEG_VERSION_1,
};
static const Mp3FrameTag::Layer kLayerIndex[] = {
  Mp3FrameTag::LAYER_UNKNOWN,
  Mp3FrameTag::MPEG_LAYER_III,
  Mp3FrameTag::MPEG_LAYER_II,
  Mp3FrameTag::MPEG_LAYER_I,
};


// bits | V1,L1 | V1,L2 | V1,L3 | V2,L1 | V2, L2 & L3 |
static const int kBitrate[] = {
  // free free free free free
  0,   0,   0,   0,   0,
  32,  32,  32,  32,  8,
  64,  48,  40,  48,  16,
  96,  56,  48,  56,  24,
  128, 64,  56,  64,  32,
  160, 80,  64,  80,  40,
  192, 96,  80,  96,  48,
  224, 112, 96,  112, 56,
  256, 128, 112, 128, 64,
  288, 160, 128, 144, 80,
  320, 192, 160, 160, 96,
  352, 224, 192, 176, 112,
  384, 256, 224, 192, 128,
  416, 320, 256, 224, 144,
  448, 384, 320, 256, 160,
  -1,   -1,  -1,  -1,  -1,
};

static const int kSamplerate[] = {
  // MPEG1 MPEG2 MPEG2.5
  44100, 22050, 11025,
  48000, 24000, 12000,
  32000, 16000, 8000,
  0,     0,     0
};

streaming::TagReadStatus Mp3FrameTag::ExtractHeader(
    const uint8 buffer[kMp3HeaderLen]) {
  if ( buffer[0] != 0xFF || (buffer[1] & 0xE0) != 0xE0 ) {
    LOG_ERROR << " Found wrong header in mp3 data."
              << hex << "0x" << static_cast<int>(buffer[0])
              << " - " << "0x" << static_cast<int>(buffer[1]) << dec;
    return streaming::READ_CORRUPTED_FAIL;
  }
  const uint8 version_index = ((buffer[1] & 0x18)  >> 3) & 0x03;
  version_ = kVersionIndex[version_index];
  const uint8 layer_index   = ((buffer[1] & 0x06)  >> 1) & 0x03;
  layer_ = kLayerIndex[layer_index];
  if ( version_ == VERSION_UNKNOWN || layer_ == LAYER_UNKNOWN ) {
    LOG_ERROR << " Mp3 error: version_index=" << static_cast<int>(version_index)
              << " layer_index=" << static_cast<int>(layer_index);
    return streaming::READ_CORRUPTED_FAIL;
  }
  has_crc_ = ((buffer[1] & 0x01) == 0);
  const uint8 bitrate_index = ((buffer[2] & 0xF0)  >> 4) & 0x0F;
  if ( bitrate_index == 0x0 ) {
    LOG_ERROR << " Mp3 error: version_index=" << static_cast<int>(version_index)
              << " layer_index=" << static_cast<int>(layer_index)
              << " bitrate_index=" << static_cast<int>(bitrate_index);
    return streaming::READ_UNKNOWN;
  }
  if ( bitrate_index == 0xF ) {
    LOG_ERROR << " Mp3 error: version_index=" << version_index
              << " layer_index=" << layer_index
              << " bitrate_index=" << bitrate_index;
    return streaming::READ_CORRUPTED_FAIL;
  }
  if ( version_ == MPEG_VERSION_1 ) {
    bitrate_kbps_ = kBitrate[bitrate_index * 5 + layer_];
  } else {
    bitrate_kbps_ = kBitrate[bitrate_index * 5 + 3 + (layer_ != MPEG_LAYER_I)];
  }
  const uint8 sampling_rate_index = ((buffer[2] & 0x0C)  >> 2) & 0x03;
  sampling_rate_hz_ = kSamplerate[sampling_rate_index * 3 + version_];
  if ( sampling_rate_hz_ == 0 ) {
    LOG_ERROR << " Mp3 error: version_index=" << static_cast<int>(version_index)
              << " layer_index=" << static_cast<int>(layer_index)
              << " bitrate_index=" << static_cast<int>(bitrate_index);
    return streaming::READ_CORRUPTED_FAIL;
  }
  padding_ = ((buffer[2] & 0x02) >> 1) != 0;
  const uint8 channel_index = ((buffer[3] & 0xC0)  >> 6) & 0x03;
  channel_mode_ = ChannelMode(channel_index);

  if ( layer_ == MPEG_LAYER_I ) {
    frame_length_bytes_ = (12 * 1000 * bitrate_kbps_ / sampling_rate_hz_ +
                           (padding_ == true)) * 4;
  } else {
    frame_length_bytes_ = (144 * 1000 * bitrate_kbps_ / sampling_rate_hz_);
    if ( version_ == Mp3FrameTag::MPEG_VERSION_2_5 &&
         (channel_index == SINGLE_CHANNEL ||
          channel_index == JOINT_STEREO) ) {
      frame_length_bytes_ /= 2;
    }
    if ( padding_ ) {
      frame_length_bytes_++;
    }
  }
  /* TODO(miancule): The code below seems to be correct.. but it's not working.
  if ( has_crc_ ) {
    frame_length_bytes_ += sizeof(uint16);
  }
  */

  VLOG(10) << "Extracted an MP3 header: " << ToString();
  return streaming::READ_OK;
}

//////////////////////////////////////////////////////////////////////

const char* Mp3FrameTag::VersionName(Version version) {
  switch (version) {
    CONSIDER(VERSION_UNKNOWN);
    CONSIDER(MPEG_VERSION_1);
    CONSIDER(MPEG_VERSION_2);
    CONSIDER(MPEG_VERSION_2_5);
  }
  return "VERSION-WRONG";
}
const char* Mp3FrameTag::LayerName(Layer layer) {
  switch (layer) {
    CONSIDER(LAYER_UNKNOWN);
    CONSIDER(MPEG_LAYER_I);
    CONSIDER(MPEG_LAYER_II);
    CONSIDER(MPEG_LAYER_III);
  }
  return "LAYER-WRONG";
}

const char* Mp3FrameTag::ChannelModeName(ChannelMode channel) {
  switch (channel) {
    CONSIDER(CHANNEL_UNKNOWN);
    CONSIDER(STEREO);
    CONSIDER(JOINT_STEREO);
    CONSIDER(DUAL_CHANNEL);
    CONSIDER(SINGLE_CHANNEL);
  }
  return "CHANNEL_WRONG";
}
}
