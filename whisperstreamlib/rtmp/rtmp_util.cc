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

#include <whisperstreamlib/rtmp/rtmp_util.h>
#include <whisperstreamlib/rtmp/events/rtmp_event_notify.h>

namespace rtmp {

void ExtractFlvTags(const rtmp::Event& event, int64 timestamp_ms,
    vector<scoped_ref<streaming::FlvTag> >* out) {
  if ( event.event_type() == rtmp::EVENT_NOTIFY ) {
    const rtmp::EventNotify& notify =
        static_cast<const rtmp::EventNotify&>(event);
    if ( notify.name().value() != "@setDataFrame" ||
         notify.values().size() != 2 ||
         notify.values()[0]->object_type() != CObject::CORE_STRING ||
         notify.values()[1]->object_type() != CObject::CORE_STRING_MAP ) {
      return;
    }
    const rtmp::CStringMap* str_map =
        static_cast<const rtmp::CStringMap*>(notify.values()[1]);

    scoped_ref<streaming::FlvTag> tag = new streaming::FlvTag(
        0, streaming::kDefaultFlavourMask,
        timestamp_ms, streaming::FLV_FRAMETYPE_METADATA);
    tag->mutable_metadata_body().mutable_name()->set_value(
        streaming::kOnMetaData);
    tag->mutable_metadata_body().mutable_values()->SetAll(*str_map);
    out->push_back(tag);
    return;
  }
  if ( event.event_type() == rtmp::EVENT_VIDEO_DATA ) {
    const rtmp::BulkDataEvent& bd =
        static_cast<const rtmp::BulkDataEvent&>(event);
    if ( bd.data().IsEmpty() ) {
      // An empty EVENT_VIDEO_DATA, nothing to decode
      return;
    }
    scoped_ref<streaming::FlvTag> tag = new streaming::FlvTag(
        0, streaming::kDefaultFlavourMask,
        timestamp_ms, streaming::FLV_FRAMETYPE_VIDEO);
    io::MemoryStream& bd_data = const_cast<io::MemoryStream&>(bd.data());
    bd_data.MarkerSet();
    streaming::TagReadStatus result = tag->mutable_body().Decode(
        bd_data, bd_data.Size());
    bd_data.MarkerRestore();
    if ( result != streaming::READ_OK ) {
      LOG_ERROR << "Failed to decode Video tag, result: "
                << streaming::TagReadStatusName(result);
      return;
    }
    out->push_back(tag);
    return;
  }
  if ( event.event_type() == rtmp::EVENT_AUDIO_DATA ) {
    const rtmp::BulkDataEvent& bd =
        static_cast<const rtmp::BulkDataEvent&>(event);
    if ( bd.data().IsEmpty() ) {
      // An empty EVENT_AUDIO_DATA, nothing to decode
      return;
    }
    scoped_ref<streaming::FlvTag> tag = new streaming::FlvTag(
        0, streaming::kDefaultFlavourMask,
        timestamp_ms, streaming::FLV_FRAMETYPE_AUDIO);
    io::MemoryStream& bd_data = const_cast<io::MemoryStream&>(bd.data());
    bd_data.MarkerSet();
    streaming::TagReadStatus result = tag->mutable_body().Decode(
        bd_data, bd_data.Size());
    bd_data.MarkerRestore();
    if ( result != streaming::READ_OK ) {
      LOG_ERROR << "Failed to decode Audio tag, result: "
                << streaming::TagReadStatusName(result);
      return;
    }
    out->push_back(tag);
    return;
  }
  if ( event.event_type() == rtmp::EVENT_MEDIA_DATA ) {
    const rtmp::BulkDataEvent& bd  =
        static_cast<const rtmp::BulkDataEvent&>(event);
    if ( bd.data().IsEmpty() ) {
      // An empty EVENT_MEDIA_DATA, nothing to decode
      return;
    }
    io::MemoryStream& bd_data = const_cast<io::MemoryStream&>(bd.data());
    bd_data.MarkerSet();
    while ( bd_data.Size() > 4 ) {
      int8 type = io::NumStreamer::ReadByte(&bd_data);
      int32 size = io::NumStreamer::ReadUInt24(&bd_data, common::BIGENDIAN);
      int32 timestamp_delta_low =
          io::NumStreamer::ReadUInt24(&bd_data, common::BIGENDIAN);
      int32 timestamp_delta_hi = io::NumStreamer::ReadByte(&bd_data);
      int32 timestamp = ((timestamp_delta_low & 0xFFFFFF) |
                         ((timestamp_delta_hi & 0xFF) << 24));
      int32 stream_id =
          io::NumStreamer::ReadUInt24(&bd_data, common::BIGENDIAN);

      if ( type != streaming::FLV_FRAMETYPE_AUDIO &&
           type != streaming::FLV_FRAMETYPE_VIDEO &&
           type != streaming::FLV_FRAMETYPE_METADATA ) {
        LOG_ERROR << "Invalid tag type: " << strutil::StringPrintf("%02x", type)
                  << " in event: " << event.ToString();
        break;
      }
      streaming::FlvFrameType frame_type =
          static_cast<streaming::FlvFrameType>(type);

      scoped_ref<streaming::FlvTag> tag = new streaming::FlvTag(
          0, streaming::kDefaultFlavourMask,
          timestamp_ms + timestamp, frame_type);
      tag->set_stream_id(stream_id);

      streaming::TagReadStatus result = tag->mutable_body().Decode(
          bd_data, size);
      if ( result != streaming::READ_OK ) {
        LOG_ERROR << "Failed to decode " << FlvFrameTypeName(frame_type)
                  << " tag, result: " << streaming::TagReadStatusName(result);
        break;
      }
      // skip unknown 4 bytes
      bd_data.Skip(4);

      out->push_back(tag);
    }
    bd_data.MarkerRestore();
  }
}
}
