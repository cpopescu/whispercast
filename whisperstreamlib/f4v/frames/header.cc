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

#include <sstream>

#include <whisperlib/common/base/log.h>
#include "f4v/frames/header.h"

namespace streaming {
namespace f4v {

#define CONSIDER_STR(type) case type: { static const string str(#type); return str; }
//static
const string& FrameHeader::TypeName(FrameHeader::Type type) {
  switch(type) {
    CONSIDER_STR(AUDIO_FRAME);
    CONSIDER_STR(VIDEO_FRAME);
    CONSIDER_STR(RAW_FRAME);
    default:
      LOG_FATAL << "Illegal FrameHeader::Type: " << type;
      static const string str("Unknown");
      return str;
  }
}
FrameHeader::FrameHeader()
  : offset_(0),
    size_(0),
    decoding_timestamp_(0),
    composition_timestamp_(0),
    duration_(0),
    sample_index_(0),
    type_(AUDIO_FRAME),
    is_keyframe_(false) {
}
FrameHeader::FrameHeader(int64 offset, int64 size, int64 decoding_timestamp,
                         int64 composition_timestamp, int64 duration,
                         int64 sample_index, Type type, bool is_keyframe)
  : offset_(offset),
    size_(size),
    decoding_timestamp_(decoding_timestamp),
    composition_timestamp_(composition_timestamp),
    duration_(duration),
    sample_index_(sample_index),
    type_(type),
    is_keyframe_(is_keyframe) {
}

FrameHeader::~FrameHeader() {
}

string FrameHeader::ToString() const {
  ostringstream oss;
  oss << "{offset_: " << offset_
      << ", size_: " << size_
      << ", decoding_timestamp_: " << decoding_timestamp_
      << ", composition_timestamp_: " << composition_timestamp_
      << ", duration_: " << duration_
      << ", sample_index: " << sample_index_
      << ", type_: " << type_name()
      << ", is_keyframe_: " << is_keyframe_
      << "}";
  return oss.str();
}

} // namespace f4v
} // namespace streaming
