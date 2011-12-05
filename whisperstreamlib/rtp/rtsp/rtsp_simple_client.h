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
// Author: Cosmin Tudorache

#ifndef __MEDIA_RTP_RTSP_SIMPLE_CLIENT_H__
#define __MEDIA_RTP_RTSP_SIMPLE_CLIENT_H__

#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperlib/net/base/selector.h>
#include <whisperlib/net/base/udp_connection.h>
#include <whisperlib/net/base/timeouter.h>
#include <whisperstreamlib/rtp/rtsp/rtsp_processor.h>
#include <whisperstreamlib/rtp/rtsp/rtsp_connection.h>
#include <whisperstreamlib/rtp/rtp_sdp.h>

namespace streaming {
namespace rtsp {

// This is a simple implementation for a RTSP client.
class SimpleClient : public Processor {
public:
  static const string kUserAgent; // used in rtsp Requests
  static const string kInvalidSessionId;
  static const int64 kRequestTimeoutId = 1;
  static const int64 kRequestTimeoutMs = 15000;
  enum State {
    DISCONNECTED,     // totally disconnected
    CONNECTING,       // TCP connect in progress
    DESCRIBE_SENT,    // DESCRIBE sent, waiting for response
    SETUP_AUDIO_SENT, // SETUP audio stream sent, waiting for response
    SETUP_VIDEO_SENT, // SETUP video stream sent, waiting for response
    PLAY_SENT,        // PLAY sent, waiting for response
    PLAYING,          // connected, playing media
    TEARDOWN_SENT,    // TEARDOWN sent, waiting for response
    DISCONNECTING,    // TCP disconnect in progress
  };
  static const char* StateName(State state);

  // params: rtp packet, is_audio
  typedef Callback2<const io::MemoryStream*, bool> RtpHandler;
  // param: sdp
  typedef Callback1<const rtp::Sdp*> SdpHandler;
  // param: is_error
  typedef Callback1<bool> CloseHandler;

public:
  SimpleClient(net::Selector* selector);
  virtual ~SimpleClient();

  // Asynchronous: Starts TCP connect and handshake.
  void Open(const net::HostPort& addr, const string& media);
  // Asynchronous: Begins the closing. Sends TEARDOWN over RTSP.
  void Close();

  State state() const;
  const char* state_name() const;

  // ALL handlers MUST be permanent callbacks.
  // We take ownership of these handlers and will delete them.
  void set_rtp_handler(RtpHandler* handler);
  void set_sdp_handler(SdpHandler* handler);
  void set_close_handler(CloseHandler* handler);

private:
  void set_state(State state);

  uint16 NextCSeq();

  void InvokeCloseHandler(bool is_error);

  void Send(const Request& req);

  // Directly close udp listeners, close rtsp connection.
  // This method does not send TEARDOWN over RTSP, and completes immediately.
  void InternalClose();

  // callback for timeouter_
  void TimeoutHandler(int64 timeout_id);

  /////////////////////////////////////////////////////////////////////
  // Processor methods
  virtual void HandleConnectionConnected(Connection* conn);
  virtual void HandleRequest(Connection* conn, const Request* req);
  virtual void HandleResponse(Connection* conn, const Response* rsp);
  virtual void HandleInterleavedFrame(Connection* conn,
      const InterleavedFrame* frame);
  virtual void HandleConnectionClose(Connection* conn);

  /////////////////////////////////////////////////////////////////////
  // UDP listeners handlers
  bool ReceiverReadHandler(bool is_audio, bool is_control);
  bool ReceiverWriteHandler(bool is_audio, bool is_control);
  void ReceiverCloseHandler(bool is_audio, bool is_control, int err);

private:
  net::Selector* selector_;

  // the rtsp connection
  Connection connection_;

  // Application handlers. An external user will set these to receive
  // media/sdp/notifications.
  RtpHandler* rtp_handler_;
  SdpHandler* sdp_handler_;
  CloseHandler* close_handler_;

  // we send the media request to this server
  net::HostPort server_addr_;
  // the media we request
  string server_media_;
  // A combination of 'server_addr_' and 'server_media_' in a single RTSP URL.
  // Used as a cache because we have to provide this URL on every request.
  URL* server_url_; // format "rtsp://<server_addr_>/<server_media_>

  // we save the sdp received
  rtp::Sdp sdp_;

  // UDP listeners for RTP/RTCP
  net::UdpConnection audio_stream_receiver_;  // rtp
  net::UdpConnection audio_control_receiver_; // rtcp
  net::UdpConnection video_stream_receiver_;  // rtp
  net::UdpConnection video_control_receiver_; // rtcp

  // client session id
  string session_id_;

  // used to generate consecutive cseq numbers
  uint16 next_cseq_;

  // client state
  State state_;

  // used to implement various timeouts during handshake/shutdown
  net::Timeouter timeouter_;
};

} // namespace rtsp
} // namespace streaming

#endif // __MEDIA_RTP_SIMPLE_CLIENT_H__
