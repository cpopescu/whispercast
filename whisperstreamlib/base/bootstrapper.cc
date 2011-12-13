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
// Author: Catalin Popescu

#include <whisperstreamlib/base/bootstrapper.h>

namespace streaming {

namespace {
void MaybeSendTag(CallbacksManager* callback_manager,
    int64 timestamp, const Bootstrapper::BootstrapTag& tag) {
  if ( tag.tag_.get() != NULL ) {
    callback_manager->DistributeTag(tag.tag_.get(),
      (timestamp < 0) ? tag.timestamp_ms_ : timestamp);
  }
}
void MaybeSendTag(ProcessingCallback* callback,
    int64 timestamp, const Bootstrapper::BootstrapTag& tag) {
  if ( tag.tag_.get() != NULL ) {
    callback->Run(tag.tag_.get(),
      (timestamp < 0) ? tag.timestamp_ms_ : timestamp);
  }
}
}

// bootstrap tags are to be sent at the stream begin
void Bootstrapper::PlayAtBegin(CallbacksManager* callback_manager,
    int64 timestamp_ms, uint32 flavour_mask) const {
  timestamp_ms = (timestamp_ms < 0) ? 0 : timestamp_ms;

  // The begin bootstrap tag
  callback_manager->DistributeTag(scoped_ref<Tag>(
      new BootstrapBeginTag(0, flavour_mask)).get(), 0);

  // regular bootstrap
  for (deque<BootstrapTag>::const_iterator it =
      source_started_tags_.begin(); it != source_started_tags_.end(); ++it) {
    MaybeSendTag(callback_manager, timestamp_ms, *it);
  }

  MaybeSendTag(callback_manager, timestamp_ms, metadata_);
  MaybeSendTag(callback_manager, timestamp_ms, cue_points_);
  MaybeSendTag(callback_manager, timestamp_ms, avc_sequence_header_);
  MaybeSendTag(callback_manager, timestamp_ms, aac_header_tag_);
  MaybeSendTag(callback_manager, timestamp_ms, moov_tag_);

  // media bootstrap
  for ( int i = 0; i < media_bootstrap_.size(); ++i ) {
    MaybeSendTag(callback_manager, timestamp_ms, media_bootstrap_[i]);
  }

  // The end bootstrap tag
  callback_manager->DistributeTag(scoped_ref<Tag>(
      new BootstrapEndTag(0, flavour_mask)).get(),
          (timestamp_ms < 0) ? 0 : timestamp_ms);
}

void Bootstrapper::PlayAtBegin(ProcessingCallback* callback,
    int64 timestamp_ms, uint32 flavour_mask) const {
  timestamp_ms = (timestamp_ms < 0) ? 0 : timestamp_ms;

  // The begin bootstrap tag
  callback->Run(scoped_ref<Tag>(
      new BootstrapBeginTag(0, flavour_mask)).get(), 0);

  // regular bootstrap
  for (deque<BootstrapTag>::const_iterator it =
      source_started_tags_.begin(); it != source_started_tags_.end(); ++it) {
    MaybeSendTag(callback, 0, *it);
  }

  MaybeSendTag(callback, timestamp_ms, metadata_);
  MaybeSendTag(callback, timestamp_ms, cue_points_);
  MaybeSendTag(callback, timestamp_ms, avc_sequence_header_);
  MaybeSendTag(callback, timestamp_ms, aac_header_tag_);
  MaybeSendTag(callback, timestamp_ms, moov_tag_);

  // media bootstrap
  for ( int i = 0; i < media_bootstrap_.size(); ++i ) {
    MaybeSendTag(callback, timestamp_ms, media_bootstrap_[i]);
  }

  // The end bootstrap tag
  callback->Run(scoped_ref<Tag>(
      new BootstrapEndTag(0,
          flavour_mask)).get(), timestamp_ms);
}

void Bootstrapper::PlayAtEnd(ProcessingCallback* callback,
    int64 timestamp_ms, uint32 flavour_mask) const {
  timestamp_ms = (timestamp_ms < 0) ? 0 : timestamp_ms;

  // synthesize source ended tags
  for (deque<BootstrapTag>::const_reverse_iterator it =
      source_started_tags_.rbegin(); it != source_started_tags_.rend(); ++it) {
    if ( ((*it).tag_.get()->flavour_mask() & flavour_mask) != 0 ) {
      scoped_ref<SourceEndedTag> tag(new SourceEndedTag(
        (*it).tag_.get()->attributes(),
        (*it).tag_.get()->flavour_mask(),
        reinterpret_cast<const SourceStartedTag*>((*it).tag_.get())->
          source_element_name(),
        reinterpret_cast<const SourceStartedTag*>((*it).tag_.get())->
          path(),
        reinterpret_cast<const SourceStartedTag*>((*it).tag_.get())->
          is_final()
      ));
      MaybeSendTag(callback, timestamp_ms, BootstrapTag(tag.get(), 0));
    }
  }
}

void Bootstrapper::PlayAtEnd(streaming::CallbacksManager* callback_manager,
    int64 timestamp_ms, uint32 flavour_mask) const {
  timestamp_ms = (timestamp_ms < 0) ? 0 : timestamp_ms;

  for (deque<BootstrapTag>::const_reverse_iterator it =
      source_started_tags_.rbegin(); it != source_started_tags_.rend(); ++it) {
    if ( ((*it).tag_.get()->flavour_mask() & flavour_mask) != 0 ) {
      scoped_ref<SourceEndedTag> tag(new SourceEndedTag(
        (*it).tag_.get()->attributes(),
        (*it).tag_.get()->flavour_mask(),
        reinterpret_cast<const SourceStartedTag*>((*it).tag_.get())->
          source_element_name(),
        reinterpret_cast<const SourceStartedTag*>((*it).tag_.get())->
          path(),
        reinterpret_cast<const SourceStartedTag*>((*it).tag_.get())->
          is_final()
      ));
      MaybeSendTag(callback_manager, timestamp_ms, BootstrapTag(tag.get(), 0));
    }
  }
}

void Bootstrapper::ClearMediaBootstrap() {
  media_bootstrap_.clear();
}

void Bootstrapper::ClearBootstrap() {
  source_started_tags_.clear();

  avc_sequence_header_ = BootstrapTag();
  aac_header_tag_ = BootstrapTag();
  metadata_ = BootstrapTag();
  cue_points_ = BootstrapTag();
  moov_tag_ = BootstrapTag();
}

void Bootstrapper::GetBootstrapTags(vector<BootstrapTag>* out) {
  for (deque<BootstrapTag>::const_iterator it =
      source_started_tags_.begin(); it != source_started_tags_.end(); ++it) {
    out->push_back(*it);
  }

  if ( metadata_.tag_.get() != NULL ) {
    out->push_back(metadata_);
  }
  if ( cue_points_.tag_.get() != NULL ) {
    out->push_back(cue_points_);
  }
  if ( avc_sequence_header_.tag_.get() != NULL ) {
    out->push_back(avc_sequence_header_);
  }
  if ( aac_header_tag_.tag_.get() != NULL ) {
    out->push_back(aac_header_tag_);
  }
  if ( moov_tag_.tag_.get() != NULL ) {
    out->push_back(moov_tag_);
  }
}

void Bootstrapper::ProcessTag(const Tag* tag, int64 timestamp_ms) {
  if ( tag->type() == Tag::TYPE_SOURCE_ENDED ) {
    const streaming::SourceEndedTag* source_ended =
        static_cast<const streaming::SourceEndedTag*>(tag);

    if ( !source_started_tags_.empty() ) {
      scoped_ref<const streaming::SourceStartedTag> source_started =
          reinterpret_cast<const SourceStartedTag*>(
              source_started_tags_.rbegin()->tag_.get());
      source_started_tags_.pop_back();

      if ( source_started->source_element_name() !=
          source_ended->source_element_name() ) {
        LOG_ERROR << "Received a TYPE_SOURCE_ENDED with the wrong media"
                          " name - expected ["
                  << source_started->source_element_name() << "], got ["
                  << source_ended->source_element_name()
                  << "]";
      }
    } else {
      LOG_ERROR << "Received a TYPE_SOURCE_ENDED without the corresponding"
                        " TYPE_SOURCE_STARTED - ["
                << source_ended->source_element_name()
                << "]";
    }

    media_bootstrap_.clear();

    avc_sequence_header_ = BootstrapTag();
    aac_header_tag_ = BootstrapTag();
    metadata_ = BootstrapTag();
    cue_points_ = BootstrapTag();
    moov_tag_ = BootstrapTag();

    return;
  }

  //////////////////////////////////////////////////////////////////////////
  // extract bootstrap tags
  if ( tag->type() == Tag::TYPE_SOURCE_STARTED ) {
    source_started_tags_.push_back(BootstrapTag(
        reinterpret_cast<const SourceStartedTag*>(tag), timestamp_ms));
  }

  if ( tag->type() == Tag::TYPE_FLV ) {
    const FlvTag* flv_tag = reinterpret_cast<const FlvTag*>(tag);

    if ( flv_tag->body().type() == FLV_FRAMETYPE_VIDEO &&
         flv_tag->video_body().video_codec() == FLV_FLAG_VIDEO_CODEC_AVC &&
         flv_tag->video_body().video_avc_packet_type() == AVC_SEQUENCE_HEADER ) {
      avc_sequence_header_ = BootstrapTag(
          tag, timestamp_ms);
      return;
    }

    if ( flv_tag->body().type() == FLV_FRAMETYPE_VIDEO &&
         flv_tag->video_body().video_avc_moov().get() != NULL ) {
      moov_tag_ = BootstrapTag(
          flv_tag->video_body().video_avc_moov().get(), timestamp_ms);
      return;
    }

    if ( flv_tag->body().type() == FLV_FRAMETYPE_METADATA &&
         flv_tag->metadata_body().name().value() == kOnMetaData ) {
      metadata_ = BootstrapTag(
          tag, timestamp_ms);
      return;
    }

    if ( flv_tag->body().type() == FLV_FRAMETYPE_AUDIO &&
         flv_tag->audio_body().audio_format() == FLV_FLAG_SOUND_FORMAT_AAC &&
         flv_tag->audio_body().audio_is_aac_header() ) {
      aac_header_tag_ = BootstrapTag(
          tag, timestamp_ms);
      return;
    }
  }
  if ( tag->type() == Tag::TYPE_CUE_POINT ) {
    cue_points_ = BootstrapTag(tag, timestamp_ms);
    return;
  }

  //////////////////////////////////////////////////////////////////////////
  // remember media from last keyframe -> .. present
  // NOTE: if stream contains only audio then don't bootstrap any media.
  if ( keep_media_ ) {
    bool keyframe = tag->is_video_tag() && tag->can_resync();
    if ( keyframe ) {
      ClearMediaBootstrap();
    }
    if ( media_bootstrap_.empty() && !keyframe ) {
      return;
    }
    media_bootstrap_.push_back(BootstrapTag(tag, timestamp_ms));
  }
}
}
