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
#include "common/base/types.h"
#include "common/base/log.h"
#include "common/base/system.h"
#include "common/base/gflags.h"

#include "net/http/http_server_protocol.h"
#include "net/http/http_proxy.h"
#include "net/base/selector.h"

//////////////////////////////////////////////////////////////////////

DEFINE_int32(port,
             8081,
             "Serve on this port");
DEFINE_bool(dlog_level,
            false,
            "Log more stuff ?");
DEFINE_string(remote_server,
              "",
              "Act as a proxy for this guy ?");

//////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
  common::Init(argc, argv);
  CHECK(!FLAGS_remote_server.empty()) << "Please provide a remote server !";
  http::ServerParams params;
  params.max_reply_buffer_size_ = 1<<18;
  params.dlog_level_ = FLAGS_dlog_level;
  net::Selector selector;
  net::NetFactory net_factory(&selector);
  net::PROTOCOL net_protocol = net::PROTOCOL_TCP;
  http::Server server("Test Server", &selector, net_factory, params);
  http::ClientParams cparams;
  cparams.dlog_level_ = true;

  http::Proxy proxy(&selector, net_factory, net_protocol, &server, true);
  proxy.RegisterPath("",
                     net::HostPort(FLAGS_remote_server.c_str()),
                     &cparams);
  server.AddAcceptor(net_protocol, net::HostPort(0, FLAGS_port));
  selector.RunInSelectLoop(NewCallback(&server,
                                       &http::Server::StartServing));
  selector.Loop();
}
