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

#define SPLIT_LOG(n)   LOG(n) << "[" << name_ << "]: "
#define SPLIT_DLOG(n) DLOG(n) << "[" << name_ << "]: "

#define SPLIT_LOG_DEBUG    SPLIT_LOG(LDEBUG)
#define SPLIT_LOG_INFO     SPLIT_LOG(LINFO)
#define SPLIT_LOG_WARNING  SPLIT_LOG(LWARNING)
#define SPLIT_LOG_ERROR    SPLIT_LOG(LERROR)
#define SPLIT_LOG_FATAL    SPLIT_LOG(LFATAL)

const TagSplitter::Type FlvTagSplitter::kType = TagSplitter::TS_FLV;

FlvTagSplitter::FlvTagSplitter(const string& name)
    : streaming::TagSplitter(kType, name),
      first_tag_timestamp_ms_(-1),
      tag_timestamp_ms_(0),
      tags_to_send_next_(),
      has_audio_(false),
      has_video_(false),
      first_audio_(),
      first_video_(),
      first_metadata_(),
      media_info_extracted_(false),
      bootstrapping_(true) {
}
FlvTagSplitter::~FlvTagSplitter() {
}


TagReadStatus FlvTagSplitter::GetNextTagInternal(io::MemoryStream* in,
                                                 scoped_ref<Tag>* tag,
                                                 bool is_eos) {
  *tag = NULL;
  if ( !tags_to_send_next_.empty() && !bootstrapping_ ) {
    *tag = tags_to_send_next_.front();
    tags_to_send_next_.pop_front();

    SPLIT_DLOG(10) << "Next tag: " << (*tag)->ToString();
    return READ_OK;
  }

  // We need to defend ourselves from the header-in stream position...
  if ( in->Size() < sizeof(uint32) ) {
    if ( is_eos && bootstrapping_ ) {
      EndBootstrapping();
      return READ_SKIP;
    }
    return READ_NO_DATA;
  }

  ///////////////////////////////////////////////////
  // Read FLV header, if any
  const uint32 prev_tag_size = io::NumStreamer::PeekInt32(in,common::BIGENDIAN);
  if ( (prev_tag_size & kFlvHeaderMask) == kFlvHeaderMark ) {
    int cb = 0;
    scoped_ref<FlvHeader> header;
    TagReadStatus err = FlvCoder::DecodeHeader(in, &header, &cb);
    if ( err != READ_OK ) {
      SPLIT_LOG_ERROR << "Failed to decode header: " << TagReadStatusName(err);
      return err;
    }
    CHECK_NOT_NULL(header.get());
    SPLIT_DLOG(10) << "Decoded FLV header: " << header->ToString();

    has_audio_ = header->has_audio();
    has_video_ = header->has_video();

    if ( bootstrapping_ ) {
      tags_to_send_next_.push_back(header.get());
      return READ_SKIP;
    }
    *tag = header.get();
    SPLIT_DLOG(10) << "Next tag: " << (*tag)->ToString();
    return READ_OK;
  }

  ///////////////////////////////////////////////////
  // Read FLV tag
  int cb = 0;

  scoped_ref<FlvTag> flv_tag;
  TagReadStatus result = FlvCoder::DecodeTag(in, &flv_tag, &cb);
  if ( result != READ_OK ) {
    if ( result != READ_NO_DATA ) {
      SPLIT_LOG_ERROR << "Failed to decode tag: " << TagReadStatusName(result);
    }
    if ( result == READ_NO_DATA && is_eos && bootstrapping_ ) {
      EndBootstrapping();
      return READ_SKIP;
    }
    return result;
  }
  CHECK_NOT_NULL(flv_tag.get());

  ///////////////////////////////////////////////////
  // Update the statistics..
  switch ( flv_tag->body().type() ) {
    case FLV_FRAMETYPE_VIDEO: {
      const FlvTag::Video& video_body = flv_tag->video_body();
      flv_tag->add_attributes(Tag::ATTR_VIDEO);
      if ( video_body.video_frame_type() ==
           FLV_FLAG_VIDEO_FRAMETYPE_KEYFRAME ) {
        flv_tag->add_attributes(Tag::ATTR_CAN_RESYNC);
      }

      if ( video_body.video_codec() == FLV_FLAG_VIDEO_CODEC_AVC ) {
        flv_tag->mutable_video_body().set_video_avc_moov(
            FlvCoder::DecodeAuxiliaryMoovTag(flv_tag.get()));

        if ( video_body.video_avc_packet_type() != AVC_SEQUENCE_HEADER ) {
          flv_tag->add_attributes(Tag::ATTR_DROPPABLE);
        }
      } else {
        flv_tag->add_attributes(Tag::ATTR_DROPPABLE);
      }
    }
    break;
    case FLV_FRAMETYPE_AUDIO: {
      const FlvTag::Audio& audio_body = flv_tag->audio_body();
      flv_tag->add_attributes(Tag::ATTR_AUDIO |
                              Tag::ATTR_CAN_RESYNC);
      if ( audio_body.audio_format() != FLV_FLAG_SOUND_FORMAT_AAC ||
           !audio_body.audio_is_aac_header() ) {
        flv_tag->add_attributes(Tag::ATTR_DROPPABLE);
      }
    }
    break;
    case FLV_FRAMETYPE_METADATA: {
      FlvTag::Metadata& metadata = flv_tag->mutable_metadata_body();
      if ( metadata.name().value() == kOnMetaData ) {
        // If RetrieveCuePoints succeeds it fills in cue_points_.
        //   if cue_points_ is extracted: send the cue_points_ now, and
        ///                               postpone sending the metadata.
        //   else: send the metadata tag now.
        scoped_ref<CuePointTag> cue_points =
            RetrieveCuePoints(metadata, flv_tag->timestamp_ms());
        // This will modify the 'flv_tag' according to limits/controller
        UpdateMetadata(metadata);

        if ( cue_points.get() != NULL ) {
          tags_to_send_next_.push_back(cue_points.get());
        }
      }
    }
    break;
  };

  if ( first_tag_timestamp_ms_ == -1 ) {
    first_tag_timestamp_ms_ = flv_tag->timestamp_ms();
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
      util::ExtractMediaInfoFromFlv(*first_metadata_.get(),
          first_audio_.get(), first_video_.get(), &media_info_);

      SPLIT_LOG_DEBUG << media_info_.ToString();
      media_info_extracted_ = true;
    }
    if ( tags_to_send_next_.size() > kMediaInfoMaxWait ) {
      SPLIT_LOG_ERROR << "Failed to extract media info, in the first "
                      << kMediaInfoMaxWait << " tags";
      media_info_extracted_ = true;
    }
  }

  if ( bootstrapping_ ) {
    tags_to_send_next_.push_back(flv_tag.get());

    if ( media_info_extracted_ ) {
      EndBootstrapping();
    }
    return READ_SKIP;
  }

  tag_timestamp_ms_ = flv_tag->timestamp_ms();
  *tag = flv_tag.get();

  SPLIT_DLOG(10) << "Next tag: " << (*tag)->ToString();
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
      new CuePointTag(0, kDefaultFlavourMask, timestamp);

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
      const int64 time_ms = static_cast<int64>(
          ::floor(time_double * static_cast<double>(1000.00)));

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

void FlvTagSplitter::EndBootstrapping() {
  for (list< scoped_ref<Tag> >::iterator it = tags_to_send_next_.begin();
      it != tags_to_send_next_.end(); ++it) {
    if ( (*it)->timestamp_ms() < first_tag_timestamp_ms_ ) {
      it->reset(it->get()->Clone(first_tag_timestamp_ms_));
      continue;
    }
    break;
  }

  bootstrapping_ = false;
  tags_to_send_next_.push_back(
      new BosTag(0, kDefaultFlavourMask, tag_timestamp_ms_));
}

void FlvTagSplitter::UpdateMetadata(FlvTag::Metadata& metadata) {
  if ( metadata.name().value() != kOnMetaData ) {
    return;
  }
  metadata.mutable_values()->Erase("cuePoints");

  SPLIT_DLOG(8) << "Metadata adjusted to: \n" << metadata.ToString();
}
}
