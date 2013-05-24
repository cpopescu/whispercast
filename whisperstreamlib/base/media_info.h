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
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperstreamlib/f4v/frames/header.h>
#include <whisperstreamlib/rtmp/objects/rtmp_objects.h>

namespace streaming {

namespace f4v {
class MoovAtom;
}

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
    string ToString(const char* glue = ", ") const;

    Format format_;
    uint8 channels_; // 1 or 2
    uint32 sample_rate_; // e.g. 44100
    uint32 sample_size_; // e.g. 16
    uint32 bps_; // e.g. 96000 ( = 96 kbps)

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
    string ToString(const char* glue = ", ") const;

    Format format_;
    uint32 width_;  // in pixels
    uint32 height_; // in pixels
    uint32 clock_rate_; // e.g. 90000 for H264/AVC
    float frame_rate_; // fps
    uint32 bps_; // e.g. 512000 ( = 512 kbps)

    uint32 timescale_; // clock ticks / second

    // MP4 specific
    uint32 mp4_moov_position_;

    // H264 specific
    uint8 h264_configuration_version_;
    uint8 h264_profile_;
    uint8 h264_profile_compatibility_;
    uint8 h264_level_;
    uint8 h264_nalu_length_size_;
    vector<string> h264_sps_; // sequence parameter set, binary format
    vector<string> h264_pps_; // picture parameter set, binary format

    // In FLV container we have:
    //  <7 unknown bytes> <2 bytes NALU size> <NALU body> <2 bytes NALU size> ..
    // In MP4 container we have:
    //  <4 bytes NALU size> <NALU body> <4 bytes NALU size> ...
    bool h264_flv_container_;

    // If true: <NALU start code> <NALU body> <NALU start code> <NALU body> ...
    //   false: <2/4 bytes NALU size> <NALU body> ...
    // The <NALU start code> can be 3 bytes: 0x000001, or 4 bytes: 0x00000001
    bool h264_nalu_start_code_;

    // the complete AVCC atom body (without the header type & size: 8 bytes)
    string h264_avcc_;
  };
  struct Frame {
    // true = audio frame
    // false = video frame
    bool is_audio_;
    // frame size in bytes
    uint32 size_;
    // decoding timestamp
    int64 decoding_ts_;
    // Composition Time = Decoding Time + Composition Offset Ms
    uint32 composition_offset_ms_;
    // valid on Video frames only. Marks a keyframe.
    bool is_keyframe_;
    Frame(bool is_audio, uint32 size, int64 decoding_ts,
             uint32 composition_offset_ms, bool is_keyframe)
      : is_audio_(is_audio), size_(size), decoding_ts_(decoding_ts),
        composition_offset_ms_(composition_offset_ms),
        is_keyframe_(is_keyframe) {}
    string ToString() const;
  };

public:
  MediaInfo();
  MediaInfo(const MediaInfo& other);
  virtual ~MediaInfo();

  bool has_audio() const;
  bool has_video() const;

  const Audio& audio() const;
  Audio* mutable_audio();

  const Video& video() const;
  Video* mutable_video();

  uint32 duration_ms() const;
  void set_duration_ms(uint32 duration_ms);

  uint64 file_size() const;
  void set_file_size(uint64 file_size);

  bool seekable() const;
  void set_seekable(bool seekable);

  bool pausable() const;
  void set_pausable(bool pausable);

  const vector<Frame>& frames() const;
  vector<Frame>* mutable_frames();
  void set_frames(const vector<f4v::FrameHeader*>& frames);

  const rtmp::CMixedMap& flv_extra_metadata() const;
  rtmp::CMixedMap* mutable_flv_extra_metadata();

  const f4v::MoovAtom* mp4_moov() const;
  f4v::MoovAtom* mutable_mp4_moov();
  void set_mp4_moov(const f4v::MoovAtom& moov);

  string ToString() const;

private:
  // audio attributes
  Audio audio_;
  // video attributes
  Video video_;

  // media duration
  uint32 duration_ms_;
  // in bytes; useful if media is a file, otherwise 0
  uint64 file_size_;
  // is media seekable
  bool seekable_;
  // is media pausable (more like a stream controller attribute)
  bool pausable_;

  // stream frames. Some formats have them: mp4.
  // For other formats this is empty: flv, mp3, aac.
  vector<Frame> frames_;

  // extra metadata values that are not extracted into audio/video/duration..
  rtmp::CMixedMap flv_extra_metadata_;

  // TODO(cosmin): the whole MOOV, because I don't have a solution for storing
  //               the extra atoms MOOV may contain.
  f4v::MoovAtom* mp4_moov_;
};

}

#endif // __MEDIA_SOURCE_MEDIA_INFO_H__
