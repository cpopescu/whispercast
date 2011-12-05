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

#ifndef __MEDIA_RTP_SENDER_H__
#define __MEDIA_RTP_SENDER_H__

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperlib/net/base/selector.h>

namespace streaming {
namespace rtp {

// This is an interface for sending RTP packets.
// The actual implementation may use a UDP transport or an existing RTSP
// connection as transport.
class Sender {
public:
  enum Type {
    UDP,
    RTSP,
  };
public:
  Sender(Type type, net::Selector* selector)
    : type_(type), selector_(selector), ref_count_(0) {}
  virtual ~Sender() {
    // Don't call virtual methods on destructor!
    CHECK(ref_count_ == 0);
  }

  Type type() const { return type_; }

  void Release() {
    CHECK(ref_count_ > 0);
    ref_count_--;
    if ( ref_count_ == 0 ) {
      // Stop everything.
      CloseSender();
      // Delay the delete.
      selector_->DeleteInSelectLoop(this);
    }
  }
  void Retain() {
    ref_count_++;
  }

  // rtp_packet: a complete RTP packet, including header.
  // returns: true on success.
  //          false on transport fail. The upper layer will close this sender.
  virtual bool SendRtp(const io::MemoryStream& rtp_packet, bool is_audio) = 0;
  // returns how much space is left in the oubuf queue, in terms of RTP packets
  virtual uint32 OutQueueSpace() const = 0;
  // call this closure when OutQueueSpace() > 0
  // We use this mechanism to unpause flow control.
  virtual void SetCallOnOutQueueSpace(Closure* closure) = 0;
  // close everything, this sender becomes a vegetable.
  // Important because on Release, we need to stop this sender completely, then
  // delay the actual delete.
  virtual void CloseSender() = 0;

  virtual string ToString() const = 0;
private:
  const Type type_;
  net::Selector* selector_;
  uint32 ref_count_;
};

} // namespace rtp
} // namespace streaming

#endif // __MEDIA_RTP_SENDER_H__
