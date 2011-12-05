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

#include <whisperlib/common/base/scoped_ptr.h>
#include <whisperstreamlib/rtp/rtsp/rtsp_session.h>
#include <whisperstreamlib/rtp/rtp_sdp.h>
#include <whisperstreamlib/rtp/rtp_util.h>

#define ILOG_DEBUG   RTSP_LOG_DEBUG << Info()
#define ILOG_INFO    RTSP_LOG_INFO << Info()
#define ILOG_WARNING RTSP_LOG_WARNING << Info()
#define ILOG_ERROR   RTSP_LOG_ERROR << Info()
#define ILOG_FATAL   RTSP_LOG_FATAL << Info()

namespace streaming {
namespace rtsp {

Session::Session(net::Selector* selector,
                 MediaInterface* media_interface,
                 SessionListener& listener,
                 const string& id)
  : selector_(selector),
    media_interface_(media_interface),
    listener_(listener),
    id_(id),
    media_(),
    broadcaster_(NULL),
    timeouter_(selector, NewPermanentCallback(this, &Session::TimeoutHandler)),
    active_req_(NULL) {
}
Session::~Session() {
  Teardown();
}

const string& Session::id() const {
  return id_;
}
const string& Session::media() const {
  return media_;
}
uint16 Session::local_port() const {
  if ( broadcaster_ == NULL ||
       broadcaster_->sender()->type() != rtp::Sender::UDP ) {
    return 0;
  }
  return broadcaster_->tsender<rtp::UdpSender>()->udp_local_port();
}

const Request* Session::active_request() const {
  return active_req_;
}
void Session::set_active_request(const Request* req) {
  active_req_ = req;
}

void Session::Setup(const string& media,
                    const net::HostPort& udp_media_dst,
                    const net::HostPort& udp_control_dst,
                    bool audio) {
  if ( media_ == "" ) {
    // first Setup
    media_ = media;
    rtp::UdpSender* udp_sender = new rtp::UdpSender(selector_);
    if ( !udp_sender->Initialize() ) {
      ILOG_ERROR << "Failed to initialize UDP sender";
      delete udp_sender;
      udp_sender = NULL;
      listener_.HandleSessionClosed(this);
      return;
    }
    udp_sender->SetDestination(udp_media_dst, audio);
    CHECK_NULL(broadcaster_);
    broadcaster_ = new rtp::Broadcaster(udp_sender, NewPermanentCallback(this,
        &Session::BroadcasterEosHandler));
    if ( !media_interface_->Attach(broadcaster_, media) ) {
      ILOG_ERROR << "Failed to attach to media: [" << media << "]";
      Teardown();
      return;
    }

    // when Describe() completes the Setup resumes with ContinueSetup()
    if ( !media_interface_->Describe(media, NewCallback(this,
        &Session::ContinueSetup, media)) ) {
      ILOG_ERROR << "Media not found: [" << media << "]";
      Teardown();
      return;
    }

    return;
  }
  // secondary setup to configure audio/video ports
  if ( media != media_ ) {
    ILOG_ERROR << "Setup on a different media, old: [" << media_ << "]"
                  ", new: [" << media << "]";
    Teardown();
    return;
  }
  CHECK_NOT_NULL(broadcaster_);
  if ( broadcaster_->sender()->type() == rtp::Sender::UDP ) {
    broadcaster_->tsender<rtp::UdpSender>()->SetDestination(
        udp_media_dst, audio);
  }
  listener_.HandleSessionSetupCompleted(this);
  timeouter_.SetTimeout(kSetupTimeoutId, kSetupTimeoutMs);
}
void Session::Setup(const string& media,
                    Connection* rtsp_conn,
                    uint8 media_ch,
                    bool is_audio) {
  if ( media_ == "" ) {
    // first setup
    media_ = media;

    CHECK_NULL(broadcaster_);
    broadcaster_ = new rtp::Broadcaster(rtsp_conn, NewPermanentCallback(this,
        &Session::BroadcasterEosHandler));
    if ( !media_interface_->Attach(broadcaster_, media) ) {
      ILOG_ERROR << "Failed to attach to media: [" << media << "]";
      Teardown();
      return;
    }
    rtsp_conn->set_rtp_ch(media_ch, is_audio);

    // when Describe() completes the Setup resumes with ContinueSetup()
    if ( !media_interface_->Describe(media, NewCallback(this,
        &Session::ContinueSetup, media)) ) {
      ILOG_ERROR << "Media not found: [" << media << "]";
      Teardown();
      return;
    }
    return;
  }

  // second setup
  if ( media_ != media ) {
    ILOG_ERROR << "Setup on a different media, old: [" << media_ << "]"
                  ", new: [" << media << "]";
    Teardown();
    return;
  }
  CHECK_NOT_NULL(broadcaster_);
  rtsp_conn->set_rtp_ch(media_ch, is_audio);
  listener_.HandleSessionSetupCompleted(this);
  timeouter_.SetTimeout(kSetupTimeoutId, kSetupTimeoutMs);
}
void Session::ContinueSetup(string media, const MediaInfo* info) {
  CHECK_NOT_NULL(info);
  broadcaster_->SetMediaInfo(*info);
  listener_.HandleSessionSetupCompleted(this);
  timeouter_.SetTimeout(kSetupTimeoutId, kSetupTimeoutMs);
}
void Session::Play(bool play) {
  CHECK_NOT_NULL(broadcaster_);
  timeouter_.UnsetTimeout(kSetupTimeoutId);
  media_interface_->Play(broadcaster_, play);
}
void Session::Teardown() {
  if ( broadcaster_ == NULL ) {
    // already shutdown
    return;
  }
  media_interface_->Detach(broadcaster_);
  delete broadcaster_;
  broadcaster_ = NULL;
  listener_.HandleSessionClosed(this);
}

string Session::ToString() const {
  return strutil::StringPrintf("rtsp::Session{id_: %s, media_interface_: %p, "
      "media_: %s, broadcaster_: %p}",
      id_.c_str(), media_interface_, media_.c_str(), broadcaster_);
}

string Session::Info() const {
  return strutil::StringPrintf("[Session ID: %s, media: %s] ",
      id_.c_str(), media_.c_str());
}
void Session::TimeoutHandler(int64 timeout_id) {
  if ( timeout_id == kSetupTimeoutId ) {
    RTSP_LOG_ERROR << "Timeout SETUP / PLAY on session: " << ToString();
    Teardown();
    return;
  }
  RTSP_LOG_FATAL << "Unknown timeout_id: " << timeout_id;
}
void Session::BroadcasterEosHandler(bool is_error) {
  // this methods is called by the broadcaster_. So it would be wise not to
  // delete the broadcaster_ (in Teardown()) from this call.
  if ( is_error ) {
    RTSP_LOG_ERROR << Info() << "BroadcasterEosHandler";
  } else {
    RTSP_LOG_INFO << Info() << "BroadcasterEosHandler";
  }
  selector_->RunInSelectLoop(NewCallback(this, &Session::Teardown));
}

} // namespace rtsp
} // namespace streaming
