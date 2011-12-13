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


#include <whisperstreamlib/base/tag_dropper.h>

namespace streaming {

scoped_ref<Tag> TagDropper::ProcessTag(streaming::Tag* tag, int64 timestamp_ms) {
  if ( tag->type() == Tag::TYPE_SEEK_PERFORMED ||
       tag->type() == Tag::TYPE_SOURCE_STARTED ) {
    is_audio_synced_ = is_video_synced_ = false;
  }

  // drop tags in the past
  if ( timestamp_ms < 0 || timestamp_ms <= last_timestamp_ms_ ) {
    if ( tag->is_audio_tag() ) {
      is_audio_synced_ = false;
    } else if ( tag->is_video_tag() ) {
      is_video_synced_ = false;
    }
    return NULL;
  }

  // Until synchronized for this element, drop audio / video
  if ( !is_audio_synced_ && tag->is_audio_tag() ) {
    if ( !tag->can_resync() ) {
      return NULL;
    }
    is_audio_synced_ = true;
  }
  if ( !is_video_synced_ && tag->is_video_tag() ) {
    if ( !tag->can_resync() ) {
      return NULL;
    }
    is_video_synced_ = true;
  }

  last_timestamp_ms_ = timestamp_ms;
  return tag;
}

}
