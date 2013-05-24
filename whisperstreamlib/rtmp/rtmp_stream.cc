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
#include <whisperstreamlib/rtmp/rtmp_connection.h>
#include <whisperstreamlib/raw/raw_tag_splitter.h>

namespace rtmp {

Stream::Stream(const StreamParams& params,
               ServerConnection* const connection)
    : params_(params),
      connection_(connection),
      ref_count_(0) {
  connection_->IncRef();
  for (int i = 0; i < kMaxNumChannels; ++i) {
    first_timestamp_ms_[i] = -1;
    last_timestamp_ms_[i] = -1;
  }
}
Stream::~Stream() {
  CHECK_EQ(ref_count_, 0);
  connection_->DecRef();
}

void Stream::SendEvent(scoped_ref<Event> event,
                       int64 timestamp_ms,
                       const io::MemoryStream* buffer,
                       bool force_write,
                       bool dec_ref) {
  if ( !connection_->net_selector()->IsInSelectThread() ) {
    CHECK_NULL(buffer);
    IncRef();
    connection_->net_selector()->RunInSelectLoop(NewCallback(this,
        &Stream::SendEvent, event, timestamp_ms, buffer, force_write, true));
    return;
  }
  CHECK(connection_->net_selector()->IsInSelectThread());
  if ( event.get() != NULL ) {
    uint32 channel_id = event->header().channel_id();
    DCHECK(channel_id < kMaxNumChannels);

    if ( timestamp_ms < 0 ) {
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
          event->mutable_header()->set_timestamp_ms(timestamp_ms -
              last_timestamp_ms_[channel_id]);
        } else {
          event->mutable_header()->set_is_timestamp_relative(false);
          event->mutable_header()->set_timestamp_ms(timestamp_ms);
        }
      }
    }
    last_timestamp_ms_[channel_id] = timestamp_ms;
  }

  connection_->SendEvent(*event.get(), buffer, force_write);

  if ( dec_ref ) {
    DecRef();
  }
}
void Stream::SendEventOnSystemStream(scoped_ref<Event> event,
                                     int64 timestamp_ms,
                                     const io::MemoryStream* buffer,
                                     bool force_write,
                                     bool dec_ref) {
  if ( !connection_->net_selector()->IsInSelectThread() ) {
    CHECK_NULL(buffer);
    IncRef();
    connection_->net_selector()->RunInSelectLoop(NewCallback(this,
        &Stream::SendEventOnSystemStream, event, timestamp_ms, buffer,
        force_write, true));
    return;
  }
  CHECK(connection_->net_selector()->IsInSelectThread());
  if ( connection_->system_stream() != NULL ) {
    connection_->system_stream()->SendEvent(event, timestamp_ms, buffer,
        force_write);
  }
  if ( dec_ref ) {
    DecRef();
  }
}
bool Stream::ReceiveEvent(const rtmp::Event* event) {
  CHECK(connection_->net_selector()->IsInSelectThread());
  const uint32 channel_id = event->header().channel_id();
  DCHECK(channel_id < kMaxNumChannels);

  int64 timestamp_ms = 0;
  if ( event->header().is_timestamp_relative() ) {
    if (last_timestamp_ms_[channel_id] < 0) {
      timestamp_ms = event->header().timestamp_ms();
    } else {
      timestamp_ms = last_timestamp_ms_[channel_id] +
                     event->header().timestamp_ms();
    }
  } else {
    timestamp_ms = event->header().timestamp_ms();
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
  // RACE: when using net_selector->DeleteInSelectLoop(this), release
  //       the mutex first! Because this DecRef() may run on media_selector.
  if ( do_delete ) {
    connection_->net_selector()->DeleteInSelectLoop(this);
  }
}

} // namespace rtmp
