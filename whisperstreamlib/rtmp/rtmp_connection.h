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

#ifndef __NET_RTMP_RTMP_CONNECTION_H__
#define __NET_RTMP_RTMP_CONNECTION_H__

#include <string>
#include <vector>
#include <map>
#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/alarm.h>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperlib/net/base/connection.h>
#include <whisperlib/net/util/ipclassifier.h>
#include <whisperstreamlib/stats2/stats_collector.h>
#include <whisperstreamlib/rtmp/events/rtmp_event.h>
#include <whisperstreamlib/rtmp/events/rtmp_event_ping.h>
#include <whisperstreamlib/rtmp/events/rtmp_event_invoke.h>
#include <whisperstreamlib/rtmp/rtmp_flags.h>
#include <whisperstreamlib/rtmp/rtmp_stream.h>
#include <whisperstreamlib/rtmp/rtmp_coder.h>
#include <whisperstreamlib/rtmp/rtmp_util.h>

namespace rtmp {
class StreamManager;

// Rtmp connection on server side. Acts as a link between physical network
// connection and the more advanced Stream.
class ServerConnection {
  // timeout IDs
  static const int64 kConnectTimeoutID = 1;
  static const int64 kWriteTimeoutID   = 2;

 public:
  enum State {
    HANDSHAKE_WAIT_INIT = 0,
    HANDSHAKE_WAIT_REPLY = 1,
    HANDSHAKED = 2,
    CONNECTED = 3,
    CLOSED = 4,
  };
  static const char* StateName(State state);

  ServerConnection(net::Selector* net_selector,
                   net::Selector* media_selector,
                   streaming::ElementMapper* element_mapper,
                   net::NetConnection* net_connection,
                   const string& connection_id,
                   streaming::StatsCollector* stats_collector,
                   Closure* delete_callback,
                   const vector<const net::IpClassifier*>* classifiers,
                   const rtmp::ProtocolFlags* flags,
                   MissingStreamCache* missing_stream_cache);
  virtual ~ServerConnection();

  net::Selector* media_selector() { return media_selector_; }
  net::Selector* net_selector() { return net_selector_; }
  const ProtocolFlags& flags() const { return *flags_; }
  bool is_closed() const {
    return state_ == CLOSED;
  }
  const string& info() const {
    return info_;
  }
  const State state() const {
    return state_;
  }
  const char* state_name() const {
    return StateName(state());
  }
  const net::HostPort& local_address() const {
    return connection_->local_address();
  }
  const net::HostPort& remote_address() const {
    return connection_->remote_address();
  }
  const ConnectionBegin& connection_begin_stats() const {
    return connection_begin_stats_;
  }
  const ConnectionEnd& connection_end_stats() const {
    return connection_end_stats_;
  }
  Stream* system_stream() {
    return system_stream_;
  }

  int32 outbuf_size() const {
    DCHECK(connection_ != NULL);
    return connection_->outbuf()->Size();
  }

  MissingStreamCache* missing_stream_cache() {
    return missing_stream_cache_;
  }

  // self explanatory
  void CloseConnection();

  // Helpers to create specific events
  scoped_ref<Event> CreateClearPing(int stream_id, int arg) {
    return new rtmp::EventPing(
        Header(kChannelPing, stream_id, rtmp::EVENT_PING, 0, false),
        rtmp::EventPing::STREAM_CLEAR, arg, -1, -1);
  }
  scoped_ref<Event> CreateClearBufferPing(int stream_id, int arg) {
    return new rtmp::EventPing(
        Header(kChannelPing, stream_id, rtmp::EVENT_PING, 0, false),
        rtmp::EventPing::STREAM_CLEAR_BUFFER, arg, -1, -1);
  }
  scoped_ref<Event> CreateResetPing(int stream_id, int arg) {
    return new rtmp::EventPing(
        Header(kChannelPing, stream_id, rtmp::EVENT_PING, 0, false),
        rtmp::EventPing::STREAM_RESET, arg, -1, -1);
  }
  scoped_ref<Event> Create31Ping(int stream_id, int arg) {
    return new rtmp::EventPing(
        Header(kChannelPing, stream_id, rtmp::EVENT_PING, 0, false),
        rtmp::EventPing::PING_31, arg, -1, -1);
  }
  scoped_ref<Event> Create32Ping(int stream_id, int arg) {
    return new rtmp::EventPing(
        Header(kChannelPing, stream_id, rtmp::EVENT_PING, 0, false),
        rtmp::EventPing::PING_32, arg, -1, -1);
  }
  scoped_ref<Event> CreatePongPing(int stream_id, int arg) {
      return new rtmp::EventPing(
          Header(kChannelPing, stream_id, rtmp::EVENT_PING, 0, false),
          rtmp::EventPing::PONG_SERVER, arg, -1, -1);
  }
  scoped_ref<Event> CreateInvokeResultEvent(const string& method,
                                            int stream_id,
                                            int channel_id,
                                            int invoke_id);
  scoped_ref<Event> CreateStatusEvent(int stream_id,
                                      int channel_id,
                                      int invoke_id,
                                      const string& code,
                                      const string& description,
                                      const string& detail,
                                      const char* method = NULL,
                                      const char* level = NULL);
  scoped_ref<Event> CreateChunkSize(int stream_id) {
    return new rtmp::EventChunkSize(
        Header(kChannelPing, stream_id, rtmp::EVENT_CHUNK_SIZE, 0, false),
        flags().chunk_size_);
  }

