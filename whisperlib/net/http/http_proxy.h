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
// You can register  a path in an http server as a proxy to another server :)
// (helps with rpc and other stuff..)
//
// NOTE: This is for demonstration only, the performance is pretty poor !!
//

#ifndef __NET_HTTP_HTTP_PROXY_H__
#define __NET_HTTP_HTTP_PROXY_H__

#include <set>
#include <map>
#include <string>

#include <whisperlib/common/base/types.h>
#include <whisperlib/net/http/http_server_protocol.h>
#include <whisperlib/net/http/http_client_protocol.h>

namespace http {

class Proxy {
 public:
  Proxy(net::Selector* selector,
        const net::NetFactory& net_factory,
        net::PROTOCOL net_protocol,
        http::Server* server,
        bool dlog_level);

  // Well .. you should take care of the pending requests blah ..
  ~Proxy();

  void RegisterPath(const string& path,
                    net::HostPort remote_server,
                    const ClientParams* params);
  void UnregisterPath(const string& path);

 private:
  struct ProxyReqStruct {
    ClientRequest* creq_;
    ServerRequest* sreq_;
    ClientStreamReceiverProtocol* proto_;
    Closure* callback_;
    bool http_header_sent_;
    bool paused_;
    ProxyReqStruct()
        : creq_(NULL), sreq_(NULL), proto_(NULL), callback_(NULL),
          http_header_sent_(false), paused_(false) {
    }
  };

  void ProcessRequest(net::HostPort remote_server,
                      const ClientParams* params,
                      ServerRequest* request);
  void StreamingCallback(ProxyReqStruct* preq);
  void FinalizeRequest(ProxyReqStruct* preq);
  void ClearRequest(ProxyReqStruct* preq);

  net::Selector* const selector_;
  const net::NetFactory& net_factory_;
  const net::PROTOCOL net_protocol_;
  http::Server* const server_;
  const bool dlog_level_;

  set<string> registered_paths_;

  typedef set<ProxyReqStruct*> ReqSet;
  ReqSet pending_requests_;
 private:
  DISALLOW_EVIL_CONSTRUCTORS(Proxy);
};

////////////////////////////////////////////////////////////////////////////////
}

#endif  // __NET_HTTP_HTTP_PROXY_H__
