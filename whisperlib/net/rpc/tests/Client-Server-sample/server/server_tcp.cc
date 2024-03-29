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
#include "common/base/log.h"
#include "common/base/types.h"
#include "common/base/errno.h"
#include "common/base/gflags.h"
#include "common/base/signal_handlers.h"
#include "common/sync/event.h"
#include "common/sync/thread.h"
#include "net/base/selector.h"
#include "net/base/address.h"
#include "net/rpc/lib/types/rpc_all_types.h"
#include "net/rpc/lib/server/rpc_server.h"
#include "net/rpc/lib/server/rpc_services_manager.h"

#include "auto/types.h"
#include "auto/invokers.h"

#include "services_impl.h"

DEFINE_int32(port,
             5611,
             "RPC server port.");

synch::Event* evProgEnd = NULL;
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

  rpc::ServicesManager servicesManager;

  // manually create & register service invokers
  //
  RPCServiceGigel serviceGigel("gigel");
  RPCServiceMitica serviceMitica("mitica");
  RPCServiceGlobal serviceGlobal("global");
  bool success = servicesManager.RegisterService(serviceGigel) &&
                 servicesManager.RegisterService(serviceMitica) &&
                 servicesManager.RegisterService(serviceGlobal);
  if (!success) {
    LOG_ERROR << "Failed to register services invokers.";
  }

  //create the rpc-executor
  //
  rpc::ExecutionPool executor(servicesManager);
  success = executor.Start(10);
  if (!success) {
    LOG_ERROR << "Failed to start rpc-execution-pool: "
    << GetLastSystemErrorDescription();
  }

  {
    net::SelectorThread selectorThread;
    net::Selector* selector = selectorThread.mutable_selector();

    // create net factory
    net::NetFactory net_factory(selector);

    selectorThread.Start();

    //create the rpc server
    //
    rpc::Server rpcServer(*selector, net_factory, net::PROTOCOL_TCP, executor);
    success = rpcServer.Open(net::HostPort(0, FLAGS_port));
    if (!success) {
      LOG_ERROR << "Cannot open port " << FLAGS_port
                << " error: " << GetLastSystemErrorDescription();
    } else {
      // Let it run. Wait for Ctrl+C
      //
      LOG_INFO << "Use Ctrl+C to exit program.";
      evProgEnd->Wait();
    }

    // stop the rpc server
    //
    rpcServer.Shutdown();
  }

  // stop the rpc-executor
  //
  executor.Stop();

  // manually unregister services
  //
  servicesManager.UnregisterService(serviceGigel);
  servicesManager.UnregisterService(serviceMitica);
  servicesManager.UnregisterService(serviceGlobal);

  LOG_INFO << "Bye.";

  common::Exit(0);
  return 0;
}
