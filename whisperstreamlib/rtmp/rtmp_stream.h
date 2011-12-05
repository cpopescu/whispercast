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
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author: Catalin Popescu

#ifndef __NET_RTMP_RTMP_STREAM_H__
#define __NET_RTMP_RTMP_STREAM_H__

#include <string>
#include <map>
#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/callback.h>
#include <whisperlib/net/base/selector.h>

#include <whisperstreamlib/base/request.h>
#include <whisperstreamlib/base/element_mapper.h>

#include <whisperstreamlib/base/stream_auth.h>

#include <whisperstreamlib/stats2/stats_collector.h>

#include <whisperstreamlib/rtmp/rtmp_consts.h>
#include <whisperstreamlib/rtmp/events/rtmp_event.h>

namespace rtmp {

class Protocol;

struct StreamParams {
  int stream_id_;               // identifies us inside the protocol
  string application_name_;     // from tcUrl in connect
  string application_url_;      // the tcUrl itself

  string command_;              // from the 'publish' request.
                                // can be "live", "record" or "append"

  streaming::AuthorizerRequest auth_req_;  // credentials & stuff

  StreamParams()
      : stream_id_(0) {
  }
  StreamParams(const StreamParams& s)
      : stream_id_(s.stream_id_),
        application_name_(s.application_name_),
        application_url_(s.application_url_),
        command_(s.command_),
        auth_req_(s.auth_req_) {
  }
  string ToString() const {
    return strutil::StringPrintf("StreamParams{stream_id_: %d"
        ", application_name_: %s, application_url_: %s, command_: %s}",
        stream_id_, application_name_.c_str(), application_url_.c_str(),
        command_.c_str());
  }
};

class Stream {
 public:
  Stream(const StreamParams& params,
         Protocol* const protocol);
  virtual ~Stream();

  int ref_count() const { return ref_count_; }
  int stream_id() const { return params_.stream_id_; }
  string stream_name() const { return stream_name_; }
  const StreamParams& params() const { return params_; }

  void ResetChannelTiming(int channel_id = -1) {
    if (channel_id < 0) {
      for (int i = 0; i < kMaxNumChannels; ++i) {
        first_timestamp_ms_[i] = last_timestamp_ms_[i] = -1;
      }
      return;
    }
    DCHECK(channel_id >= 0 && channel_id < kMaxNumChannels);
    first_timestamp_ms_[channel_id] = last_timestamp_ms_[channel_id] = -1;
  }

  void SendEvent(rtmp::Event* event,
                 int64 timestamp_ms = -1,
                 const io::MemoryStream* buffer = NULL,
                 bool force_write = false);
  bool ReceiveEvent(rtmp::Event* event);

  virtual void NotifyOutbufEmpty(int32 outbuf_size) = 0;

  virtual bool ProcessEvent(rtmp::Event* event, int64 timestamp_ms) = 0;

  virtual void Close() = 0;

  virtual bool IsPublishing(const string& stream_name) {
    return false;
  }

  void IncRef();
  void DecRef();

 protected:
  StreamParams params_;
  Protocol* const protocol_;
  mutable synch::Mutex mutex_;

  string stream_name_;

  static const int kMaxNumChannels = 32;
  int64 last_timestamp_ms_[kMaxNumChannels];
  int64 first_timestamp_ms_[kMaxNumChannels];

  int ref_count_;
  DISALLOW_EVIL_CONSTRUCTORS(Stream);
};

}

#endif  // __NET_RTMP_RTMP_STREAM_H__
