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
class HttpServerImportData;

class HttpServerElement
    : public ImportElement<ServiceInvokerHttpServerElementService,
                           HttpServerImportData,
                           HttpServerElementDataSpec> {
  typedef ImportElement<ServiceInvokerHttpServerElementService,
                        HttpServerImportData,
                        HttpServerElementDataSpec> BaseClase;
 public:
  // Creates a new http media server element using an existing http server.
  // If splitter_creation_callback is NULL we use a default creator based on
  // the content type and standard splitters.
  HttpServerElement(const string& name,
                    const string& id,
                    streaming::ElementMapper* mapper,
                    const string& url_escaped_listen_path,
                    net::Selector* selector,
                    const string& media_dir,
                    http::Server* http_server,
                    const string& rpc_path,
                    rpc::HttpServer* rpc_server,
                    io::StateKeepUser* state_keeper,
                    streaming::SplitterCreator* splitter_creator,
                    const string& authorizer_name);
  virtual ~HttpServerElement();

  static const char kElementClassName[];

 protected:
  virtual HttpServerImportData* CreateNewImport(const char* import_name,
                                                const char* full_name,
                                                Tag::Type tag_type,
                                                const char* save_dir,
                                                bool append_only,
                                                bool disable_preemption,
                                                int32 prefill_buffer_ms,
                                                int32 advance_media_ms,
                                                int32 buildup_interval_sec,
                                                int32 buildup_delay_sec);
 private:
  const string url_escaped_listen_path_;
  http::Server*  const http_server_;

  // used by the dedicated http_server
  static const http::ServerParams http_server_default_params_;

  DISALLOW_EVIL_CONSTRUCTORS(HttpServerElement);
};
}

#endif  // __MEDIA_BASE_HTTP_MEDIA_SERVER_SOURCE_H__
