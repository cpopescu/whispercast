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

// TODO(cpopescu) : make relative timestamps on options !!

#include <whisperlib/net/util/ipclassifier.h>
#include <whisperlib/common/io/buffer/memory_stream.h>

#include <whisperstreamlib/rtmp/rtmp_acceptor.h>
#include <whisperstreamlib/rtmp/rtmp_connection.h>
#include <whisperstreamlib/rtmp/rtmp_stream.h>

namespace rtmp {

// Constructor for accepting connection
ServerAcceptor::ServerAcceptor(
    net::Selector* media_selector,
    const vector<net::SelectorThread*>* client_threads,
    streaming::ElementMapper* element_mapper,
    streaming::StatsCollector* stats_collector,
    const vector<const net::IpClassifier*>* classifiers,
    const rtmp::ProtocolFlags* flags)
  : media_selector_(media_selector),
    element_mapper_(element_mapper),
    ssl_context_(NULL),
    net_acceptor_(NULL),
    classifiers_(classifiers),
    stats_collector_(stats_collector),
    next_connection_id_(0),
    num_accepted_connections_(0),
    flags_(flags),
    sync_() {
  if ( flags_->ssl_certificate_ != "" && flags_->ssl_key_ != "" ) {
    ssl_context_ = net::SslConnection::SslCreateContext(
        flags_->ssl_certificate_, flags_->ssl_key_);
    if ( ssl_context_ != NULL ) {
      LOG_INFO << "Using RTMP Acceptor over SSL..";
      net::SslConnectionParams ssl_connection_params;
      ssl_connection_params.ssl_context_ = ssl_context_;
      net::SslAcceptorParams ssl_acceptor_params(ssl_connection_params);
      if ( client_threads ) {
        ssl_acceptor_params.set_client_threads(client_threads);
      }
      net_acceptor_ = new net::SslAcceptor(media_selector_, ssl_acceptor_params);
    }
  }
  if ( net_acceptor_ == NULL ) {
    net::TcpConnectionParams tcp_connection_params;
    net::TcpAcceptorParams tcp_acceptor_params(tcp_connection_params);
    if ( client_threads ) {
      tcp_acceptor_params.set_client_threads(client_threads);
    }
    net_acceptor_ = new net::TcpAcceptor(media_selector_, tcp_acceptor_params);
  }
  net_acceptor_->SetFilterHandler(
      NewPermanentCallback(this, &rtmp::ServerAcceptor::AcceptorFilterHandler), true);
  net_acceptor_->SetAcceptHandler(
      NewPermanentCallback(this, &rtmp::ServerAcceptor::AcceptorAcceptHandler), true);
}

ServerAcceptor::~ServerAcceptor() {
  delete net_acceptor_;
  net_acceptor_ = NULL;
  net::SslConnection::SslDeleteContext(ssl_context_);
  ssl_context_ = NULL;
}

void ServerAcceptor::StartServing(int port, const char* local_address) {
  CHECK(port > 0 && port <= 0xffff) << "Invalid port: " << port;
  if ( local_address == NULL ) {
    net::HostPort local_addr("0.0.0.0", port);
    CHECK(net_acceptor_->Listen(local_addr))
        << "Failed to listen on port: " << port;
    LOG_INFO << "RTMP Turned on listening on port " << port
             << " - all interfaces";
  } else {
    net::HostPort local_addr(local_address, port);
    CHECK(net_acceptor_->Listen(local_addr));
    LOG_INFO << "RTMP Turned on listening on port " << port
             << " - " << local_address;
  }
}
void ServerAcceptor::StopServing() {
  net_acceptor_->Close();
}

void ServerAcceptor::NotifyConnectionClose() {
  CHECK(media_selector_->IsInSelectThread());
  {
    synch::MutexLocker lock(&sync_);
    num_accepted_connections_--;
    LOG_DEBUG << "NotifyConnectionClose, remaining clients = "
              << num_accepted_connections_;
    CHECK_GE(num_accepted_connections_, 0);
  }
}

bool ServerAcceptor::AcceptorFilterHandler(const net::HostPort& peer) {
  LOG_DEBUG << net_acceptor_->PrefixInfo()
           << "Filtering an accept from: " << peer;
  if ( num_accepted_connections_ >= flags_->max_num_connections_ ) {
    LOG_WARNING << net_acceptor_->PrefixInfo()
                << "Too many connections ! - refusing"
                   ", num_accepted_connections_: " << num_accepted_connections_
                << ", flags_->max_num_connections_: " << flags_->max_num_connections_;
    return false;
  }
  return true;
}

void ServerAcceptor::AcceptorAcceptHandler(net::NetConnection* peer) {
  // NOTE!: Multiple secondary network threads call this function, concurrently.
  CHECK(peer->net_selector()->IsInSelectThread());
  LOG_DEBUG << net_acceptor_->PrefixInfo()
            << "Handling an accept from: " << peer->remote_address();

  uint32 connection_id = 0;
  {
    synch::MutexLocker lock(&sync_);
    num_accepted_connections_++;
    connection_id = next_connection_id_++;
  }

  new rtmp::ServerConnection(
      peer->net_selector(),
      media_selector_,
      element_mapper_,
      peer,
      strutil::StringPrintf("rtmp.%"PRId32"", connection_id),
      stats_collector_,
      NewCallback(this, &ServerAcceptor::NotifyConnectionClose),
      classifiers_,
      flags_);   // auto deleted ..
  DLOG_DEBUG << "=====>> RTMP Client accepted from " << peer->remote_address();
}

}
