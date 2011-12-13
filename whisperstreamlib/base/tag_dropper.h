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
// Author: Mihai Ianculescu

#ifndef __MEDIA_SOURCE_TAG_DROPPER_H__
#define __MEDIA_SOURCE_TAG_DROPPER_H__

#include <whisperstreamlib/base/tag.h>

namespace streaming {

// Utility class that remembers the flow of tags and puts them in order ..
// may be adjusting their timestamp..
class TagDropper {
 public:
  TagDropper()
      : is_audio_synced_(false),
        is_video_synced_(false),
        last_timestamp_ms_(-1) {
  }
  ~TagDropper() {
  }
  // Give us a tag, we tell you what to send further .. (or NULL to drop);
  scoped_ref<Tag> ProcessTag(streaming::Tag* tag, int64 timestamp_ms);

 private:
  bool is_audio_synced_;
  bool is_video_synced_;
  int64 last_timestamp_ms_;

  DISALLOW_EVIL_CONSTRUCTORS(TagDropper);
};
}

#endif  // __MEDIA_SOURCE_TAG_DROPPER_H__
