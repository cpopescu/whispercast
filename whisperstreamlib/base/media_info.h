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

#ifndef __MEDIA_SOURCE_MEDIA_INFO_H__
#define __MEDIA_SOURCE_MEDIA_INFO_H__

#include <string>
#include <vector>
#include <whisperlib/common/base/types.h>

namespace streaming {

class MediaInfo {
public:
  struct Audio {
    enum Format {
      FORMAT_AAC,
      FORMAT_MP3,
    };
    static const char* FormatName(Format format);

    Audio();
    ~Audio();
    const char* format_name() const;
    string ToString() const;

    Format format_;
    uint8 channels_; // 1 or 2
    uint32 sample_rate_; // e.g. 44100

    // MP4 specific
    string mp4_language_; // e.g. "eng"

    // AAC specific
    uint8 aac_level_;
    uint8 aac_profile_;
    uint8 aac_config_[2]; // 2 bytes AAC configuration
    // AAC Frame format inside FLV container: <2 bytes unknown> <aac frame>
    // AAC Frame format inside MP4/regular container: <aac frame>
    bool aac_flv_container_;

    // MP3 specific
    // MP3 Frame format inside FLV container:
    //   <0x2e> <4 bytes mp3 header> <mp3 data> ..
    // MP3 Frame format inside MP3/regular container:
    //   <4 bytes mp3 header> <mp3 data> ..
    bool mp3_flv_container_;
  };
  struct Video {
    enum Format {
      FORMAT_H263,
      FORMAT_H264,
      FORMAT_ON_VP6,
    };
    static const char* FormatName(Format format);

    Video();
    ~Video();
    const char* format_name() const;
    string ToString() const;

    Format format_;
    uint32 width_;  // in pixels
    uint32 height_; // in pixels
    uint32 clock_rate_; // e.g. 90000 for H264/AVC
    float frame_rate_; // fps

    // MP4 specific
    uint32 mp4_moov_position_;

    // H264 specific
    vector<uint8> h264_pps_; // picture parameter set, binary format
    vector<uint8> h264_sps_; // sequence parameter set, binary format
    uint8 h264_profile_;
    uint8 h264_profile_compatibility_;
    uint8 h264_level_;
    // In FLV container we have:
    //  <7 unknown bytes> <2 bytes NALU size> <NALU body> <2 bytes NALU size> ..
    // In MP4 container we have:
    //  <4 bytes NALU size> <NALU body> <4 bytes NALU size> ...
    bool h264_flv_container_;
  };

public:
  MediaInfo();
  virtual ~MediaInfo();

  bool has_audio() const;
  bool has_video() const;

  const Audio& audio() const;
  Audio* mutable_audio();

  const Video& video() const;
  Video* mutable_video();

  uint32 duration_ms() const;
  void set_duration_ms(uint32 duration_ms);

  string ToString() const;

private:
  Audio audio_;
  Video video_;
  uint32 duration_ms_;
};

}

#endif // __MEDIA_SOURCE_MEDIA_INFO_H__
