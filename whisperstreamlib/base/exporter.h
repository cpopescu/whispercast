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
// Author: Mihai Ianculescu

#ifndef __MEDIA_BASE_EXPORTER_H__
#define __MEDIA_BASE_EXPORTER_H__

#include <stack>
#include <queue>

#include <whisperlib/net/base/selector.h>
#include <whisperstreamlib/base/element_mapper.h>
#include <whisperstreamlib/base/tag.h>
#include <whisperstreamlib/base/request.h>
#include <whisperstreamlib/base/stream_auth.h>
#include <whisperstreamlib/base/tag_normalizer.h>
#include <whisperstreamlib/stats2/stats_keeper.h>
#include <whisperstreamlib/flv/flv_tag.h>
#include <whisperstreamlib/flv/flv_coder.h>

namespace streaming {

class Exporter {
 protected:
  class AutoDecRef {
   public:
    AutoDecRef(Exporter* c) : c_(c) {}
    ~AutoDecRef() { if ( c_ != NULL ) c_->DecRef(); }
   private:
    Exporter* c_;
  };
 public:
  enum State {
    STATE_CREATED,
    STATE_LOOKING_UP,
    STATE_AUTHORIZING,
    STATE_PLAYING,
    STATE_PAUSED,
    STATE_CLOSED
  };
 public:
  Exporter(const char* protocol,
      streaming::ElementMapper* element_mapper,
      net::Selector* media_selector,
      net::Selector* net_selector,
      streaming::StatsCollector* stats_collector,
      int32 max_write_ahead_ms) :
        state_(STATE_CREATED),
        normalizer_(media_selector, max_write_ahead_ms),
        request_(NULL),
        processing_callback_(NewPermanentCallback(this,
            &Exporter::ProcessTag)),
        auth_(*media_selector),
        is_export_registered_(false),
        is_request_registered_(false),
        protocol_type_(protocol),
        element_mapper_(element_mapper),
        media_selector_(media_selector),
        net_selector_(net_selector),
        stats_collector_(stats_collector),
        stats_keeper_(stats_collector),
        start_ts_(media_selector_->now()),
        stream_stats_opened_(false),
        media_id_(0),
        dropping_interframes_(false),
        pausing_(false),
        scheduled_tags_length_(0) {
    DCHECK(net_selector_->IsInSelectThread());
  }
  virtual ~Exporter() {
    DCHECK(net_selector_->IsInSelectThread());

    CHECK_NULL(request_);
    CHECK(!stream_stats_opened_);
    CHECK(!is_export_registered_);

    delete processing_callback_;
    processing_callback_ = NULL;
  }

  State state() const {
    return state_;
  }
  void set_state(State state) {
    state_ = state;
  }

 protected:
  // we need to be ref counted
  virtual void IncRef() = 0;
  virtual void DecRef() = 0;

  // synchronize Net & Media common objects.
  virtual synch::Mutex* mutex() const = 0;

  // self explanatory
  virtual bool is_closed() const = 0;

  // These are needed to initialize the stream_begin_stats_.
  virtual const ConnectionBegin& connection_begin_stats() const = 0;
  virtual const ConnectionEnd& connection_end_stats() const = 0;

  virtual void OnStreamNotFound() = 0;
  virtual void OnAuthorizationFailed() = 0;
  virtual void OnTooManyClients() = 0;
  virtual void OnAddRequestFailed() = 0;
  virtual void OnPlay() = 0;

  // returns: true => there's enough outbuf space, continue sending tags
  //          false => outbuf full, dont't call SendTag()
  virtual bool CanSendTag() const = 0;
  // The protocol must call ProcessLocalizedTag() when there's enough
  // outbuf space.
  virtual void SetNotifyReady() = 0;
  virtual void SendTag(const streaming::Tag* tag, int64 tag_timestamp_ms) = 0;

