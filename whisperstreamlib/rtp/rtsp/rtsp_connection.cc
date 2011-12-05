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

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/system.h>
#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/base/timer.h>
#include <whisperlib/common/io/ioutil.h>

#include <whisperstreamlib/rtp/rtp_consts.h>
#include <whisperstreamlib/rtp/rtsp/rtsp_connection.h>
#include <whisperstreamlib/rtp/rtsp/rtsp_processor.h>
#include <whisperstreamlib/rtp/rtsp/types/rtsp_request.h>
#include <whisperstreamlib/rtp/rtsp/types/rtsp_response.h>

namespace streaming {
namespace rtsp {

const rtp::Sender::Type Connection::kType = rtp::Sender::RTSP;
const uint32 Connection::kOutbufMaxSize = 1 << 20; // 1MB
Connection::Connection(net::Selector* selector,
                       Processor* processor)
    : rtp::Sender(kType, selector),
      selector_(selector),
      tcp_connection_(NULL),
      decoder_(),
      processor_(processor),
      call_on_out_queue_space_(NULL),
      rtp_audio_ch_(0),
      rtp_video_ch_(0),
      session_id_() {
  CHECK_NOT_NULL(processor_);
}
Connection::~Connection() {
  delete tcp_connection_;
  tcp_connection_ = NULL;
  delete call_on_out_queue_space_;
  call_on_out_queue_space_ = NULL;
}

void Connection::Wrap(net::TcpConnection* tcp_connection) {
  CHECK_NULL(tcp_connection_);
  tcp_connection_ = tcp_connection;
  tcp_connection_->SetReadHandler(NewPermanentCallback(this,
      &Connection::ConnectionReadHandler), true);
  tcp_connection_->SetWriteHandler(NewPermanentCallback(this,
      &Connection::ConnectionWriteHandler), true);
  tcp_connection_->SetCloseHandler(NewPermanentCallback(this,
      &Connection::ConnectionCloseHandler), true);
  decoder_.set_remote_address(tcp_connection_->remote_address().ToString());
}
void Connection::Connect(const net::HostPort& addr) {
  CHECK_NULL(tcp_connection_);
  tcp_connection_ = new net::TcpConnection(selector_,
      net::TcpConnectionParams());
  tcp_connection_->SetConnectHandler(NewPermanentCallback(this,
      &Connection::ConnectionConnectHandler), true);
  tcp_connection_->SetReadHandler(NewPermanentCallback(this,
      &Connection::ConnectionReadHandler), true);
  tcp_connection_->SetWriteHandler(NewPermanentCallback(this,
      &Connection::ConnectionWriteHandler), true);
  tcp_connection_->SetCloseHandler(NewPermanentCallback(this,
      &Connection::ConnectionCloseHandler), true);
  if ( !tcp_connection_->Connect(addr) ) {
    LOG_ERROR << "Failed to connect to: " << addr.ToString();
    delete tcp_connection_;
    tcp_connection_ = NULL;
    processor_->HandleConnectionClose(this);
    return;
  }
}
void Connection::Close() {
  if ( tcp_connection_ == NULL ) {
    return;
  }
  tcp_connection_->Close(net::TcpConnection::CLOSE_READ_WRITE);
}

const net::HostPort& Connection::local_address() const {
  return tcp_connection_->local_address();
}
const net::HostPort& Connection::remote_address() const {
  return tcp_connection_->remote_address();
}
const string& Connection::session_id() const {
  return session_id_;
}
void Connection::set_rtp_audio_ch(uint8 rtp_audio_ch) {
  rtp_audio_ch_ = rtp_audio_ch;
}
void Connection::set_rtp_video_ch(uint8 rtp_video_ch) {
  rtp_video_ch_ = rtp_video_ch;
}
void Connection::set_rtp_ch(uint8 rtp_ch, bool is_audio) {
  if ( is_audio ) {
    set_rtp_audio_ch(rtp_ch);
  } else {
    set_rtp_video_ch(rtp_ch);
  }
}
void Connection::set_session_id(const string& session_id) {
  session_id_ = session_id;
}
void Connection::Send(const Request& request) {
  CHECK_NOT_NULL(tcp_connection_);
  Encoder::Encode(request, tcp_connection_->outbuf());
  tcp_connection_->RequestWriteEvents(true);
}
void Connection::Send(const Response& response) {
  CHECK_NOT_NULL(tcp_connection_);
  Encoder::Encode(response, tcp_connection_->outbuf());
  tcp_connection_->RequestWriteEvents(true);
}
bool Connection::SendRtp(const io::MemoryStream& rtp_packet, bool is_audio) {
  CHECK_NOT_NULL(tcp_connection_);
  Encoder::EncodeInterleavedFrame(is_audio ? rtp_audio_ch_ : rtp_video_ch_,
      rtp_packet, tcp_connection_->outbuf());
  tcp_connection_->RequestWriteEvents(true);
  return true;
}
uint32 Connection::OutQueueSpace() const {
  uint32 outbuf_size = tcp_connection_->outbuf()->Size();
  return outbuf_size < kOutbufMaxSize ?
         (outbuf_size - kOutbufMaxSize) / rtp::kRtpMtu : 0;
}
void Connection::CloseSender() {
  delete tcp_connection_;
  tcp_connection_ = NULL;
}
void Connection::SetCallOnOutQueueSpace(Closure* closure) {
  // The mechanism requires a non permanent closure.
  // Because we need: udp_connection_.RequestWriteEvents(true) every time
  CHECK(!closure->is_permanent());
  delete call_on_out_queue_space_;
  call_on_out_queue_space_ = closure;
  tcp_connection_->RequestWriteEvents(true);
}
string Connection::ToString() const {
  if ( tcp_connection_ == NULL ) {
    return "[unconnected]";
  }
  return strutil::StringPrintf("[%d -> %s]",
      tcp_connection_->local_address().port(),
      tcp_connection_->remote_address().ToString().c_str());
}

void Connection::ConnectionConnectHandler() {
  decoder_.set_remote_address(tcp_connection_->remote_address().ToString());
  processor_->HandleConnectionConnected(this);
}
bool Connection::ConnectionReadHandler() {
  //RTSP_LOG_DEBUG << "ConnectionReadHandler";
  io::MemoryStream* in = tcp_connection_->inbuf();
  while ( true ) {
    Message* msg = NULL;
    DecodeStatus result = decoder_.Decode(*in, &msg);
    if ( result == DECODE_ERROR ) {
      CHECK_NULL(msg);
      RTSP_LOG_ERROR << "Error parsing message, closing connection";
      return false;
    }
    if ( result == DECODE_NO_DATA ) {
      // wait for more data
      CHECK_NULL(msg);
      return true;
    }
    CHECK_EQ(result, DECODE_SUCCESS);
    CHECK_NOT_NULL(msg);
    switch ( msg->type() ) {
      case Message::MESSAGE_REQUEST: {
        Request* req = static_cast<Request*>(msg);
        req->set_connection(this);
        processor_->Handle(this, req);
        break;
      }
      case Message::MESSAGE_RESPONSE:
        processor_->Handle(this, static_cast<Response*>(msg));
        break;
      case Message::MESSAGE_INTERLEAVED_FRAME:
        processor_->Handle(this, static_cast<InterleavedFrame*>(msg));
        break;
    }
  }
}

bool Connection::ConnectionWriteHandler() {
  //RTSP_LOG_DEBUG << "ConnectionWriteHandler()";
  if ( call_on_out_queue_space_ != NULL && OutQueueSpace() > 0 ) {
    call_on_out_queue_space_->Run();
    call_on_out_queue_space_ = NULL;
  }
  return true;
}

void Connection::ConnectionCloseHandler(int err,
                                        net::NetConnection::CloseWhat what) {
  RTSP_LOG_INFO << "ConnectionCloseHandler(), err: " << err << ", what: "
                << net::NetConnection::CloseWhatName(what);
  if ( what != net::NetConnection::CLOSE_READ_WRITE ) {
    tcp_connection_->Close(net::NetConnection::CLOSE_READ_WRITE);
    return;
  }
  // Just for safety: make sure we don't use this tcp_connection_ anymore.
  CHECK_NOT_NULL(tcp_connection_)
      << " Multiple calls to ConnectionCloseHandler";
  delete tcp_connection_;
  tcp_connection_ = NULL;

  processor_->HandleConnectionClose(this);
}

} // namespace rtsp
} // namespace streaming
