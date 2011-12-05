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

#include <whisperstreamlib/base/consts.h>
#include <whisperstreamlib/rtp/rtp_util.h>
#include <whisperstreamlib/rtp/rtp_udp_sender.h>

namespace streaming {
namespace rtp {

const UdpSender::Type UdpSender::kType = Sender::UDP;
uint32 UdpSender::kOutQueueMaxSize = 100;
UdpSender::UdpSender(net::Selector* selector)
  : Sender(kType, selector),
    selector_(selector),
    udp_connection_(selector),
    udp_audio_dst_(),
    udp_video_dst_(),
    call_on_out_queue_space_(NULL) {
  udp_connection_.SetReadHandler(NewPermanentCallback(this,
      &UdpSender::ConnectionReadHandler), true);
  udp_connection_.SetWriteHandler(NewPermanentCallback(this,
      &UdpSender::ConnectionWriteHandler), true);
  udp_connection_.SetCloseHandler(NewPermanentCallback(this,
      &UdpSender::ConnectionCloseHandler), true);
}
UdpSender::~UdpSender() {
  udp_connection_.ForceClose();
  delete call_on_out_queue_space_;
  call_on_out_queue_space_ = NULL;
}

bool UdpSender::Initialize() {
  if ( !udp_connection_.Open(net::HostPort()) ) {
    RTP_LOG_ERROR << "Failed to open UDP connection";
    return false;
  }
  return true;
}

const net::HostPort& UdpSender::udp_audio_dst() const {
  return udp_audio_dst_;
}
const net::HostPort& UdpSender::udp_video_dst() const {
  return udp_video_dst_;
}
uint16 UdpSender::udp_local_port() const {
  return udp_connection_.local_addr().port();
}
void UdpSender::SetDestination(const net::HostPort& udp_dst, bool audio) {
  if ( audio ) {
    udp_audio_dst_ = udp_dst;
    RTP_LOG_INFO << "Streaming audio to: " << udp_audio_dst_;
  } else {
    udp_video_dst_ = udp_dst;
    RTP_LOG_INFO << "Streaming video to: " << udp_video_dst_;
  }
}

bool UdpSender::SendRtp(const io::MemoryStream& rtp_packet, bool is_audio) {
  if ( !udp_connection_.IsOpen() ) {
    RTP_LOG_ERROR << "Cannot send, UDP connection is closed";
    return false;
  }
  if ( (is_audio && udp_audio_dst_.port() == 0) ||
       (!is_audio && udp_video_dst_.port() == 0) ) {
    RTP_LOG_ERROR << "Cannot send, required destination port is missing";
    return true;
  }
  udp_connection_.SendDatagram(rtp_packet, is_audio ? udp_audio_dst_
                                                    : udp_video_dst_);
  RTP_LOG_DEBUG << (is_audio ? "Audio" : "Video") << " datagram sent.. "
                << rtp_packet.Size() << " bytes";
  return true;
}
uint32 UdpSender::OutQueueSpace() const {
  uint32 out_queue_size = udp_connection_.out_queue_size();
  return kOutQueueMaxSize > out_queue_size ?
         kOutQueueMaxSize - out_queue_size : 0;
}
void UdpSender::SetCallOnOutQueueSpace(Closure* closure) {
  // The mechanism requires a non permanent closure.
  // Because we need: udp_connection_.RequestWriteEvents(true) every time
  CHECK(!closure->is_permanent());
  delete call_on_out_queue_space_;
  call_on_out_queue_space_ = closure;
  udp_connection_.RequestWriteEvents(true);
}
void UdpSender::CloseSender() {
  udp_connection_.ForceClose();
}
string UdpSender::ToString() const {
  ostringstream oss;
  oss << "UdpSender{udp_audio_dst_: " << udp_audio_dst_
      << ", udp_video_dst_: " << udp_video_dst_ << "}";
  return oss.str();
}

bool UdpSender::ConnectionReadHandler() {
  udp_connection_.RequestReadEvents(false);
  return true;
}
bool UdpSender::ConnectionWriteHandler() {
  if ( call_on_out_queue_space_ != NULL && OutQueueSpace() > 0 ) {
    call_on_out_queue_space_->Run();
    call_on_out_queue_space_ = NULL;
  }
  // the UdpConnection auto-stops Write Events
  return true;
}
void UdpSender::ConnectionCloseHandler(int err) {
  RTP_LOG_INFO << "ConnectionCloseHandler(), err: "
               << GetSystemErrorDescription(err);
}

} // namespace rtp
} // namespace streaming

