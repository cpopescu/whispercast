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

#include <whisperstreamlib/aac/aac_tag_splitter.h>
#include <whisperstreamlib/flv/flv_tag_splitter.h>
#include <whisperstreamlib/mp3/mp3_tag_splitter.h>
#include <whisperstreamlib/internal/internal_tag_splitter.h>
#include <whisperstreamlib/raw/raw_tag_splitter.h>

#include "elements/standard_library/http_poster/http_poster_element.h"

namespace streaming {

HttpPosterElement::HttpPosterElement(
    const char* name,
    const char* id,
    ElementMapper* mapper,
    net::Selector* selector,
    const Host2IpMap* host_aliases,
    const char* media_name,
    const char* server_name,
    const uint16 server_port,
    const char* url_escaped_query_path,
    Tag::Type tag_type,
    const char* user_name,
    const char* password,
    int32 max_buffer_size,
    int32 desired_http_chunk_size,
    int64 media_retry_timeout_ms,
    int64 http_retry_timeout_ms,
    const http::ClientParams* http_client_params)
    : Element(kElementClassName, name, id, mapper),
      selector_(selector),
      net_factory_(selector_),
      host_aliases_(host_aliases),
      media_name_(media_name),
      server_name_(server_name),
      server_port_(server_port),
      url_escaped_query_path_(url_escaped_query_path),
      tag_type_(tag_type),
      remote_user_name_(user_name),
      remote_password_(password),
      max_buffer_size_(max_buffer_size),
      desired_http_chunk_size_(desired_http_chunk_size),
      media_retry_timeout_ms_(media_retry_timeout_ms),
      http_retry_timeout_ms_(http_retry_timeout_ms),
      http_client_params_(http_client_params != NULL
                          ? http_client_params : &http_default_params_),
      serializer_(NULL),
      http_process_callback_(
          NewPermanentCallback(
              this, &HttpPosterElement::ProcessHttp)),
       media_process_callback_(
           NewPermanentCallback(
               this, &HttpPosterElement::ProcessTag)),
      retry_callback_(
          NewPermanentCallback(
              this, &HttpPosterElement::StartRequest)),
      req_(NULL),
      http_req_(NULL),
      http_protocol_(NULL),
      paused_(false),
      dropping_audio_(false),
      dropping_video_(false) {
  http_default_params_.default_request_timeout_ms_ = 0;
  http_default_params_.max_chunk_size_ = 2048;
  // http_default_params_.dlog_level_ = true;
}

const char HttpPosterElement::kElementClassName[] = "http_poster";

HttpPosterElement::~HttpPosterElement() {
  CloseRequest(-1);
  selector_->UnregisterAlarm(retry_callback_);
  delete http_process_callback_;
  delete retry_callback_;
  delete media_process_callback_;
  delete serializer_;
}

bool HttpPosterElement::Initialize() {
  CreateSerializer();
  selector_->RegisterAlarm(retry_callback_, 0);
  return true;
}

bool HttpPosterElement::AddRequest(const char* media,
                                   streaming::Request* req,
                                   streaming::ProcessingCallback* callback) {
  return false;
}

void HttpPosterElement::RemoveRequest(streaming::Request* req) {
}

bool HttpPosterElement::HasMedia(const char* media, Capabilities* out) {
  return false;
}

void HttpPosterElement::ListMedia(const char* media_dir,
                                  streaming::ElementDescriptions* medias) {
}
bool HttpPosterElement::DescribeMedia(const string& media,
                                      MediaInfoCallback* callback) {
  // the HttpPosterElement does not provide any stream
  return false;
}


void HttpPosterElement::Close(Closure* call_on_close) {
  CloseRequest(-1);  // no retry
  call_on_close->Run();
}

void HttpPosterElement::ProcessTag(const Tag* tag) {
  if ( tag->type() == streaming::Tag::TYPE_EOS ) {
    CloseRequest(media_retry_timeout_ms_);
    return;
  }
  // TODO [cpopescu]: add flow control option (similar to protocol)
  if ( out_.Size() > max_buffer_size_ && tag->is_droppable() ) {
    if ( tag->is_video_tag() ) {
      if ( !dropping_video_ ) {
        LOG_WARNING << name() << " publisher out of buffer size: "
                    << out_.Size() << " dropping video.. ";
      }
      dropping_video_ = true;
    } else if ( tag->is_audio_tag() ) {
      if ( !dropping_audio_ ) {
        LOG_WARNING << name() << " publisher out of buffer size: "
                    << out_.Size() << " dropping audio.. ";
      }
      dropping_audio_ = true;
    }
    return;
  }
  if ( dropping_video_ && tag->is_droppable() && tag->is_video_tag() ) {
    if ( !tag->can_resync() )  return;
    dropping_video_ = false;
    LOG_INFO << name() << " stopped dropping video @" << out_.Size();
  } else if ( dropping_audio_ && tag->is_droppable() && tag->is_audio_tag() ) {
    if ( !tag->can_resync() ) return;
    LOG_INFO << name() << " stopped dropping audio @" << out_.Size();
    dropping_audio_ = false;
  }
  CHECK(http_protocol_ != NULL);
  serializer_->Serialize(tag, &out_);
  if ( paused_ && out_.Size() > 8000 ) {
    paused_ = false;
    http_protocol_->ResumeWriting();
  }
}

void HttpPosterElement::StartRequest() {
  // run the request
  LOG_INFO << name() << " - Starting to publishing POST to ["
           << " http://" << server_name_ << ":" << server_port_
           << url_escaped_query_path_ << "].";

  CHECK(req_ == NULL &&
        http_req_ == NULL &&
        http_protocol_ == NULL);

  net::IpAddress ip(server_name_.c_str());
  if ( ip.IsInvalid() ) {
    if ( host_aliases_ != NULL ) {
      HttpPosterElement::Host2IpMap::const_iterator
          it = host_aliases_->find(server_name_.c_str());
      if ( it != host_aliases_->end() ) {
        ip = net::IpAddress(it->second.c_str());
      }
    }
  }
  if ( ip.IsInvalid() ) {
    LOG_ERROR << name() << " Invalid host to connect to: [" << server_name_
              << "] and no aliases defined";
    if ( !selector_->IsExiting() ) {
      CloseRequest(kRetryOnWrongParamsMs);
    }
    return;
  }
  http_req_ = new http::ClientRequest(http::METHOD_POST,
                                      url_escaped_query_path_);

  http::Header* const hs = http_req_->request()->client_header();
  hs->SetChunkedTransfer(true);

  if ( !remote_user_name_.empty() || !remote_password_.empty() ) {
    if ( !hs->SetAuthorizationField(remote_user_name_, remote_password_) ) {
      LOG_WARNING << name() << "Error seting remote user/pass in http request";
    }
  }
  hs->AddField(http::kHeaderContentType,
               streaming::GetContentTypeFromStreamType(tag_type_),
               true, true);

  http_protocol_ = new http::ClientStreamingProtocol(
      http_client_params_,
      // TODO: we need https also
      new http::SimpleClientConnection(selector_, net_factory_,
                                       net::PROTOCOL_TCP),
      net::HostPort(ip, server_port_));
  http_protocol_->BeginStreaming(http_req_, http_process_callback_);

  req_ = new streaming::Request();
  req_->mutable_info()->internal_id_ = id();

  if ( !mapper_->AddRequest(media_name_.c_str(),
                            req_, media_process_callback_) ) {
    LOG_WARNING << name() << " Cannot register to media: " << media_name_;
    delete req_;
    req_ = NULL;
    CloseRequest(kRetryOnWrongParamsMs);
    return;
  }

  serializer_->Initialize(&out_);
}

void HttpPosterElement::CloseRequest(int64 retry_timeout_ms) {
  LOG_INFO << name() << " closing post request, retry in: " << retry_timeout_ms;
  if ( req_ != NULL ) {
    mapper_->RemoveRequest(req_, media_process_callback_);
    req_ = NULL;
  }
  if ( http_req_ != NULL ) {
    CHECK(http_protocol_ != NULL);
    selector_->DeleteInSelectLoop(http_protocol_);
    selector_->DeleteInSelectLoop(http_req_);
    http_protocol_ = NULL;
    http_req_ = NULL;
  } else {
    CHECK(http_protocol_ == NULL);
  }
  if ( retry_timeout_ms > 0 ) {
    selector_->RegisterAlarm(retry_callback_,
                               retry_timeout_ms);
  } else {
    selector_->UnregisterAlarm(retry_callback_);
  }
  out_.Clear();
  dropping_audio_ = false;
  dropping_video_ = false;
  paused_ = false;
}

bool HttpPosterElement::ProcessHttp(int32 available_out) {
  if ( available_out < 0 ) {  // terminated
    CloseRequest(http_retry_timeout_ms_);
    return false;
  }
  if ( req_ == NULL ) {
    CloseRequest(media_retry_timeout_ms_);
    return false;
  }
  if ( out_.IsEmpty() ) {
    http_protocol_->PauseWriting();
    paused_ = true;
  } else {
    while ( !out_.IsEmpty() && available_out > 0 &&
            out_.Size() > kMinChunkSize ) {
      const int32 to_send = min(
          out_.Size(),
          min(desired_http_chunk_size_, available_out));
      http_req_->request()->client_data()->AppendStream(&out_, to_send);
      available_out -= to_send;
    }
  }
  return true;
}

void HttpPosterElement::CreateSerializer() {
  switch ( tag_type_ ) {
    case streaming::Tag::TYPE_MP3:
      serializer_ = new streaming::Mp3TagSerializer();
      break;
    case streaming::Tag::TYPE_FLV:
      serializer_ = new streaming::FlvTagSerializer();
      break;
    case streaming::Tag::TYPE_AAC:
      serializer_ = new streaming::AacTagSerializer();
      break;
    case streaming::Tag::TYPE_INTERNAL:
      serializer_ = new streaming::InternalTagSerializer();
      break;
    default:
      serializer_ = new streaming::RawTagSerializer();
      break;
  }
}

}
