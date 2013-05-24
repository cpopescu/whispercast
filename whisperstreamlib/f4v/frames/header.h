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

#ifndef __MEDIA_F4V_F4V_FRAME_HEADER_H__
#define __MEDIA_F4V_F4V_FRAME_HEADER_H__

#include <whisperlib/common/base/types.h>

namespace streaming {
namespace f4v {

class FrameHeader {
 public:
  enum Type {
    AUDIO_FRAME,
    VIDEO_FRAME,
    RAW_FRAME,
  };
  static const string& TypeName(Type type);
 public:
  FrameHeader();
  FrameHeader(int64 offset, int64 size, int64 decoding_timestamp,
              int64 composition_offset_ms, int64 duration,
              int64 sample_index, Type type, bool is_keyframe);
  virtual ~FrameHeader();

  int64 offset() const { return offset_; }
  int64 size() const { return size_; }
  int64 timestamp() const { return decoding_timestamp_; }
  int64 decoding_timestamp() const { return decoding_timestamp_; }
  int64 composition_offset_ms() const { return composition_offset_ms_; }
  int64 composition_timestamp() const { return decoding_timestamp_ + composition_offset_ms_; }
  int64 duration() const { return duration_; }
  int64 sample_index() const { return sample_index_; }
  Type type() const { return type_; }
  const string& type_name() const { return TypeName(type_); }
  bool is_keyframe() const { return is_keyframe_; }

  void set_keyframe(bool keyframe) { is_keyframe_ = keyframe; }

  bool Equals(const FrameHeader& other) const;

  string ToString() const;
 private:
  // offset from file begin
  int64 offset_;
  // in bytes
  int64 size_;
  // milliseconds, relative to MDAT body start
  int64 decoding_timestamp_;
  // milliseconds, always greater than decoding_timestamp
  int64 composition_offset_ms_;
  // milliseconds duration
  int64 duration_;
  // samples, relative to MDAT body start
  int64 sample_index_;
  // frame type
  Type type_;
  // key frame, applicable to video frames only
  bool is_keyframe_;
};

} // namespace f4v
} // namespace streaming

#endif // __MEDIA_F4V_F4V_FRAME_HEADER_H__
