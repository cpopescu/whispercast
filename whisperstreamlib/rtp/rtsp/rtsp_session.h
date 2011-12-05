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

#ifndef __MEDIA_RTP_RTSP_SESSION_H__
#define __MEDIA_RTP_RTSP_SESSION_H__

#include <whisperlib/common/base/types.h>
#include <whisperstreamlib/rtp/rtp_broadcaster.h>
#include <whisperstreamlib/rtp/rtp_udp_sender.h>
#include <whisperstreamlib/rtp/rtsp/rtsp_consts.h>
#include <whisperstreamlib/rtp/rtsp/rtsp_connection.h>
#include <whisperstreamlib/rtp/rtsp/rtsp_media_interface.h>
#include <whisperstreamlib/rtp/rtsp/rtsp_session_listener.h>
#include <whisperstreamlib/rtp/rtsp/types/rtsp_request.h>

namespace streaming {
namespace rtsp {

// This class manages a streaming Session for 1 client.
// It receives commands from the ServerProcessor to start or stop a
// rtp::Broadcaster. It sends notification back to the processor through the
// 'SessionListener' interface.
class Session {
public:
  static const int64 kSetupTimeoutId = 1;
  static const int64 kSetupTimeoutMs = 5000;
public:
  Session(net::Selector* selector,
          MediaInterface* media_interface,
          SessionListener& listener,
          const string& id);
  virtual ~Session();

  const string& id() const;
  const string& media() const;
  uint16 local_port() const;

  const Request* active_request() const;
  void set_active_request(const Request* req);

  // Setup is asynchronous. These methods start the setup process;
  // when setup completes, we call one of:
  //  - SessionListener::HandleSessionSetupCompleted or
  //  - SessionListener::HandleSessinClosed

  // Setup for regular RTP over UDP
  // TODO(cosmin): replace the SetupCallback with a SessionNotifier method
  void Setup(const string& media,
             const net::HostPort& udp_media_dst,
             const net::HostPort& udp_control_dst,
             bool audio);
  // Setup for RTP over RTSP interleaved
  void Setup(const string& media,
             Connection* rtsp_conn,
             uint8 media_ch,
             bool is_audio);
private:
  void ContinueSetup(string media, const MediaInfo* info);

public:
  // Play/Pause
  void Play(bool play);
  // Shutdown
  void Teardown();

  string ToString() const;

private:
  string Info() const;

  // callback for timeouter_
  void TimeoutHandler(int64 timeout_id);

  // callback for broadcaster_
  void BroadcasterEosHandler(bool is_error);

private:
  net::Selector* selector_;
  MediaInterface* media_interface_;
  SessionListener& listener_;

  // session id
  const string id_;

  string media_;
  rtp::Broadcaster* broadcaster_;

  net::Timeouter timeouter_;

  // sometimes there's an active request waiting for the session to complete sth
  // e.g. The SETUP request is waiting the asynchronous Setup to complete.
  // We don't do anything with this request, we are just the keeper, someone
  // else externally uses this request.
  const Request* active_req_;
};

} // namespace rtsp
} // namespace streaming

#endif // __MEDIA_RTP_RTSP_SESSION_H__
