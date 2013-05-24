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
    tag->LearnAttributes();
    out->push_back(tag);
    return;
  }
  if ( event.event_type() == rtmp::EVENT_VIDEO_DATA ) {
    scoped_ref<streaming::FlvTag> tag;
    static_cast<const rtmp::EventVideoData&>(event).DecodeTag(
        timestamp_ms, &tag);
    if ( tag.get() != NULL ) {
      out->push_back(tag);
    }
    return;
  }
  if ( event.event_type() == rtmp::EVENT_AUDIO_DATA ) {
    scoped_ref<streaming::FlvTag> tag;
    static_cast<const rtmp::EventAudioData&>(event).DecodeTag(
        timestamp_ms, &tag);
    if ( tag.get() != NULL ) {
      out->push_back(tag);
    }
    return;
  }
  if ( event.event_type() == rtmp::EVENT_MEDIA_DATA ) {
    static_cast<const rtmp::MediaDataEvent&>(event).DecodeTags(
        timestamp_ms, out);
    return;
  }
}
}
