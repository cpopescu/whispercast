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

#include "rtmp/rtmp_connection.h"
#include "rtmp/rtmp_stream.h"

#define LOG_RTMP LOG_INFO_IF(flags_->dlog_level_) << protocol_->name()

namespace rtmp {

// Constructor for serving connection
ServerConnection::ServerConnection(
    net::Selector* net_selector,
    net::Selector* media_selector,
    rtmp::StreamManager* rtmp_stream_manager,
    net::NetConnection* net_connection,
    const string& connection_id,
    streaming::StatsCollector* stats_collector,
    Closure* delete_callback,
    const vector<const net::IpClassifier*>* classifiers,
    const rtmp::ProtocolFlags* flags)
  : net_selector_(net_selector),
    media_selector_(media_selector),
    rtmp_stream_manager_(rtmp_stream_manager),
    net_connection_(net_connection),
    classifiers_(classifiers),
    delete_callback_(delete_callback),
    connection_id_(connection_id),
    stats_collector_(stats_collector),
    protocol_(NULL),
    flags_(flags),
    timeouter_(net_selector, NewPermanentCallback(this,
        &ServerConnection::TimeoutHandler)) {
  CHECK(net_selector_->IsInSelectThread());
  net_connection_->SetReadHandler(NewPermanentCallback(
      this, &rtmp::ServerConnection::ConnectionReadHandler), true);
  net_connection_->SetWriteHandler(NewPermanentCallback(
      this, &rtmp::ServerConnection::ConnectionWriteHandler), true);
  net_connection_->SetCloseHandler(NewPermanentCallback(
      this, &rtmp::ServerConnection::ConnectionCloseHandler), true);

  if ( flags_->send_buffer_size_ > 0 ) {
    net_connection_->SetSendBufferSize(flags_->send_buffer_size_);
  }

  connection_begin_stats_.connection_id_ = connection_id_;
  connection_begin_stats_.timestamp_utc_ms_ = timer::Date::Now();
  connection_begin_stats_.remote_host_ =
      net_connection_->remote_address().ip_object().ToString();
  connection_begin_stats_.remote_port_ =
      net_connection_->remote_address().port();
  connection_begin_stats_.local_host_ =
      net_connection_->local_address().ip_object().ToString(),
  connection_begin_stats_.local_port_ =
      net_connection_->local_address().port();
  connection_begin_stats_.protocol_ = "rtmp";

  connection_end_stats_.connection_id_ = connection_id_;
  connection_end_stats_.timestamp_utc_ms_ =
      connection_begin_stats_.timestamp_utc_ms_;
  connection_end_stats_.bytes_up_ = 0;
  connection_end_stats_.bytes_down_ = 0;
  connection_end_stats_.result_ = ConnectionResult("ACTIVE");

  stats_collector_->StartStats(&connection_begin_stats_,
      &connection_end_stats_);

  //////////////////////////////////////////////////////////////////////

  //if ( classifiers_ != NULL ) {
  //  for ( int i = 0; i < classifiers_->size(); ++i ) {
  //    if ( (*classifiers_)[i]->IsInClass(
  //        net_connection_->remote_address().ip_object()) ) {
  //      // TODO [cpopescu]: do something w/ the IP class ?
  //      break;
  //    }
  //  }
  //}

  protocol_ = new rtmp::Protocol(
      net_selector_,
      media_selector_,
      this,
      rtmp_stream_manager_,
      flags_);
  protocol_->IncRef();
}

ServerConnection::~ServerConnection() {
  if ( delete_callback_ ) {
    media_selector_->RunInSelectLoop(delete_callback_);
  }
  if ( connection_end_stats_.result_.get().result_.get() == "ACTIVE" ) {
    connection_end_stats_.timestamp_utc_ms_ = timer::Date::Now();
    connection_end_stats_.result_ =
        net_connection_ != NULL && net_connection_->last_error_code() != 0
        ? ConnectionResult("ERROR") : ConnectionResult("SERVER CLOSE");
    stats_collector_->EndStats(&connection_end_stats_);
  }
  CHECK_NOT_NULL(net_connection_) << "Why is net_connection_ NULL??";
  delete net_connection_;
  net_connection_ = NULL;
}

void ServerConnection::SendOutbufData() {
  if ( net_connection_->state() != net::NetConnection::CONNECTED ||
       net_connection_->outbuf()->IsEmpty() ) {
    return;
  }
  timeouter_.SetTimeout(kWriteEvent, flags_->write_timeout_ms_);
  net_connection_->RequestWriteEvents(true);
}

void ServerConnection::CloseConnection() {
  if ( net_connection_ != NULL ) {
    LOG_DEBUG << net_connection_->PrefixInfo()
             << " RTMP connection closing per protocol request";
    net_connection_->FlushAndClose();
  }
}

bool ServerConnection::ConnectionReadHandler() {
  connection_end_stats_.bytes_down_ =
      net_connection_->count_bytes_read();
  protocol_->HandleClientData(net_connection_->inbuf());
  return true;
}

bool ServerConnection::ConnectionWriteHandler() {
  connection_end_stats_.bytes_up_ =
      net_connection_->count_bytes_written();
  if ( net_connection_->outbuf()->IsEmpty() ) {
    timeouter_.UnsetTimeout(kWriteEvent);
  } else {
    timeouter_.SetTimeout(kWriteEvent, flags_->write_timeout_ms_);
  }
  protocol_->NotifyConnectionWrite();
  return true;
}
void ServerConnection::ConnectionCloseHandler(
    int err, net::NetConnection::CloseWhat what) {
  if ( what != net::NetConnection::CLOSE_READ_WRITE ) {
    net_connection_->FlushAndClose();
    return;
  }
  // TCP connection completely closed.
  // Now close the application.
  LOG_DEBUG << net_connection_->PrefixInfo()
            << " Connection Close in state: " << net_connection_->StateName()
            << " left in outbuf: " << net_connection_->outbuf()->Size();
  connection_end_stats_.timestamp_utc_ms_ = timer::Date::Now();
  connection_end_stats_.result_.ref().result_ = "CLOSE";
  stats_collector_->EndStats(&connection_end_stats_);
  protocol_->NotifyConnectionClose();
  protocol_->DecRef();
  protocol_ = NULL;
}

void ServerConnection::TimeoutHandler(int64 timeout_id) {
  LOG_ERROR << "Timeout id: " << timeout_id << " , closing connection...";
  net_connection_->ForceClose();
}

//////////////////////////////////////////////////////////////////////////////
//
// ServerAcceptor
//

// Constructor for accepting connection
ServerAcceptor::ServerAcceptor(
    net::Selector* media_selector,
    const vector<net::SelectorThread*>* client_threads,
    rtmp::StreamManager* rtmp_stream_manager,
    const string& name,
    streaming::StatsCollector* stats_collector,
    const vector<const net::IpClassifier*>* classifiers,
    const rtmp::ProtocolFlags* flags)
  : media_selector_(media_selector),
    rtmp_stream_manager_(rtmp_stream_manager),
    name_(name),
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
      rtmp_stream_manager_,
      peer,
      strutil::StringPrintf(
          "%s.%"PRId32"",
          name_.c_str(),
          (connection_id)),
      stats_collector_,
      NewCallback(this, &ServerAcceptor::NotifyConnectionClose),
      classifiers_,
      flags_);   // auto deleted ..
  DLOG_DEBUG << "=====>> RTMP Client accepted from " << peer->remote_address();
}

}
