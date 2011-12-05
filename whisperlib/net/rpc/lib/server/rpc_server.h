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

#ifndef __NET_RPC_LIB_SERVER_RPC_SERVER_H__
#define __NET_RPC_LIB_SERVER_RPC_SERVER_H__

#include <whisperlib/common/sync/event.h>
#include <whisperlib/net/base/connection.h>
#include <whisperlib/net/base/address.h>
#include <whisperlib/net/rpc/lib/server/execution/rpc_execution_pool.h>

// This is the TCP server, listening and creating rpc::ServerConnections for
// every accepted client.

namespace rpc {

class Server {
 public:
  //enum { DEFAULT_PORT = 5678 };
  typedef Callback1<bool> OpenCompletedCallback;

 public:
  Server(net::Selector& selector,
         net::NetFactory& net_factory,
         net::PROTOCOL net_protocol,
         IAsyncQueryExecutor& executor);
  virtual ~Server();


  //////////////////////////////////////////////////////////////////////
  //
  //        Methods available to any external thread.
  //

  //  Opens service on the given local address & port.
  //  To close the server use Shutdown().
  // open_completed_callback: if NULL: This function is synchronous
  //                                   and blocks untill Open completes.
  //                          non-NULL: This function is asynchronous,
  //                                    returns immediately and
  //                                    open_completed_callback will be called
  //                                    when Open completes.
  //
  //  IMPORTANT: do NOT delete the rpc::Server from within
  //             open_completed_callback !
  bool Open(const net::HostPort& addr,
            OpenCompletedCallback* open_completed_callback = NULL);

  // Test if the server is running.
  bool IsOpen() const {
    return net_acceptor_->state() == net::NetAcceptor::LISTENING;
  }

  // Close the server.
  // Does nothing if the server is already closed.
  void Shutdown();

 private:
  //////////////////////////////////////////////////////////////////////
  //
  //     Methods available only from the selector thread.
  //
  //

  // Opens the server in Listen mode on the given local addr & port.
  void OpenInSelectThread(net::HostPort addr);

  //////////////////////////////////////////////////////////////////////
  //
  //                net::NetAcceptor handlers
  //
  bool AcceptorFilterHandler(const net::HostPort& peer_address);
  void AcceptorAcceptHandler(net::NetConnection* peer_connection);

 private:
  net::Selector& selector_;

  net::NetFactory& net_factory_;      // the factory used to create
                                      // the TCP or SSL acceptor

  net::NetAcceptor* net_acceptor_;    // the acceptor (TCP or SSL)

  IAsyncQueryExecutor& executor_;     // the queries executor
  synch::Event open_completed_;       // signaled when the Open procedure
                                      // is completed
  synch::Event shutdown_completed_;   // ......... Shutdown .............

  // external function, called when Open procedure completed
  OpenCompletedCallback* open_completed_callback_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(Server);
};
}
#endif   // __NET_RPC_LIB_SERVER_RPC_SERVER_H__
