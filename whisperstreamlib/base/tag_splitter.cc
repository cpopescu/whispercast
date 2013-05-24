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

#include "base/tag_splitter.h"

namespace streaming {

TagReadStatus TagSplitter::GetNextTag(io::MemoryStream* in,
                                      scoped_ref<Tag>* tag,
                                      int64* timestamp_ms,
                                      bool is_at_eos) {
  while ( true ) {
    if ( next_tag_to_send_.get() != NULL ) {
      next_tag_to_send_.MoveTo(tag);
      *timestamp_ms = next_tag_to_send_ts_;
      return READ_OK;
    }

    if ( crt_composed_tag_.get() != NULL ) {
      if ( crt_composed_tag_->tags().is_full() ||
           crt_composed_tag_->duration_ms() > max_composition_tag_time_ms_ ) {
        crt_composed_tag_.MoveTo(tag);
        *timestamp_ms = crt_composed_tag_ts_;
        return READ_OK;
      }
    }

    TagReadStatus err = GetNextTagInternal(in, tag, timestamp_ms, is_at_eos);
    if ( err == READ_NO_DATA ) {
      if ( crt_composed_tag_.get() != NULL && is_at_eos ) {
        crt_composed_tag_.MoveTo(tag);
        *timestamp_ms = crt_composed_tag_ts_;
        return READ_OK;
      }
      return is_at_eos ? READ_EOF : READ_NO_DATA;
    }
    if ( err != READ_OK ) {
      CHECK_NULL(tag->get());
      return err;
    }
    CHECK_NOT_NULL(tag->get());

    // Update stats
    stats_.num_decoded_tags_++;
    stats_.total_tag_size_ += (*tag)->size();
    if ( (*tag)->size() > stats_.max_tag_size_ )
      stats_.max_tag_size_ = (*tag)->size();
    if ( (*tag)->is_video_tag() ) {
      stats_.num_video_tags_++;
      stats_.num_video_bytes_ += (*tag)->size();
    }
    if ( (*tag)->is_audio_tag() ) {
      stats_.num_audio_tags_++;
      stats_.num_audio_bytes_ += (*tag)->size();
    }

    // maybe add to current composed tag
    if ( crt_composed_tag_.get() != NULL ) {
      if ( IsComposable(tag->get()) ) {
        crt_composed_tag_->add_tag(tag->get(), *timestamp_ms);
        continue;
      }
      // current tag is not composable, break current composed tag now
      next_tag_to_send_ = *tag;
      next_tag_to_send_ts_ = *timestamp_ms;
      *tag = crt_composed_tag_.get();
      *timestamp_ms = crt_composed_tag_ts_;
      crt_composed_tag_ = NULL;
      return READ_OK;
    }

    // no current composed tag, create a new composed tag now
    if ( max_composition_tag_time_ms_ > 0 &&
         IsComposable(tag->get()) ) {
      crt_composed_tag_ = new ComposedTag(0, kDefaultFlavourMask);
      crt_composed_tag_ts_ = *timestamp_ms;
      crt_composed_tag_->add_tag(tag->get(), *timestamp_ms);
      continue;
    }

    // cannot compose tag, just return it
    return READ_OK;
  }
  LOG_FATAL << name() << ": This place should not be reached";
  return READ_OK;  // keep gcc happy
}

}
