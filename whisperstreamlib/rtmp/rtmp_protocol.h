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

#ifndef __NET_RTMP_RTMP_PROTOCOL_H__
#define __NET_RTMP_RTMP_PROTOCOL_H__

#include <string>
#include <whisperlib/common/base/types.h>

#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/alarm.h>
#include <whisperlib/common/base/callback.h>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperlib/common/sync/mutex.h>
#include <whisperlib/net/base/address.h>

#include <whisperstreamlib/rtmp/rtmp_consts.h>
#include <whisperstreamlib/rtmp/rtmp_coder.h>
#include <whisperstreamlib/rtmp/events/rtmp_event.h>
#include <whisperstreamlib/rtmp/events/rtmp_event_invoke.h>
#include <whisperstreamlib/rtmp/rtmp_protocol_data.h>
#include <whisperstreamlib/rtmp/rtmp_flags.h>
#include <whisperstreamlib/rtmp/rtmp_stream.h>
#include <whisperstreamlib/rtmp/rtmp_manager.h>
#include <whisperstreamlib/rtmp/rtmp_handshake.h>

#include <whisperstreamlib/base/element_mapper.h>
#include <whisperstreamlib/stats2/stats_collector.h>

#include <whisperstreamlib/base/tag.h>
#include <whisperstreamlib/flv/flv_tag.h>
#include <whisperstreamlib/flv/flv_tag_splitter.h>

#include <whisperstreamlib/rtmp/events/rtmp_event_ping.h>

namespace rtmp {

//////////////////////////////////////////////////////////////////////
//
// Various constants used in the protocol.
//
static const char kMethodConnect[]           = "connect";
static const char kMethodCreateStream[]      = "createStream";
static const char kMethodDeleteStream[]      = "deleteStream";
static const char kMethodError[]             = "_error";
static const char kMethodResult[]            = "_result";
static const char kMethodOnBWDone[]          = "onBWDone";
static const char kMethodSetBandwidthLimit[] = "setBandwidthLimit";
static const char kMethodPublish[]           = "publish";
static const char kMethodUnpublish[]         = "unpublish";
static const char kMethodPlay[]              = "play";
static const char kMethodPause[]             = "pause";
static const char kMethodPauseRaw[]          = "pauseRaw";
static const char kMethodSeek[]              = "seek";
static const char kMethodGetStreamLength[]   = "getStreamLength";

static const int  kMaxChunkSize              = 65536;

class ServerConnection;
// This declares Protocol - a class that holds the state of an
// rtmp communication channel (between the client and server).

class Protocol {
 public:
  static const int kUnused0Channel   = 0;
  static const int kUnused1Channel   = 1;
  static const int kPingChannel      = 2;
  static const int kConnectChannel   = 3;

  static const int kReplyChannel     = 4;

  static const int kMetadataChannel  = 5;
  static const int kAudioChannel     = 6;
  static const int kVideoChannel     = 7;

  enum State {
    HANDSHAKE_WAIT_INIT = 0,
    HANDSHAKE_WAIT_REPLY = 1,
    HANDSHAKED = 2,
    CONNECTED = 3,
    CLOSED = 4,
  };
  static const char* StateName(State state);

  Protocol(net::Selector* net_selector,
           net::Selector* media_selector,
           ServerConnection* connection,
           StreamManager* rtmp_stream_manager,
           const rtmp::ProtocolFlags* flags);
  virtual ~Protocol();

  //////////

  const string& info() const {
    return info_;
  }

  const net::HostPort& local_address() const;
  const net::HostPort& remote_address() const;
  State state() const;
  const char* state_name() const;
  bool is_closed() const;
  const ConnectionBegin& connection_begin_stats() const;
  const ConnectionEnd& connection_end_stats() const;

  const rtmp::ProtocolFlags* flags() const;
  const ProtocolData& protocol_data() const;
  ProtocolData* mutable_protocol_data();
  Stream* system_stream();
  net::Selector* net_selector() const { return net_selector_; }
  net::Selector* media_selector() const { return media_selector_; }

 private:
  void set_state(State state);

  // called from either net or media. Completely closes the protocol (including
  // net connection, and streams); this action leads to protocol deletion.
  void CloseProtocol();
  void CloseInNetSelector();
  void CloseInMediaSelector();

 public:
  // Helpers to create specific events
  rtmp::Event* CreateClearPing(int stream_id, int arg) {
    return new rtmp::EventPing(
        rtmp::EventPing::STREAM_CLEAR, arg,
        &protocol_data_, kPingChannel, stream_id);
  }
  rtmp::Event* CreateClearBufferPing(int stream_id, int arg) {
    return new rtmp::EventPing(
        rtmp::EventPing::STREAM_CLEAR_BUFFER, arg,
        &protocol_data_, kPingChannel, stream_id);
  }
  rtmp::Event* CreateResetPing(int stream_id, int arg) {
    return new rtmp::EventPing(
        rtmp::EventPing::STREAM_RESET, arg,
        &protocol_data_, kPingChannel, stream_id);
  }
  rtmp::Event* Create31Ping(int stream_id, int arg) {
    return new rtmp::EventPing(
        (rtmp::EventPing::Type)31, arg,
        &protocol_data_, kPingChannel, stream_id);
  }
  rtmp::Event* Create32Ping(int stream_id, int arg) {
    return new rtmp::EventPing(
        (rtmp::EventPing::Type)32, arg,
        &protocol_data_, kPingChannel, stream_id);
  }
  rtmp::Event* CreatePongPing(int stream_id, int arg) {
      return new rtmp::EventPing(
          rtmp::EventPing::PONG_SERVER, arg,
          &protocol_data_, kPingChannel, stream_id);
  }