 protected:
  void StartRequest(const string& path) {
    DCHECK(media_selector_->IsInSelectThread());
    DCHECK(request_ != NULL);

    request_path_ = strutil::StrUnescape(
        strutil::SplitFirst(path, "?").first, '%');

    set_state(STATE_LOOKING_UP);
    OpenStreamStats();

    IncRef();
    element_mapper_->GetMediaDetails(protocol_type_, request_path_, request_,
        NewCallback(this, &Exporter::GetMediaDetailsCompleted));
  }
  void CloseRequest(const char* reason) {
    DCHECK(media_selector_->IsInSelectThread());

    CloseStreamStats(reason);
    set_state(STATE_CLOSED);

    if ( request_ != NULL ) {
      if ( is_request_registered_ ) {
        element_mapper_->RemoveRequest(request_, processing_callback_);
      } else {
        delete request_;
      }
      request_ = NULL;
      is_request_registered_ = false;
    }

    normalizer_.Reset(NULL);

    if ( is_export_registered_ ) {
      element_mapper_->RemoveExportClient(protocol_type_, export_path_);
      is_export_registered_ = false;
    }

    auth_.Stop();
  }

  void GetMediaDetailsCompleted(bool success) {
    DCHECK(media_selector_->IsInSelectThread());
    AutoDecRef auto_dec_ref(this);

    if ( is_closed() ) {
      return;
    }

    if ( !success ||
         streaming::FlavourId(request_->caps().flavour_mask_) < 0 ) {
      LOG_INFO << "Cannot lookup stream [" << request_path_ << "]"
                  ", success: " << success << ", flavour_mask: "
               << request_->caps().flavour_mask_ << ", failing request";

      OnStreamNotFound();
      CloseRequest("STREAM NOT FOUND");
      return;
    }

    if ( request_->serving_info().flavour_mask_is_set_ ) {
      request_->mutable_caps()->flavour_mask_ =
          request_->serving_info().flavour_mask_;
    }


    set_state(STATE_AUTHORIZING);

    if ( request_->serving_info().authorizer_name_ == "" ) {
      AuthorizeCompleted(true);
      return;
    }

    streaming::Authorizer* authorizer = element_mapper_->GetAuthorizer(
        request_->serving_info().authorizer_name_);
    if ( authorizer == NULL ) {
      AuthorizeCompleted(false);
      return;
    }

    // no need to IncRef(), auth_ automatically Cancels on destructor

    auth_.Start(authorizer, request_->info().auth_req_,
        NewCallback(this, &Exporter::AuthorizeCompleted),
        NewCallback(this, &Exporter::ReauthorizeFailed));
  }

  void AuthorizeCompleted(bool allowed) {
    DCHECK(media_selector_->IsInSelectThread());

    if ( is_closed() ) {
      // exporter closed meanwhile, authorization is meaningless now
      return;
    }

    if ( !allowed ) {
      LOG_ERROR << "NOT AUTHORIZED: [" << request_path_ <<
                   "], failing request: " << request_->ToString();
      OnAuthorizationFailed();
      CloseRequest("AUTHORIZATION FAILED");
      return;
    }

    export_path_ = request_->serving_info().export_path_;

    flow_control_video_ms_ =
        request_->serving_info().flow_control_video_ms_;
    flow_control_total_ms_ =
        request_->serving_info().flow_control_total_ms_;

    const uint32 client_count = element_mapper_->AddExportClient(
        protocol_type_, export_path_);
    is_export_registered_ = true;
    if ( request_->serving_info().max_clients_ >= 0 &&
         request_->serving_info().max_clients_ < client_count ) {
      LOG_WARNING << "Too many clients on export path: ["
          << export_path_ << "], current: "
          << client_count << ", limit: "
          << request_->serving_info().max_clients_ << " when playing ["
          << request_path_ << "]";

      OnTooManyClients();
      CloseRequest("TOO MANY REQUESTS");
      return;
    }

    if ( !element_mapper_->AddRequest(
        request_->serving_info().media_name_.c_str(),
        request_, processing_callback_) ) {
      LOG_ERROR << "Cannot add request on export path: ["
          << export_path_ << "], when playing ["
          << request_path_ << "]";

      OnAddRequestFailed();
      CloseRequest("STREAM NOT FOUND");
      return;
    }
    is_request_registered_ = true;

    OnPlay();
    if ( is_closed() ) {
      CloseRequest("OnPlay failed");
      return;
    }

    normalizer_.Reset(request_);
    set_state(STATE_PLAYING);
  }

