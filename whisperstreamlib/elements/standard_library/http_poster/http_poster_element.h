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

#ifndef __MEDIA_BASE_HTTP_POSTER_ELEMENT_H__
#define __MEDIA_BASE_HTTP_POSTER_ELEMENT_H__

// A element of media that gets its data from talking to a server
#include <string>

#include <whisperlib/net/base/address.h>
#include <whisperlib/net/http/http_client_protocol.h>
#include <whisperstreamlib/base/element.h>
#include <whisperstreamlib/base/tag_splitter.h>

namespace streaming {

class HttpPosterElement : public Element {
 public:
  typedef map<string, string>  Host2IpMap;

  // Creates a new http sourece. If splitter_creation_callback is NULL
  // we use a default creator based on the content type and standard
  // splitters.
  HttpPosterElement(const char* name,
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
                    const http::ClientParams* http_client_params = NULL);

  virtual ~HttpPosterElement();

  // streaming::Element interface methods
  virtual bool Initialize();
  virtual bool AddRequest(const char* media, streaming::Request* req,
                          streaming::ProcessingCallback* callback);
  virtual void RemoveRequest(streaming::Request* req);
  virtual bool HasMedia(const char* media, Capabilities* out);
  virtual void ListMedia(const char* media_dir,
                         streaming::ElementDescriptions* medias);
  virtual bool DescribeMedia(const string& media,
                             MediaInfoCallback* callback);
  virtual void Close(Closure* call_on_close);

  static const char kElementClassName[];

 private:
  void StartRequest();
  void CloseRequest(int64 retry_timeout_ms);

  bool ProcessHttp(int32 available_out);
  void ProcessTag(const Tag* tag);

  void CreateSerializer();

 private:
  net::Selector* const selector_;          // runs us
  net::NetFactory net_factory_;            // creates TCP or SSL connections
  const Host2IpMap* const host_aliases_;
                                           // can map server_name_ -> ip
  const string media_name_;                // media name
  const string server_name_;               // we connect to this server(alias)
  const uint16 server_port_;               // we connect on this port
  const string url_escaped_query_path_;    // and post on this path
  const Tag::Type tag_type_;           // what we expect to send
  const string remote_user_name_;          // If set we use this user for
                                           // basic authentication
  const string remote_password_;           // If set we use this pass for
                                           // basic authentication
  const int32 max_buffer_size_;            // when we reach this much in buffer
                                           // we close ..
  const int32 desired_http_chunk_size_;
  const int64 media_retry_timeout_ms_;     // when to retry on media error
  const int64 http_retry_timeout_ms_;      // when to retry on http error

  const http::ClientParams* const http_client_params_;

  streaming::TagSerializer* serializer_;     // serializes data on the connection

  http::ClientStreamingProtocol::StreamingCallback* const
  http_process_callback_;
  streaming::ProcessingCallback* const media_process_callback_;
  Closure* const retry_callback_;

  streaming::Request* req_;
  http::ClientRequest* http_req_;
  http::ClientStreamingProtocol* http_protocol_;

  bool paused_;
  bool dropping_audio_;
  bool dropping_video_;
  io::MemoryStream out_;

  http::ClientParams http_default_params_;
  static const int kRetryOnWrongParamsMs = 15000;

  static const int32 kMinChunkSize = 2048;

  DISALLOW_EVIL_CONSTRUCTORS(HttpPosterElement);
};
}

#endif  // __MEDIA_BASE_HTTP_POSTER_ELEMENT_H__