  rtmp::Event* CreateInvokeResultEvent(const char* method,
                                      int stream_id, int channel_id,
                                      int invoke_id);

  rtmp::Event* CreateStatusEvent(int stream_id, int channel_id, int invoke_id,
                                const char* code,
                                const char* description, const char* detail,
                                const char* method = NULL,
                                const char* level = NULL);

  //////////////////////////////////////////////////////////////////////
  //
  // NET SELECTOR STUFF
  //
  //////////////////////////////////////////////////////////////////////

  int32 outbuf_size() const;

  void CloseConnection();

  // Called from connection when things happen:
  void NotifyConnectionClose();
  void NotifyConnectionWrite();
  void HandleClientData(io::MemoryStream* in);

  void SendEvent(rtmp::Event* event,
                 const io::MemoryStream* buffer,
                 bool force_write,
                 bool dec_ref = false);

  void SendChunkSize(int stream_id);

 private:
  // Protocol state advance things - independent of streams..
  enum Result {
    RESULT_SUCCESS,
    RESULT_NO_DATA,
    RESULT_ERROR,
  };
  Result DoHandshakeInit(io::MemoryStream* in);
  Result DoHandshakeReply(io::MemoryStream* in);
  Result ProcessConnectionData(io::MemoryStream* in);

  void NetThreadSendChunkSize(int stream_id);
  void NetThreadScheduledSendEvent(rtmp::Event* event, bool force_write);
  void MayBeReregisterPauseTimeout(bool force);

  //////////////////////////////////////////////////////////////////////
  //
  // MEDIA SELECTOR STUFF
  //
  //////////////////////////////////////////////////////////////////////

 public:
  // Called from streams to notify that the stream has paused
  void NotifyStreamPause(Stream* stream, int32 resume_at);
  // Called from streams to notify that the stream has been closed
  void NotifyStreamClose(Stream* stream);

 private:
  // Called from net thread to notify that the output buffer is empty
  void NotifyOutbufEmpty(int32 outbuf_size);

  // The events read from net thread are processed in media thread here
  void ProcessEvent(rtmp::Event* event, bool dec_ref);

  bool InvokeConnect(rtmp::EventInvoke* invoke);
  bool InvokeCreateStream(rtmp::EventInvoke* invoke);
  bool InvokeDeleteStream(rtmp::EventInvoke* invoke);
  bool InvokePublish(rtmp::EventInvoke* invoke);
  bool InvokePlay(rtmp::EventInvoke* invoke);
  bool InvokeUnhandled(rtmp::EventInvoke* invoke);

 public:
  int32 ref_count() const { return ref_count_; }
  void IncRef() {
    synch::MutexLocker l(&mutex_);
    CHECK_GE(ref_count_, 0);
    ++ref_count_;
  }
  void DecRef() {
    mutex_.Lock();
    CHECK_GT(ref_count_, 0);
    --ref_count_;
    bool do_delete = false;
    if (ref_count_ == 0) {
      do_delete = true;
      // bug trap, so that a further IncRef() will crash
      ref_count_ = -1;
    }
    mutex_.Unlock();
    if ( do_delete ) {
      DCHECK(is_closed()) << "Illegal state: "
          << state_ << "(" << state_name() << ")";
      net_selector_->DeleteInSelectLoop(this);
    }
  }

 private:
  net::Selector* const net_selector_;
  net::Selector* const media_selector_;
  ServerConnection* connection_;

  // Small info for us (for logging mostly)
  const string info_;

  // not thread safe  - call from media selector thread
  StreamManager* const stream_manager_;

  // Flags dictate parameters of how we behave
  const rtmp::ProtocolFlags* const flags_;

  //////////////////////////////////////////////////////////////////////
  //
  // Members who need accessed locked
  //
  mutable synch::Mutex mutex_;
  int32 ref_count_;

  // The state we are in.
  State state_;

  //////////////////////////////////////////////////////////////////////
  //
  // NET SELECTOR THREAD stuff
  //

  // Codes and decodes events for us..
  rtmp::Coder rtmp_coder_;

  // Parameters regarding current conversation
  ProtocolData protocol_data_;

  // timeout on connect
  util::Alarm connect_timeout_alarm_;

  // we interrupt pause on this timeout
  util::Alarm pause_timeout_alarm_;
  // when we last registered the pause timeout alarm (so we don't overdo it .. )
  int64 last_pause_registration_time_;

  //////////////////////////////////////////////////////////////////////
  //
  // MEDIA SELECTOR THREAD stuff
  //

  // The params for the next stream to create. Filled by event CONNECT,
  // used by CreateStream, Play, Publish.
  StreamParams next_stream_params_;
  // The id of the next stream we create - increment..
  int pending_stream_id_;

  // ???
  Stream* system_stream_;

  // help send EventBytesRead from time to time
  int64 bytes_read_;
  int64 bytes_read_reported_;

  typedef map<int, Stream*> StreamMap;
  StreamMap streams_;

  DISALLOW_EVIL_CONSTRUCTORS(Protocol);
};
}

#endif  // __NET_RTMP_RTMP_PROTOCOL_H__
