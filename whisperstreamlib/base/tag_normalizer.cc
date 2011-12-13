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


#include <whisperstreamlib/base/tag_normalizer.h>

namespace streaming {

TagNormalizer::TagNormalizer(net::Selector* selector,
    int64 flow_control_max_write_ahead_ms) :
    selector_(selector),
    flow_control_max_write_ahead_ms_(flow_control_max_write_ahead_ms),
    flow_control_write_ahead_ms_(0),
    request_(NULL),
    element_seq_id_(0),
    is_flow_control_first_tag_(true),
    flow_control_first_tag_time_(0),
    unpause_flow_control_alarm_(*selector) {
}
TagNormalizer::~TagNormalizer() {
  DCHECK(request_ == NULL);
}

void TagNormalizer::Reset(streaming::Request* request) {
  request_ = request;
  if ( request_ != NULL ) {
    flow_control_write_ahead_ms_ = request_->info().write_ahead_ms_;
  }

  unpause_flow_control_alarm_.Stop();
}

void TagNormalizer::ProcessTag(const streaming::Tag* tag, int64 timestamp_ms) {
  stream_time_calculator_.ProcessTag(tag, timestamp_ms);

  if ( flow_control_write_ahead_ms_ > 0 ) {
    // the timestamp of these tags is not relevant
    if ( tag->type() == Tag::TYPE_FLV_HEADER ||
        tag->type() == Tag::TYPE_BOOTSTRAP_BEGIN ||
        tag->type() == Tag::TYPE_BOOTSTRAP_END ||
        tag->type() == Tag::TYPE_EOS ||
        tag->type() == Tag::TYPE_SOURCE_ENDED ) {
      return;
    }

    const int64 now = selector_->now();

    const int64 stream_time_ms = stream_time_calculator_.stream_time_ms();

    if ( tag->type() == streaming::Tag::TYPE_SOURCE_ENDED ) {
      element_seq_id_++;
    }

    if ( tag->type() == streaming::Tag::TYPE_SOURCE_STARTED ||
         tag->type() == streaming::Tag::TYPE_SEEK_PERFORMED ) {

      // restart flow control from scratch
      flow_control_first_tag_time_ = now - stream_time_ms;
      if (tag->type() == streaming::Tag::TYPE_SEEK_PERFORMED) {
        LOG_ERROR << "============= SEEK PERFORMED @" << stream_time_ms;
      } else {
        LOG_ERROR << "============= SOURCE STARTED @" << stream_time_ms;
      }

      if ( unpause_flow_control_alarm_.IsStarted() ) {
        Closure* closure = unpause_flow_control_alarm_.Release();

        DCHECK(closure != NULL);
        DCHECK(closure->is_permanent());

        closure->Run();
        delete closure;
      }
    }

    if ( is_flow_control_first_tag_ ) {
      flow_control_first_tag_time_ = now - stream_time_ms;
      is_flow_control_first_tag_ = false;
    } else {
      int64 real_ts = now - flow_control_first_tag_time_;
      const int64 delta =
          stream_time_ms - real_ts - flow_control_write_ahead_ms_;

      streaming::ElementController* controller = request_->controller();
      if ( delta > 0 &&
           controller != NULL &&
           controller->SupportsPause() &&
           !unpause_flow_control_alarm_.IsStarted() ) {
        controller->Pause(true);
        unpause_flow_control_alarm_.Set(NewPermanentCallback(
            this, &TagNormalizer::Unpause,
            element_seq_id_), true, delta+flow_control_write_ahead_ms_/2,
            false, true);
      }
    }
  }
}

void TagNormalizer::Unpause(int64 element_seq_id) {
  if ( element_seq_id != element_seq_id_ ) {
    return;
  }

  streaming::ElementController* controller = request_->controller();
  if ( controller != NULL && controller->SupportsPause() ) {
    controller->Pause(false);
  }
}

}
