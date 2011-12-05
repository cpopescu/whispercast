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
// Author: Catalin Popescu & Cosmin Tudorache
//
// A simple client that talks with the rtmp server -- not much ..
//

#ifndef __NET_RTMP_RTMP_CLIENT_H__
#define __NET_RTMP_RTMP_CLIENT_H__

#include <list>

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/timer.h>

#include <whisperlib/net/base/selector.h>
#include <whisperlib/net/base/connection.h>
#include <whisperlib/net/url/url.h>
#include <whisperstreamlib/base/tag.h>
#include <whisperstreamlib/base/consts.h>
#include <whisperstreamlib/flv/flv_tag.h>
#include <whisperstreamlib/rtmp/rtmp_protocol_data.h>
#include <whisperstreamlib/rtmp/rtmp_coder.h>
#include <whisperstreamlib/rtmp/rtmp_util.h>

namespace rtmp {

class SimpleClient {
 private:
  static const int64 kConnectEvent = 1;
  static const int64 kReadEvent = 2;
  static const char* TimeoutIdName(int64 timeout_id) {
    switch ( timeout_id ) {
      CONSIDER(kConnectEvent);
      CONSIDER(kReadEvent);
    }
    return "UNKNOWN";
  }

  // ms
  static const int64 kConnectTimeout = 1000000;
  static const int64 kReadTimeout = 1000000;

  enum State {
    StateHandshakeNotStarted,
    StateHandshaking,
    StateHandshakeDone,
    StateConnectSent,
    StateCreateStreamSent,
    StatePlaySent,
  };

 public:
  typedef Closure ReadHandler;
  typedef Closure CloseHandler;

  SimpleClient(net::Selector* selector);
  virtual ~SimpleClient();

  net::Selector* selector() { return selector_; }
  const string& stream_name() const { return stream_name_; }

  // The client calls the 'read_handler' every time new tags arrive.
  // We take ownership of the callback.
  void set_read_handler(ReadHandler* read_handler);

  // The client calls the 'close_handler' when it gets closed
  // (either due to an error, or as a result of Close())
  // We take ownership of the callback.
  void set_close_handler(CloseHandler* close_handler);

  // set loglevel for various events
  void set_event_log_level(int32 event_log_level);
  void set_tag_log_level(int32 tag_log_level);

  // pop from last received tags. If there are no more tags, returns NULL.
  scoped_ref<streaming::FlvTag> PopTag();

  // connect to the given server, play 'stream_name'
  bool Open(const net::HostPort& remote_address,
            const string& app_name, const string& stream_name);
  // connect to given rtmp url
  bool Open(const string& url);

  // close connection. Asynchronous method. When the client is completely
  // closed, the app is notified by 'close_handler_'
  void Close();

  // Seek stream
  void Seek(int64 position_ms);

  // Pause/Resume stream
  void Pause(bool pause);

 private:
  void TimeoutHandler(int64 timeout_id);

  // TCP connection handlers
  void ConnectionConnectHandler();
  bool ConnectionReadHandler();
  bool ConnectionWriteHandler();
  void ConnectionCloseHandler(int err, net::NetConnection::CloseWhat what);

  // send rtmp Event
  void Send(Event& event);

 private:
  net::Selector * selector_;
  net::NetConnection* net_connection_;
  rtmp::ProtocolData* protocol_;
  rtmp::Coder* coder_;

  string app_name_;
  string stream_name_;
  string tc_url_;

  // client state
  State state_;
  // stream id
  int stream_id_;
  // current stream time (in ms) per channel
  int64 channel_time_ms_[10];

  net::Timeouter timeouter_;

  // last received tags. The user is notified through the ReadHandler and
  // pops them one by one.
  list< scoped_ref<streaming::FlvTag> > tags_;

  // set by the application. The client calls these on specific events.
  ReadHandler* read_handler_;
  CloseHandler* close_handler_;

  // log received rtmp events, tags.. (just for debug & logging). 0 == NO LOG
  int32 event_log_level_;
  int32 tag_log_level_;
};

} // namespace rtmp

#endif // __NET_RTMP_RTMP_CLIENT_H__

