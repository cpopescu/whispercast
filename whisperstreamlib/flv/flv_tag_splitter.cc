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

#include <math.h>

#include <whisperstreamlib/base/media_info_util.h>

#include <whisperstreamlib/flv/flv_tag_splitter.h>
#include <whisperstreamlib/flv/flv_tag.h>
#include <whisperstreamlib/flv/flv_coder.h>

#include <whisperstreamlib/rtmp/objects/rtmp_objects.h>
#include <whisperstreamlib/rtmp/objects/amf/amf_util.h>

//////////////////////////////////////////////////////////////////////

namespace streaming {

FlvTagSplitter::FlvTagSplitter(const string& name)
    : TagSplitter(MFORMAT_FLV, name),
      has_audio_(false),
      has_video_(false),
      first_audio_(),
      first_video_(),
      first_metadata_() {
}
FlvTagSplitter::~FlvTagSplitter() {
}


TagReadStatus FlvTagSplitter::GetNextTagInternal(io::MemoryStream* in,
                                                 scoped_ref<Tag>* tag,
                                                 int64* timestamp_ms,
                                                 bool is_eos) {
  *tag = NULL;
  if ( !tags_to_send_next_.empty() && media_info_extracted_ ) {
    tag->reset(tags_to_send_next_.front().tag_.get());
    *timestamp_ms = tags_to_send_next_.front().timestamp_ms_;
    tags_to_send_next_.pop_front();

    VLOG(10) << "Next tag: " << (*tag)->ToString();
    return READ_OK;
  }

  if ( in->Size() < sizeof(uint32) ) {
    return is_eos ? READ_EOF : READ_NO_DATA;
  }

  ///////////////////////////////////////////////////
  // Read FLV header, if any
  const uint32 prev_tag_size = io::NumStreamer::PeekInt32(in,common::BIGENDIAN);
  if ( (prev_tag_size & kFlvHeaderMask) == kFlvHeaderMark ) {
    int cb = 0;
    scoped_ref<FlvHeader> header;
    TagReadStatus err = FlvCoder::DecodeHeader(in, &header, &cb);
    if ( err != READ_OK ) {
      LOG_ERROR << "Failed to decode header: " << TagReadStatusName(err);
      return err;
    }
    CHECK_NOT_NULL(header.get());
    VLOG(10) << "Decoded FLV header: " << header->ToString();

    has_audio_ = header->has_audio();
    has_video_ = header->has_video();
    if ( !generic_tags_ ) {
      *tag = header.get();
      *timestamp_ms = 0;
      return READ_OK;
    }
    return READ_SKIP;
  }

  ///////////////////////////////////////////////////
  // Read FLV tag
  int cb = 0;
  scoped_ref<FlvTag> flv_tag;
  TagReadStatus result = FlvCoder::DecodeTag(in, &flv_tag, &cb);
  if ( result != READ_OK ) {
    if ( result != READ_NO_DATA ) {
      LOG_ERROR << "Failed to decode tag: " << TagReadStatusName(result);
    }
    if ( result == READ_NO_DATA && is_eos ) {
      return READ_EOF;
    }
    return result;
  }
  CHECK_NOT_NULL(flv_tag.get());

  if ( !generic_tags_ ) {
    *tag = flv_tag.get();
    *timestamp_ms = flv_tag->timestamp_ms();
    return READ_OK;
  }

  ////////////////////////////////////////////////////////////////
  // extract Cue Points
  if ( flv_tag->body().type() == FLV_FRAMETYPE_METADATA &&
       flv_tag->mutable_metadata_body().name().value() == kOnMetaData ) {
    scoped_ref<CuePointTag> cue_points =
        RetrieveCuePoints(flv_tag->metadata_body(), flv_tag->timestamp_ms());

    if ( cue_points.get() != NULL ) {
      tags_to_send_next_.push_back(
          TagToSend(cue_points.get(), flv_tag->timestamp_ms()));
    }
  }

  if ( !media_info_extracted_ &&
        first_metadata_.get() == NULL &&
        (flv_tag->body().type() == FLV_FRAMETYPE_AUDIO ||
         flv_tag->body().type() == FLV_FRAMETYPE_VIDEO) ) {
    LOG_ERROR << "No metadata before the first media tag"
                 ", failed to extract media info";
    media_info_extracted_ = true;
  }

  ///////////////////////////////////////////////////
  // Update media_info...
  if ( !media_info_extracted_ ) {
    if ( first_audio_.get() == NULL &&
         flv_tag->body().type() == FLV_FRAMETYPE_AUDIO ) {
      first_audio_ = flv_tag;
    }
    if ( first_video_.get() == NULL &&
         flv_tag->body().type() == FLV_FRAMETYPE_VIDEO ) {
      first_video_ = flv_tag;
    }
    if ( first_metadata_.get() == NULL &&
         flv_tag->body().type() == FLV_FRAMETYPE_METADATA ) {
      first_metadata_ = flv_tag;
    }
    if ( first_metadata_.get() != NULL &&
         (!has_audio_ || first_audio_.get() != NULL) &&
         (!has_video_ || first_video_.get() != NULL ) ) {
      if ( !util::ExtractMediaInfoFromFlv(*first_metadata_.get(),
                first_audio_.get(), first_video_.get(), &media_info_) ) {
        LOG_ERROR << "ExtractMediaInfoFromFlv failed";
        return READ_CORRUPTED;
      }

      media_info_extracted_ = true;

      // mask the metadata (replaced by MediaInfo)
      if ( !generic_tags_ || first_metadata_.get() != flv_tag.get() ) {
        tags_to_send_next_.push_back(
            TagToSend(flv_tag.get(), flv_tag->timestamp_ms()));
      }

      *tag = new MediaInfoTag(0, kDefaultFlavourMask, media_info_);
      *timestamp_ms = 0;

      return READ_OK;
    }
    if ( tags_to_send_next_.size() > kMediaInfoMaxWait ) {
      LOG_ERROR << "Failed to extract media info, in the first "
                << kMediaInfoMaxWait << " tags"
                << ", has_audio_: " << strutil::BoolToString(has_audio_)
                << ", has_video_: " << strutil::BoolToString(has_video_)
                << ", first_audio_: " << first_audio_.ToString()
                << ", first_video_: " << first_video_.ToString()
                << ", first_metadata_: " << first_metadata_.ToString();
      media_info_extracted_ = true;
    }
  }

  // mask the metadata (replaced by MediaInfo)
  if ( generic_tags_ &&
       flv_tag->body().type() == FLV_FRAMETYPE_METADATA &&
       flv_tag->metadata_body().name().value() == kOnMetaData ) {
    return READ_SKIP;
  }

  if ( !media_info_extracted_ ) {
    // still waiting => buffer this tag
    tags_to_send_next_.push_back(
      TagToSend(flv_tag.get(), flv_tag->timestamp_ms()));
    return READ_SKIP;
  }

  *timestamp_ms = flv_tag->timestamp_ms();
  *tag = flv_tag.get();

  VLOG(10) << "Next tag: " << (*tag)->ToString();
  return READ_OK;
}

scoped_ref<CuePointTag> FlvTagSplitter::RetrieveCuePoints(
    const FlvTag::Metadata& metadata, int64 timestamp) {
  if ( metadata.name().value() != kOnMetaData ) {
    return NULL;
  }

  const rtmp::CObject* cues_obj = metadata.values().Find("cuePoints");
  if ( cues_obj == NULL ||
       cues_obj->object_type() != rtmp::CObject::CORE_MIXED_MAP ) {
    return NULL;
  }
  const rtmp::CMixedMap* cues = static_cast<const rtmp::CMixedMap*>(cues_obj);

  scoped_ref<CuePointTag> cue_points =
      new CuePointTag(0, kDefaultFlavourMask);

  for ( rtmp::CMixedMap::Map::const_iterator it = cues->data().begin();
        it != cues->data().end(); ++it ) {
    if ( it->second->object_type() != rtmp::CObject::CORE_MIXED_MAP ) {
      continue;
    }
    const rtmp::CMixedMap* const crt_cue =
        static_cast<const rtmp::CMixedMap*>(it->second);

    const rtmp::CObject* time_obj = crt_cue->Find("time");
    if ( time_obj != NULL &&
         time_obj->object_type() == rtmp::CObject::CORE_NUMBER ) {
      const double time_double =
          static_cast<const rtmp::CNumber*>(time_obj)->value();
      // NOTE: without floor we get some nastyness
      const int64 time_ms = static_cast<int64>(::floor(time_double * 1000.0));

      const rtmp::CObject* params_obj = crt_cue->Find("parameters");
      if ( params_obj != NULL &&
           params_obj->object_type() == rtmp::CObject::CORE_MIXED_MAP ) {
        const rtmp::CMixedMap* params =
            static_cast<const rtmp::CMixedMap*>(params_obj);

        const rtmp::CObject* pos_obj = params->Find("pos");
        if ( pos_obj != NULL &&
             pos_obj->object_type() == rtmp::CObject::CORE_NUMBER ) {
          // NOTE: without floor we get some nastyness
          const int64 pos = static_cast<int64>(
              ::floor(static_cast<const rtmp::CNumber*>(pos_obj)->value()));
          cue_points->mutable_cue_points().push_back(make_pair(time_ms, pos));
        }
      }
    }
  }
  ::sort(cue_points->mutable_cue_points().begin(),
         cue_points->mutable_cue_points().end());
  return cue_points;
}
}
