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
//

#ifndef __MEDIA_STANDARD_LIBRARY_LOOKUP_ELEMENT_H__
#define __MEDIA_STANDARD_LIBRARY_LOOKUP_ELEMENT_H__


// A element of media that gets its data from talking to a server
#include <string>
#include <map>

#include <whisperlib/common/base/types.h>
#include WHISPER_HASH_MAP_HEADER

#include <whisperlib/net/base/address.h>
#include <whisperstreamlib/base/element.h>
#include <whisperlib/net/http/failsafe_http_client.h>

#include <whisperstreamlib/elements/standard_library/http_client/http_client_element.h>

namespace streaming {
// Use
//  ${RESOURCE} in your stream for
//  ${REQ_QUERY} the request query part (e.g. wsp (seek_pos) and so on)
//  ${AUTH_QUERY} the authorization request query part (e.g. wuname and so on)
class LookupElement : public Element {
 public:
  LookupElement(const string& name,
                ElementMapper* mapper,
                net::Selector* selector,
                HttpClientElement* http_client_element,
                const vector<net::HostPort>& lookup_servers,
                const string& lookup_query_path_format,
                const vector< pair<string, string> >& lookup_http_headers,
                int lookup_num_retries,
                int lookup_max_concurrent_requests,
                int lookup_req_timeout_ms,
                const string& lookup_force_host_header,
                bool local_lookup_first);

  ~LookupElement();

  // streaming::Element interface methods
  virtual bool Initialize();
  virtual bool AddRequest(const string& media, Request* req,
                          ProcessingCallback* callback);
  virtual void RemoveRequest(streaming::Request* req);
  virtual bool HasMedia(const string& media);
  virtual void ListMedia(const string& media_dir, vector<string>* out);
  virtual bool DescribeMedia(const string& media, MediaInfoCallback* callback);
  virtual void Close(Closure* call_on_close);

  static const char kElementClassName[];

 private:
  struct LookupReqStruct {
    const string media_name_;
    streaming::Request* req_;
    streaming::ProcessingCallback* callback_;
    bool cancelled_;

    http::ClientRequest* http_request_;
    LookupReqStruct(const string& media,
                    streaming::Request* req,
                    streaming::ProcessingCallback* callback)
        : media_name_(media),
          req_(req),
          callback_(callback),
          cancelled_(false),
          http_request_(NULL) {
    }
    ~LookupReqStruct() {
      delete http_request_;
    }
  };
  LookupReqStruct* PrepareLookupStruct(const string&_media,
                                       streaming::Request* req,
                                       streaming::ProcessingCallback* callback);
  void LookupCompleted(LookupReqStruct* lr);
  bool StartLocalRedirect(LookupReqStruct* lr,
                          const string& media_path);
  bool StartFetch(LookupReqStruct* lr,
                  const net::HostPort& hp,
                  const string& query_path);
  void HttpClientClosed();

  net::Selector* selector_;
  net::NetFactory net_factory_;
  http::FailSafeClient* failsafe_client_;
  const string lookup_query_path_format_;
  vector< pair<string, string> > lookup_http_headers_;
  const bool local_lookup_first_;

  http::ClientParams client_params_;
  ResultClosure<http::BaseClientConnection*>* connection_factory_;

  static const int32 kReopenHttpConnectionIntervalMs = 2000;

  HttpClientElement* http_client_element_;

  bool closing_;
  bool http_closed_;
  Closure* call_on_close_;
  int64 next_client_id_;
  int64 next_lookup_id_;

  typedef hash_map<streaming::Request*, LookupReqStruct*> LookupMap;
  LookupMap lookup_ops_;
  typedef hash_map<streaming::Request*, string*> FetchMap;

  DISALLOW_EVIL_CONSTRUCTORS(LookupElement);
};
}

#endif  // __MEDIA_STANDARD_LIBRARY_LOOKUP_ELEMENT_H__
