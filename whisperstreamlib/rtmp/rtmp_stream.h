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

struct StreamParams {
  int stream_id_;               // identifies us inside the connection
  string application_name_;     // from tcUrl in connect
  string application_url_;      // the tcUrl itself

  streaming::AuthorizerRequest auth_req_;  // credentials

  StreamParams()
      : stream_id_(0) {
  }
  string ToString() const {
    return strutil::StringPrintf("StreamParams{stream_id_: %d"
        ", application_name_: %s, application_url_: %s}",
        stream_id_, application_name_.c_str(), application_url_.c_str());
  }
};

class ServerConnection;
class Stream {
 protected:
  class AutoDecRef {
   public:
    AutoDecRef(Stream* c) : c_(c) {}
    ~AutoDecRef() { if ( c_ != NULL ) c_->DecRef(); }
   private:
    Stream* c_;
 };

 public:
  Stream(const StreamParams& params,
         ServerConnection* const connection);
  virtual ~Stream();

  int ref_count() const { return ref_count_; }
  int stream_id() const { return params_.stream_id_; }
  string stream_name() const { return stream_name_; }
  const StreamParams& params() const { return params_; }

  void SendEvent(scoped_ref<Event> event,
                 int64 timestamp_ms = -1,
                 const io::MemoryStream* buffer = NULL,
                 bool force_write = false,
                 bool dec_ref = false);
  void SendEventOnSystemStream(scoped_ref<Event> event,
                               int64 timestamp_ms = -1,
                               const io::MemoryStream* buffer = NULL,
                               bool force_write = false,
                               bool dec_ref = false);

  // returns: true -> success, continue connection
  //          false -> error, terminate connection
  bool ReceiveEvent(const rtmp::Event* event);

  virtual void NotifyOutbufEmpty(int32 outbuf_size) = 0;

 protected:
  virtual bool ProcessEvent(const rtmp::Event* event, int64 timestamp_ms) = 0;

 public:
  virtual void Close() = 0;

  // 'connection_' become closed.
  virtual void NotifyConnectionClosed() = 0;

  void IncRef();
  void DecRef();

 protected:
  StreamParams params_;
  ServerConnection* const connection_;
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
