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

#include <whisperstreamlib/base/media_info_util.h>
#include <whisperstreamlib/flv/flv_tag.h>
#include <whisperstreamlib/flv/flv_coder.h>

namespace streaming {

const Tag::Type FlvHeader::kType = Tag::TYPE_FLV_HEADER;
const Tag::Type FlvTag::kType = Tag::TYPE_FLV;
const FlvFrameType FlvTag::Audio::kType = FLV_FRAMETYPE_AUDIO;
const FlvFrameType FlvTag::Video::kType = FLV_FRAMETYPE_VIDEO;
const FlvFrameType FlvTag::Metadata::kType = FLV_FRAMETYPE_METADATA;

void FlvHeader::Write(io::MemoryStream* out) const {
  io::NumStreamer::WriteUInt24(out, unknown(), common::BIGENDIAN);
  io::NumStreamer::WriteByte(out, version());
  io::NumStreamer::WriteByte(out, audio_video_flags());
  io::NumStreamer::WriteInt32(out, data_offset(), common::BIGENDIAN);
}
void FlvHeader::AppendStandardHeader(io::MemoryStream* out,
                                     bool has_video,
                                     bool has_audio) {
  scoped_ref<FlvHeader> h(FlvHeader::StandardHeader(has_video, has_audio));
  h->Write(out);
}

void FlvTag::LearnAttributes() {
  switch ( body().type() ) {
    case FLV_FRAMETYPE_VIDEO: {
      const FlvTag::Video& video_body = this->video_body();
      add_attributes(Tag::ATTR_VIDEO);
      if ( video_body.frame_type() == FLV_FLAG_VIDEO_FRAMETYPE_KEYFRAME ) {
        add_attributes(Tag::ATTR_CAN_RESYNC);
      }

      if ( video_body.codec() == FLV_FLAG_VIDEO_CODEC_AVC ) {
        mutable_video_body().set_avc_moov(
            FlvCoder::DecodeAuxiliaryMoovTag(this));

        if ( video_body.avc_packet_type() != AVC_SEQUENCE_HEADER ) {
          add_attributes(Tag::ATTR_DROPPABLE);
        }
      } else {
        add_attributes(Tag::ATTR_DROPPABLE);
      }
      break;
    }
    case FLV_FRAMETYPE_AUDIO: {
      const FlvTag::Audio& audio_body = this->audio_body();
      add_attributes(Tag::ATTR_AUDIO |
                              Tag::ATTR_CAN_RESYNC);
      if ( audio_body.format() != FLV_FLAG_SOUND_FORMAT_AAC ||
           !audio_body.is_aac_header() ) {
        add_attributes(Tag::ATTR_DROPPABLE);
      }
      break;
    }
    case FLV_FRAMETYPE_METADATA: {
      add_attributes(Tag::ATTR_METADATA);
      break;
    }
  }
}

TagReadStatus FlvTag::Audio::Decode(io::MemoryStream& in, uint32 size) {
  if ( in.Size() < size )  {
    return READ_NO_DATA;
  }
  data_.Clear();
  data_.AppendStream(&in, size);
  return FlvCoder::DecodeAudioFlags(data_, &type_, &format_,
      &rate_, &size_, &is_aac_header_);
}
void FlvTag::Audio::Encode(io::MemoryStream* out) const {
  out->AppendStreamNonDestructive(&data_);
}
string FlvTag::Audio::ToString() const {
  return strutil::StringPrintf("Audio{data_: %d bytes, type_: %s, "
      "format_: %s, rate_: %s, size_: %s, is_aac_header_: %s}",
      data_.Size(),
      FlvFlagSoundTypeName(type_),
      FlvFlagSoundFormatName(format_),
      FlvFlagSoundRateName(rate_),
      FlvFlagSoundSizeName(size_),
      strutil::BoolToString(is_aac_header_).c_str());
}

TagReadStatus FlvTag::Video::Decode(io::MemoryStream& in, uint32 size) {
  if ( in.Size() < size )  {
    return READ_NO_DATA;
  }
  data_.Clear();
  data_.AppendStream(&in, size);
  return FlvCoder::DecodeVideoFlags(data_, &codec_,
      &frame_type_, &avc_packet_type_,
      &avc_composition_offset_ms_);
}
void FlvTag::Video::Encode(io::MemoryStream* out) const {
  out->AppendStreamNonDestructive(&data_);
}
string FlvTag::Video::ToString() const {
  string more_info;
  if ( codec_ == FLV_FLAG_VIDEO_CODEC_AVC ) {
    more_info += strutil::StringPrintf(", avc_packet_type_: %s, "
        "avc_composition_offset_ms_: %d, avc_moov_: %s",
        AvcPacketTypeName(avc_packet_type_),
        avc_composition_offset_ms_,
        avc_moov_.ToString().c_str());
  }
  return strutil::StringPrintf("Video{data_: %d bytes, codec_: %s,"
      "frame_type_: %s%s}",
      data_.Size(),
      FlvFlagVideoCodecName(codec_),
      FlvFlagVideoFrameTypeName(frame_type_),
      more_info.c_str());
}

TagReadStatus FlvTag::Metadata::Decode(io::MemoryStream& ms, uint32 size) {
  // use a temporary memory stream, to prevent reading over 'size' boundary
  uint32 initial_size = ms.Size();
  ms.MarkerSet();

  rtmp::AmfUtil::ReadStatus status = name_.Decode(
      &ms, rtmp::AmfUtil::AMF0_VERSION);
  if ( status != rtmp::AmfUtil::READ_OK ) {
    ms.MarkerRestore();
    LOG_ERROR << "Failed to decode Metadata name, status: " <<
                 rtmp::AmfUtil::ReadStatusName(status)
              << " input stream: " << ms.DumpContent();
    return READ_CORRUPTED;
  }
  status = values_.Decode(
      &ms, rtmp::AmfUtil::AMF0_VERSION);
  if ( status != rtmp::AmfUtil::READ_OK ) {
    ms.MarkerRestore();
    LOG_ERROR << "Failed to decode Metadata value, status: " <<
                 rtmp::AmfUtil::ReadStatusName(status)
              << " input stream: " << ms.DumpContent();
    return READ_CORRUPTED;;
  }
  ms.MarkerClear();

  if ( initial_size - ms.Size() != size ) {
    LOG_ERROR << "Incorrect Metadata size, advertised: " << size
              << ", real: " << (initial_size - ms.Size());
  }

  return READ_OK;
}
void FlvTag::Metadata::Encode(io::MemoryStream* out) const {
  name_.Encode(out, rtmp::AmfUtil::AMF0_VERSION);
  values_.Encode(out, rtmp::AmfUtil::AMF0_VERSION);
}
uint32 FlvTag::Metadata::Size() const {
  return name_.EncodingSize(rtmp::AmfUtil::AMF0_VERSION) +
         values_.EncodingSize(rtmp::AmfUtil::AMF0_VERSION);
}
string FlvTag::Metadata::ToString() const {
  return strutil::StringPrintf("Metadata{name_: [%s], values_: %s}",
                                name_.value().c_str(),
                                values_.ToString().c_str());
}

//////////////////////////////////////////////////////////////////////

FlvTagSerializer::FlvTagSerializer(bool write_header,
                                   bool has_video, bool has_audio)
  : TagSerializer(MFORMAT_FLV),
    write_header_(write_header),
    has_video_(has_video),
    has_audio_(has_audio),
    previous_tag_size_(0),
    last_timestamp_ms_(0) {
}

FlvTagSerializer::~FlvTagSerializer() {
}

void FlvTagSerializer::Initialize(io::MemoryStream* out) {
  previous_tag_size_ = 0;
  last_timestamp_ms_ = 0;
  if ( write_header_ ) {
    FlvHeader::AppendStandardHeader(out, has_video_, has_audio_);
  }
}
bool FlvTagSerializer::SerializeInternal(const streaming::Tag* tag,
                                         int64 timestamp_ms,
                                         io::MemoryStream* out) {
  CHECK_GE(timestamp_ms, 0) << "Illegal timestamp: " << timestamp_ms;
  if ( tag->type() == Tag::TYPE_MEDIA_INFO ) {
    const MediaInfoTag* inf = static_cast<const MediaInfoTag*>(tag);
    scoped_ref<FlvTag> meta;
    if ( !streaming::util::ComposeFlv(inf->info(), &meta) ) {
      LOG_ERROR << "Cannot compose Metadata from: " << tag->ToString();
      return false;
    }
    return SerializeInternal(meta.get(), timestamp_ms, out);
  }
  // Establish frame type
  FlvFrameType flv_frame_type;
  const FlvTag* flv_metadata_tag = NULL;
  if ( tag->is_audio_tag() ) {
    flv_frame_type = FLV_FRAMETYPE_AUDIO;
  } else if ( tag->is_video_tag() ) {
    flv_frame_type = FLV_FRAMETYPE_VIDEO;
  } else if ( tag->is_metadata_tag() ) {
    if ( tag->type() != Tag::TYPE_FLV ) {
      return true;
    }
    flv_frame_type = FLV_FRAMETYPE_METADATA;
    flv_metadata_tag = static_cast<const FlvTag*>(tag);
  } else {
    // probably a signaling tag
    LOG_WARNING << "Ignoring signaling tag: " << tag->ToString();
    return true;
  }

  // Establish frame size
  uint32 size = 0;
  if ( tag->is_audio_tag() || tag->is_video_tag() ) {
    CHECK_NOT_NULL(tag->Data());
    size = tag->Data()->Size();
  } else {
    CHECK_NOT_NULL(flv_metadata_tag);
    size = flv_metadata_tag->size();
  }

  io::NumStreamer::WriteInt32(out, previous_tag_size_, common::BIGENDIAN);
  const int32 initial_size = out->Size();
  io::NumStreamer::WriteByte(out, flv_frame_type);
  io::NumStreamer::WriteUInt24(out, size, common::BIGENDIAN);
  io::NumStreamer::WriteUInt24(out, timestamp_ms & 0xFFFFFF,  // 24 bit mask
                              common::BIGENDIAN);
  io::NumStreamer::WriteByte(out, (timestamp_ms >> 24) & 0xFF);
  io::NumStreamer::WriteUInt24(out, 0, common::BIGENDIAN);
  if ( tag->is_audio_tag() || tag->is_video_tag() ) {
    CHECK_NOT_NULL(tag->Data());
    out->AppendStreamNonDestructive(tag->Data());
  } else {
    CHECK_NOT_NULL(flv_metadata_tag);
    flv_metadata_tag->body().Encode(out);
  }
  previous_tag_size_ = out->Size() - initial_size;
  return true;
}

void FlvTagSerializer::Finalize(io::MemoryStream* out) {
  // nothing to do. A flv file abruptly ends.
}

uint32 FlvTagSerializer::EncodingSize(const FlvTag* tag) {
  return 4 +  // int32:  previous tag size
         1 +   // byte:   frame type
         3 +   // uint24: body size
         3 +   // uint24: timestamp
         1 +   // byte:   timestamp extension
         3 +   // uint24: stream id
         tag->body().Size();
}

}
