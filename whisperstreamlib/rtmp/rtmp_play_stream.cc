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
#include <whisperstreamlib/base/media_info_util.h>
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
      handle_stream_not_found_alarm_(*net_selector_),
      f4v2flv_(),
      f4v_cue_point_number_(0),
      dropping_interframes_(true),
      first_tag_ts_(-1),
      first_media_segment_(true) {
}

PlayStream::~PlayStream() {
  CHECK_NULL(request_);
}

void PlayStream::NotifyOutbufEmpty(int32 outbuf_size) {
  DCHECK(net_selector_->IsInSelectThread());
  ProcessLocalizedTags();
}

bool PlayStream::ProcessEvent(const Event* event, int64 timestamp_ms) {
  CHECK(net_selector_->IsInSelectThread());

  if ( event->event_type() == EVENT_INVOKE ) {
    RTMP_LOG_INFO << "INVOKE - " << event->ToString();
    const EventInvoke* invoke = static_cast<const EventInvoke *>(event);
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
    const EventFlexMessage* flex = static_cast<const EventFlexMessage *>(event);
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

  // reject missing stream without trying AddRequest()
  if ( IsMissingStream(stream_name) ) {
    stream_name_ = stream_name;
    handle_stream_not_found_alarm_.Set(
        NewPermanentCallback(this,&PlayStream::HandleStreamNotFound),
        true, connection_->flags().reject_delay_ms_, false, true);
    return true;
  }

  return ProcessPlay(stream_name);
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

  return ProcessPlay(stream_name);
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

bool PlayStream::ProcessPlay(const string& stream_name) {
  CHECK(net_selector_->IsInSelectThread());
  if ( state_ != STATE_CREATED ) {
    return false;
  }

  HandlePlay(stream_name);
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

void PlayStream::HandlePlay(string stream_name, bool dec_ref) {
  if ( !media_selector_->IsInSelectThread() ) {
    IncRef();
    media_selector_->RunInSelectLoop(NewCallback(this, &PlayStream::HandlePlay,
        stream_name, true));
    return;
  }
  Stream::AutoDecRef auto_dec_ref(dec_ref ? this : NULL);
  CHECK_NULL(request_);

  stream_name_ = stream_name;

  URL full_url(strutil::JoinPaths(params().application_url_, stream_name));

  request_ = new streaming::Request(full_url);
  request_->mutable_info()->remote_address_ = connection_->remote_address();
  request_->mutable_info()->local_address_ = connection_->local_address();

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

    SendClearBuffer();
    SendResetPings(-1);

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
    return;
  }

  seeking_ = true;
  RTMP_LOG_INFO << "Seeking to " << seek_time_ms;
  controller->Seek(seek_time_ms);

  const bool was_paused = (state_ == STATE_PAUSED);
  set_state(STATE_PLAYING);

  if ( was_paused ) {
    controller->Pause(false);
  }
}

void PlayStream::HandleStreamNotFound() {
  SendEvent(
    connection_->CreateStatusEvent(
        stream_id(), kChannelReply, 0,
        "NetStream.Play.StreamNotFound", "Export not found",
        stream_name_.c_str()).get(), -1, NULL, true);
  CloseInternal();
}

//////////////////////////////////////////////////////////////////////

void PlayStream::OnStreamNotFound() {
  RTMP_LOG_DEBUG << "Notifying STREAM NOT FOUND: [" << stream_name_ << "]";

  AddMissingStream(stream_name_);
  HandleStreamNotFound();
}
void PlayStream::OnTooManyClients() {
  RTMP_LOG_DEBUG << "Notifying TOO MANY CLIENTS";

  SendEvent(
    connection_->CreateStatusEvent(
        stream_id(), kChannelReply, 0,
        "NetStream.Play.StreamNotFound", "Too many requests",
        stream_name_.c_str()).get(), -1, NULL, true);
  CloseInternal();
}
void PlayStream::OnAuthorizationFailed() {
  RTMP_LOG_DEBUG << "Notifying AUTHORIZATION FAILED";

  SendEvent(
    connection_->CreateStatusEvent(
        stream_id(), kChannelReply, 0,
        "NetStream.Play.StreamNotFound", "Authorization failed",
        stream_name_.c_str()).get(), -1, NULL, true);
  CloseInternal();
}
void PlayStream::OnAddRequestFailed() {
  RTMP_LOG_DEBUG << "Notifying ADD REQUEST FAILED";

  AddMissingStream(stream_name_);
  SendEvent(
    connection_->CreateStatusEvent(
        stream_id(), kChannelReply, 0,
        "NetStream.Play.StreamNotFound", "Stream not found",
        stream_name_.c_str()).get(), -1, NULL, true);
  CloseInternal();
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
    return;
  }
  if ( tag->type() == streaming::Tag::TYPE_SOURCE_ENDED ) {
    //SendSwitchMedia(tag_timestamp_ms);
    return;
  }
  if ( tag->type() == streaming::Tag::TYPE_SEEK_PERFORMED ) {
    if ( seeking_ ) {
      seeking_ = false;
      // the pings are going through stream 0 (system stream)...
      SendClearBuffer();
      SendResetPings(tag_timestamp_ms);

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

  if ( tag->type() == streaming::Tag::TYPE_MEDIA_INFO ) {
    const streaming::MediaInfoTag* info_tag =
        static_cast<const streaming::MediaInfoTag*>(tag);
    scoped_ref<streaming::FlvTag> flv_tag;
    if ( !streaming::util::ComposeFlv(info_tag->info(), &flv_tag) ) {
      LOG_ERROR << "Failed to compose Metadata from MediaInfo: "
                << info_tag->info().ToString()
                << " . Closing connection.";
      Close();
      return;
    }
    // send reset events before metadata! because these events reset
    // all stream properties (set by any previous metadata).
    if ( !first_media_segment_ ) {
      SendSwitchMedia(tag_timestamp_ms);
    }
    first_media_segment_ = false;
    SendResetAudio(tag_timestamp_ms);
    SendResetPings(tag_timestamp_ms);
    SendFlvTag(flv_tag.get(), tag_timestamp_ms);
    return;
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

  scoped_ref<BulkDataEvent> event;

  stats_keeper_.bytes_up_add(flv_tag->size());

  const io::MemoryStream* flv_tag_data = NULL;
  switch ( flv_tag->body().type() ) {
    case streaming::FLV_FRAMETYPE_AUDIO: {
      const streaming::FlvTag::Audio& audio_body = flv_tag->audio_body();
      flv_tag_data = &audio_body.data();
      event = new EventAudioData(
          Header(kChannelAudio, stream_id(), EVENT_AUDIO_DATA, 0, false));
      break;
    }
    case streaming::FLV_FRAMETYPE_VIDEO: {
      const streaming::FlvTag::Video& video_body = flv_tag->video_body();
      flv_tag_data = &video_body.data();
      if ( dropping_interframes_ && video_body.frame_type() !=
               streaming::FLV_FLAG_VIDEO_FRAMETYPE_KEYFRAME ) {
        RTMP_LOG_INFO << "Dropping flv_tag waiting for first media keyframe: "
                      << flv_tag->ToString();
        return;
      }
      dropping_interframes_ = false;
      event = new EventVideoData(
          Header(kChannelVideo, stream_id(), EVENT_VIDEO_DATA, 0, false));
      if ( first_tag_ts_ == -1 && video_body.frame_type() ==
               streaming::FLV_FLAG_VIDEO_FRAMETYPE_KEYFRAME ) {
        first_tag_ts_ = tag_timestamp_ms;
      }
      break;
    }
    case streaming::FLV_FRAMETYPE_METADATA: {
      event = new EventStreamMetadata(Header(kChannelMetadata, stream_id(),
          EVENT_STREAM_METAEVENT, 0, false));
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
  if ( flv_tag->body().type() == streaming::FLV_FRAMETYPE_METADATA ) {
    if ( media_event_.get() != NULL ) {
      RTMP_LOG_DEBUG << "Forcing a media event out: " << media_event_->ToString()
                     << " to make way for a METADATA: " << flv_tag->ToString();
      // Finalize it first ...
      SendMediaTag();
    }
    // send Metadata
    SendEvent(event.get(), tag_timestamp_ms, flv_tag_data, true);
    return;
  }

  if ( connection_->flags().media_chunk_size_ms_ > 0 &&
       tag_timestamp_ms - first_tag_ts_ > 1000 ) {
    // accumulate & send MediaDataEvent
    if ( AppendToMediaTag(flv_tag, tag_timestamp_ms) ) {
      SendMediaTag();
    }
    return;
  }

  // send a simple event
  SendEvent(event.get(), tag_timestamp_ms, flv_tag_data, true);
}

bool PlayStream::AppendToMediaTag(const streaming::FlvTag* flv_tag,
                                  int64 tag_timestamp_ms) {
  DCHECK(net_selector_->IsInSelectThread());

  if ( media_event_.get() == NULL ) {
    media_event_ = new MediaDataEvent(Header(kChannelMetadata, stream_id(),
        EVENT_MEDIA_DATA, 0, false));
  }
  media_event_->EncodeAddTag(*flv_tag, tag_timestamp_ms);

  return media_event_->duration_ms() > connection_->flags().media_chunk_size_ms_ ||
         (flv_tag->body().type() == streaming::FLV_FRAMETYPE_VIDEO &&
          flv_tag->video_body().frame_type() ==
            streaming::FLV_FLAG_VIDEO_FRAMETYPE_KEYFRAME) ;
}

void PlayStream::SendMediaTag() {
  DCHECK(net_selector_->IsInSelectThread());
  CHECK_NOT_NULL(media_event_.get());

  media_event_->EncodeFinalize();
  SendEvent(media_event_.get(), media_event_->first_tag_ts(), NULL, true);

  media_event_ = NULL;
}

void PlayStream::SendResetAudio(int64 ts) {
  // send an empty audio tag
  scoped_ref<streaming::FlvTag> audio(
      new streaming::FlvTag(
          streaming::Tag::ATTR_AUDIO | streaming::Tag::ATTR_CAN_RESYNC,
          streaming::kDefaultFlavourMask,
          0, streaming::FLV_FRAMETYPE_AUDIO));
  SendFlvTag(audio.get(), ts);
}

void PlayStream::SendResetPings(int64 ts) {
  // the pings are going through stream 0 (system stream)...
  SendEventOnSystemStream(
      connection_->CreateResetPing(0, stream_id()).get(),
      -1, NULL, true);
  SendEventOnSystemStream(
      connection_->CreateClearPing(0, stream_id()).get(),
      -1,  NULL, true);
}

void PlayStream::SendClearBuffer() {
  SendEventOnSystemStream(
      connection_->CreateClearBufferPing(0, stream_id()).get(),
      -1, NULL, true);
}

void PlayStream::SendSwitchMedia(int64 ts) {
  SendEvent(
      connection_->CreateStatusEvent(
        stream_id(),
        kChannelReply,
        0,
        "NetStream.Play.Complete", "SWITCH",
        stream_name_).get(),
        ts,  NULL, true);
  SendEvent(
      connection_->CreateStatusEvent(
        stream_id(),
        kChannelReply,
        0,
        "NetStream.Play.Switch", "SWITCH",
        stream_name_).get(),
        ts,  NULL, true);
}

void PlayStream::CloseInternal(bool dec_ref) {
  if ( !media_selector_->IsInSelectThread() ) {
    IncRef();
    media_selector_->RunInSelectLoop(NewCallback(this,
        &PlayStream::CloseInternal, true));
    return;
  }
  Stream::AutoDecRef auto_dec_ref(dec_ref ? this : NULL);

  // close some things in media selector
  CloseRequest("CLOSED");
  seek_alarm_.Stop();
  media_event_ = NULL;

  // close other things in net selector
  CloseInternalNet();
}

void PlayStream::CloseInternalNet(bool dec_ref) {
  if ( !net_selector_->IsInSelectThread() ) {
    IncRef();
    net_selector_->RunInSelectLoop(NewCallback(this,
        &PlayStream::CloseInternalNet, true));
    return;
  }
  Stream::AutoDecRef auto_dec_ref(dec_ref ? this : NULL);

  connection_->CloseConnection();
}
void PlayStream::AddMissingStream(const string& stream_name) {
  RTMP_LOG_WARNING << "Caching missing stream: [" << stream_name << "]";
  connection_->missing_stream_cache()->Add(stream_name, true, false);
}
bool PlayStream::IsMissingStream(const string& stream_name) {
  return connection_->missing_stream_cache()->Get(stream_name);
}


} // namespace rtmp
