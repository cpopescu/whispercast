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

#ifndef __NET_RTMP_RTMP_ACCEPTOR_H__
#define __NET_RTMP_RTMP_ACCEPTOR_H__

#include <string>
#include <vector>
#include <whisperlib/common/base/types.h>
#include <whisperlib/net/base/connection.h>
#include <whisperstreamlib/base/element_mapper.h>
#include <whisperstreamlib/rtmp/rtmp_flags.h>
#include <whisperstreamlib/stats2/stats_collector.h>

namespace net {
class IpClassifier;
}

namespace rtmp {
class ServerAcceptor {
 public:
  ServerAcceptor(net::Selector* media_selector,
                 const vector<net::SelectorThread*>* client_threads,
                 streaming::ElementMapper* element_mapper,
                 streaming::StatsCollector* stats_collector,
                 const vector<const net::IpClassifier*>* classifiers,
                 const rtmp::ProtocolFlags* flags);

  virtual ~ServerAcceptor();

  // Opens the server acceptor on the given port.
  void StartServing(int port,
                    const char* local_address);

  void StopServing();

 private:
  // Callback in the accepting connection when a serving connection dies
  void NotifyConnectionClose();

  // Called every time a client wants to connect.
  // We return false to shutdown this guy, or true to accept it.
  bool AcceptorFilterHandler(const net::HostPort& peer_address);
  // After AcceptorFilterHandler returned true,
  // the client connection is delivered here.
  void AcceptorAcceptHandler(net::NetConnection* peer_connection);

  // Increments num_accepted_connections_
  void IncAcceptedConnections();

 private:
  net::Selector* media_selector_;
  streaming::ElementMapper* element_mapper_;

  SSL_CTX* ssl_context_;
  net::NetAcceptor* net_acceptor_;

  const vector<const net::IpClassifier*>* const classifiers_;

  // used for stats logging
  streaming::StatsCollector* stats_collector_;

  // Id for the next created connection.
  int64 next_connection_id_;

  // Number of serving connections (meaningful only in an accepting connection)
  int32 num_accepted_connections_;

  // Protocol parameters:
  const ProtocolFlags* const flags_;

  // synchronize secondary network threads which call AcceptorAcceptHandler()
  synch::Mutex sync_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ServerAcceptor);
};
}

#endif // __NET_RTMP_RTMP_ACCEPTOR_H__
