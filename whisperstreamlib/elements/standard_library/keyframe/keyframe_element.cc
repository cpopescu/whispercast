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

#include "elements/standard_library/keyframe/keyframe_element.h"

namespace {

class KeyFrameExtractorCallbackData : public streaming::FilteringCallbackData {
 public:
  KeyFrameExtractorCallbackData(int64 ms_between_video_frames,
                                bool drop_audio)
      : ms_between_video_frames_(ms_between_video_frames),
        drop_audio_(drop_audio),
        st_calculator_(),
        last_keyframe_ts_(0) {}
  virtual ~KeyFrameExtractorCallbackData() {}

  /////////////////////////////////////////////////////////////////////
  // FilteringCallbackData methods
  virtual void FilterTag(const streaming::Tag* tag,
                         int64 timestamp_ms,
                         TagList* out) {
    st_calculator_.ProcessTag(tag, timestamp_ms);
    int64 tag_ts = st_calculator_.stream_time_ms();

    // drop audio
    if ( tag->is_audio_tag() && drop_audio_ ) {
      return;
    }

    // drop interframes
    if ( tag->is_video_tag() && !tag->can_resync() ) {
      return;
    }

    // maybe drop video keyframes that come too fast
    if ( tag->is_video_tag() && tag->can_resync() &&
         tag_ts - last_keyframe_ts_ < ms_between_video_frames_ ) {
      return;
    }

    // default: forward the remains
    out->push_back(FilteredTag(tag, timestamp_ms));
    if ( tag->is_video_tag() ) {
      last_keyframe_ts_ = tag_ts;
    }
  }

 private:
  const int64 ms_between_video_frames_;
  const bool drop_audio_;

  streaming::StreamTimeCalculator st_calculator_;
  int64 last_keyframe_ts_;

  DISALLOW_EVIL_CONSTRUCTORS(KeyFrameExtractorCallbackData);
};
}

namespace streaming {
const char KeyFrameExtractorElement::kElementClassName[] = "keyframe";

//////////////////////////////////////////////////////////////////////
//
//  KeyFrameExtractorElement
//
KeyFrameExtractorElement::KeyFrameExtractorElement(
    const string& name,
    ElementMapper* mapper,
    net::Selector* selector,
    int64 ms_between_video_frames,
    bool drop_audio)
    : FilteringElement(kElementClassName, name, mapper, selector),
      ms_between_video_frames_(ms_between_video_frames),
      drop_audio_(drop_audio) {
}
KeyFrameExtractorElement::~KeyFrameExtractorElement() {
}

bool KeyFrameExtractorElement::Initialize() {
  return true;
}

FilteringCallbackData* KeyFrameExtractorElement::CreateCallbackData(
    const string& media_name,
    streaming::Request* req) {
  return new KeyFrameExtractorCallbackData(ms_between_video_frames_,
                                           drop_audio_);
}

}
