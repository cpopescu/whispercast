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

#include <whisperlib/net/url/url.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperstreamlib/rtmp/rtmp_play_stream.h>

#define RTMP_LOG(level) if ( connection_->flags().log_level_ < level ); \
                        else LOG(INFO) << connection_->info() << ": "

#define RTMP_LOG_DEBUG   RTMP_LOG(LDEBUG)
#define RTMP_LOG_INFO    RTMP_LOG(LINFO)
#define RTMP_LOG_WARNING RTMP_LOG(LWARNING)
#define RTMP_LOG_ERROR   RTMP_LOG(LERROR)
#define RTMP_LOG_FATAL   RTMP_LOG(LFATAL)

// Defined in rtmp_flags.cc
DECLARE_bool(rtmp_fancy_escape);

namespace rtmp {

PlayStream::PlayStream(const StreamParams& params,
                       ServerConnection* connection,
                       streaming::ElementMapper* element_mapper,
                       streaming::StatsCollector* stats_collector)
    : Stream(params, connection),
      streaming::Exporter(
          "rtmp",
          element_mapper,
          connection->media_selector(),
          connection->net_selector(),
          stats_collector,
          connection->flags().max_write_ahead_ms_),
      seeking_(false),
      seek_alarm_(*net_selector_),
      f4v2flv_(),
      f4v_cue_point_number_(0),
      video_codec_((streaming::FlvFlagVideoCodec)0),
      video_frame_type_((streaming::FlvFlagVideoFrameType)0),
      media_build_state_(WAITING_VIDEO),
      composed_media_data_tag_delta_(0),
      media_event_size_(0),
      media_event_timestamp_ms_(0),
      media_event_duration_ms_(0),
      bootstrapping_(false),
      send_reset_audio_(true),
      send_reset_pings_(true),
      send_switch_media_(false) {
}

PlayStream::~PlayStream() {
  CHECK_NULL(request_);
}

void PlayStream::NotifyOutbufEmpty(int32 outbuf_size) {
  DCHECK(net_selector_->IsInSelectThread());
  ProcessLocalizedTags();
}

bool PlayStream::ProcessEvent(Event* event, int64 timestamp_ms) {
  CHECK(net_selector_->IsInSelectThread());

  if ( event->event_type() == EVENT_INVOKE ) {
    RTMP_LOG_INFO << "INVOKE - " << event->ToString();
    EventInvoke* invoke = static_cast<EventInvoke *>(event);
    const string& method = invoke->call()->method_name();
    if ( method == kMethodPlay )     { return InvokePlay(invoke); }
    if ( method == kMethodPause ||
         method == kMethodPauseRaw ) { return InvokePause(invoke); }
    if ( method == kMethodSeek )     { return InvokeSeek(invoke); }
    RTMP_LOG_WARNING << "Unhandled INVOKE: " << event->ToString();
    return true;
  }
  if ( event->event_type() == EVENT_FLEX_MESSAGE ) {
    RTMP_LOG_INFO << "INVOKE - " << event->ToString();
    EventFlexMessage* flex = static_cast<EventFlexMessage *>(event);
    if ( flex->data().empty() ||
         flex->data()[0]->object_type() != CObject::CORE_STRING ) {
      RTMP_LOG_WARNING << "Invalid INVOKE: " << event->ToString();
      return true;
    }
    const string& method = static_cast<const CString*>(flex->data()[0])->value();
    if ( method == kMethodPlay )     { return FlexPlay(flex); }
    if ( method == kMethodPause ||
         method == kMethodPauseRaw ) { return FlexPause(flex); }
    if ( method == kMethodSeek )     { return FlexSeek(flex); }
    RTMP_LOG_WARNING << "Unhandled FLEX: " << event->ToString();
    return true;
  }
  RTMP_LOG_WARNING << "Unknown event: " << event->ToString();
  return true;
}

//////////////////////////////////////////////////////////////////////

bool PlayStream::InvokePlay(const EventInvoke* invoke) {
  CHECK(net_selector_->IsInSelectThread());
  if ( invoke->call()->arguments().size() < 1 ||
       invoke->call()->arguments()[0]->object_type() != CObject::CORE_STRING ) {
    RTMP_LOG_ERROR << "Invalid PLAY call: " << invoke->ToString();
    return false;
  }

  const string stream_name = URL::UrlUnescape(
      static_cast<const CString*>(invoke->call()->arguments()[0])->value());

  return ProcessPlay(stream_name, 0);
}
bool PlayStream::FlexPlay(const EventFlexMessage* flex) {
  CHECK(net_selector_->IsInSelectThread());
  if ( flex->data().size() < 4 ||
       flex->data()[1]->object_type() != CObject::CORE_NUMBER ||
       flex->data()[3]->object_type() != CObject::CORE_STRING) {
    RTMP_LOG_ERROR << "Invalid PLAY call: " << flex->ToString();
    return false;
  }
  const string stream_name = URL::UrlUnescape(
           static_cast<const CString*>(flex->data()[3])->value());

  return ProcessPlay(stream_name, 0);
}

//////////

bool PlayStream::InvokePause(const EventInvoke* invoke) {
  CHECK(net_selector_->IsInSelectThread());
  if ( invoke->call()->arguments().size() != 2 ||
       invoke->call()->arguments()[0]->object_type() != CObject::CORE_BOOLEAN ||
       invoke->call()->arguments()[1]->object_type() != CObject::CORE_NUMBER) {
    RTMP_LOG_ERROR << "Invalid PAUSE call: " << invoke->ToString();
    return true;
  }
  const CBoolean* const pause = static_cast<const CBoolean*>(
      invoke->call()->arguments()[0]);
  const CNumber* const moment = static_cast<const CNumber*>(
      invoke->call()->arguments()[1]);
  return ProcessPause(pause->value(), moment->int_value());
}
bool PlayStream::FlexPause(const EventFlexMessage* flex) {
  CHECK(net_selector_->IsInSelectThread());
  if ( flex->data().size() < 5 ||
       flex->data()[1]->object_type() != CObject::CORE_NUMBER ||
       flex->data()[3]->object_type() != CObject::CORE_BOOLEAN ||
       flex->data()[4]->object_type() != CObject::CORE_NUMBER) {
    RTMP_LOG_ERROR << "Invalid PAUSE call: " << flex->ToString();
    return true;
  }
  const bool pause = static_cast<const CBoolean*>(flex->data()[3])->value();
  const int64 moment = static_cast<int64>(
      static_cast<const CNumber*>(flex->data()[4])->value());
  return ProcessPause(pause, moment);
}

//////////

bool PlayStream::InvokeSeek(const EventInvoke* invoke) {
  CHECK(net_selector_->IsInSelectThread());
  if ( invoke->call()->arguments().empty() ||
       invoke->call()->arguments()[0]->object_type() != CObject::CORE_NUMBER ) {
    RTMP_LOG_ERROR << "Invalid SEEK call: " << invoke->ToString();
    return true;
  }
  const CNumber* const seek =
      static_cast<const CNumber*>(invoke->call()->arguments()[0]);
  const int64 seek_time_ms = static_cast<int64>(seek->value());
  return ProcessSeek(seek_time_ms);
}
bool PlayStream::FlexSeek(const EventFlexMessage* flex) {
  CHECK(net_selector_->IsInSelectThread());
  if ( flex->data().size() < 4 ||
       flex->data()[1]->object_type() != CObject::CORE_NUMBER ||
       flex->data()[3]->object_type() != CObject::CORE_NUMBER) {
    RTMP_LOG_ERROR << "Invalid SEEK call: " << flex->ToString();
    return true;
  }
  const int64 seek_time_ms = static_cast<int64>(
      static_cast<const CNumber*>(flex->data()[3])->value());
  return ProcessSeek(seek_time_ms);
}

//////////////////////////////////////////////////////////////////////

bool PlayStream::ProcessPlay(const string& stream_name, int64 seek_time_ms) {
  CHECK(net_selector_->IsInSelectThread());
  if ( state_ != STATE_CREATED ) {
    return false;
  }

  HandlePlay(stream_name, seek_time_ms);
  return true;
}

//////////

bool PlayStream::ProcessPause(bool pause, int64 stream_time_ms) {
  CHECK(net_selector_->IsInSelectThread());
  if ( state_ != STATE_PLAYING && state_ != STATE_PAUSED ) {
    RTMP_LOG_ERROR << "The stream is not playing, discarding PAUSE request";
    return true;
  }

  HandlePause(pause);
  return true;
}

//////////

bool PlayStream::ProcessSeek(int64 seek_time_ms) {
  CHECK(net_selector_->IsInSelectThread());

  if ( connection_->flags().seek_processing_delay_ms_ > 0 ) {
    // Don't use IncRef(); because seek_alarm_.Set may cancel the previous alarm
    // and so we lose the previous corresponding DecRef().
    seek_alarm_.Set(NewPermanentCallback(this, &PlayStream::HandleSeek,
        seek_time_ms, false),
        true,
        connection_->flags().seek_processing_delay_ms_,
        false, true);
    return true;
  }
  HandleSeek(seek_time_ms);
  return true;
}

//////////

void PlayStream::HandlePlay(string stream_name, int64 seek_time_ms,
    bool dec_ref) {
  if ( !media_selector_->IsInSelectThread() ) {
    IncRef();
    media_selector_->RunInSelectLoop(NewCallback(this, &PlayStream::HandlePlay,
        stream_name, seek_time_ms, true));
    return;
  }
  Stream::AutoDecRef auto_dec_ref(dec_ref ? this : NULL);
  DCHECK(request_ == NULL);

  stream_name_ = stream_name;

  if ( params_.application_url_.empty() ) {
    params_.application_url_ = "rtmp://internal/";
  }

  URL full_url(params_.application_url_+
      (params_.application_url_[params_.application_url_.length()-1] != '/' ?
          "/" : "") + stream_name);

  request_ = new streaming::Request(full_url);
  request_->mutable_info()->remote_address_ = connection_->remote_address();
  request_->mutable_info()->local_address_ = connection_->local_address();
  request_->mutable_info()->seek_pos_ms_ = seek_time_ms;
  request_->mutable_caps()->tag_type_ = streaming::Tag::TYPE_FLV;

  request_->mutable_info()->auth_req_.net_address_ =
      connection_->remote_address().ToString();
  request_->mutable_info()->auth_req_.resource_ = full_url.spec();
  request_->mutable_info()->auth_req_.action_ = streaming::kActionView;

  StartRequest(stream_name);
}
//////////

void PlayStream::HandlePause(bool pause, bool dec_ref) {
  if ( !media_selector_->IsInSelectThread() ) {
    IncRef();
    media_selector_->RunInSelectLoop(NewCallback(this, &PlayStream::HandlePause,
        pause, true));
    return;
  }
  Stream::AutoDecRef auto_dec_ref(dec_ref ? this : NULL);

  streaming::ElementController* controller = request_->controller();
  if ( controller == NULL || !controller->SupportsPause() ) {
    return;
  }
  if ( pause ) {
    controller->Pause(true);
    set_state(STATE_PAUSED);

    RTMP_LOG_INFO << "Notifying PAUSE";

    SendEvent(
        connection_->CreateStatusEvent(
            stream_id(), kChannelReply, 0,
            "NetStream.Pause.Notify", "Stream paused",
            stream_name_.c_str()).get(), -1, NULL, true);
  } else {
    RTMP_LOG_INFO << "Notifying UNPAUSE";

    // the pings are going through stream 0 (system stream)...
    connection_->system_stream()->SendEvent(
        connection_->CreateClearBufferPing(0, stream_id()).get());
    connection_->system_stream()->SendEvent(
        connection_->CreateResetPing(0, stream_id()).get());
    connection_->system_stream()->SendEvent(
        connection_->CreateClearPing(0, stream_id()).get());

    SendEvent(
      connection_->CreateStatusEvent(
          stream_id(), kChannelReply, 0,
          "NetStream.Unpause.Notify", "Stream resumed",
          stream_name_.c_str()).get(), -1, NULL, true);

    dropping_interframes_ = true;

    controller->Pause(false);
    set_state(STATE_PLAYING);
  }
}

//////////

void PlayStream::HandleSeek(int64 seek_time_ms, bool dec_ref) {
  if ( !media_selector_->IsInSelectThread() ) {
    IncRef();
    media_selector_->RunInSelectLoop(NewCallback(this, &PlayStream::HandleSeek,
        seek_time_ms, true));
    return;
  }
  Stream::AutoDecRef auto_dec_ref(dec_ref ? this : NULL);

  if ( state_ != STATE_PLAYING && state_ != STATE_PAUSED ) {
    RTMP_LOG_ERROR << "The stream is not playing, discarding SEEK request";
    return;
  }

  DCHECK(!seek_alarm_.IsStarted());

  streaming::ElementController* controller = request_->controller();
  if ( controller == NULL || !controller->SupportsSeek() ) {
    RTMP_LOG_ERROR << "The controller does not exist, it doesn't support"
        "seeking or the stream is terminated, failing SEEK request";
    SendEvent(
        connection_->CreateStatusEvent(
          stream_id(), kChannelReply, 0,
          "NetStream.Seek.Failed", "Seek failed",
          stream_name_.c_str()).get(), -1, NULL, true);
  }
  seeking_ = true;

  RTMP_LOG_INFO << "Seeking to " << seek_time_ms;
  controller->Seek(seek_time_ms);

  const bool was_paused = (state_ == STATE_PAUSED);
  set_state(STATE_PLAYING);

  if ( was_paused && controller != NULL && controller->SupportsPause() ) {
    controller->Pause(false);
  }
}

//////////////////////////////////////////////////////////////////////

void PlayStream::OnStreamNotFound() {
  RTMP_LOG_DEBUG << "Notifying STREAM NOT FOUND";

  SendEvent(
    connection_->CreateStatusEvent(
        stream_id(), kChannelReply, 0,
        "NetStream.Play.StreamNotFound", "Export not found",
        stream_name_.c_str()).get(), -1, NULL, true);
}
void PlayStream::OnTooManyClients() {
  RTMP_LOG_DEBUG << "Notifying TOO MANY CLIENTS";

  SendEvent(
    connection_->CreateStatusEvent(
        stream_id(), kChannelReply, 0,
        "NetStream.Play.Failed", "Too many requests",
        stream_name_.c_str()).get(), -1, NULL, true);
}
void PlayStream::OnAuthorizationFailed(bool is_reauthorization) {
  RTMP_LOG_DEBUG << "Notifying AUTHORIZATION FAILED";

  SendEvent(
    connection_->CreateStatusEvent(
        stream_id(), kChannelReply, 0,
        "NetStream.Play.Failed", "Authorization failed",
        stream_name_.c_str()).get(), -1, NULL, true);
}
void PlayStream::OnAddRequestFailed() {
  RTMP_LOG_DEBUG << "Notifying ADD REQUEST FAILED";

  SendEvent(
    connection_->CreateStatusEvent(
        stream_id(), kChannelReply, 0,
        "NetStream.Play.StreamNotFound", "Stream not found",
        stream_name_.c_str()).get(), -1, NULL, true);
}

void PlayStream::OnPlay() {
  RTMP_LOG_DEBUG << "Notifying PLAY";

  DCHECK(request_ != NULL);
  request_->mutable_info()->write_ahead_ms_ =
    connection_->flags().default_write_ahead_ms_;

  SendEvent(
      connection_->CreateChunkSize(0).get());

  SendEvent(
      connection_->CreateStatusEvent(
        stream_id(), kChannelReply, 0,
        "NetStream.Play.Reset", "Play reset", stream_name_.c_str()).get());
  SendEvent(
      connection_->CreateStatusEvent(
        stream_id(), kChannelReply, 0,
        "NetStream.Play.Start", "Play started", stream_name_.c_str()).get(),
        -1, NULL, true);
}

bool PlayStream::CanSendTag() const {
  if ( is_closed() ) {
    return false;
  }
  const int32 outbuf_size = connection_->outbuf_size() +
      ((media_event_.get() != NULL) ? media_event_->data().Size() : 0);
  return outbuf_size < connection_->flags().max_outbuf_size_/2;
}
void PlayStream::SetNotifyReady() {
  if ( is_closed() ) {
    return;
  }
  // rtmp always calls NotifyOutbufEmpty() .. so there's nothing to do here
}
void PlayStream::SendTag(const streaming::Tag* tag, int64 tag_timestamp_ms) {
  DCHECK(net_selector_->IsInSelectThread());

  //LOG_ERROR << "@" << LTIMESTAMP(tag_timestamp_ms) << tag->ToString();

  if ( is_closed() ) {
    return;
  }

  if ( tag->type() == streaming::Tag::TYPE_COMPOSED ) {
    /*
    const streaming::ComposedTag* composed_tag =
        static_cast<const streaming::ComposedTag*>(tag);
    for ( int i = 0; i < composed_tag->tags().size(); ++i ) {
      if ( is_closed() ) {
        return;
      }

      const streaming::Tag* ltag = composed_tag->tags().tag(i).get();
      SendSimpleTag(ltag, tag_timestamp_ms + ltag->timestamp_ms());
    }
    */
  } else {
    SendSimpleTag(tag, tag_timestamp_ms);
  }
}

//////////////////////////////////////////////////////////////////////

void PlayStream::SendSimpleTag(const streaming::Tag* tag,
    int64 tag_timestamp_ms) {
  DCHECK(net_selector_->IsInSelectThread());

  if ( tag->type() == streaming::Tag::TYPE_EOS ) {
    const streaming::EosTag* eos = static_cast<const streaming::EosTag*>(tag);
    SendEvent(
      connection_->CreateStatusEvent(
        stream_id(),
        kChannelReply,
        0,
        "NetStream.Play.Complete", "EOS",
        stream_name_.c_str()).get(),
        tag_timestamp_ms,  NULL, true);
    SendEvent(
      connection_->CreateStatusEvent(
        stream_id(),
        kChannelReply,
        0,
        "NetStream.Play.Stop", "EOS",
        stream_name_.c_str()).get(),
        tag_timestamp_ms,  NULL, true);

    if ( eos->forced() ) {
      // close connection
      connection_->CloseConnection();
    }

    // Now: let the client disconnect, and we'll be notified by:
    // NotifyConnectionClosed() -> CloseInternal() -> CloseRequest().
    return;
  }

  if ( tag->type() == streaming::Tag::TYPE_SOURCE_STARTED ) {
    send_reset_audio_ = true;
    return;
  }
  if ( tag->type() == streaming::Tag::TYPE_SOURCE_ENDED ) {
    send_switch_media_ = true;
    return;
  }
  if ( tag->type() == streaming::Tag::TYPE_FLUSH ) {
    send_reset_audio_ = true;
    send_reset_pings_ = true;
    return;
  }
  if ( tag->type() == streaming::Tag::TYPE_SEEK_PERFORMED ) {
    if ( seeking_ ) {
      seeking_ = false;
      // the pings are going through stream 0 (system stream)...
      connection_->system_stream()->SendEvent(
          connection_->CreateClearBufferPing(0, stream_id()).get(),
          -1, NULL, true);
      connection_->system_stream()->SendEvent(
          connection_->CreateResetPing(0, stream_id()).get(),
          -1, NULL, true);
      connection_->system_stream()->SendEvent(
          connection_->CreateClearPing(0, stream_id()).get(),
          tag_timestamp_ms,  NULL, true);

      send_reset_pings_ = false;

      SendEvent(
        connection_->CreateStatusEvent(
          stream_id(), kChannelReply, 0,
          "NetStream.Seek.Notify", "Seek completed",
          stream_name_.c_str()).get(),
          tag_timestamp_ms, NULL, true);
      SendEvent(
        connection_->CreateStatusEvent(
          stream_id(), kChannelReply, 0,
          "NetStream.Play.Start", "Play started",
          stream_name_.c_str()).get(),
          -1, NULL, true);

      // send an empty audio tag
      scoped_ref<streaming::FlvTag> audio(
          new streaming::FlvTag(
              streaming::Tag::ATTR_AUDIO | streaming::Tag::ATTR_CAN_RESYNC,
              streaming::kDefaultFlavourMask,
              tag_timestamp_ms, streaming::FLV_FRAMETYPE_AUDIO));
      SendFlvTag(audio.get(), tag_timestamp_ms);
    }
    return;
  }

  if ( tag->type() == streaming::Tag::TYPE_BOOTSTRAP_BEGIN ) {
    bootstrapping_ = true;
    // reset this, as we want it to be recomputed
    video_codec_ = (streaming::FlvFlagVideoCodec)0;
    return;
  }
  if ( tag->type() == streaming::Tag::TYPE_BOOTSTRAP_END ) {
    bootstrapping_ = false;
    // check if we need to send the bootstrap end FLV video stuff
    if ( video_codec_ != (streaming::FlvFlagVideoCodec)0 ) {
      /* TODO: Clarify why is this corrupting display
      // send the bootstrap end signal
      scoped_ref<streaming::FlvTag> video(
          new streaming::FlvTag(
              streaming::Tag::ATTR_VIDEO | streaming::Tag::ATTR_CAN_RESYNC,
              streaming::kDefaultFlavourMask,
              tag_timestamp_ms, streaming::FLV_FRAMETYPE_VIDEO));
      char d[2];
      d[0] = (video_frame_type_ << 4) | video_codec_;
      d[1] = 0x01;
      video.get()->mutable_video_body().append_data(d, 2);

      SendFlvTag(
          video.get(), tag_timestamp_ms);
      */
    }
    return;
  }

  if ( send_switch_media_ ) {
    SendEvent(
        connection_->CreateStatusEvent(
          stream_id(),
          kChannelReply,
          0,
          "NetStream.Play.Complete", "SWITCH",
          stream_name_.c_str()).get(),
          tag_timestamp_ms,  NULL, true);
    SendEvent(
        connection_->CreateStatusEvent(
          stream_id(),
          kChannelReply,
          0,
          "NetStream.Play.Switch", "SWITCH",
          stream_name_.c_str()).get(),
          tag_timestamp_ms,  NULL, true);
    send_switch_media_ = false;
  }

  if ( send_reset_pings_ ) {
    // the pings are going through stream 0 (system stream)...
    connection_->system_stream()->SendEvent(
        connection_->CreateResetPing(0, stream_id()).get(),
        -1, NULL, true);
    connection_->system_stream()->SendEvent(
        connection_->CreateClearPing(0, stream_id()).get(),
        tag_timestamp_ms,  NULL, true);

    send_reset_pings_ = false;
  }

  if ( send_reset_audio_ ) {
    // send an empty audio tag
    scoped_ref<streaming::FlvTag> audio(
        new streaming::FlvTag(
            streaming::Tag::ATTR_AUDIO | streaming::Tag::ATTR_CAN_RESYNC,
            streaming::kDefaultFlavourMask,
            tag_timestamp_ms, streaming::FLV_FRAMETYPE_AUDIO));
    SendFlvTag(audio.get(), tag_timestamp_ms);

    send_reset_audio_ = false;
  }

  if ( tag->type() == streaming::Tag::TYPE_FLV ) {
    const streaming::FlvTag* flv_tag =
        static_cast<const streaming::FlvTag*>(tag);
    SendFlvTag(flv_tag, tag_timestamp_ms);
    return;
  }
  if ( tag->type() == streaming::Tag::TYPE_F4V ) {
    const streaming::F4vTag* f4v_tag =
        static_cast<const streaming::F4vTag*>(tag);
    vector< scoped_ref<streaming::FlvTag> > flv_tags;
    f4v2flv_.ConvertTag(f4v_tag, &flv_tags);
    if ( f4v_tag->is_frame() ) {
      const streaming::f4v::FrameHeader& f4v_header =
          f4v_tag->frame()->header();
      if ( f4v_header.is_keyframe() ) {
        streaming::FlvTag* cue_point = f4v2flv_.CreateCuePoint(
            f4v_header,
            tag_timestamp_ms,
            f4v_cue_point_number_);
        f4v_cue_point_number_++;
        flv_tags.push_back(scoped_ref<streaming::FlvTag>(cue_point));
      }
    }
    for ( int i = 0; i < flv_tags.size(); ++i ) {
      SendFlvTag(flv_tags[i].get(), tag_timestamp_ms);
    }
    return;
  }
  RTMP_LOG_FATAL << "Don't know how to handle media tag: " << tag->ToString();
}

//////////////////////////////////////////////////////////////////////

void PlayStream::SendFlvTag(const streaming::FlvTag* flv_tag,
                                       int64 tag_timestamp_ms) {
  DCHECK(net_selector_->IsInSelectThread());

  // check if we need to send the bootstrap begin FLV video stuff
  if ( bootstrapping_ && (video_codec_ == (streaming::FlvFlagVideoCodec)0) ) {
    if ( flv_tag->body().type() == streaming::FLV_FRAMETYPE_VIDEO ) {
      // ignore H264 sequence headers
      if ( flv_tag->video_body().video_codec() !=
          streaming::FLV_FLAG_VIDEO_CODEC_AVC ||
          flv_tag->video_body().video_avc_packet_type() !=
              streaming::AVC_SEQUENCE_HEADER ) {
        video_codec_ = flv_tag->video_body().video_codec();
        video_frame_type_ = flv_tag->video_body().video_frame_type();

        /* TODO: Clarify why is this corrupting display
        // send the bootstrap begin signal
        scoped_ref<streaming::FlvTag> video(
            new streaming::FlvTag(
                streaming::Tag::ATTR_VIDEO | streaming::Tag::ATTR_CAN_RESYNC,
                streaming::kDefaultFlavourMask,
                tag_timestamp_ms, streaming::FLV_FRAMETYPE_VIDEO));
        char d[2];
        d[0] = (video_frame_type_ << 4) | video_codec_;
        d[1] = 0x00;
        video.get()->mutable_video_body().append_data(d, 2);

        SendFlvTag(
            video.get(), tag_timestamp_ms);
        */
      }
    }
  }

  scoped_ref<BulkDataEvent> event;

  stats_keeper_.bytes_up_add(flv_tag->size());

  const MediaBuildState initial_media_build_state = media_build_state_;
  const io::MemoryStream* flv_tag_data = NULL;
  switch ( flv_tag->body().type() ) {
    case streaming::FLV_FRAMETYPE_AUDIO: {
      const streaming::FlvTag::Audio& audio_body = flv_tag->audio_body();
      flv_tag_data = &audio_body.data();
      event = new EventAudioData(connection_->mutable_protocol_data(),
                                       kChannelAudio,
                                       stream_id());
      if ( media_build_state_ == WAITING_AUDIO ) {
        if ( (tag_timestamp_ms - composed_media_data_tag_delta_) > 1000 ) {
          media_build_state_ = BUILDING_MEDIA;
        }
      }
      break;
    }
    case streaming::FLV_FRAMETYPE_VIDEO: {
      const streaming::FlvTag::Video& video_body = flv_tag->video_body();
      flv_tag_data = &video_body.data();
      if ( media_build_state_ == WAITING_VIDEO ) {
        if ( video_body.video_frame_type() !=
             streaming::FLV_FLAG_VIDEO_FRAMETYPE_KEYFRAME ) {
          RTMP_LOG_INFO << "Dropping flv_tag waiting for first media keyframe: "
                   << flv_tag->ToString();
          return;
        }
        composed_media_data_tag_delta_ = tag_timestamp_ms;
      }

      event = new EventVideoData(connection_->mutable_protocol_data(),
                                       kChannelVideo,
                                       stream_id());
      if ( media_build_state_ == WAITING_VIDEO &&
           video_body.video_frame_type() ==
               streaming::FLV_FLAG_VIDEO_FRAMETYPE_KEYFRAME &&
           !(video_body.video_codec() ==
               streaming::FLV_FLAG_VIDEO_CODEC_AVC &&
             video_body.video_avc_packet_type() ==
                 streaming::AVC_SEQUENCE_HEADER) ) {
        media_build_state_ = WAITING_AUDIO;
      }
      break;
    }
    case streaming::FLV_FRAMETYPE_METADATA: {
      event = new EventStreamMetadata(connection_->mutable_protocol_data(),
                                            kChannelMetadata,
                                            stream_id());
      flv_tag->metadata_body().Encode(event->mutable_data());
      break;
    }
  }

  //////////////////////////////////////////////////////////////////////

  if ( event.get() == NULL ) {
    RTMP_LOG_ERROR << "Cannot generate an event for: " << flv_tag->ToString();
    return;
  }

  // Force a write on the media event before a metadata to some the messups
  if ( media_event_.get() != NULL &&
       flv_tag->body().type() == streaming::FLV_FRAMETYPE_METADATA ) {
    RTMP_LOG_DEBUG << "Forcing a media event out: " << media_event_->ToString()
                   << " To make way for a METADATA: " << flv_tag->ToString();
    // Finalize it first ...
    SendMediaTag();
  }

  if ( flv_tag->body().type() != streaming::FLV_FRAMETYPE_METADATA &&
       initial_media_build_state == BUILDING_MEDIA &&
       (media_event_.get() != NULL ||
           flv_tag->body().type() != streaming::FLV_FRAMETYPE_VIDEO) &&
       connection_->flags().media_chunk_size_ms_ != 0 ) {
    event = NULL;
    if ( AppendToMediaTag(flv_tag, tag_timestamp_ms) ) {
      SendMediaTag();
    }
    return;
  }

  SendEvent(event.get(), tag_timestamp_ms, flv_tag_data, true);
}

bool PlayStream::AppendToMediaTag(const streaming::FlvTag* flv_tag,
                                  int64 tag_timestamp_ms) {
  DCHECK(net_selector_->IsInSelectThread());

  if ( media_event_.get() == NULL ) {
    media_event_ = new MediaDataEvent(
        new Header(connection_->mutable_protocol_data()));

    media_event_->mutable_header()->set_channel_id(kChannelMetadata);
    media_event_->mutable_header()->set_stream_id(stream_id());
    media_event_->mutable_header()->set_event_type(EVENT_MEDIA_DATA);
    media_event_->mutable_header()->set_event_size(0);

    media_event_timestamp_ms_ = tag_timestamp_ms;
    media_event_duration_ms_ = 0;
  } else {
    CHECK_GE(media_event_size_, 0);
    io::NumStreamer::WriteInt32(media_event_->mutable_data(),
                                media_event_size_, common::BIGENDIAN);
  }

  const int32 original_size = media_event_->data().Size();
  io::NumStreamer::WriteByte(media_event_->mutable_data(),
                             flv_tag->body().type());
  io::NumStreamer::WriteUInt24(media_event_->mutable_data(),
                               flv_tag->body().Size(), common::BIGENDIAN);
  const int32 timestamp = tag_timestamp_ms
                          - composed_media_data_tag_delta_;
  io::NumStreamer::WriteUInt24(media_event_->mutable_data(),
                               timestamp & 0xFFFFFF,
                               common::BIGENDIAN);
  io::NumStreamer::WriteByte(media_event_->mutable_data(),
                             (timestamp >> 24) & 0xFF);
  io::NumStreamer::WriteUInt24(media_event_->mutable_data(),
                               flv_tag->stream_id(), common::BIGENDIAN);
  flv_tag->body().Encode(media_event_->mutable_data());

  media_event_duration_ms_ = tag_timestamp_ms - media_event_timestamp_ms_;
  media_event_size_ = media_event_->data().Size() - original_size;

  if ( media_event_duration_ms_ < 0 ||
       media_event_duration_ms_ > connection_->flags().media_chunk_size_ms_ ||
       (flv_tag->body().type() == streaming::FLV_FRAMETYPE_VIDEO &&
        flv_tag->video_body().video_frame_type() ==
          streaming::FLV_FLAG_VIDEO_FRAMETYPE_KEYFRAME) ) {
    return true;
  }
  return false;
}

void PlayStream::SendMediaTag() {
  DCHECK(net_selector_->IsInSelectThread());
  CHECK_NOT_NULL(media_event_.get());

  // Finalize with a pure 0...
  io::NumStreamer::WriteInt32(media_event_->mutable_data(), 0,
                              common::BIGENDIAN);

  media_event_->mutable_header()->set_event_size(media_event_->data().Size());
  SendEvent(media_event_.get(), media_event_timestamp_ms_, NULL, true);

  media_event_ = NULL;
  media_event_size_ = 0;
  media_event_timestamp_ms_ = 0;
  media_event_duration_ms_ = 0;
}

void PlayStream::CloseInternal(bool dec_ref) {
  if ( !media_selector_->IsInSelectThread() ) {
    IncRef();
    media_selector_->RunInSelectLoop(NewCallback(this,
        &PlayStream::CloseInternal, true));
    return;
  }
  Stream::AutoDecRef auto_dec_ref(dec_ref ? this : NULL);

  CloseRequest("CLOSED");

  seek_alarm_.Stop();

  media_event_ = NULL;

  connection_->CloseConnection();
}

} // namespace rtmp
