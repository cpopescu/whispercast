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
// Author: Cosmin Tudorache & Catalin Popescu

#include <arpa/inet.h>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperlib/common/io/num_streaming.h>
#include <whisperlib/common/base/gflags.h>
#include "flv/flv_coder.h"
#include "flv/flv_tag.h"

#include "rtmp/objects/rtmp_objects.h"
#include "rtmp/objects/amf/amf_util.h"

//////////////////////////////////////////////////////////////////////

DEFINE_int32(flv_max_tag_size,
             5 << 20,  // 4 MB
             "Maximum accepted FLV tag size");
DEFINE_int32(flv_coder_search_for_head, 0,
             "We search for the header before decoding in these first bytes..");

//////////////////////////////////////////////////////////////////////

namespace streaming {

streaming::TagReadStatus FlvCoder::DecodeHeader(io::MemoryStream* in,
                                                scoped_ref<FlvHeader>* header,
                                                int* cb) {
  if ( FLAGS_flv_coder_search_for_head > 0 ) {
    if ( in->Size() < FLAGS_flv_coder_search_for_head ) {
      return streaming::READ_NO_DATA;
    }
    in->MarkerSet();
    string s;
    in->ReadString(&s, FLAGS_flv_coder_search_for_head);
    in->MarkerRestore();
    size_t pos_flv = s.find(string("FLV", 3));
    if ( pos_flv == string::npos ) {
     return streaming::READ_CORRUPTED_FAIL;
    }
    LOG_INFO << " Header found at position: " << pos_flv;
    in->Skip(pos_flv);
  }
  *cb = 0;
  if ( in->Size() < kFlvHeaderSize ) {
    return streaming::READ_NO_DATA;
  }
  *cb += kFlvHeaderSize;

  // unknown 3 bytes
  const uint32 unknown = io::NumStreamer::ReadUInt24(in, common::BIGENDIAN);
  const uint8 version = io::NumStreamer::ReadByte(in);
  const uint8 audio_video_flags = io::NumStreamer::ReadByte(in);
  const uint32 data_offset = io::NumStreamer::ReadUInt32(in, common::BIGENDIAN);

  *header = new FlvHeader(0,
      kDefaultFlavourMask,
      unknown, version, audio_video_flags, data_offset);

  return streaming::READ_OK;
}


streaming::TagReadStatus FlvCoder::DecodeTag(io::MemoryStream* in,
                                             scoped_ref<FlvTag>* tag,
                                             int* cb) {
  *cb = 0;
  if ( in->Size() < kFlvTagHeaderSize ) {
    return streaming::READ_NO_DATA;
  }
  in->MarkerSet();
  const uint32 prev_tag_size =
      io::NumStreamer::ReadUInt32(in, common::BIGENDIAN);
  const uint8  type = io::NumStreamer::ReadByte(in);
  const uint32 body_size = io::NumStreamer::ReadUInt24(in, common::BIGENDIAN);
  if ( body_size > FLAGS_flv_max_tag_size ) {
    in->MarkerRestore();
    LOG_ERROR << "Oversized FLV tag size found: " << body_size;
    return streaming::READ_OVERSIZED_TAG;
  }

  if ( type != FLV_FRAMETYPE_VIDEO &&
       type != FLV_FRAMETYPE_AUDIO &&
       type != FLV_FRAMETYPE_METADATA ) {
    in->MarkerRestore();
    LOG_ERROR << "Illegal frame type: "
              << strutil::StringPrintf("0x%02x", type)
              << " in stream: " << in->DumpContent();
    return streaming::READ_CORRUPTED_FAIL;
  }
  FlvFrameType data_type = static_cast<FlvFrameType>(type);

  if ( (data_type == FLV_FRAMETYPE_AUDIO ||
        data_type == FLV_FRAMETYPE_VIDEO) && body_size == 0 ) {
    // No header for audio or video tag
    LOG_ERROR << "Zero sized FLV tag size found";
    in->MarkerRestore();
    return streaming::READ_CORRUPTED_FAIL;
  }

  // The lower 3 bytes of timestamp. This is how the protocol was
  // firstly designed. The longest stream would be 2^24ms = 4.66hours
  const uint32 timestamp_low =
      io::NumStreamer::ReadUInt24(in, common::BIGENDIAN);

  // The most significant byte of timestamp. This is a protocol extension
  // for stream durations beyond 4.66 hours.
  const uint32 timestamp_hi = io::NumStreamer::ReadByte(in);

  // recompose timestamp
  const uint32 timestamp = (timestamp_low | (timestamp_hi << 24));

  const uint32 stream_id =
      io::NumStreamer::ReadUInt24(in, common::BIGENDIAN);

  if ( in->Size() < body_size ) {
    in->MarkerRestore();
    return streaming::READ_NO_DATA;
  }
  in->MarkerClear();

  *cb = kFlvTagHeaderSize + body_size;

  // compose Tag
  *tag = new FlvTag(0, kDefaultFlavourMask, timestamp, data_type);
  (*tag)->set_previous_tag_size(prev_tag_size);
  (*tag)->set_stream_id(stream_id);
  (*tag)->mutable_body().Decode(*in, body_size);

  return streaming::READ_OK;
}

TagReadStatus FlvCoder::DecodeVideoFlags(const io::MemoryStream& const_ms,
    FlvFlagVideoCodec* video_codec,
    FlvFlagVideoFrameType* video_frame_type,
    AvcPacketType* video_avc_packet_type,
    int32* video_avc_composition_time) {
  io::MemoryStream& ms = const_cast<io::MemoryStream&>(const_ms);
  ms.MarkerSet();
  if ( ms.Size() < 1 ) {
    ms.MarkerRestore();
    return READ_NO_DATA;
  }
  uint8 fb = io::NumStreamer::ReadByte(&ms);
  *video_codec = static_cast<FlvFlagVideoCodec>(
      (fb & kFlvMaskVideoCodec) >> 0);
  *video_frame_type = static_cast<FlvFlagVideoFrameType>(
      (fb & kFlvMaskVideoFrameType) >> 4);

  if ( *video_codec == FLV_FLAG_VIDEO_CODEC_AVC ) {
    if ( ms.Size() < sizeof(uint32) ) {
      ms.MarkerRestore();
      return READ_NO_DATA;
    }
    *video_avc_packet_type = static_cast<streaming::AvcPacketType>(
        io::NumStreamer::ReadByte(&ms));
    *video_avc_composition_time =
        io::NumStreamer::ReadUInt24(&ms, common::BIGENDIAN);
  }
  ms.MarkerRestore();
  return READ_OK;
}

FlvTag* FlvCoder::DecodeAuxiliaryMoovTag(const FlvTag* tag) {
  // Raugh stuff for h264 / AFLME 3.0
  static const uint8 kAfme3_0_MoovBeginning[] = {
    0x17,   0x01,   0x00,   0x00,  0x00
  };
  // comes after a size of 4:
  static const int kAfme3_0_MoovSignalBegin = 9;
  static const uint8 kAfme3_0_MoovSignal[] = {
    0x65,   0x88,   0x80,   0x40
    // 0x65 - NALU type and stuff..
  };

  // Raugh stuff for x264 encoded stuff
  static const uint8 kMoovBeginning[] = {
    0x17,   0x01,   0x00,   0x00,  0x00,
    0x00,   0x00,   0x00,   0x02,  0x09,   0x10   // size and type..
  };
  static const int kMoovMarkBegin = 35;
  static const uint8 kMoovMark[] = {
    0x78,   0x32,   0x36,   0x34     // x264
  };
  if ( tag->body().type() != FLV_FRAMETYPE_VIDEO ) {
    return NULL;
  }
  const FlvTag::Video& video_body = tag->video_body();
  if ( video_body.video_frame_type() != FLV_FLAG_VIDEO_FRAMETYPE_KEYFRAME ||
       video_body.video_codec() != FLV_FLAG_VIDEO_CODEC_AVC ||
       video_body.video_avc_packet_type() != streaming::AVC_NALU ) {
    return NULL;
  }

  // Mega fucked up stuff part1..
  if ( video_body.data().Size() < kMoovMarkBegin + sizeof(kMoovMark) ) {
    return NULL;
  }
  char check_buffer[kMoovMarkBegin + sizeof(kMoovMark)];
  video_body.data().Peek(check_buffer, sizeof(check_buffer));

  // This should be cool ALME
  int32 size_moov = 0;
  if ( memcmp(check_buffer,
              kAfme3_0_MoovBeginning,  sizeof(kAfme3_0_MoovBeginning)) ||
       memcmp(check_buffer + kAfme3_0_MoovSignalBegin,
              kAfme3_0_MoovSignal, sizeof(kAfme3_0_MoovSignalBegin))) {
    if ( memcmp(check_buffer, kMoovBeginning, sizeof(kMoovBeginning)) ||
         memcmp(check_buffer + kMoovMarkBegin, kMoovMark, sizeof(kMoovMark)) ) {
      return NULL;
    }
    size_moov = sizeof(kMoovBeginning) + sizeof(uint32) + ntohl(
        *reinterpret_cast<uint32*>(check_buffer + sizeof(kMoovBeginning)));
  } else {
    size_moov = sizeof(kAfme3_0_MoovBeginning) + sizeof(uint32) + ntohl(
        *reinterpret_cast<uint32*>(check_buffer + sizeof(kAfme3_0_MoovBeginning)));
  }
  if ( size_moov > video_body.data().Size() ||
       size_moov <= 0 ) {
    LOG_WARNING << " Corrupted moov - or something: " << size_moov;
    return NULL;
  }

  scoped_ref<FlvTag::Video> ret = new FlvTag::Video();
  io::MemoryStream& video_data =
      const_cast<io::MemoryStream&>(video_body.data());
  video_data.MarkerSet();
  TagReadStatus result = ret->Decode(video_data, size_moov);
  video_data.MarkerRestore();
  if ( result != READ_OK ) {
    LOG_ERROR << "Failed to read moov, result: " << TagReadStatusName(result)
              << ", moov size: " << size_moov
              << ", stream size: " << video_data.Size();
    return NULL;
  }

  return new FlvTag(tag->attributes(), tag->flavour_mask(),
      tag->timestamp_ms(), ret.get());
}


TagReadStatus FlvCoder::DecodeAudioFlags(const io::MemoryStream& ms,
    FlvFlagSoundType* audio_type,
    FlvFlagSoundFormat* audio_format,
    FlvFlagSoundRate* audio_rate,
    FlvFlagSoundSize* audio_size,
    bool* audio_is_aac_header) {
  uint8 fb;
  if ( sizeof(fb) != ms.Peek(&fb, sizeof(fb)) ) {
    return READ_NO_DATA;
  }
  *audio_type = static_cast<FlvFlagSoundType>((fb & kFlvMaskSoundType) >> 0);
  *audio_size = static_cast<FlvFlagSoundSize>((fb & kFlvMaskSoundSize) >> 1);
  *audio_rate = static_cast<FlvFlagSoundRate>((fb & kFlvMaskSoundRate) >> 2);
  *audio_format = static_cast<FlvFlagSoundFormat>(
      (fb & kFlvMaskSoundFormat) >> 4);

  if ( *audio_format == FLV_FLAG_SOUND_FORMAT_AAC ) {
    uint8 aac_header[4] = {0,};
    if ( sizeof(aac_header) != ms.Peek(aac_header, sizeof(aac_header)) ) {
      return READ_NO_DATA;
    }
    *audio_is_aac_header = (aac_header[1] == 0x00);
  }
  return streaming::READ_OK;
}

bool FlvCoder::UpdateTimeInCuePoint(FlvTag::Metadata* metadata,
                                    int64 min_time_ms,
                                    int64 crt_time_ms,
                                    bool is_relative) {
  rtmp::CObject* time_obj = metadata->mutable_values()->Find("time");
  if ( time_obj != NULL &&
       time_obj->object_type() == rtmp::CObject::CORE_NUMBER ) {
    rtmp::CNumber* ctime = static_cast<rtmp::CNumber*>(time_obj);
    int64 time_cue_ms = static_cast<int64>(ctime->value() * 1000.0);
    if ( min_time_ms > time_cue_ms ) {
      return false;
    }
    if ( is_relative ) {
      ctime->set_value((time_cue_ms + crt_time_ms) / 1000.0);
    } else {
      ctime->set_value(crt_time_ms / 1000.0);
    }
    return true;
  }
  return false;
}

int32 FlvCoder::ExtractVideoCodecData(const FlvTag* flv_tag,
    uint8* buffer, int32 buffer_size) {
  if ( flv_tag->body().type() != streaming::FLV_FRAMETYPE_VIDEO ) {
    return -1;
  }
  const FlvTag::Video& video_body = flv_tag->video_body();
  io::MemoryStream video_data;
  video_data.AppendStreamNonDestructive(&video_body.data());

  // The easy stuff:
  int skip_size = -1;
  switch ( video_body.video_codec() ) {
    case streaming::FLV_FLAG_VIDEO_CODEC_H263:
      skip_size = 1;
      break;
    case streaming::FLV_FLAG_VIDEO_CODEC_VP6:
    case streaming::FLV_FLAG_VIDEO_CODEC_VP6_WITH_ALPHA:
      skip_size = 2;
      break;
    case streaming::FLV_FLAG_VIDEO_CODEC_AVC:
      skip_size = 5;
      break;
    case streaming::FLV_FLAG_VIDEO_CODEC_JPEG:
    case streaming::FLV_FLAG_VIDEO_CODEC_SCREEN:
    case streaming::FLV_FLAG_VIDEO_CODEC_SCREEN_VIDEO_V2:
      break;
  }
  if ( skip_size == -1 ) {
      LOG_ERROR << "Cannot ExtractVideoCodecData, unknown codec: "
                << FlvFlagVideoCodecName(video_body.video_codec())
                << " For: " << flv_tag->ToString();
      return -1;
  }
  if ( video_data.Size() < skip_size ) {
    return -1;
  }
  video_data.Skip(skip_size);

  //////////////////////////////////////////////////////////////////////
  //
  // Easy path, VP6 or H263
  //   -- just read the data after what we skip
  //
  if ( video_body.video_codec() != streaming::FLV_FLAG_VIDEO_CODEC_AVC ) {
    const int32 len = video_data.Size();
    if ( len > buffer_size ) return 0;
    video_data.Read(buffer, len);
    return len;
  }

  //////////////////////////////////////////////////////////////////////
  //
  // Semi-Hard path H264 - non AVC header :)
  //  - read NALU after NALU and write them w/ AVC header (0x00000001)
  //
  io::MemoryStream tmp;
  if ( video_body.video_avc_packet_type() != AVC_SEQUENCE_HEADER ) {
    while ( video_data.Size() > sizeof(uint32) ) {
      const uint32 crt_size = io::NumStreamer::ReadUInt32(
          &video_data, common::BIGENDIAN);
      if ( video_data.Size() >= crt_size ) {
        io::NumStreamer::WriteUInt32(&tmp, 0x00000001, common::BIGENDIAN);
        tmp.AppendStream(&video_data, crt_size);
      } else {
        return -1;
      }
    }
    const int32 len = tmp.Size();
    if ( len > buffer_size ) return 0;
    tmp.Read(buffer, len);
    return len;
  }

  //////////////////////////////////////////////////////////////////////
  //
  // Hard path - H264 AVC header
  //  - read from header the SPS and PPS-s and convert them to AVC:
  //     0x00000001 <32bit sps size> <sps> <32bit pps0 size> <pps0> ...
  //              ... <32bitppsN size> <ppsN>
  //
  io::NumStreamer::WriteUInt32(&tmp, 0x00000001, common::BIGENDIAN);
  if ( video_data.Size() < 8 ) {
    return -1;
  }
  video_data.Skip(6);   // some header..
  const int32 sps_size = io::NumStreamer::ReadUInt16(&video_data,
                                                     common::BIGENDIAN);
  io::NumStreamer::WriteInt32(&tmp, sps_size, common::BIGENDIAN);
  if ( video_data.Size() < sps_size ) {
    return -1;
  }
  tmp.AppendStream(&video_data, sps_size);
  if ( video_data.Size() < 1 ) {
    return -1;
  }
  int num_pps = io::NumStreamer::ReadByte(&video_data);
  while ( num_pps > 0 ) {
    if ( video_data.Size() < 2 ) {
      return -1;
    }
    const int32 pps_size = io::NumStreamer::ReadUInt16(&video_data,
                                                       common::BIGENDIAN);
    io::NumStreamer::WriteInt32(&tmp, pps_size, common::BIGENDIAN);
    if ( video_data.Size() < pps_size ) {
      return -1;
    }
    tmp.AppendStream(&video_data, pps_size);
    --num_pps;
  }
  const int32 len = tmp.Size();
  if ( len > buffer_size ) return 0;
  tmp.Read(buffer, len);
  return len;
}

//////////////////////////////////////////////////////////////////////
}
