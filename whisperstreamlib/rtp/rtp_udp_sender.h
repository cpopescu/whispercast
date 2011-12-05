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

#ifndef __MEDIA_RTP_RTP_SENDER_H__
#define __MEDIA_RTP_RTP_SENDER_H__

#include <whisperlib/common/base/types.h>
#include <whisperlib/net/base/address.h>
#include <whisperlib/net/base/udp_connection.h>
#include <whisperstreamlib/rtp/rtp_sender.h>

namespace streaming {
namespace rtp {

// Implementation of rtp::Sender. This class uses UDP transport to send RTP
// packets to a specific IP/port.
class UdpSender : public Sender {
public:
  static const Type kType;
  static uint32 kOutQueueMaxSize;
  UdpSender(net::Selector* selector);
  virtual ~UdpSender();

  // Initializes the UDP connection
  bool Initialize();

  const net::HostPort& udp_audio_dst() const;
  const net::HostPort& udp_video_dst() const;
  uint16 udp_local_port() const;

  // Set UDP destination address for audio/video.
  // If video address is not set, then we don't stream video. Same for audio.
  void SetDestination(const net::HostPort& udp_dst, bool audio);

  /////////////////////////////////////////////////////////////////////////
  // rtp::Sender methods
  virtual bool SendRtp(const io::MemoryStream& rtp_packet, bool is_audio);
  virtual uint32 OutQueueSpace() const;
  virtual void SetCallOnOutQueueSpace(Closure* closure);
  virtual void CloseSender();
  virtual string ToString() const;

private:
  bool ConnectionReadHandler();
  bool ConnectionWriteHandler();
  void ConnectionCloseHandler(int err);

public:


private:
  net::Selector* selector_;
  net::UdpConnection udp_connection_;

  net::HostPort udp_audio_dst_;
  net::HostPort udp_video_dst_;

  Closure* call_on_out_queue_space_;
};

} // namespace rtp
} // namespace streaming

#endif // __MEDIA_RTP_RTP_SENDER_H__
