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
// Author: Cosmin Tudorache

#ifndef __MEDIA_BASE_HTTP_MEDIA_SERVER_SOURCE_H__
#define __MEDIA_BASE_HTTP_MEDIA_SERVER_SOURCE_H__

// A media element that gets its data from a remote http client through
// upload. This element is a HTTP server waiting for upload.

// TODO(cpopescu): add user/password for uploads..

#include <string>
#include <map>
#include <whisperlib/net/base/address.h>
#include <whisperlib/net/http/http_header.h>
#include <whisperlib/net/http/http_server_protocol.h>
#include <whisperstreamlib/base/element.h>
#include <whisperlib/net/rpc/lib/server/rpc_http_server.h>
#include <whisperlib/common/io/checkpoint/state_keeper.h>

#include <whisperstreamlib/elements/standard_library/http_server/import_element.h>
#include <whisperstreamlib/elements/standard_library/auto/standard_library_invokers.h>

namespace streaming {
class HttpServerElement : public ImportElement,
                          public ServiceInvokerHttpServerElementService {
 public:
  static const char kElementClassName[];

  // Creates a new http media server element using an existing http server.
  HttpServerElement(const string& name,
                    ElementMapper* mapper,
                    const string& listen_path,
                    net::Selector* selector,
                    http::Server* http_server,
                    const string& rpc_path,
                    rpc::HttpServer* rpc_server,
                    io::StateKeepUser* state_keeper,
                    const string& authorizer_name);
  virtual ~HttpServerElement();

  using ImportElement::AddImport;
  using ImportElement::DeleteImport;

  /////////////////////////////////////////////////////////////////////////
  // overridden ImportElement methods
  virtual bool Initialize();
 protected:
  virtual ImportElementData* CreateNewImport(const string& import_name);

  /////////////////////////////////////////////////////////////////////////
  // methods from ServiceInvokerHttpServerElementService
  void AddImport(rpc::CallContext<MediaOpResult>* call, const string& name);
  void DeleteImport(rpc::CallContext<rpc::Void>* call, const string& name);
  void GetImports(rpc::CallContext< vector<string> >* call);

 private:
  const string listen_path_;
  http::Server* const http_server_;

  DISALLOW_EVIL_CONSTRUCTORS(HttpServerElement);
};
}

#endif  // __MEDIA_BASE_HTTP_MEDIA_SERVER_SOURCE_H__
