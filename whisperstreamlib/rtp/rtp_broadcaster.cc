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

#include <whisperlib/common/base/timer.h>
#include <whisperstreamlib/base/consts.h>
#include <whisperstreamlib/rtp/rtp_broadcaster.h>
#include <whisperstreamlib/rtp/rtp_util.h>
#include <whisperstreamlib/rtp/rtp_header.h>

namespace streaming {
namespace rtp {

Broadcaster::Broadcaster(Sender* sender, EosCallback* eos_callback)
  : sender_(sender),
    eos_callback_(eos_callback),
    video_packetizer_(NULL),
    audio_packetizer_(NULL),
    rtp_audio_seq_number_(0),
    rtp_audio_payload_(0),
    rtp_audio_sample_rate_(0),
    rtp_video_seq_number_(0),
    rtp_video_payload_(0),
    rtp_video_clock_rate_(0),
    dropped_frames_(0),
    last_missing_packetizer_log_ts_(0),
    last_packetizer_error_log_ts_(0) {
  sender_->Retain();
  CHECK(eos_callback->is_permanent());
}
Broadcaster::~Broadcaster() {
  sender_->Release();
  sender_ = NULL;
  delete eos_callback_;
  eos_callback_ = NULL;
}

void Broadcaster::SetMediaInfo(const MediaInfo& info) {
  Sdp sdp;
  util::ExtractSdpFromMediaInfo(0, 0, info, &sdp);

  RTP_LOG_DEBUG << "Using SDP: \n" << sdp.WriteString();
  const streaming::rtp::Sdp::Media* audio_media = sdp.media(true);
  if ( audio_media != NULL ) {
    rtp_audio_payload_ = audio_media->avp_payload_type_;
    rtp_audio_sample_rate_ = audio_media->clock_rate_;
    // initialize audio packetizer
    CHECK_NULL(audio_packetizer_);
    switch ( audio_media->avp_payload_type_ ) {
      case AVP_PAYLOAD_MPA:
        audio_packetizer_ = new streaming::rtp::Mp3Packetizer(
            info.audio().mp3_flv_container_);
        break;
      case AVP_PAYLOAD_DYNAMIC_AAC:
        audio_packetizer_ = new streaming::rtp::Mp4aPacketizer(
            info.audio().aac_flv_container_);
        break;
    };
    if ( audio_packetizer_ != NULL ) {
      RTP_LOG_INFO << "Using audio packetizer: "
                   << audio_packetizer_->ToString();
    } else {
      RTP_LOG_ERROR << "Don't know how to packetize payload: "
                    << audio_media->avp_payload_type_
                    << " (" << streaming::rtp::AvpPayloadTypeName(
                        audio_media->avp_payload_type_) << ")";
    }
  }
  const streaming::rtp::Sdp::Media* video_media = sdp.media(false);
  if ( video_media != NULL ) {
    rtp_video_payload_ = video_media->avp_payload_type_;
    rtp_video_clock_rate_ = video_media->clock_rate_;
    // initialize video packetizer
    CHECK_NULL(video_packetizer_);
    switch ( video_media->avp_payload_type_ ) {
      case AVP_PAYLOAD_MPV:
        video_packetizer_ = new streaming::rtp::H263Packetizer();
        break;
      case AVP_PAYLOAD_DYNAMIC_H264:
        video_packetizer_ = new streaming::rtp::H264Packetizer(
            info.video().h264_flv_container_);
        break;
    };
    if ( video_packetizer_ != NULL ) {
      RTP_LOG_INFO << "Using video packetizer: "
                   << video_packetizer_->ToString();
    } else {
      RTP_LOG_ERROR << "Don't know how to packetize payload: "
                    << video_media->avp_payload_type_
                    << " (" << streaming::rtp::AvpPayloadTypeName(
                        video_media->avp_payload_type_) << ")";
    }
  }
}

void Broadcaster::HandleTag(const streaming::Tag* tag) {
  bool success = true;
  switch ( tag->type() ) {
    case Tag::TYPE_FLV:
      success = HandleFlvTag(static_cast<const streaming::FlvTag&>(*tag));
      break;
    case Tag::TYPE_F4V:
      success = HandleF4vTag(static_cast<const streaming::F4vTag&>(*tag));
      break;
    case Tag::TYPE_MP3:
      success = HandleMp3Tag(static_cast<const streaming::Mp3FrameTag&>(*tag),
          tag->timestamp_ms());
      break;
    case Tag::TYPE_EOS:
      eos_callback_->Run(false);
      success = true;
      break;
    default:
      RTP_LOG_DEBUG << "Ignoring tag: " << tag->type_name();
      success = true;
      break;
  }
  if ( !success ) {
    eos_callback_->Run(true);
  }
}
bool Broadcaster::HandleFlvTag(const streaming::FlvTag& tag) {
  if ( tag.body().type() == streaming::FLV_FRAMETYPE_METADATA ) {
    RTP_LOG_WARNING << "Metadata: " << tag.ToString();
    return true;
  }
  bool is_audio = (tag.body().type() == streaming::FLV_FRAMETYPE_AUDIO);
  uint32 timestamp = (uint32)(is_audio
      ? (((uint64)tag.timestamp_ms()) * rtp_audio_sample_rate_ / 1000)
      : (((uint64)tag.timestamp_ms()) * rtp_video_clock_rate_ / 1000));
  const io::MemoryStream& data = is_audio ?
      tag.audio_body().data() : tag.video_body().data();
  return RtpSend(data, timestamp, is_audio);
}
bool Broadcaster::HandleF4vTag(const streaming::F4vTag& tag) {
  if ( !tag.is_frame() ) {
    return true;
  }
  const streaming::f4v::Frame* frame = tag.frame();
  if ( frame->header().type() == streaming::f4v::FrameHeader::RAW_FRAME) {
    // ignore RAW frames
    return true;
  }
  bool is_audio = (frame->header().type() ==
                   streaming::f4v::FrameHeader::AUDIO_FRAME);
  uint32 timestamp = is_audio ? frame->header().sample_index()
                              : frame->header().composition_timestamp()
                                 * (rtp_video_clock_rate_ / 1000);
  return RtpSend(frame->data(), timestamp, is_audio);
}
bool Broadcaster::HandleMp3Tag(const streaming::Mp3FrameTag& tag,
    uint32 timestamp) {
  return RtpSend(tag.data(), timestamp * 90, true);
}

bool Broadcaster::RtpSend(const io::MemoryStream& frame_data,
    uint32 timestamp, bool is_audio) {
  const char* frame_type = is_audio ? "Audio" : "Video";
  streaming::rtp::Packetizer* packetizer = (is_audio ? audio_packetizer_ :
                                                       video_packetizer_);
  const int64 now_ts = timer::TicksMsec();
  if ( packetizer == NULL ) {
    dropped_frames_++;
    if ( now_ts - last_missing_packetizer_log_ts_ > 5000 ) {
      RTP_LOG_WARNING << "No packetizer.. dropping " << frame_type << " frame"
                         ", total: " << dropped_frames_ << " frames dropped.";
      last_missing_packetizer_log_ts_ = now_ts;
    }
    return true;
  }

  vector<io::MemoryStream*> rtp_packets;
  if ( !packetizer->Packetize(frame_data, &rtp_packets) ) {
    dropped_frames_++;
    if ( now_ts - last_packetizer_error_log_ts_ > 5000 ) {
      LOG_ERROR << "Packetize failed, corrupted " << frame_type
                << " frame in stream.. ignoring"
                   ", total: " << dropped_frames_ << " frames dropped.";
      last_packetizer_error_log_ts_ = now_ts;
    }
    //Stop();
    return true;
  }

  bool success = true;
  for ( uint32 i = 0 ; i < rtp_packets.size(); i++ ) {
    bool mark = (is_audio || i == rtp_packets.size() - 1);
    Header rtp_header;
    rtp_header.set_flag_version(2);
    rtp_header.set_flag_padding(0);
    rtp_header.set_flag_extension(0);
    rtp_header.set_flag_cssid(0);
    rtp_header.set_payload_type_marker(mark ? 1 : 0);
    rtp_header.set_payload_type(is_audio ? rtp_audio_payload_
                                         : rtp_video_payload_);
    rtp_header.set_sequence_number(is_audio ? rtp_audio_seq_number_
                                            : rtp_video_seq_number_);
    rtp_header.set_timestamp(timestamp);
    rtp_header.set_ssrc(is_audio ? 0xceafa03c : 0x52d8e95a);

    io::MemoryStream ms;
    rtp_header.Encode(&ms);
    RTP_LOG_DEBUG << "Sending " << frame_type
                 << " payload: " << rtp_packets[i]->DumpContentInline();
    ms.AppendStreamNonDestructive(rtp_packets[i]);

    if ( !sender_->SendRtp(ms, is_audio) ) {
      success = false;
    }

    if ( is_audio ) {
      rtp_audio_seq_number_++;
    } else {
      rtp_video_seq_number_++;
    }

    delete rtp_packets[i];
    rtp_packets[i] = NULL;
  }
  return success;
}

string Broadcaster::ToString() const {
  ostringstream oss;
  oss << "Broadcaster{sender_: " << sender_->ToString()
      << ", rtp_audio_seq_number_: " << rtp_audio_seq_number_
      << ", rtp_audio_payload_: " << rtp_audio_payload_
      << ", rtp_video_seq_number_: " << rtp_video_seq_number_
      << ", rtp_video_payload_: " << rtp_video_payload_
      << ", rtp_video_clock_rate_: " << rtp_video_clock_rate_
      << "}";
  return oss.str();
}

} // namespace rtp
} // namespace streaming

