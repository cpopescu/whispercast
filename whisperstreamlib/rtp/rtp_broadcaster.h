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

#ifndef __MEDIA_RTP_BROADCASTER_H__
#define __MEDIA_RTP_BROADCASTER_H__

#include <whisperlib/common/base/types.h>
#include <whisperlib/net/base/address.h>
#include <whisperlib/net/base/udp_connection.h>

#include <whisperstreamlib/base/tag.h>
#include <whisperstreamlib/base/media_info.h>
#include <whisperstreamlib/flv/flv_tag.h>
#include <whisperstreamlib/f4v/f4v_tag.h>
#include <whisperstreamlib/mp3/mp3_frame.h>
#include <whisperstreamlib/rtp/rtp_packetization.h>
#include <whisperstreamlib/rtp/rtp_sdp.h>
#include <whisperstreamlib/rtp/rtp_sender.h>

namespace streaming {
namespace rtp {

// This class broadcasts RTP stream over UDP to a specific IP destination.
class Broadcaster {
public:
  // The broadcaster calls this on error or on EOS.
  // EosCallback(bool is_error);
  typedef Callback1<bool> EosCallback;
public:
  // eos_callback: permanent callback. We own it.
  Broadcaster(Sender* sender, EosCallback* eos_callback);
  virtual ~Broadcaster();

  Sender* sender() {
    return sender_;
  }
  template<typename SENDER>
  SENDER* tsender() {
    CHECK(sender_->type() == SENDER::kType);
    return static_cast<SENDER*>(sender_);
  }

  // The broadcaster needs to know what kind of data will receive.
  void SetMediaInfo(const MediaInfo& info);

  // Receive a tag from the library, send it to client through RTP.
  void HandleTag(const streaming::Tag* tag);
private:
  bool HandleFlvTag(const streaming::FlvTag& tag);
  bool HandleF4vTag(const streaming::F4vTag& tag);
  bool HandleMp3Tag(const streaming::Mp3FrameTag& tag, uint32 timestmap);

  bool RtpSend(const io::MemoryStream& frame_data, uint32 timestamp,
      bool is_audio);

public:
  string ToString() const;

private:
  // The rtp transport
  // We keep a reference on the sender to prevent sender deletion.
  Sender* sender_;

  // transmits EOS events to our superior
  EosCallback* eos_callback_;

  streaming::rtp::Packetizer* video_packetizer_;
  streaming::rtp::Packetizer* audio_packetizer_;

  uint32 rtp_audio_seq_number_;
  uint32 rtp_audio_payload_;
  uint32 rtp_audio_sample_rate_;
  uint32 rtp_video_seq_number_;
  uint32 rtp_video_payload_;
  uint32 rtp_video_clock_rate_;

  // just to prevent error logging on each frame
  uint64 dropped_frames_;
  int64 last_missing_packetizer_log_ts_;
  int64 last_packetizer_error_log_ts_;
};

} // namespace rtp
} // namespace streaming

#endif // __MEDIA_RTP_BROADCASTER_H__
