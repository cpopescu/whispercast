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

#include <whisperstreamlib/rtp/rtsp/rtsp_server.h>

namespace streaming {
namespace rtsp {

Server::Server(net::Selector* selector,
               net::NetAcceptor* net_acceptor,
               const net::HostPort& rtsp_local_addr,
               MediaInterface* media_interface)
  : selector_(selector),
    net_acceptor_(net_acceptor),
    rtsp_local_addr_(rtsp_local_addr),
    processor_(selector, media_interface) {
}
Server::~Server() {
  delete net_acceptor_;
  net_acceptor_ = NULL;
}

bool Server::Start() {
  net_acceptor_->SetFilterHandler(NewPermanentCallback(this,
      &Server::AcceptorFilterHandler), true);
  net_acceptor_->SetAcceptHandler(NewPermanentCallback(this,
      &Server::AcceptorAcceptHandler), true);
  if ( !net_acceptor_->Listen(rtsp_local_addr_) ) {
    RTSP_LOG_ERROR << "Failed to open rtsp_local_addr: " << rtsp_local_addr_;
    return false;
  }
  LOG_INFO << "rtsp::Server running on: " << net_acceptor_->local_address();
  return true;
}
void Server::Stop() {
  LOG_INFO << "Stop, stopping selector..";
  net_acceptor_->Close();
}

bool Server::AcceptorFilterHandler(const net::HostPort& remote_addr) {
  // accept everyone
  return true;
}
void Server::AcceptorAcceptHandler(net::NetConnection* net_conn) {
  net::TcpConnection* tcp_conn = static_cast<net::TcpConnection*>(net_conn);
  Connection* rtsp_conn = new Connection(selector_, &processor_);
  rtsp_conn->Wrap(tcp_conn);
  // the corresponding release: rtsp::ServerProcessor::HandleConnectionClose
  rtsp_conn->Retain();
}

} // namespace rtsp
} // namespace streaming
