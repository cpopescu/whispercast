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

#include <whisperlib/common/base/gflags.h>

#include <vector>
#include "stream_request.h"
#include <whisperlib/net/util/ipclassifier.h>

#include <whisperstreamlib/internal/internal_frame.h>

#include <whisperstreamlib/aac/aac_tag_splitter.h>
#include <whisperstreamlib/flv/flv_tag_splitter.h>
#include <whisperstreamlib/mp3/mp3_tag_splitter.h>
#include <whisperstreamlib/internal/internal_tag_splitter.h>
#include <whisperstreamlib/raw/raw_tag_splitter.h>
#include <whisperstreamlib/f4v/f4v_encoder.h>

DECLARE_string(http_base_media_path);

//////////////////////////////////////////////////////////////////////

DECLARE_int32(http_port);
DECLARE_string(http_default_content_type);

DEFINE_int32(http_max_write_ahead_ms,
             5000,
             "We keep the HTTP stream "
             "at most this much ahead of the real time");

DEFINE_int32(http_connection_log_level,
             1,
             "The log level of messages for RTMP connections");

#define HTTP_LOG(level) if ( FLAGS_http_connection_log_level < level ); \
                        else LOG(level) << "INFO " << ": "
#define HTTP_DLOG(level) if ( FLAGS_http_connection_log_level < level ); \
                         else DLOG(level) << "INFO " << ": "

#define HTTP_LOG_DEBUG   HTTP_LOG(LDEBUG)
#define HTTP_LOG_INFO    HTTP_LOG(LINFO)
#define HTTP_LOG_WARNING HTTP_LOG(LWARNING)
#define HTTP_LOG_ERROR   HTTP_LOG(LERROR)
#define HTTP_LOG_FATAL   HTTP_LOG(LFATAL)

#define HTTP_DLOG_DEBUG   HTTP_DLOG(LDEBUG)
#define HTTP_DLOG_INFO    HTTP_DLOG(LINFO)
#define HTTP_DLOG_WARNING HTTP_DLOG(LWARNING)
#define HTTP_DLOG_ERROR   HTTP_DLOG(LERROR)
#define HTTP_DLOG_FATAL   HTTP_DLOG(LFATAL)

//////////////////////////////////////////////////////////////////////

StreamRequest::StreamRequest(int64 connection_id,
    net::Selector* media_selector,
    streaming::ElementMapper* element_mapper,
    streaming::StatsCollector* stats_collector,
    http::ServerRequest* http_request)
    : streaming::ExporterT(
          "http",
          element_mapper,
          media_selector,
          http_request->net_selector(),
          stats_collector,
          FLAGS_http_max_write_ahead_ms),
      ref_count_(0),
      http_request_(http_request),
      serializer_(NULL) {
  connection_begin_stats_.connection_id_ =
      strutil::StringPrintf("%"PRId64"", (connection_id));
  connection_begin_stats_.timestamp_utc_ms_ = timer::Date::Now();
  connection_begin_stats_.remote_host_ =
      http_request_->remote_address().ip_object().ToString();
  connection_begin_stats_.remote_port_ =
      http_request_->remote_address().port();
  connection_begin_stats_.local_host_ = "127.0.0.1";
  connection_begin_stats_.local_port_ = FLAGS_http_port;
  connection_begin_stats_.protocol_ = "http";

  connection_end_stats_.connection_id_ = connection_begin_stats_.connection_id_;
  connection_end_stats_.timestamp_utc_ms_ =
      connection_begin_stats_.timestamp_utc_ms_;
  connection_end_stats_.bytes_up_ = 0;
  connection_end_stats_.bytes_down_ = 0;
  connection_end_stats_.result_ = ConnectionResult("ACTIVE");

  stats_collector_->StartStats(&connection_begin_stats_,
                               &connection_end_stats_);
}

StreamRequest::~StreamRequest() {
  delete serializer_;

  connection_end_stats_.timestamp_utc_ms_ = timer::Date::Now();
  connection_end_stats_.result_.ref().result_ = "CLOSE";

  stats_collector_->EndStats(&connection_end_stats_);
}

//////////////////////////////////////////////////////////////////////

