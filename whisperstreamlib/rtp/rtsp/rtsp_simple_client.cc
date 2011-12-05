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
#include <whisperstreamlib/rtp/rtsp/rtsp_simple_client.h>

namespace streaming {
namespace rtsp {

const string SimpleClient::kUserAgent("Whispercast Rtsp Client");
const string SimpleClient::kInvalidSessionId("");

const char* SimpleClient::StateName(SimpleClient::State state) {
  switch ( state ) {
    CONSIDER(DISCONNECTED);
    CONSIDER(CONNECTING);
    CONSIDER(DESCRIBE_SENT);
    CONSIDER(SETUP_AUDIO_SENT);
    CONSIDER(SETUP_VIDEO_SENT);
    CONSIDER(PLAY_SENT);
    CONSIDER(PLAYING);
    CONSIDER(TEARDOWN_SENT);
    CONSIDER(DISCONNECTING);
  }
  RTSP_LOG_FATAL << "Illegal state: " << state;
  return "Unknown";
}

SimpleClient::SimpleClient(net::Selector* selector)
  : Processor(),
    selector_(selector),
    connection_(selector, this),
    rtp_handler_(NULL),
    sdp_handler_(NULL),
    close_handler_(NULL),
    server_addr_(),
    server_media_(),
    server_url_(NULL),
    sdp_(),
    audio_stream_receiver_(selector),
    audio_control_receiver_(selector),
    video_stream_receiver_(selector),
    video_control_receiver_(selector),
    session_id_(kInvalidSessionId),
    next_cseq_(1),
    state_(DISCONNECTED),
    timeouter_(selector, NewPermanentCallback(this,
        &SimpleClient::TimeoutHandler)) {
  audio_stream_receiver_.SetReadHandler(NewPermanentCallback(this,
      &SimpleClient::ReceiverReadHandler, true, false), true);
  audio_stream_receiver_.SetWriteHandler(NewPermanentCallback(this,
      &SimpleClient::ReceiverWriteHandler, true, false), true);
  audio_stream_receiver_.SetCloseHandler(NewPermanentCallback(this,
      &SimpleClient::ReceiverCloseHandler, true, false), true);

  audio_control_receiver_.SetReadHandler(NewPermanentCallback(this,
      &SimpleClient::ReceiverReadHandler, true, true), true);
  audio_control_receiver_.SetWriteHandler(NewPermanentCallback(this,
      &SimpleClient::ReceiverWriteHandler, true, true), true);
  audio_control_receiver_.SetCloseHandler(NewPermanentCallback(this,
      &SimpleClient::ReceiverCloseHandler, true, true), true);

  video_stream_receiver_.SetReadHandler(NewPermanentCallback(this,
      &SimpleClient::ReceiverReadHandler, false, false), true);
  video_stream_receiver_.SetWriteHandler(NewPermanentCallback(this,
      &SimpleClient::ReceiverWriteHandler, false, false), true);
  video_stream_receiver_.SetCloseHandler(NewPermanentCallback(this,
      &SimpleClient::ReceiverCloseHandler, false, false), true);

  video_control_receiver_.SetReadHandler(NewPermanentCallback(this,
      &SimpleClient::ReceiverReadHandler, false, true), true);
  video_control_receiver_.SetWriteHandler(NewPermanentCallback(this,
      &SimpleClient::ReceiverWriteHandler, false, true), true);
  video_control_receiver_.SetCloseHandler(NewPermanentCallback(this,
      &SimpleClient::ReceiverCloseHandler, false, true), true);
}
SimpleClient::~SimpleClient() {
  InternalClose();
  delete rtp_handler_;
  rtp_handler_ = NULL;
  delete sdp_handler_;
  sdp_handler_ = NULL;
  delete close_handler_;
  close_handler_ = NULL;
  delete server_url_;
  server_url_ = NULL;
}

void SimpleClient::Open(const net::HostPort& addr, const string& media) {
  CHECK_EQ(state(), DISCONNECTED);
  server_addr_ = addr;
  if ( server_addr_.IsInvalidPort() ) {
    server_addr_.set_port(rtsp::kDefaultPort);
  }
  server_media_ = media;
  server_url_ = new URL("rtsp://" + server_addr_.ToString()
        + "/" + server_media_);
  if ( !server_url_->is_valid() ) {
    RTSP_LOG_ERROR << "Error creating URL with media: [" << server_media_
                   << "]";
    selector_->RunInSelectLoop(NewCallback(this,
        &SimpleClient::InvokeCloseHandler, false));
    return;
  }

  set_state(CONNECTING);
  connection_.Connect(server_addr_);
}
void SimpleClient::Close() {
  if ( state() == DISCONNECTED ) {
    return;
  }
  if ( session_id_ == kInvalidSessionId ) {
    // Includes states: CONNECTING
    //                  DESCRIBE_SENT
    //                  SETUP_AUDIO_SENT
    InternalClose();
    return;
  }
  Request req(METHOD_TEARDOWN);
  req.mutable_header()->set_url(new URL(*server_url_));
  req.mutable_header()->add_field(new CSeqHeaderField(NextCSeq()));
  req.mutable_header()->add_field(new UserAgentHeaderField(kUserAgent));
  req.mutable_header()->add_field(new SessionHeaderField(session_id_, 0));
  Send(req);
  set_state(TEARDOWN_SENT);
}

SimpleClient::State SimpleClient::state() const {
  return state_;
}
const char* SimpleClient::state_name() const {
  return StateName(state());
}

void SimpleClient::set_rtp_handler(RtpHandler* handler) {
  CHECK(handler->is_permanent());
  delete rtp_handler_;
  rtp_handler_ = handler;
}
void SimpleClient::set_sdp_handler(SdpHandler* handler) {
  CHECK(handler->is_permanent());
  delete sdp_handler_;
  sdp_handler_ = handler;
}
void SimpleClient::set_close_handler(CloseHandler* handler) {
  CHECK(handler->is_permanent());
  delete close_handler_;
  close_handler_ = handler;
}
void SimpleClient::set_state(SimpleClient::State state) {
  if ( state == state_ ) {
    return;
  }
  RTSP_LOG_INFO << "Transitioning: " << state_name()
                << " ==> " << StateName(state);
  state_ = state;
}

uint16 SimpleClient::NextCSeq() {
  return next_cseq_++;
}

void SimpleClient::InvokeCloseHandler(bool is_error) {
  if ( close_handler_ == NULL ) {
    return;
  }
  close_handler_->Run(is_error);
}

void SimpleClient::Send(const Request& req) {
  RTSP_LOG_INFO << "Send: " << req.ToString();
  connection_.Send(req);
  timeouter_.SetTimeout(kRequestTimeoutId, kRequestTimeoutMs);
}

void SimpleClient::InternalClose() {
  timeouter_.UnsetTimeout(kRequestTimeoutId);
  audio_stream_receiver_.ForceClose();
  audio_control_receiver_.ForceClose();
  video_stream_receiver_.ForceClose();
  video_control_receiver_.ForceClose();
  set_state(DISCONNECTING);
  connection_.Close();
}
void SimpleClient::TimeoutHandler(int64 timeout_id) {
  if ( state() == DISCONNECTED ) {
    // a late timeout event, ignore it
    return;
  }
  if ( timeout_id == kRequestTimeoutId ) {
    RTSP_LOG_ERROR << "Timeout in state: " << state_name() << ", closing";
    // In request timeout don't send TEARDOWN again by means of Close().
    InternalClose();
    return;
  }
  RTSP_LOG_FATAL << "Unknown timeout_id: " << timeout_id;
}

void SimpleClient::HandleConnectionConnected(Connection* conn) {
  if ( state() == DISCONNECTING ) {
    // a late connect event. In the meanwhile we already called the disconnect.
    return;
  }
  CHECK_EQ(state(), CONNECTING);
  ///////////////////////////////////////////////////////////////////////////
  // send rtsp DESCRIBE
  Request req(METHOD_DESCRIBE);
  req.mutable_header()->set_url(new URL(*server_url_));
  req.mutable_header()->add_field(new CSeqHeaderField(NextCSeq()));
  req.mutable_header()->add_field(new AcceptHeaderField("application/sdp"));
  req.mutable_header()->add_field(new UserAgentHeaderField(kUserAgent));
  Send(req);
  set_state(DESCRIBE_SENT);
}
void SimpleClient::HandleRequest(Connection* conn, const Request* req) {
  scoped_ptr<const Request> auto_del_req(req);
  RTSP_LOG_ERROR << "Unexpected request: " << req->ToString();
}
void SimpleClient::HandleResponse(Connection* conn, const Response* rsp) {
  RTSP_LOG_INFO << "HandleResponse: " << rsp->ToString();
  scoped_ptr<const Response> auto_del_rsp(rsp);
  timeouter_.UnsetTimeout(kRequestTimeoutId);
  if ( rsp->header().status() != STATUS_OK ) {
    RTSP_LOG_ERROR << "Invalid Response in state: " << state_name()
                   << ", response: " << rsp->ToString();
    Close();
    return;
  }
  if ( state() == DESCRIBE_SENT ) {
    // We have got a Response from DESCRIBE
    Response* r = const_cast<Response*>(rsp);
    r->mutable_data()->MarkerSet();
    bool success = sdp_.ReadStream(*(r->mutable_data()));
    r->mutable_data()->MarkerRestore();
    if ( !success ) {
      RTSP_LOG_ERROR << "Failed to read SDP from response: " << rsp->ToString();
      Close();
      return;
    }
    RTSP_LOG_INFO << "Sdp from DESCRIBE: " << sdp_.ToString();
    // send SDP to application
    sdp_handler_->Run(&sdp_);
    // if we have a audio stream in sdp, then send rtsp SETUP for Audio
    if ( sdp_.media(true) ) {
      // open local RTP/RTCP receivers for audio
      if ( !audio_stream_receiver_.Open(net::HostPort(0,0)) ) {
        RTSP_LOG_ERROR << "Failed to open audio RTP listener";
        Close();
        return;
      }
      if ( !audio_control_receiver_.Open(net::HostPort(0,0)) ) {
        RTSP_LOG_ERROR << "Failed to open audio RTCP listener";
        Close();
        return;
      }
      // send rtsp SETUP for Audio
      Request req(METHOD_SETUP);
      req.mutable_header()->set_url(new URL(strutil::StringPrintf("%s?%s=%s",
          server_url_->spec().c_str(), kTrackIdName.c_str(),
          kTrackIdAudio.c_str())));
      req.mutable_header()->add_field(new CSeqHeaderField(NextCSeq()));
      req.mutable_header()->add_field(new UserAgentHeaderField(kUserAgent));
      TransportHeaderField* transport = new TransportHeaderField();
      transport->set_transport("RTP");
      transport->set_profile("AVP");
      transport->set_transmission_type(TransportHeaderField::TT_UNICAST);
      transport->set_client_port(make_pair(
          audio_stream_receiver_.local_addr().port(),
          audio_control_receiver_.local_addr().port()));
      req.mutable_header()->add_field(transport);
      Send(req);
      set_state(SETUP_AUDIO_SENT);
      return;
    }
  }
  if ( state() == DESCRIBE_SENT || state() == SETUP_AUDIO_SENT ) {
    if ( state() == SETUP_AUDIO_SENT ) {
      // We got a positive Audio SETUP Response
      const SessionHeaderField* s = rsp->header().tget_field<
          SessionHeaderField>();
      if ( s == NULL ) {
        RTSP_LOG_ERROR << "No session field in response: " << rsp->ToString();
        Close();
        return;
      }
      session_id_ = s->id();
    }
    // if we have a video stream in sdp, then send rtsp SETUP for Video
    if ( sdp_.media(false) ) {
      // open local RTP/RTCP receivers for video
      if ( !video_stream_receiver_.Open(net::HostPort(0,0)) ) {
        RTSP_LOG_ERROR << "Failed to open video RTP listener";
        Close();
        return;
      }
      if ( !video_control_receiver_.Open(net::HostPort(0,0)) ) {
        RTSP_LOG_ERROR << "Failed to open video RTCP listener";
        Close();
        return;
      }
      // send rtsp SETUP for Video
      Request req(METHOD_SETUP);
      req.mutable_header()->set_url(new URL(strutil::StringPrintf("%s?%s=%s",
          server_url_->spec().c_str(), kTrackIdName.c_str(),
          kTrackIdVideo.c_str())));
      req.mutable_header()->add_field(new CSeqHeaderField(NextCSeq()));
      req.mutable_header()->add_field(new UserAgentHeaderField(kUserAgent));
      if ( session_id_ != kInvalidSessionId ) {
        req.mutable_header()->add_field(new SessionHeaderField(session_id_, 0));
      }
      TransportHeaderField* transport = new TransportHeaderField();
      transport->set_transport("RTP");
      transport->set_profile("AVP");
      transport->set_transmission_type(TransportHeaderField::TT_UNICAST);
      transport->set_client_port(make_pair(
          video_stream_receiver_.local_addr().port(),
          video_control_receiver_.local_addr().port()));
      req.mutable_header()->add_field(transport);
      Send(req);
      set_state(SETUP_VIDEO_SENT);
      return;
    }
  }
  if ( state() == DESCRIBE_SENT ) {
    RTSP_LOG_ERROR << "No media stream! Nothing to SETUP.";
    Close();
    return;
  }
  if ( state() == SETUP_VIDEO_SENT ) {
    // We got a positive Video SETUP Response
    const SessionHeaderField* s = rsp->header().tget_field<
        SessionHeaderField>();
    if ( s == NULL ) {
      RTSP_LOG_ERROR << "No session field in response: " << rsp->ToString();
      Close();
      return;
    }
    if ( session_id_ == kInvalidSessionId ) {
      session_id_ = s->id();
    }
    if ( session_id_ != s->id() ) {
      RTSP_LOG_ERROR << "Different session ID. For Audio we got: "
          << session_id_ << " and for video: " << s->id();
      Close();
      return;
    }
  }
  if ( state() == SETUP_AUDIO_SENT || state() == SETUP_VIDEO_SENT ) {
    // send rtsp PLAY
    Request req(METHOD_PLAY);
    req.mutable_header()->set_url(new URL(*server_url_));
    req.mutable_header()->add_field(new CSeqHeaderField(NextCSeq()));
    req.mutable_header()->add_field(new UserAgentHeaderField(kUserAgent));
    req.mutable_header()->add_field(new SessionHeaderField(session_id_, 0));
    Send(req);
    set_state(PLAY_SENT);
    return;
  }
  if ( state() == PLAY_SENT ) {
    // We got a positive Response to PLAY
    set_state(PLAYING);
    return;
  }
  if ( state() == TEARDOWN_SENT ) {
    // We got a positive Response to TEARDOWN
    InternalClose();
    return;
  }
  if ( state() == DISCONNECTING ) {
    // Ignore a late response
    return;
  }
  RTSP_LOG_ERROR << "In state: " << state_name() << ", don't know how to"
                    " handle response: " << rsp->ToString();
}
void SimpleClient::HandleInterleavedFrame(Connection* conn,
    const InterleavedFrame* frame) {
  RTSP_LOG_ERROR << "SimpleClient::HandleInterleavedFrame: TODO implement";
}
void SimpleClient::HandleConnectionClose(Connection* conn) {
  CHECK(state() != DISCONNECTED) << " Multiple calls to HandleConnectionClose";
  set_state(DISCONNECTED);
  if ( session_id_ == kInvalidSessionId ) {
    RTSP_LOG_ERROR << "Failed to connect to server: "
                   << server_addr_.ToString();
    InvokeCloseHandler(true);
    return;
  }
  InvokeCloseHandler(false);
}

bool SimpleClient::ReceiverReadHandler(bool is_audio, bool is_control) {
  //RTSP_LOG_WARNING << "ReceiverReadHandler" << std::boolalpha
  //                 << " is_audio: " << is_audio
  //                 << ", is_control: " << is_control;
  net::UdpConnection& conn =
      (is_audio && !is_control) ? audio_stream_receiver_ :
      (is_audio && is_control) ? audio_control_receiver_ :
      (!is_audio && !is_control) ? video_stream_receiver_ :
                                   video_control_receiver_;
  const net::UdpConnection::Datagram* d = conn.PopRecvDatagram();
  scoped_ptr<const net::UdpConnection::Datagram> auto_del_d(d);
  if ( is_control ) {
    // ignore RTCP packets
    return true;
  }
  rtp_handler_->Run(&d->data_, is_audio);
  return true;
}
bool SimpleClient::ReceiverWriteHandler(bool is_audio, bool is_control) {
  return true;
}
void SimpleClient::ReceiverCloseHandler(bool is_audio, bool is_control,
    int err) {
  RTSP_LOG_INFO << "UDP Receiver close, is_audio: " << is_audio
                << ", is_control: " << is_control
                << ", err: " << err;
}


} // namespace rtsp
} // namespace streaming