  void ReauthorizeFailed() {
    LOG_INFO << "Reauthorization failed while playing ["
             << request_path_ << "]";
    OnAuthorizationFailed();
    CloseRequest("REAUTHORIZATION FAILED");
  }

  void ProcessTag(const streaming::Tag* tag, int64 timestamp_ms) {
    DCHECK(media_selector_->IsInSelectThread());

    normalizer_.ProcessTag(tag, timestamp_ms);
    //LOG_ERROR << "############ ProcessTag:"
    //          << " stream_time: " << normalizer_.stream_time_ms()
    //          << ", media_time: " << normalizer_.media_time_ms()
    //          << ", tag_ts: " << timestamp_ms
    //          << ", " << tag->ToString();

    switch ( tag->type() ) {
      case streaming::Tag::TYPE_SOURCE_ENDED: {
        const streaming::SourceEndedTag* source_ended =
            static_cast<const streaming::SourceEndedTag*>(tag);

        if ( !source_started_tags_.empty() ) {
          scoped_ref<const streaming::SourceStartedTag> source_started =
              source_started_tags_.top();
          source_started_tags_.pop();

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

        stats_keeper_.CloseMediaStats(
            source_ended->source_element_name(),
            "SOURCE_ENDED",
            normalizer_.media_time_ms());

        // set media_name to previous SourceStarted media name
        {
          synch::MutexLocker l(mutex());
          media_name_ = source_started_tags_.empty() ? "" :
                        source_started_tags_.top()->source_element_name();
        }
        HandleTag(tag);
        return;
      }

      case streaming::Tag::TYPE_SOURCE_STARTED: {
        const streaming::SourceStartedTag* source_started =
          static_cast<const streaming::SourceStartedTag*>(tag);
        {
          synch::MutexLocker l(mutex());
          media_name_ = source_started->source_element_name();
        }

        const string crt_media_id(
            strutil::StringPrintf("%s.%s.%lld",
                request_->info().session_id_.c_str(),
                connection_begin_stats().connection_id_.get().c_str(),
                static_cast<long long int>(media_id_++)));
        stats_keeper_.OpenMediaStats(
            stream_begin_stats_,
            crt_media_id,
            source_started->source_element_name(),
            normalizer_.media_time_ms(),
            normalizer_.stream_time_ms());

        source_started_tags_.push(source_started);

        dropping_interframes_ = true;
        HandleTag(tag);
        return;
      }

      // signaling tags that need to get into the network thread
      case streaming::Tag::TYPE_SEEK_PERFORMED:
        dropping_interframes_ = true;
        HandleTag(tag);
        return;

      case streaming::Tag::TYPE_EOS:
        // Instead of RemoveCallback() which requires separate context from EOS,
        // forward the EosTag to network, and let network call CloseRequest()
        HandleTag(tag);
        return;

      // all the media tags need to get into the network thread
      case streaming::Tag::TYPE_MEDIA_INFO:
      case streaming::Tag::TYPE_COMPOSED:
      case streaming::Tag::TYPE_FLV:
      case streaming::Tag::TYPE_F4V:
      case streaming::Tag::TYPE_MTS:
      case streaming::Tag::TYPE_MP3:
      case streaming::Tag::TYPE_AAC:
      case streaming::Tag::TYPE_RAW:
      case streaming::Tag::TYPE_INTERNAL:
        HandleTag(tag);
        return;

      // these are just ignored
      case streaming::Tag::TYPE_BOOTSTRAP_BEGIN:
      case streaming::Tag::TYPE_BOOTSTRAP_END:
      case streaming::Tag::TYPE_FLV_HEADER:
      case streaming::Tag::TYPE_FEATURE_FOUND:
      case streaming::Tag::TYPE_CUE_POINT:
      case streaming::Tag::TYPE_OSD:
        break;
    }
  }

 public:
  void Pause() {
    DCHECK(media_selector_->IsInSelectThread());
    if ( pausing_ || request_ == NULL || request_->controller() == NULL ||
         !request_->controller()->SupportsPause() ) {
      return;
    }

    pausing_ = true;
    request_->controller()->Pause(true);
  }
  void Resume(bool dec_ref = false) {
    if ( !media_selector_->IsInSelectThread() ) {
      IncRef();
      media_selector_->RunInSelectLoop(NewCallback(this,
          &Exporter::Resume, true));
      return;
    }
    AutoDecRef auto_dec_ref(dec_ref ? this : NULL);

    if ( request_ == NULL || request_->controller() == NULL ||
         !request_->controller()->SupportsPause() ) {
      return;
    }

    request_->controller()->Pause(false);
  }

 private:
  void HandleTag(const streaming::Tag* tag) {
    DCHECK(media_selector_->IsInSelectThread());

    int64 scheduled_ms = 0;
    {
      synch::MutexLocker l(mutex());
      scheduled_ms = scheduled_tags_length_;
    }

    // Flow control on schedule_tags_ length. Because we don't have a constant
    // rate (like a live stream). Happens on HTTP download.
    if ( flow_control_total_ms_ == 0 && scheduled_ms > 500 ) {
      Pause();
    }

    bool drop_tag =
        // drop interframes
        (dropping_interframes_ && tag->is_video_tag() &&
            tag->is_droppable() && !tag->can_resync()) ||
        // drop audio&video tags above flow_control_total_ms_
        ((tag->is_video_tag() || tag->is_audio_tag()) &&
            flow_control_total_ms_ > 0 && tag->is_droppable() &&
            scheduled_ms > flow_control_total_ms_) ||
        // drop video tags above flow_control_video_ms_
        (tag->is_video_tag() && flow_control_video_ms_ > 0 &&
            tag->is_droppable() && scheduled_ms > flow_control_video_ms_);
    dropping_interframes_ = dropping_interframes_ || drop_tag;

    // maybe drop tag, update stats
    if ( drop_tag && tag->is_droppable() ) {
      VLOG(8) << "Dropping: " << tag->ToString();
      if ( tag->is_video_tag() ) { stats_keeper_.video_frames_dropped_add(1); }
      if ( tag->is_audio_tag() ) { stats_keeper_.audio_frames_dropped_add(1); }
      return;
    }

    // update stats
    if ( tag->is_video_tag() ) { stats_keeper_.video_frames_add(1); }
    if ( tag->is_audio_tag() ) { stats_keeper_.audio_frames_add(1); }

    // stop dropping interframes on first keyframe
    if ( dropping_interframes_ && tag->is_video_tag() &&
        tag->can_resync() && tag->is_droppable() ) {
      VLOG(8) << "Resync on: " << tag->ToString();
      dropping_interframes_ = false;
    }

    // update media info before sending to network
    if ( tag->type() == Tag::TYPE_MEDIA_INFO ) {
      scoped_ref<MediaInfoTag> info =
          new MediaInfoTag(*static_cast<const MediaInfoTag*>(tag));
      UpdateMediaInfo(info->mutable_info());
      ScheduleTag(info.get());
      return;
    }

    // update onCuePoint metadata before sending to network
    if ( tag->type() == Tag::TYPE_FLV ) {
      const FlvTag* flv_tag = static_cast<const FlvTag*>(tag);
      if ( flv_tag->body().type() == FLV_FRAMETYPE_METADATA &&
           flv_tag->metadata_body().name().value() == kOnCuePoint ) {
        scoped_ref<FlvTag> metadata = new FlvTag(*flv_tag, -1, true);
        UpdateFlvCuePoint(metadata->mutable_metadata_body());
        ScheduleTag(metadata.get());
        return;
      }
    }

    ScheduleTag(tag);
  }

  // enqueue to 'scheduled_tags_'.
  // Until now the tag circulated through MEDIA selector.
  void ScheduleTag(const streaming::Tag* tag) {
    DCHECK(media_selector_->IsInSelectThread());

    ScheduledTag stag(tag, normalizer_.stream_time_ms());

    bool process = false;
    {
    synch::MutexLocker l(mutex());

    process = scheduled_tags_.empty();
    scheduled_tags_.push(stag);

    scheduled_tags_length_ = stag.stream_time_ms_ -
        scheduled_tags_.front().stream_time_ms_;
    }

    if ( process ) {
      ProcessLocalizedTags();
    }

    if ( tag->type() == streaming::Tag::TYPE_EOS ) {
      normalizer_.Reset(NULL);
    }
  }

 protected:
  // dequeue from 'scheduled_tags_'
  // The rest of the processing happens in NET selector.
  void ProcessLocalizedTags(bool dec_ref = false) {
    if ( !net_selector_->IsInSelectThread() ) {
      IncRef();
      net_selector_->RunInSelectLoop(NewCallback(this,
          &Exporter::ProcessLocalizedTags, true));
      return;
    }
    AutoDecRef auto_dec_ref(dec_ref ? this : NULL);

    while ( true ) {
      if ( !CanSendTag() ) {
        SetNotifyReady();
        break;
      }

      // pop a tag
      ScheduledTag stag;
      {
        synch::MutexLocker l(mutex());
        if ( scheduled_tags_.empty() ) {
          break;
        }

        stag = scheduled_tags_.front();
        scheduled_tags_length_ = scheduled_tags_.back().stream_time_ms_ -
            stag.stream_time_ms_;
        scheduled_tags_.pop();
      }

      // send the tag
      SendTag(stag.tag_.get(), stag.stream_time_ms_);
    }

    // Flow control on schedule_tags_ length. Because we don't have a constant
    // rate (like a live stream). Happens on HTTP download.
    if ( flow_control_total_ms_ == 0 &&
         scheduled_tags_length_ == 0 &&
         pausing_ ) {
      pausing_ = false;
      Resume();
    }
  }

 private:
  void UpdateMediaInfo(MediaInfo* media_info) {
    // media_info should be updated in media thread, before sending to net
    CHECK(media_selector_->IsInSelectThread());
    streaming::ElementController* controller = request_->controller();

    if ( controller == NULL || !controller->SupportsSeek() ) {
      media_info->set_seekable(false);
      media_info->mutable_flv_extra_metadata()->Set(
          "canSeekToEnd", new rtmp::CBoolean(false));
    }
    if ( controller == NULL || !controller->SupportsPause() ) {
      media_info->set_pausable(false);
    }
    media_info->mutable_flv_extra_metadata()->Set(
        "origin", new rtmp::CNumber((normalizer_.stream_time_ms() -
            normalizer_.media_time_ms())/1000.0));
    media_info->mutable_flv_extra_metadata()->Set(
        "media", new rtmp::CString(media_name_));
  }
  void UpdateFlvCuePoint(FlvTag::Metadata& metadata) {
    CHECK_EQ(metadata.name().value(), streaming::kOnCuePoint);
    FlvCoder::UpdateTimeInCuePoint(&metadata, 0,
        normalizer_.media_time_ms(), false);
  }

 private:
  void OpenStreamStats() {
    DCHECK(media_selector_->IsInSelectThread());
    DCHECK(request_ != NULL);
    if ( !stream_stats_opened_ ) {
      stream_begin_stats_.stream_id_ =
          strutil::StringPrintf("%s.0",
              connection_begin_stats().connection_id_.get().c_str());
      stream_begin_stats_.timestamp_utc_ms_ = timer::Date::Now();
      stream_begin_stats_.connection_ = connection_begin_stats();
      stream_begin_stats_.session_id_ = request_->info().session_id_;
      stream_begin_stats_.client_id_ = request_->info().client_id_;
      stream_begin_stats_.affiliate_id_ = request_->info().affiliate_id_;
      stream_begin_stats_.user_agent_ = request_->info().user_agent_;

      stream_end_stats_.stream_id_ = stream_begin_stats_.stream_id_;
      stream_end_stats_.result_ = StreamResult("ACTIVE");

      stats_collector_->StartStats(&stream_begin_stats_, &stream_end_stats_);
      stream_stats_opened_ = true;
    }
  }
  void CloseStreamStats(const char* reason) {
    DCHECK(media_selector_->IsInSelectThread());
    if ( stream_stats_opened_ ) {
      while ( !source_started_tags_.empty() ) {
        scoped_ref<const streaming::SourceStartedTag> source_started =
            source_started_tags_.top();
        source_started_tags_.pop();

        stats_keeper_.CloseMediaStats(
            source_started->source_element_name(),
            "SOURCE_ENDED",
            RunTimeMs());
      }

      stream_end_stats_.timestamp_utc_ms_ = timer::Date::Now();
      stream_end_stats_.result_ = StreamResult(reason);

      stats_collector_->EndStats(&stream_end_stats_);
      stream_stats_opened_ = false;
    }
  }
  // milliseconds this export has been running
  int64 RunTimeMs() const {
    CHECK(media_selector_->IsInSelectThread());
    return media_selector_->now() - start_ts_;
  }

 protected:
  //////////////////////////////////////////////////////////////////////
  //
  // MEDIA THREAD members
  //

  State state_;

  streaming::TagNormalizer normalizer_;

  streaming::Request* request_;
  streaming::ProcessingCallback* processing_callback_;

  streaming::AsyncAuthorize auth_;

  string request_path_;
  string export_path_;

  bool is_export_registered_;
  bool is_request_registered_;

  // "http", or "rtmp". Because we have no constants for these.
  const string protocol_type_;

  streaming::ElementMapper* const element_mapper_;

  net::Selector* const media_selector_;
  net::Selector* const net_selector_;

  streaming::StatsCollector* const stats_collector_;
  streaming::StatsKeeper stats_keeper_;

  // the moment this exported started running
  const int64 start_ts_;

  bool stream_stats_opened_;

  // for statistics: count media, each SourceStarted marks a new media
  int64 media_id_;
  // last SourceStarted: media name
  string media_name_;

  StreamBegin stream_begin_stats_;
  StreamEnd stream_end_stats_;

  stack<scoped_ref<const streaming::SourceStartedTag> > source_started_tags_;

  int64 flow_control_video_ms_;
  int64 flow_control_total_ms_;

  // flow control & anti image degradation
  bool dropping_interframes_;

  bool pausing_;

  //////////////////////////////////////////////////////////////////////
  //
  // NET THREAD members
  //

  struct ScheduledTag {
    scoped_ref<const streaming::Tag> tag_;
    int64 stream_time_ms_;

    ScheduledTag() : tag_(), stream_time_ms_(0) {}
    ScheduledTag(const Tag* tag, int64 stream_time_ms)
        : tag_(tag), stream_time_ms_(stream_time_ms) {}
  };

  queue<ScheduledTag> scheduled_tags_;
  int64 scheduled_tags_length_;
};

}

#endif // __MEDIA_BASE_EXPORTER_H__
