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

#ifndef __APP_STREAMING_SERVER_STREAM_REQUEST_H__
#define __APP_STREAMING_SERVER_STREAM_REQUEST_H__

#include <whisperlib/net/base/selector.h>
#include <whisperlib/net/http/http_server_protocol.h>
#include <whisperstreamlib/stats2/stats_keeper.h>
#include <whisperstreamlib/stats2/stats_collector.h>
#include <whisperstreamlib/base/tag.h>
#include <whisperstreamlib/base/element.h>
#include <whisperstreamlib/base/exporter.h>

#include "media_mapper.h"

class StreamRequest : protected streaming::ExporterT {
public:
  StreamRequest(int64 connection_id,
      net::Selector* media_selector,
      streaming::ElementMapper* element_mapper,
      streaming::StatsCollector* stats_collector,
      http::ServerRequest* http_request);
  virtual ~StreamRequest();

 public:
  void Play(const char* content_type);

  //////////////////////////////////////////////////////////////////////
  //
  // ExporterT implementation
  //
 protected:
  virtual void IncRef() {
    synch::MutexLocker lock(&mutex_);
    ++ref_count_;
  }
  virtual void DecRef() {
    synch::MutexLocker lock(&mutex_);
    CHECK_GT(ref_count_, 0);
    --ref_count_;
    if ( ref_count_ == 0 ) {
      media_selector_->DeleteInSelectLoop(this);
    }
  }
  virtual synch::Mutex* mutex() const {
    return &mutex_;
  }

  virtual bool is_closed() const {
    synch::MutexLocker l(&mutex_);
    return http_request_ == NULL;
  }

  virtual const ConnectionBegin& connection_begin_stats() const {
    return connection_begin_stats_;
  }
  virtual const ConnectionEnd& connection_end_stats() const {
    return connection_end_stats_;
  }

  virtual void OnStreamNotFound();
  virtual void OnTooManyClients();
  virtual void OnAuthorizationFailed();
  virtual void OnReauthorizationFailed();
  virtual void OnAddRequestFailed();
  virtual void OnPlay();
  virtual void OnTerminate(const char* reason);

  virtual bool CanSendTag() const;
  virtual void SetNotifyReady();
  virtual void SendTag(const streaming::Tag* tag, int64 timestamp_ms);

  virtual void AuthorizeCompleted(int64 seek_time_ms,
      streaming::Authorizer* authorizer);

  ///////////////////////////////////////////////////////////////////////////
  // StreamRequest own methods
 protected:
  void ResumeLocalizedTags();

 private:
  void SendSimpleTag(const streaming::Tag* tag, int64 timestamp_ms);

  void MediaRequestClosedCallback(http::HttpReturnCode reason);
  void HttpRequestClosedCallback();

  void HandleEosInternal(const char* reason);

 private:
  mutable synch::Mutex mutex_;
  int32 ref_count_;

  //////////////////////////////////////////////////////////////////////
  //
  // MEDIA THREAD members
  //

  // the connection stats
  ConnectionBegin connection_begin_stats_;
  ConnectionEnd connection_end_stats_;

  //////////////////////////////////////////////////////////////////////
  //
  // NET THREAD members
  //

  // the associated HTTP request
  http::ServerRequest* http_request_;

  // serializes tags into the HTTP request
  streaming::TagSerializer* serializer_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(StreamRequest);
};


#endif  // __APP_STREAMING_SERVER_STREAM_REQUEST_H__