void StreamRequest::Play(const char* content_type) {
  URL* const url = http_request_->request()->url();
  CHECK(url != NULL);

  request_ = new streaming::Request(*url);

  request_->mutable_info()->remote_address_ = http_request_->remote_address();
  request_->mutable_info()->local_address_ =
      net::HostPort("127.0.0.1", FLAGS_http_port);

  request_->mutable_info()->auth_req_.net_address_ =
      http_request_->remote_address().ToString();
  request_->mutable_info()->auth_req_.resource_ = url->spec();
  request_->mutable_info()->auth_req_.action_ = streaming::kActionView;

  if ( request_->mutable_info()->auth_req_.token_.empty() ) {
    request_->mutable_info()->auth_req_.token_ =
      http_request_->request()->client_header()->FindField(http::kHeaderCookie);
  }
  if ( request_->mutable_info()->auth_req_.user_.empty() ) {
    http_request_->request()->client_header()->GetAuthorizationField(
        &request_->mutable_info()->auth_req_.user_,
        &request_->mutable_info()->auth_req_.passwd_);
  }

  if ( content_type != NULL ) {
    request_->mutable_caps()->tag_type_ =
        streaming::GetStreamTypeFromContentType(content_type);

    http_request_->request()->server_header()->AddField(
        http::kHeaderContentType, content_type, true);
  } else {
    http_request_->request()->server_header()->AddField(
        http::kHeaderContentType, FLAGS_http_default_content_type, true);
  }

  StartRequest(url->UrlUnescape(url->path().c_str(), url->path().size()).
      substr(FLAGS_http_base_media_path.size()).c_str(), 0);
}

//////////////////////////////////////////////////////////////////////

void StreamRequest::AuthorizeCompleted(int64 seek_time_ms,
    streaming::Authorizer* authorizer) {
  streaming::ExporterT::AuthorizeCompleted(seek_time_ms,
      authorizer);

  // if not abandoned, create the serializer
  if (request_ != NULL ) {
    switch ( request_->caps().tag_type_ ) {
      case streaming::Tag::TYPE_MP3:
        serializer_ = new streaming::Mp3TagSerializer();
        break;
      case streaming::Tag::TYPE_FLV:
        serializer_ = new streaming::FlvTagSerializer();
        break;
      case streaming::Tag::TYPE_F4V:
        serializer_ = new streaming::F4vTagSerializer();
        break;
      case streaming::Tag::TYPE_AAC:
        serializer_ = new streaming::AacTagSerializer();
        break;
      case streaming::Tag::TYPE_INTERNAL:
        serializer_ = new streaming::InternalTagSerializer();
        break;
      default:
        serializer_ = new streaming::RawTagSerializer();
    }
  }
}

void StreamRequest::OnStreamNotFound() {
  HTTP_LOG_DEBUG << "Notifying STREAM NOT FOUND";

  IncRef();
  net_selector_->RunInSelectLoop(
      NewCallback(this, &StreamRequest::MediaRequestClosedCallback,
          http::NOT_FOUND));
}
void StreamRequest::OnTooManyClients() {
  HTTP_LOG_DEBUG << "Notifying TOO MANY CLIENTS";

  IncRef();
  net_selector_->RunInSelectLoop(
      NewCallback(this, &StreamRequest::MediaRequestClosedCallback,
          http::NOT_ACCEPTABLE));
}
void StreamRequest::OnAuthorizationFailed() {
  HTTP_LOG_DEBUG << "Notifying AUTHORIZATION FAILED";

  IncRef();
  net_selector_->RunInSelectLoop(
      NewCallback(this, &StreamRequest::MediaRequestClosedCallback,
          http::UNAUTHORIZED));
}
void StreamRequest::OnReauthorizationFailed() {
  HTTP_LOG_DEBUG << "Notifying REAUTHORIZATION FAILED";

  IncRef();
  net_selector_->RunInSelectLoop(
      NewCallback(this, &StreamRequest::MediaRequestClosedCallback,
          http::UNAUTHORIZED));
}
void StreamRequest::OnAddRequestFailed() {
  HTTP_LOG_DEBUG << "Notifying ADD REQUEST FAILED";

  IncRef();
  net_selector_->RunInSelectLoop(
      NewCallback(this, &StreamRequest::MediaRequestClosedCallback,
          http::NOT_FOUND));
}

