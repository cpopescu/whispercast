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

//////////////////////////////////////////////////////////////////////

synch::Mutex g_sync_stream_request_count;
int64 g_stream_request_count = 0;
void IncStreamRequestCount() {
  synch::MutexLocker lock(&g_sync_stream_request_count);
  g_stream_request_count++;
  LOG_WARNING << "++StreamRequest " << g_stream_request_count;
}
void DecStreamRequestCount() {
  synch::MutexLocker lock(&g_sync_stream_request_count);
  g_stream_request_count--;
  LOG_WARNING << "--StreamRequest " << g_stream_request_count;
}

StreamRequest::StreamRequest(int64 connection_id,
    net::Selector* media_selector,
    streaming::ElementMapper* element_mapper,
    streaming::StatsCollector* stats_collector,
    http::ServerRequest* http_request)
    : streaming::Exporter(
          "http",
          element_mapper,
          media_selector,
          http_request->net_selector(),
          stats_collector,
          FLAGS_http_max_write_ahead_ms),
      ref_count_(0),
      http_request_(http_request),
      serializer_(NULL) {
  IncStreamRequestCount();
  CHECK(net_selector_->IsInSelectThread());
  connection_begin_stats_.connection_id_ =
      strutil::StringPrintf("%"PRId64"", connection_id);
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
  DecStreamRequestCount();
  CHECK(net_selector_->IsInSelectThread());

  delete serializer_;
  serializer_ = NULL;

  connection_end_stats_.timestamp_utc_ms_ = timer::Date::Now();
  connection_end_stats_.result_.ref().result_ = "CLOSE";

  stats_collector_->EndStats(&connection_end_stats_);
}

//////////////////////////////////////////////////////////////////////

void StreamRequest::Play(string content_type, bool dec_ref) {
  if ( !media_selector_->IsInSelectThread() ) {
    IncRef();
    media_selector_->RunInSelectLoop(NewCallback(this,
        &StreamRequest::Play, content_type, true));
    return;
  }
  AutoDecRef auto_dec_ref(dec_ref ? this : NULL);

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

  request_->mutable_caps()->tag_type_ =
      streaming::GetStreamTypeFromContentType(content_type);

  http_request_->request()->server_header()->AddField(
      http::kHeaderContentType, content_type, true);

  // HTTP network protocol created
  IncRef(); // will DecRef() on HTTP disconnect notification

  StartRequest(url->UrlUnescape(url->path().c_str(), url->path().size()).
      substr(FLAGS_http_base_media_path.size()).c_str());
}

//////////////////////////////////////////////////////////////////////

void StreamRequest::OnStreamNotFound() {
  VLOG(LDEBUG) << "Notifying STREAM NOT FOUND";
  CloseHttpRequest(http::NOT_FOUND);
}
void StreamRequest::OnTooManyClients() {
  VLOG(LDEBUG) << "Notifying TOO MANY CLIENTS";
  CloseHttpRequest(http::NOT_ACCEPTABLE);
}
void StreamRequest::OnAuthorizationFailed(bool is_reauthorization) {
  VLOG(LDEBUG) << "Notifying AUTHORIZATION FAILED";
  CloseHttpRequest(http::UNAUTHORIZED);
}
void StreamRequest::OnAddRequestFailed() {
  VLOG(LDEBUG) << "Notifying ADD REQUEST FAILED";
  CloseHttpRequest(http::NOT_FOUND);
}

void StreamRequest::OnPlay() {
  VLOG(LDEBUG) << "Notifying PLAY";

  // if not abandoned, create the serializer. Now we know the tag_type_
  CHECK_NULL(serializer_);
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

void StreamRequest::ResumeLocalizedTags() {
  DCHECK(net_selector_->IsInSelectThread());
  ProcessLocalizedTags();
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
  VLOG(LINFO) << "SetNotifyReady outbuf_size: "
              << http_request_->pending_output_bytes();
  http_request_->set_ready_callback(
      NewCallback(this, &StreamRequest::ResumeLocalizedTags),
      http_request_->protocol_params().max_reply_buffer_size_/4);
}
void StreamRequest::SendTag(const streaming::Tag* tag, int64 timestamp_ms) {
  DCHECK(net_selector_->IsInSelectThread());

  if ( http_request_ == NULL ) {
    return;
  }

  if ( tag->type() == streaming::Tag::TYPE_COMPOSED ) {
    /*
    const streaming::ComposedTag* composed_tag =
        static_cast<const streaming::ComposedTag*>(tag);
    for ( int i = 0;
        i < composed_tag->tags().size() && (http_request_ != NULL); ++i ) {
      const streaming::Tag* ltag = composed_tag->tags().tag(i).get();
      SendSimpleTag(ltag, tag_timestamp_ms + ltag->timestamp_ms());
    }
    */
  } else {
    SendSimpleTag(tag, timestamp_ms);
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
    int64 timestamp_ms) {
  DCHECK(net_selector_->IsInSelectThread());

  if ( http_request_ == NULL ) {
    return;
  }

  if ( tag->type() == streaming::Tag::TYPE_EOS ) {
    CloseHttpRequest(http::NO_CONTENT);
    HandleEosInternal("EOS");
    return;
  }

  if ( !http_request_->is_server_streaming() ) {
    http_request_->BeginStreamingData(http::OK, NewCallback(this,
        &StreamRequest::HttpRequestClosedCallback), true);

    serializer_->Initialize(http_request_->request()->server_data());
  }

  serializer_->Serialize(tag,
                         timestamp_ms,
                         http_request_->request()->server_data());

}

//////////////////////////////////////////////////////////////////////

void StreamRequest::CloseHttpRequest(http::HttpReturnCode rcode,
    bool dec_ref) {
  // called on stream not found / stream error (streaming event)
  if ( !net_selector_->IsInSelectThread() ) {
    IncRef();
    net_selector_->RunInSelectLoop(NewCallback(this,
        &StreamRequest::CloseHttpRequest, rcode, true));
    return;
  }
  AutoDecRef auto_dec_ref(dec_ref ? this : NULL);

  if ( http_request_ != NULL ) {
    if ( http_request_->is_server_streaming() ) {
      http_request_->EndStreamingData();
    } else {
      http_request_->ReplyWithStatus(rcode);
    }
    http_request_ = NULL;
  }

  // HTTP network protocol completely closed
  DecRef();
}
void StreamRequest::HttpRequestClosedCallback() {
  // called when http client disconnected (network event)
  DCHECK(net_selector_->IsInSelectThread());

  http_request_ = NULL;

  HandleEosInternal("END");

  // HTTP network protocol completely closed
  DecRef();
}

void StreamRequest::HandleEosInternal(const char* reason, bool dec_ref) {
  if ( !media_selector_->IsInSelectThread() ) {
    IncRef();
    media_selector_->RunInSelectLoop(NewCallback(this,
        &StreamRequest::HandleEosInternal, reason, true));
    return;
  }
  AutoDecRef auto_dec_ref(dec_ref ? this : NULL);

  CloseRequest(reason);
}
