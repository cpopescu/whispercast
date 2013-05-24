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
// Author: Cosmin Tudorache & Catalin Popescu

#ifndef __NET_RTMP_EVENTS_RTMP_HEADER_H__
#define __NET_RTMP_EVENTS_RTMP_HEADER_H__

#include <string>
#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperstreamlib/rtmp/objects/rtmp_objects.h>
#include <whisperstreamlib/rtmp/rtmp_consts.h>

namespace rtmp {

enum HeaderType {
  HEADER_NEW              = 0x00,
  HEADER_SAME_SOURCE      = 0x01,
  HEADER_TIMER_CHANGE     = 0x02,
  HEADER_CONTINUE         = 0x03,
};
const char* HeaderTypeName(HeaderType ht);
int32 HeaderLength(HeaderType ht);

class Coder;

class Header {
 public:
  static const uint32 kInvalidChannelId = kMaxUInt32;
  static const uint32 kInvalidStreamId = kMaxUInt32;
 public:
  Header();
  Header(uint32 channel_id,
         uint32 stream_id,
         EventType event_type,
         uint32 timestamp_ms,
         bool is_timestamp_relative);
  virtual ~Header();

  uint32 channel_id() const {
    return channel_id_;
  }
  uint32 stream_id() const {
    return stream_id_;
  }
  EventType event_type() const {
    return event_type_;
  }
  uint32 timestamp_ms() const {
    return timestamp_ms_;
  }
  bool is_timestamp_relative() const {
    return is_timestamp_relative_;
  }

  void set_timestamp_ms(uint32 ts) {
    timestamp_ms_ = ts;
  }
  void set_is_timestamp_relative(bool is_ts_relative) {
    is_timestamp_relative_ = is_ts_relative;
  }

  AmfUtil::ReadStatus Decode(io::MemoryStream* in,
                             AmfUtil::Version version,
                             const Coder* coder,
                             uint32* out_event_size);
  void Encode(AmfUtil::Version version,
              const Coder* coder,
              uint32 event_size,
              bool force_continue,
              io::MemoryStream* out) const;

  string ToString() const;

 private:
  // These two define the conversation inside a communication channel
  uint32 channel_id_;
  uint32 stream_id_;

  // What kind of event this header precedes
  EventType event_type_;

  // NOTE: Removed event_size_. There is no point in having this info here
  //       when you can look at the Event body size.
  // The size of the event that follows (in bytes)
  //uint32 event_size_;

  // The time of the event - this is tricky - can be relative to the precedent
  // event of the same type or absolute time from the beginning of the stream.
  uint32 timestamp_ms_;
  // Determines the kind of timestamp (see above);
  bool is_timestamp_relative_;
};
inline ostream& operator<<(ostream& os, const Header& obj) {
  return os << obj.ToString();
}
}

#endif  // __NET_RTMP_EVENTS_RTMP_HEADER_H__
