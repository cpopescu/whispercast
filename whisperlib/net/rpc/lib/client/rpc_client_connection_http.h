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

#ifndef __NET_RPC_LIB_CLIENT_RPC_CLIENT_CONNECTION_HTTP_H__
#define __NET_RPC_LIB_CLIENT_RPC_CLIENT_CONNECTION_HTTP_H__

#include <map>
#include <string>
#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/base/system.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/io/buffer/io_memory_stream.h>
#include <whisperlib/common/sync/event.h>
#include <whisperlib/common/sync/mutex.h>

#include <whisperlib/net/http/http_server_protocol.h>
#include <whisperlib/net/http/http_client_protocol.h>
#include <whisperlib/net/base/selector.h>

#include <whisperlib/net/rpc/lib/client/irpc_client_connection.h>

namespace rpc {

// This is a HTTP connection from client side working as transport layer
// for RPC calls & results.
// It sends and receives rpc::Message packets.

class ClientConnectionHTTP : public rpc::IClientConnection {
 public:
  //////////////////////////////////////////////////////////////
  //
  //  Methods available to any external thread (application).
  //
  ClientConnectionHTTP(net::Selector& selector,
                       net::NetFactory& net_factory,
                       net::PROTOCOL net_protocol,
                       const http::ClientParams& params,
                       const net::HostPort& addr,
                       rpc::CodecId codec,
                       const string& httpRequestPath);
  virtual ~ClientConnectionHTTP();

  void SetHttpAuthentication(const string& user, const string& pswd);

  // Returns a description of the last error on the http connection.
  const char* Error() const;

  // Close HTTP connection, if any.
  void Close();

  // Returns the address of the http server where this client is/was connected.
  const net::HostPort& remote_address() const {
    return remote_addr_;
  }

 protected:
  //////////////////////////////////////////////////////////////////////
  //
  //         IRPCClientConnection interface methods
  //
  virtual void Send(const rpc::Message* p);
  virtual void Cancel(uint32 xid);

  //////////////////////////////////////////////////////////////
  //
  //     Methods available only from the selector thread.
  //
 protected:
  // Send the given request in selector thread context.
  void SendRequest(http::ClientRequest* req, const rpc::Message* p);

  // Called by the selector after a http request receives full response
  // from the server. The request object is the same one you used
  // when launching the query (you probably want to delete it).
  void CallbackRequestDone(http::ClientRequest* req);
 protected:
  net::Selector& selector_;
  net::NetFactory& net_factory_;
  const net::PROTOCOL net_protocol_;
  const net::HostPort remote_addr_;

  const http::ClientParams* http_params_;
  http::ClientProtocol* http_protocol_;
  string http_request_path_;

  string http_auth_user_;
  string http_auth_pswd_;

  synch::Event disconnect_completed_;  // signal TCP close done

  typedef map<http::ClientRequest*, const rpc::Message*> QueryMap;
  QueryMap queries_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ClientConnectionHTTP);
};
}
#endif  // __NET_RPC_LIB_CLIENT_RPC_CLIENT_CONNECTION_HTTP_H__
