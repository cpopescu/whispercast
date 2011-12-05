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

synch::MutexPool FlvTag::mutex_pool_(FlvTag::kNumMutexes);

void FlvTag::Audio::append_data(const void* data, uint32 size) {
  data_.Write(data, size);
  FlvCoder::DecodeAudioFlags(data_, &audio_type_, &audio_format_,
        &audio_rate_, &audio_size_, &audio_is_aac_header_);
}
void FlvTag::Audio::append_data(const io::MemoryStream& data) {
  data_.AppendStreamNonDestructive(&data);
  FlvCoder::DecodeAudioFlags(data_, &audio_type_, &audio_format_,
        &audio_rate_, &audio_size_, &audio_is_aac_header_);
}
TagReadStatus FlvTag::Audio::Decode(io::MemoryStream& in, uint32 size) {
  if ( in.Size() < size )  {
    return READ_NO_DATA;
  }
  data_.Clear();
  data_.AppendStream(&in, size);
  return FlvCoder::DecodeAudioFlags(data_, &audio_type_, &audio_format_,
      &audio_rate_, &audio_size_, &audio_is_aac_header_);
}
void FlvTag::Audio::Encode(io::MemoryStream* out) const {
  out->AppendStreamNonDestructive(&data_);
}
string FlvTag::Audio::ToString() const {
  return strutil::StringPrintf("Audio{data_: %d bytes, audio_type_: %s, "
      "audio_format_: %s, audio_rate_: %s, audio_size_: %s, "
      "audio_is_aac_header_: %s}",
      data_.Size(),
      FlvFlagSoundTypeName(audio_type_),
      FlvFlagSoundFormatName(audio_format_),
      FlvFlagSoundRateName(audio_rate_),
      FlvFlagSoundSizeName(audio_size_),
      strutil::BoolToString(audio_is_aac_header_).c_str());
}

void FlvTag::Video::append_data(const void* data, uint32 size) {
  data_.Write(data, size);
  FlvCoder::DecodeVideoFlags(data_, &video_codec_, &video_frame_type_,
      &video_avc_packet_type_, &video_avc_composition_time_);
}
void FlvTag::Video::append_data(const io::MemoryStream& data) {
  data_.AppendStreamNonDestructive(&data);
  FlvCoder::DecodeVideoFlags(data_, &video_codec_, &video_frame_type_,
      &video_avc_packet_type_, &video_avc_composition_time_);
}
TagReadStatus FlvTag::Video::Decode(io::MemoryStream& in, uint32 size) {
  if ( in.Size() < size )  {
    return READ_NO_DATA;
  }
  data_.Clear();
  data_.AppendStream(&in, size);
  return FlvCoder::DecodeVideoFlags(data_, &video_codec_,
      &video_frame_type_, &video_avc_packet_type_,
      &video_avc_composition_time_);
}
void FlvTag::Video::Encode(io::MemoryStream* out) const {
  out->AppendStreamNonDestructive(&data_);
}
string FlvTag::Video::ToString() const {
  string more_info;
  if ( video_codec_ == FLV_FLAG_VIDEO_CODEC_AVC ) {
    more_info += strutil::StringPrintf(", video_avc_packet_type_: %s, "
        "video_avc_composition_time_: %d, video_avc_moov_: %s",
        AvcPacketTypeName(video_avc_packet_type_),
        video_avc_composition_time_,
        video_avc_moov_.ToString().c_str());
  }
  return strutil::StringPrintf("Video{data_: %d bytes, video_codec_: %s,"
      "video_frame_type_: %s%s}",
      data_.Size(),
      FlvFlagVideoCodecName(video_codec_),
      FlvFlagVideoFrameTypeName(video_frame_type_),
      more_info.c_str());
}

TagReadStatus FlvTag::Metadata::Decode(io::MemoryStream& ms, uint32 size) {
  // use a temporary memory stream, to prevent reading over 'size' boundary
  uint32 initial_size = ms.Size();
  ms.MarkerSet();

  rtmp::AmfUtil::ReadStatus status = name_.ReadFromMemoryStream(
      &ms, rtmp::AmfUtil::AMF0_VERSION);
  if ( status != rtmp::AmfUtil::READ_OK ) {
    ms.MarkerRestore();
    LOG_ERROR << "Failed to decode Metadata name, status: " <<
                 rtmp::AmfUtil::ReadStatusName(status)
              << " input stream: " << ms.DumpContent();
    return READ_CORRUPTED_FAIL;
  }
  status = values_.ReadFromMemoryStream(
      &ms, rtmp::AmfUtil::AMF0_VERSION);
  if ( status != rtmp::AmfUtil::READ_OK ) {
    ms.MarkerRestore();
    LOG_ERROR << "Failed to decode Metadata value, status: " <<
                 rtmp::AmfUtil::ReadStatusName(status)
              << " input stream: " << ms.DumpContent();
    return READ_CORRUPTED_FAIL;;
  }
  ms.MarkerClear();

  if ( initial_size - ms.Size() != size ) {
    LOG_ERROR << "Incorrect Metadata size, advertised: " << size
              << ", real: " << (initial_size - ms.Size());
  }

  return READ_OK;
}
void FlvTag::Metadata::Encode(io::MemoryStream* out) const {
  name_.WriteToMemoryStream(out, rtmp::AmfUtil::AMF0_VERSION);
  values_.WriteToMemoryStream(out, rtmp::AmfUtil::AMF0_VERSION);
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



FlvTagSerializer::FlvTagSerializer(bool write_header,
                                   bool has_video, bool has_audio)
  : streaming::TagSerializer(),
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
                                         int64 base_timestamp_ms,
                                         io::MemoryStream* out) {
  if ( tag->type() == Tag::TYPE_FLV ) {
    const FlvTag* flv_tag = static_cast<const FlvTag*>(tag);
    SerializeFlvTag(flv_tag, base_timestamp_ms, out);
    return true;
  }
  return false;
}

void FlvTagSerializer::SerializeFlvTag(const FlvTag* flv_tag,
                                       int64 base_timestamp_ms,
                                       io::MemoryStream* out) {
  int32 timestamp_ms = 0;
  last_timestamp_ms_ = base_timestamp_ms + flv_tag->timestamp_ms();
  timestamp_ms = last_timestamp_ms_;

  io::NumStreamer::WriteInt32(out, previous_tag_size_, common::BIGENDIAN);
  const int32 initial_size = out->Size();
  io::NumStreamer::WriteByte(out, flv_tag->body().type());
  io::NumStreamer::WriteUInt24(out, flv_tag->body().Size(), common::BIGENDIAN);
  io::NumStreamer::WriteUInt24(out, timestamp_ms & 0xFFFFFF,  // 24 bit mask
                              common::BIGENDIAN);
  io::NumStreamer::WriteByte(out, (timestamp_ms >> 24) & 0xFF);
  io::NumStreamer::WriteUInt24(out, flv_tag->stream_id(), common::BIGENDIAN);
  flv_tag->body().Encode(out);
  previous_tag_size_ = out->Size() - initial_size;
}

uint32 FlvTagSerializer::EncodingSize(const FlvTag* tag) {
  return 4 +  // int32:  previous tag size
         1 +  // byte:   frame type
         3 +  // uint24: body size
         3 +  // uint24: timestamp
         1 +  // byte:   timestamp extension
         3 +  // uint24: stream id
         tag->body().Size();
}

void FlvTagSerializer::Finalize(io::MemoryStream* out) {
  // Nothing here :)
}
}