void StreamRequest::OnPlay() {
  HTTP_LOG_DEBUG << "Notifying PLAY";
}
void StreamRequest::OnTerminate(const char* reason) {
  HTTP_LOG_DEBUG << "Notifying " << reason;

  // we're assuming reauthorization time-out
  net_selector_->RunInSelectLoop(
      NewCallback(this, &StreamRequest::MediaRequestClosedCallback,
          http::UNAUTHORIZED));
}

void StreamRequest::ResumeLocalizedTags() {
  DCHECK(net_selector_->IsInSelectThread());
  ProcessLocalizedTags(false);
}
bool StreamRequest::CanSendTag() const {
  // check if we have enough outbuf space
  if ( http_request_ == NULL ) {
    return false;
  }
  return http_request_->pending_output_bytes() <
         http_request_->protocol_params().max_reply_buffer_size_/2;
}
void StreamRequest::SetNotifyReady() {
  if ( http_request_ == NULL ) {
    return;
  }
  HTTP_LOG_INFO << "SetNotifyReady outbuf_size: "
                << http_request_->pending_output_bytes();
  http_request_->set_ready_callback(
      NewCallback(this, &StreamRequest::ResumeLocalizedTags),
      http_request_->protocol_params().max_reply_buffer_size_/4);
}
void StreamRequest::SendTag(const streaming::Tag* tag, int64 tag_timestamp_ms) {
  DCHECK(net_selector_->IsInSelectThread());

  if ( http_request_ == NULL ) {
    return;
  }

  if ( tag->type() == streaming::Tag::TYPE_COMPOSED ) {
    const streaming::ComposedTag* composed_tag =
        static_cast<const streaming::ComposedTag*>(tag);
    for ( int i = 0;
        i < composed_tag->tags().size() && (http_request_ != NULL); ++i ) {
      const streaming::Tag* ltag = composed_tag->tags().tag(i).get();
      SendSimpleTag(ltag, tag_timestamp_ms + ltag->timestamp_ms());
    }
  } else {
    SendSimpleTag(tag, tag_timestamp_ms);
  }

  if ( http_request_ == NULL ) {
    return;
  }

  if ( http_request_->is_server_streaming() ) {
    http_request_->ContinueStreamingData();
  }
}

//////////////////////////////////////////////////////////////////////

void StreamRequest::SendSimpleTag(const streaming::Tag* tag,
    int64 tag_timestamp_ms) {
  DCHECK(net_selector_->IsInSelectThread());

  if ( http_request_ == NULL ) {
    return;
  }

  if ( tag->type() == streaming::Tag::TYPE_EOS ) {
    if ( http_request_->is_server_streaming() ) {
      http_request_->EndStreamingData();
    } else {
      http_request_->ReplyWithStatus(http::NO_CONTENT);
    }
    HttpRequestClosedCallback();
    return;
  }

  if ( !http_request_->is_server_streaming() ) {
    http_request_->BeginStreamingData(http::OK, NewCallback(this,
        &StreamRequest::HttpRequestClosedCallback), true);

    serializer_->Initialize(http_request_->request()->server_data());
  }

  serializer_->Serialize(tag,
                         http_request_->request()->server_data(),
                         tag_timestamp_ms);
}

//////////////////////////////////////////////////////////////////////

void StreamRequest::MediaRequestClosedCallback(http::HttpReturnCode reason) {
  // called on stream not found / stream error (streaming event)
  DCHECK(net_selector_->IsInSelectThread());

  if ( http_request_ != NULL ) {
    if ( !http_request_->is_server_streaming() ) {
      http_request_->ReplyWithStatus(reason);
    } else {
      http_request_->EndStreamingData();
    }
    HttpRequestClosedCallback();
  }
  DecRef();
}
void StreamRequest::HttpRequestClosedCallback() {
  // called when http client disconnected (network event)
  DCHECK(net_selector_->IsInSelectThread());

  http_request_ = NULL;

  IncRef();
  media_selector_->RunInSelectLoop(
      NewCallback(this, &StreamRequest::HandleEosInternal,
          eos_seen_ ? "EOS" : "END"));
}

void StreamRequest::HandleEosInternal(const char* reason) {
  HandleEos(reason);
  DecRef();
}
