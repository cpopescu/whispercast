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

#ifndef __MEDIA_BASE_BOOTSTRAPPER_H__
#define __MEDIA_BASE_BOOTSTRAPPER_H__

#include <deque>

#include <whisperstreamlib/base/tag.h>
#include <whisperstreamlib/base/request.h>
#include <whisperstreamlib/base/callbacks_manager.h>
#include <whisperstreamlib/flv/flv_tag.h>

namespace streaming {

class Bootstrapper {
 public:
  Bootstrapper(bool keep_media)
      : keep_media_(keep_media) {
  }
  ~Bootstrapper() {
    ClearBootstrap();
  }

 public:
  struct BootstrapTag {
    BootstrapTag(const Tag* tag = NULL, int64 timestamp_ms = -1) :
      tag_(tag), timestamp_ms_(timestamp_ms) {
    }

    scoped_ref<const Tag> tag_;
    int64 timestamp_ms_;
  };

  void set_keep_media(bool keep_media) {
    keep_media_ = keep_media;
    if ( !keep_media ) {
      ClearMediaBootstrap();
    }
  }
  bool keep_media() const { return keep_media_; }

  // bootstrap tags are to be sent at the stream begin
  void PlayAtBegin(ProcessingCallback* callback,
      int64 timestamp_ms, uint32 flavour_mask) const;
  void PlayAtBegin(streaming::CallbacksManager* callback_manager,
      int64 timestamp_ms, uint32 flavour_mask) const;

  // bootstrap tags meant to be sent at the stream end
  void PlayAtEnd(ProcessingCallback* callback,
      int64 timestamp_ms, uint32 flavour_mask) const;
  void PlayAtEnd(streaming::CallbacksManager* callback_manager,
      int64 timestamp_ms, uint32 flavour_mask) const;

  void GetBootstrapTags(vector<BootstrapTag>* out);

  void ProcessTag(const Tag* tag, int64 timestamp_ms);

  void ClearBootstrap();
  void ClearMediaBootstrap();

 private:
  bool keep_media_;

  // The stack of source started tags
  deque<BootstrapTag> source_started_tags_;

  // Last remembered AVC video header
  BootstrapTag avc_sequence_header_;
  // AAC audio header
  BootstrapTag aac_header_tag_;
  // Last remembered stream properties
  BootstrapTag metadata_;
  // Cue points which enable downstream clients to seek
  BootstrapTag cue_points_;
  // Moov header for h264
  BootstrapTag moov_tag_;

  // Last keyframe + all intermediate tags until now
  vector<BootstrapTag> media_bootstrap_;
};

}


#endif // __MEDIA_BASE_BOOTSTRAPPER_H__
