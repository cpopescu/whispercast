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

#ifndef __MEDIA_BASE_HTTP_MEDIA_SOURCE_H__
#define __MEDIA_BASE_HTTP_MEDIA_SOURCE_H__

// A element of media that gets its data from talking to a server
#include <string>
#include <map>

#include <whisperlib/net/http/http_header.h>
#include <whisperlib/net/base/address.h>
#include <whisperlib/net/http/http_client_protocol.h>
#include <whisperstreamlib/base/element.h>
#include <whisperstreamlib/base/tag_splitter.h>

namespace streaming {

class HttpClientElementData;
class HttpClientElement : public Element {
 public:
  // Creates a new http source. If splitter_creation_callback is NULL
  // we use a default creator based on the content type and standard
  // splitters.
  HttpClientElement(const string& name,
                    ElementMapper* mapper,
                    net::Selector* selector,
                    const int32 prefill_buffer_ms = 3000,
                    const int32 advance_media_ms = 2000,
                    int media_http_maximum_tag_size = 1 << 18);
  virtual ~HttpClientElement();

  // Initializes element names w/ urls and a parameter if the connection
  // should be reopened if closed.
  // IMPORTANT NOTE: upon creation and obtaining the http header
  //    we will check if we got a correct HTTP response code. If not,
  //    we retry to open the element.
  bool AddElement(const string& name,
                  const string& server_name,
                  const uint16 server_port,
                  const string& url_escaped_query_path,
                  bool should_reopen,
                  bool fetch_only_on_request,
                  const http::ClientParams* client_params = NULL);
  bool DeleteElement(const string& name);
  // Sets the user and password for accessing a remote path
  bool SetElementRemoteUser(const string& name,
                            const string& user_name,
                            const string& password);

  // streaming::Element interface methods
  virtual bool Initialize() { return true; }
  virtual bool AddRequest(const string& media, Request* req,
                          ProcessingCallback* callback);
  virtual void RemoveRequest(streaming::Request* req);
  virtual bool HasMedia(const string& media);
  virtual void ListMedia(const string& media_dir, vector<string>* out);
  virtual bool DescribeMedia(const string& media, MediaInfoCallback* callback);
  virtual void Close(Closure* call_on_close);

  static const char kElementClassName[];

 private:
  string Info() const;

  // Callback when a streaming url is closed
  void ClosedElement(HttpClientElementData* data);
  void CloseAllElements();

 private:
  net::Selector* const selector_;
  net::NetFactory net_factory_;

  typedef map<string, HttpClientElementData*> ElementMap;
  typedef map<streaming::Request*, HttpClientElementData*> RequestMap;

  ElementMap elements_;
  RequestMap requests_;

  http::ClientParams default_params_;

  // Parameters: how we behave - check standard_library.rpc
  const int32 prefill_buffer_ms_;
  const int32 advance_media_ms_;
  const int media_http_maximum_tag_size_;

  Closure* call_on_close_;

  DISALLOW_EVIL_CONSTRUCTORS(HttpClientElement);
};
}

#endif  // __MEDIA_BASE_HTTP_MEDIA_SOURCE_H__