  // Encode & send the given event to the network.
  // If 'data != NULL', then 'data' is used instead of event->data().
  // If force_write==true, sends to network immediately; otherwise buffers.
  void SendEvent(const rtmp::Event& event, const io::MemoryStream* data,
                 bool force_write = false);

  // The protocol puts some data in the output buffer then calls this.
  void SendOutbufData();

  void IncRef() {
    CHECK(net_selector_->IsInSelectThread());
    ref_count_++;
  }
  void DecRef() {
    CHECK(net_selector_->IsInSelectThread());
    CHECK_GT(ref_count_, 0);
    ref_count_--;
    if ( ref_count_ == 0 ) {
      delete this;
    }
  }

 private:
  bool ConnectionReadHandler();
  bool ConnectionWriteHandler();
  void ConnectionCloseHandler(int err, net::NetConnection::CloseWhat what);

  enum Result {
    RESULT_SUCCESS,
    RESULT_NO_DATA,
    RESULT_ERROR,
  };
  Result DoHandshakeInit(io::MemoryStream* in);
  Result DoHandshakeReply(io::MemoryStream* in);
  Result ProcessData(io::MemoryStream* in);

  // returns success.
  // On false return: the connection will be automatically closed.
  bool ProcessEvent(const Event* event);

  bool InvokeConnect(const EventInvoke* invoke);
  bool InvokeCreateStream(const Event* event, int invoke_id);
  bool InvokeDeleteStream(const Event* event, int invoke_id);
  bool InvokePublish(const Event* event, int invoke_id);
  bool InvokePlay(const Event* event, int invoke_id);

  bool Unhandled(const Event* event, int invoke_id);

  void TimeoutHandler(int64 timeout_id);

  // TODO(cosmin): move Pause timeout to PlayStream
  void MayBeReregisterPauseTimeout(bool force);

 private:
  net::Selector* net_selector_;
  net::Selector* media_selector_;
  // link to whispercast internal elements
  streaming::ElementMapper* element_mapper_;
  // the underneath TCP client connection
  net::NetConnection* connection_;
  // maps from IP-s to class numbers
  const vector<const net::IpClassifier*>* const classifiers_;
  // We call this upon deletion (to inform acceptor about our departure)
  Closure* delete_callback_;

  // just for logging
  const string info_;

  State state_;

  // Protocol parameters:
  const ProtocolFlags* const flags_;

  // a cache of bad stream names
  MissingStreamCache* missing_stream_cache_;

  // Codes and decodes events for us..
  rtmp::Coder coder_;

  // The params for the next stream to create. Filled by event CONNECT,
  // used by CreateStream, Play, Publish.
  StreamParams next_stream_params_;

  // InvokeCreateStream was executed
  bool invoke_create_stream_called_;

  // ???
  Stream* system_stream_;

  typedef map<int, Stream*> StreamMap;
  StreamMap streams_;

  // help send EventBytesRead from time to time
  int64 bytes_read_;
  int64 bytes_read_reported_;

  // TODO(cosmin): move PAUSE timeout to PlayStream
  // we interrupt pause on this timeout
  util::Alarm pause_timeout_alarm_;
  // when we last registered the pause timeout alarm (so we don't overdo it .. )
  int64 last_pause_registration_time_;

  // Statistics
  streaming::StatsCollector* stats_collector_;
  const string connection_id_;
  ConnectionBegin connection_begin_stats_;
  ConnectionEnd connection_end_stats_;

  // various timeouts
  net::Timeouter timeouter_;

  int ref_count_;
 private:
  DISALLOW_EVIL_CONSTRUCTORS(ServerConnection);
};
}

#endif  // __NET_RTMP_RTMP_CONNECTION_H__
