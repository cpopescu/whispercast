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
#include <whisperlib/net/http/failsafe_http_client.h>

#include <whisperlib/net/rpc/lib/client/irpc_client_connection.h>

namespace rpc {

// This is a failsafe transport over HTTP connection from client side.
// The big difference between this and the other HTTP rpc transport
// is that you can use a set of rpc servers to call (i.e. uses the
// first that is available, and does load balancing) and it manages
// differently the reconnection and retries (is not on every send, but
// employs a timeout etc.)
// And you can cancel the lookup (your callback will never be called again
// on completion)
//
class FailsafeClientConnectionHTTP : public rpc::IClientConnection {
 public:
  //////////////////////////////////////////////////////////////
  //
  //  Methods available to any external thread (application).
  //
  FailsafeClientConnectionHTTP(net::Selector* selector,
                               rpc::CODEC_ID codec_id,
                               http::FailSafeClient* failsafe_client,
                               const string& httpRequestPath,
                               const string& auth_user,
                               const string& auth_pass);
  virtual ~FailsafeClientConnectionHTTP();

  const http::FailSafeClient* failsafe_client() const {
    return failsafe_client_;
  }
  const string& http_request_path() const {
    return http_request_path_;
  }
  const string& auth_user() const {
    return auth_user_;
  }
  const string& auth_pass() const {
    return auth_pass_;
  }

  // Returns a description of the last error on the http connection.
  const char* Error() const;

  // Close HTTP connection, if any.
  void Close();

  string ToString() const;

 protected:
  //////////////////////////////////////////////////////////////////////
  //
  //         IRPCClientConnection interface methods
  //
  virtual void Send(const rpc::Message* p);
  virtual void Cancel(uint32 xid);

 protected:
  struct QueryStruct {
    http::ClientRequest* const req_;
    const rpc::Message* const msg_;
    bool cancelled_;
    const uint32 xid_;
    QueryStruct(http::ClientRequest* req, const rpc::Message* msg)
        : req_(req),
          msg_(msg),
          cancelled_(false),
          xid_(msg->header_.xid_) {
    }
    ~QueryStruct() {
      delete req_;
    }
  };
 protected:
  // Called by the selector after a http request receives full response
  // from the server. The request object is the same one you used
  // when launching the query (you probably want to delete it).
  void CallbackRequestDone(QueryStruct* qs);

  http::FailSafeClient* failsafe_client_;
  const string http_request_path_;
  const string auth_user_;
  const string auth_pass_;

  typedef map<uint32, QueryStruct*> QueryMap;
  QueryMap queries_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(FailsafeClientConnectionHTTP);
};
}
#endif  // __NET_RPC_LIB_CLIENT_RPC_CLIENT_CONNECTION_HTTP_H__
