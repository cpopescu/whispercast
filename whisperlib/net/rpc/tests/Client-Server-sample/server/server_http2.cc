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

#include <signal.h>
#include "common/base/errno.h"
#include "common/base/signal_handlers.h"
#include "common/sync/event.h"
#include "common/sync/thread.h"

#include "common/base/log.h"
#include "common/base/types.h"
#include "net/base/selector.h"

#include "net/rpc/lib/server/rpc_http_server.h"
#include "net/http/http_server_protocol.h"

#include "auto/types.h"
#include "auto/invokers.h"

#include "services_impl.h"

synch::Event * evProgEnd = NULL;
void HandleNiceExit(int signum) {
  if (signum != SIGINT) {
    LOG_ERROR << "HandleNiceExit Erroneus called for " << signum << " - "
    << strsignal(signum);
    return;
  }
  LOG_WARNING << "\033[31m"  // change foreground to red
  "Cought SIGINT"
  "\033[0m";  // reset foreground
  if (common::IsApplicationHanging()) {
    exit(0);
  }
  if (evProgEnd) {
    evProgEnd->Signal();
  }
}


int main(int argc, char ** argv) {
  // - Initialize logger.
  // - Install default signal handlers.
  common::Init(argc, argv);

  // manually create & register service invokers
  // catch Ctrl+C -> SIGINT
  //
  if (SIG_ERR == ::signal(SIGINT, HandleNiceExit)) {
    LOG_FATAL << "cannot install SIGNAL-handler: "
    << GetLastSystemErrorDescription();
  }

  // create the nice exit event
  //
  synch::Event _evProgEnd(false, true);
  evProgEnd = &_evProgEnd;


  {
    net::SelectorThread selectorThread;
    net::Selector* selector = selectorThread.mutable_selector();
    net::NetFactory net_factory(selector);

    http::ServerParams params;
    params.max_reply_buffer_size_ = 1 << 18;
    params.dlog_level_ = true;
    http::Server httpServer("RPC Sample Server", selector,
                            net_factory, &params,
                            NewPermanentCallback(
                                &http::SimpleServerConnectionFactory));
    rpc::HttpServer rpcProcessor(selector, &httpServer, NULL, "rpc", true,
                                 true, 100, "");

    RPCServiceGigel serviceGigel("gigel");
    RPCServiceMitica serviceMitica("mitica");
    RPCServiceGlobal serviceGlobal("global");
    CHECK(rpcProcessor.RegisterService(&serviceGigel));
    CHECK(rpcProcessor.RegisterService(&serviceMitica));
    CHECK(rpcProcessor.RegisterService(&serviceGlobal));

    // start the http server
    //
    httpServer.AddAcceptor(net::PROTOCOL_TCP, net::HostPort(0, 8123));

    selectorThread.Start();
    selector->RunInSelectLoop(NewCallback(&httpServer,
                                          &http::Server::StartServing));

    // Let it run. Wait for Ctrl+C
    //
    LOG_INFO << "Use Ctrl+C to exit program.";
    evProgEnd->Wait();

  }

  LOG_INFO << "Bye.";
  common::Exit(0);

  return 0;
}
