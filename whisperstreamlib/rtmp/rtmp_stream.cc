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

#include <whisperlib/net/url/url.h>
#include <whisperlib/common/base/gflags.h>

#include <whisperstreamlib/rtmp/rtmp_stream.h>
#include <whisperstreamlib/rtmp/rtmp_protocol.h>
#include <whisperstreamlib/raw/raw_tag_splitter.h>
#include <whisperstreamlib/rtmp/rtmp_manager.h>

#define RTMP_LOG(level) if ( protocol_->flags()->log_level_ < level ); \
                        else LOG(level) << protocol_->info() << ": "
#define RTMP_DLOG(level) if ( !protocol_->flags()->log_level_ < level ); \
                         else DLOG(level) << protocol_->info() << ": "

#define RTMP_LOG_DEBUG   RTMP_LOG(LDEBUG)
#define RTMP_LOG_INFO    RTMP_LOG(LINFO)
#define RTMP_LOG_WARNING RTMP_LOG(LWARNING)
#define RTMP_LOG_ERROR   RTMP_LOG(LERROR)
#define RTMP_LOG_FATAL   RTMP_LOG(LFATAL)

#define RTMP_DLOG_DEBUG   RTMP_DLOG(LDEBUG)
#define RTMP_DLOG_INFO    RTMP_DLOG(LINFO)
#define RTMP_DLOG_WARNING RTMP_DLOG(LWARNING)
#define RTMP_DLOG_ERROR   RTMP_DLOG(LERROR)
#define RTMP_DLOG_FATAL   RTMP_DLOG(LFATAL)

namespace rtmp {

Stream::Stream(const StreamParams& params,
               Protocol* const protocol)
    : params_(params),
      protocol_(protocol),
      ref_count_(0) {
  protocol_->IncRef();
  ResetChannelTiming();
}
Stream::~Stream() {
  CHECK_EQ(ref_count_, 0);
  protocol_->DecRef();
}

void Stream::SendEvent(rtmp::Event* event,
                       int64 timestamp_ms,
                       const io::MemoryStream* buffer,
                       bool force_write) {
  if ( event ) {
    uint32 channel_id = event->header()->channel_id();
    DCHECK(channel_id < kMaxNumChannels);

    if (timestamp_ms < 0) {
      if ( last_timestamp_ms_[channel_id] == -1 ) {
        timestamp_ms = 0;

        event->mutable_header()->set_is_timestamp_relative(false);
        event->mutable_header()->set_timestamp_ms(timestamp_ms);
      } else {
        timestamp_ms = last_timestamp_ms_[channel_id];

        event->mutable_header()->set_is_timestamp_relative(true);
        event->mutable_header()->set_timestamp_ms(0);
      }
    } else {
      if ( last_timestamp_ms_[channel_id] == -1 ) {
        event->mutable_header()->set_is_timestamp_relative(false);
        event->mutable_header()->set_timestamp_ms(timestamp_ms);
      } else {
        if ( last_timestamp_ms_[channel_id] <= timestamp_ms ) {
          event->mutable_header()->set_is_timestamp_relative(true);
          event->mutable_header()->set_timestamp_ms(
              timestamp_ms - last_timestamp_ms_[channel_id]);
        } else {
          event->mutable_header()->set_is_timestamp_relative(false);
          event->mutable_header()->set_timestamp_ms(timestamp_ms);
        }
      }
    }
    last_timestamp_ms_[channel_id] = timestamp_ms;
  }

  protocol_->SendEvent(event, buffer, force_write);
}
bool Stream::ReceiveEvent(rtmp::Event* event) {

  const uint32 channel_id = event->header()->channel_id();
  DCHECK(channel_id < kMaxNumChannels);

  int64 timestamp_ms = 0;
  if ( event->header()->is_timestamp_relative() ) {
    if (last_timestamp_ms_[channel_id] < 0) {
      timestamp_ms = event->header()->timestamp_ms();
    } else {
      timestamp_ms = last_timestamp_ms_[channel_id] +
        event->header()->timestamp_ms();
    }
  } else {
    timestamp_ms = event->header()->timestamp_ms();
  }

  if (first_timestamp_ms_[channel_id] < 0) {
    first_timestamp_ms_[channel_id] = timestamp_ms;
  }
  last_timestamp_ms_[channel_id] = timestamp_ms;

  return ProcessEvent(event, timestamp_ms);
}

void Stream::IncRef() {
  synch::MutexLocker l(&mutex_);
  ++ref_count_;
}
void Stream::DecRef() {
  mutex_.Lock();
  CHECK_GT(ref_count_, 0);
  --ref_count_;
  bool do_delete = (ref_count_ == 0);
  mutex_.Unlock();
  // NOTE: make sure the mutex is released before deleting
  // RACE: when using media_thread->DeleteInSelectLoop(this), release
  //       the mutex first! Because this DecRef() may run on net_thread! .
  if ( do_delete ) {
    protocol_->media_selector()->DeleteInSelectLoop(this);
  }
}

} // namespace rtmp
