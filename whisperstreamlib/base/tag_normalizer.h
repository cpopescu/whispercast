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

#ifndef __MEDIA_SOURCE_TAG_NORMALIZER_H__
#define __MEDIA_SOURCE_TAG_NORMALIZER_H__

#include <whisperlib/common/base/alarm.h>

#include <whisperstreamlib/base/tag.h>
#include <whisperstreamlib/base/request.h>

namespace streaming {

// Utility class that remembers the flow of tags and puts them in order ..
// may be adjusting their timestamp..
class TagNormalizer {
 public:
  TagNormalizer(net::Selector* selector,
      int64 flow_control_max_write_ahead_ms);
  ~TagNormalizer();

 public:
  int64 last_tag_ts() const {
    return stream_time_calculator_.last_tag_ts();
  }

  int64 media_time_ms() const {
    return stream_time_calculator_.media_time_ms();
  }
  int64 stream_time_ms() const {
    return stream_time_calculator_.stream_time_ms();
  }

 public:
  // Sets up the normalizer to be used with the given request
  void Reset(streaming::Request* request);

  // Pause or resume the stream, so it flows at the normal rate
  void ProcessTag(const streaming::Tag* tag, int64 timestamp_ms);

 private:
  void Unpause(int64 element_seq_id);

 private:
  net::Selector const* const selector_;

  const int64 flow_control_max_write_ahead_ms_;
  int64 flow_control_write_ahead_ms_;

  streaming::Request* request_;

  streaming::StreamTimeCalculator stream_time_calculator_;

  int64 element_seq_id_;

  bool is_flow_control_first_tag_;
  int64 flow_control_first_tag_time_;

  util::Alarm unpause_flow_control_alarm_;

  DISALLOW_EVIL_CONSTRUCTORS(TagNormalizer);
};
}

#endif  // __MEDIA_SOURCE_TAG_NORMALIZER_H__
