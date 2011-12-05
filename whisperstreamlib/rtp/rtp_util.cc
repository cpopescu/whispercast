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
#include <whisperlib/net/util/base64.h>
#include <whisperstreamlib/base/media_info_util.h>
#include <whisperstreamlib/rtp/rtp_util.h>
#include <whisperstreamlib/rtp/rtp_consts.h>
#include <whisperstreamlib/rtp/rtp_packetization.h>
#include <whisperstreamlib/f4v/f4v_util.h>

DEFINE_int32(rtp_log_level, LERROR,
              "Enables RTP debug messages. Set to 4 for all messages.");

namespace streaming {
namespace rtp {
namespace util {

string MemoryStreamToHexa(const io::MemoryStream& in, uint32 offset=0,
                          uint32 size=0xffffffff) {
  if ( size == 0xffffffff ) {
    if ( in.Size() < offset ) {
      return "";
    }
    size = in.Size() - offset;
  }
  char* tmp_in = new char[size];
  char* tmp_out = new char[size*2+1];
  const char* hexa = "0123456789abcdef";
  io::MemoryStream& ms = const_cast<io::MemoryStream&>(in);
  ms.MarkerSet();
  ms.Skip(offset);
  ms.Read(tmp_in, size);
  ms.MarkerRestore();
  for ( uint32 i = 0; i < size; i++ ) {
    tmp_out[2*i  ] = hexa[(tmp_in[i] >> 4) & 0x0f];
    tmp_out[2*i+1] = hexa[(tmp_in[i]     ) & 0x0f];
  }
  tmp_out[2*size] = 0;

  string s(tmp_out);
  delete [] tmp_in;
  delete [] tmp_out;
  return s;
}

string MemoryStreamToBase64(const io::MemoryStream& in) {
  const uint32 size = in.Size();
  const uint32 out_size = size*2 + 8;
  char* tmp_in = new char[size];
  char* tmp_out = new char[out_size];
  in.Peek(tmp_in, size);
  base64::Encoder b64;
  const int32 len = b64.Encode(tmp_in, size, tmp_out, out_size);
  CHECK_LT(len, out_size - 4);
  const int32 len2 = b64.EncodeEnd(tmp_out + len);
  CHECK_LT(len2, 4);
  tmp_out[len + len2] = '\0';
  const string ret(tmp_out, len + len2);
  delete [] tmp_in;
  delete [] tmp_out;
  return ret;
}

void ExtractSdpFromMediaInfo(uint16 audio_dst_port, uint16 video_dst_port,
    const MediaInfo& info, Sdp* out) {
  RTP_LOG_INFO << "ExtractSdpFromMediaInfo: " << info.ToString();

  // add stream length
  if ( info.duration_ms() > 0 ) {
    out->add_attribute(strutil::StringPrintf(
        "range:npt=0-%.2f", info.duration_ms() / 1000.0f));
  }

  // audio stream configuration
  do {
    if ( !info.has_audio() ) {
      break;
    }
    uint8 payload_type = 255;
    string audio_codec_name = "unknown";
    string audio_fmtp = "";
    if ( info.audio().format_ == MediaInfo::Audio::FORMAT_AAC ) {
      payload_type = AVP_PAYLOAD_DYNAMIC_AAC;
      audio_codec_name = "mpeg4-generic";
      audio_fmtp = strutil::StringPrintf(
        "streamtype=5; profile-level-id=15; "
        "mode=AAC-hbr; config=%02x%02x; SizeLength=13; "
        "IndexLength=3; IndexDeltaLength=3; Profile=1;",
        info.audio().aac_config_[0], info.audio().aac_config_[1]);
    }
    if ( info.audio().format_ == MediaInfo::Audio::FORMAT_MP3 ) {
      payload_type = AVP_PAYLOAD_MPA;
      audio_codec_name = AvpPayloadTypeName(payload_type);
    }
    if ( payload_type == 255 ) {
      RTP_LOG_ERROR << "unknown audio codec, skipping audio stream, info: "
                    << info.ToString();
      break;
    }
    vector<string> attr;
    attr.push_back(strutil::StringPrintf("rtpmap:%d %s/%d/%d",
        payload_type, audio_codec_name.c_str(), info.audio().sample_rate_,
        info.audio().channels_));
    if ( audio_fmtp != "" ) {
      attr.push_back(strutil::StringPrintf("fmtp:%d %s",
          payload_type, audio_fmtp.c_str()));
    }
    out->AddMedia(streaming::rtp::Sdp::Media(true, audio_dst_port,
        payload_type, info.audio().sample_rate_, attr));
  } while (false);

  // video stream configuration
  do {
    if ( !info.has_video() ) {
      break;
    }
    uint8 payload_type = 255;
    string video_codec_name = "unknown";
    string video_fmtp = "";
    if ( info.video().format_ == MediaInfo::Video::FORMAT_H264 ) {
      payload_type = AVP_PAYLOAD_DYNAMIC_H264;
      video_codec_name = "H264";
      video_fmtp = strutil::StringPrintf(
        "packetization-mode=1;"
        "profile-level-id=%02x%02x%02x;"
        "sprop-parameter-sets=%s,%s;",
        info.video().h264_profile_,
        info.video().h264_profile_compatibility_,
        info.video().h264_level_,
        base64::EncodeVector(info.video().h264_sps_).c_str(),
        base64::EncodeVector(info.video().h264_pps_).c_str());
    }
    if ( info.video().format_ == MediaInfo::Video::FORMAT_H263 ) {
      payload_type = AVP_PAYLOAD_MPV;
      video_codec_name = AvpPayloadTypeName(payload_type);
    }
    if ( payload_type == 255 ) {
      RTP_LOG_ERROR << "unknown video codec, skipping video stream, info: "
                    << info.ToString();
      break;
    }
    vector<string> attr;
    attr.push_back(strutil::StringPrintf("rtpmap:%d %s/%u",
        payload_type, video_codec_name.c_str(), info.video().clock_rate_));
    if ( video_fmtp != "" ) {
      attr.push_back(strutil::StringPrintf("fmtp:%d %s",
          payload_type, video_fmtp.c_str()));
    }
    out->AddMedia(streaming::rtp::Sdp::Media(false, video_dst_port,
        payload_type, info.video().clock_rate_, attr));
  } while (false);
}

bool ExtractSdpFromFile(uint16 audio_dst_port, uint16 video_dst_port,
    const string& filename, Sdp* out) {
  MediaInfo info;
  if ( !streaming::util::ExtractMediaInfoFromFile(filename, &info) ) {
    RTP_LOG_ERROR << "Failed to extract MediaInfo from file: ["
                  << filename << "]";
    return false;
  }
  ExtractSdpFromMediaInfo(audio_dst_port, video_dst_port, info, out);
  return true;
}

}
}
}
