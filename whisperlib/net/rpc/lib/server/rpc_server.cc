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

// for ::shutdown in case of error on connection creation
#include <sys/socket.h>

#include "common/base/log.h"

#include "net/rpc/lib/server/rpc_server.h"
#include "net/rpc/lib/server/rpc_server_connection.h"

namespace rpc {
Server::Server(net::Selector& selector,
               net::NetFactory& net_factory,
               net::PROTOCOL net_protocol,
               rpc::IAsyncQueryExecutor& executor)
  : selector_(selector),
    net_factory_(net_factory),
    net_acceptor_(net_factory.CreateAcceptor(net_protocol)),
    executor_(executor),
    open_completed_(false, true),
    shutdown_completed_(false, true),
    open_completed_callback_(NULL) {
  net_acceptor_->SetFilterHandler(NewPermanentCallback(
      this, &Server::AcceptorFilterHandler), true);
  net_acceptor_->SetAcceptHandler(NewPermanentCallback(
      this, &Server::AcceptorAcceptHandler), true);
}

Server::~Server() {
  Shutdown();
  CHECK_NULL(open_completed_callback_);
  delete net_acceptor_;
  net_acceptor_ = NULL;
}

//////////////////////////////////////////////////////////////////////
//
// External methods - available from any thread

bool Server::Open(const net::HostPort& addr,
                       OpenCompletedCallback* open_completed_callback) {
  // do not try double Open
  CHECK_EQ(net_acceptor_->state(), net::NetAcceptor::DISCONNECTED);

  open_completed_callback_ = open_completed_callback;
  open_completed_.Reset();
  selector_.RunInSelectLoop(
      NewCallback(this, &Server::OpenInSelectThread, addr));
  if ( open_completed_callback ) {
    // asynchronous Open
    return true;
  }
  // synchronous Open
  if ( !open_completed_.Wait(5000) ) {
    LOG_ERROR << "Timeout waiting for open process to finish";
    return false;
  }
  return IsOpen();
}

void Server::Shutdown() {
  if ( !IsOpen() ) {
    return;
  }
  LOG_INFO << "Shutting down rpc::Server on: "
           << net_acceptor_->local_address();
  if ( !selector_.IsInSelectThread() ) {
    shutdown_completed_.Reset();
    selector_.RunInSelectLoop(NewCallback(this, &Server::Shutdown));
    if ( !shutdown_completed_.Wait(5000) ) {
      LOG_ERROR << "Timeout waiting for server shutdown";
      return;
    }
    return;
  }

  //
  // selector thread
  //
  const uint16 port = net_acceptor_->local_address().port();
  net_acceptor_->Close();
  CHECK_EQ(net_acceptor_->state(), net::NetAcceptor::DISCONNECTED);
  LOG_INFO << "rpc::Server shutdown (was on port " << port << ")";

  if ( open_completed_callback_ &&
       !open_completed_callback_->is_permanent() ) {
    delete open_completed_callback_;
  }
  open_completed_callback_ = NULL;

  // SIGNAL has to be the last statement! The destructor may be waiting us.
  shutdown_completed_.Signal();
  return;
}

//////////////////////////////////////////////////////////////////////
//
// Methods available only from the selector thread.
//
void Server::OpenInSelectThread(net::HostPort addr) {
  CHECK(selector_.IsInSelectThread());
  net_acceptor_->Listen(addr);
  if ( IsOpen() ) {
    LOG_INFO << "rpc::Server listening on: " << net_acceptor_->local_address();
  } else {
    LOG_ERROR << "Open failed for address: " << addr
              << " , error: " << GetLastSystemErrorDescription();
  }
  if ( open_completed_callback_ ) {
    LOG_DEBUG << "Running open_completed_callback_(" << std::boolalpha
              << IsOpen() << ")";
    open_completed_callback_->Run(IsOpen());
    open_completed_callback_ = NULL;
  }
  open_completed_.Signal();
}

bool Server::AcceptorFilterHandler(const net::HostPort& peer_address) {
  // TODO(cosmin): limit accept rate
  return true;
}
void Server::AcceptorAcceptHandler(net::NetConnection* peer_connection) {
  LOG_INFO << "New RPC Connection from " << peer_connection->remote_address();

  // Create the RPC connection. Uses auto_delete_on_close.
  new rpc::ServerConnection(&selector_, true, peer_connection, executor_);
};
}
