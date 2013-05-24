// Copyright (c) 2012, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Cosmin Tudorache & Catalin Popescu

#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperlib/common/io/num_streaming.h>
#include <whisperstreamlib/rtmp/events/rtmp_event.h>

namespace rtmp {

void EventVideoData::DecodeTag(int64 timestamp_ms,
                               scoped_ref<streaming::FlvTag>* out) const {
  if ( data_.IsEmpty() ) {
    // An empty EVENT_VIDEO_DATA, nothing to decode
    return;
  }
  scoped_ref<streaming::FlvTag> tag = new streaming::FlvTag(
      0, streaming::kDefaultFlavourMask,
      timestamp_ms, streaming::FLV_FRAMETYPE_VIDEO);
  io::MemoryStream& data = const_cast<io::MemoryStream&>(data_);
  data.MarkerSet();
  streaming::TagReadStatus result = tag->mutable_body().Decode(
      data, data.Size());
  data.MarkerRestore();
  if ( result != streaming::READ_OK ) {
    LOG_ERROR << "Failed to decode Video tag, result: "
              << streaming::TagReadStatusName(result);
    return;
  }
  tag->LearnAttributes();
  *out = tag;
}

void EventAudioData::DecodeTag(int64 timestamp_ms,
                               scoped_ref<streaming::FlvTag>* out) const {
  if ( data_.IsEmpty() ) {
    // An empty EVENT_AUDIO_DATA, nothing to decode
    return;
  }
  scoped_ref<streaming::FlvTag> tag = new streaming::FlvTag(
      0, streaming::kDefaultFlavourMask,
      timestamp_ms, streaming::FLV_FRAMETYPE_AUDIO);
  io::MemoryStream& data = const_cast<io::MemoryStream&>(data_);
  data.MarkerSet();
  streaming::TagReadStatus result = tag->mutable_body().Decode(
      data, data.Size());
  data.MarkerRestore();
  if ( result != streaming::READ_OK ) {
    LOG_ERROR << "Failed to decode Audio tag, result: "
              << streaming::TagReadStatusName(result);
    return;
  }
  tag->LearnAttributes();
  *out = tag;
}

void MediaDataEvent::EncodeAddTag(const streaming::FlvTag& tag,
                                  int64 timestamp_ms) {
  if ( first_tag_ts_ == -1 ) {
    first_tag_ts_ = timestamp_ms;
  }
  int64 ts = timestamp_ms - first_tag_ts_;

  if ( previous_tag_size_ > 0 ) {
    io::NumStreamer::WriteInt32(&data_, previous_tag_size_, common::BIGENDIAN);
  }
  const int32 original_size = data_.Size();
  io::NumStreamer::WriteByte(&data_, tag.body().type());
  io::NumStreamer::WriteUInt24(&data_, tag.body().Size(), common::BIGENDIAN);
  io::NumStreamer::WriteUInt24(&data_, ts & 0xFFFFFF, common::BIGENDIAN);
  io::NumStreamer::WriteByte(&data_, (ts >> 24) & 0xFF);
  io::NumStreamer::WriteUInt24(&data_, tag.stream_id(), common::BIGENDIAN);
  tag.body().Encode(&data_);

  duration_ms_ = timestamp_ms - first_tag_ts_;
  previous_tag_size_ = data_.Size() - original_size;
}

void MediaDataEvent::EncodeFinalize() {
  io::NumStreamer::WriteInt32(&data_, 0, common::BIGENDIAN);
}

void MediaDataEvent::DecodeTags(int64 timestamp_ms,
    vector<scoped_ref<streaming::FlvTag> >* out) const {
  if ( data_.IsEmpty() ) {
    // An empty EVENT_MEDIA_DATA, nothing to decode
    return;
  }
  io::MemoryStream& data = const_cast<io::MemoryStream&>(data_);
  data.MarkerSet();
  while ( data.Size() > 11 ) {
    int8 type = io::NumStreamer::ReadByte(&data);
    int32 size = io::NumStreamer::ReadUInt24(&data, common::BIGENDIAN);
    int32 timestamp_delta_low =
        io::NumStreamer::ReadUInt24(&data, common::BIGENDIAN);
    int32 timestamp_delta_hi = io::NumStreamer::ReadByte(&data);
    int32 timestamp = ((timestamp_delta_low & 0xFFFFFF) |
                       ((timestamp_delta_hi & 0xFF) << 24));
    int32 stream_id =
        io::NumStreamer::ReadUInt24(&data, common::BIGENDIAN);

    if ( type != streaming::FLV_FRAMETYPE_AUDIO &&
         type != streaming::FLV_FRAMETYPE_VIDEO &&
         type != streaming::FLV_FRAMETYPE_METADATA ) {
      LOG_ERROR << "Invalid tag type: " << strutil::StringPrintf("%02x", type)
                << " in event: " << ToString();
      break;
    }
    streaming::FlvFrameType frame_type =
        static_cast<streaming::FlvFrameType>(type);

    scoped_ref<streaming::FlvTag> tag = new streaming::FlvTag(
        0, streaming::kDefaultFlavourMask,
        timestamp_ms + timestamp, frame_type);
    tag->set_stream_id(stream_id);

    streaming::TagReadStatus result = tag->mutable_body().Decode(
        data, size);
    if ( result != streaming::READ_OK ) {
      LOG_ERROR << "Failed to decode " << FlvFrameTypeName(frame_type)
                << " tag, result: " << streaming::TagReadStatusName(result);
      break;
    }
    // skip previous tag size
    data.Skip(4);

    out->push_back(tag);
  }
  data.MarkerRestore();
}

}
