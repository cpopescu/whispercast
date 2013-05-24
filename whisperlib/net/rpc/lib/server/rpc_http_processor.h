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

#ifndef __NET_RPC_LIB_SERVER_RPC_HTTP_PROCESSOR_H__
#define __NET_RPC_LIB_SERVER_RPC_HTTP_PROCESSOR_H__

#include <string>
#include <map>
#include <whisperlib/net/http/http_server_protocol.h>
#include <whisperlib/net/rpc/lib/server/rpc_services_manager.h>
#include <whisperlib/net/rpc/lib/server/irpc_result_handler.h>
#include <whisperlib/net/rpc/lib/server/irpc_async_query_executor.h>
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_encoder.h>
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_decoder.h>

namespace rpc {

////////////////////////////////////////
//
//   HTTP tunneling
//
// A class to processes rpc queries enclosed in http requests.
//
// The general workflow:
//   1. the HTTP server, upon receiving a request, calls us to
//      process the request
//   2. we decode rpc query from http request
//   3. we asynchronous execute the query
//   4. we encode response in http request, and send it back
//
// This is somehow similar to rpc_server_connection, with the
// following exceptions:
//
// On HTTP tunneling: - There is no handshake.
//                    - The codec is specified in every request.
//
class HttpProcessor : protected IResultHandler {
 public:
  HttpProcessor(rpc::ServicesManager& manager,
                IAsyncQueryExecutor& executor,
                bool enable_auto_forms);
  virtual ~HttpProcessor();

  //  Attach this processor object to the given server, on
  //  the specified path. The authenticator can be NULL.
  // e.g. if path is "/abc/d" then all http request on URL
  //       http://<server-ip>/abc/d   and also all
  //       http://<server-ip>/abc/d/* are dispatched to this processor.
  // returns:
  //    success value.
  bool AttachToServer(http::Server* server, const string& process_path,
                      net::UserAuthenticator* authenticator = NULL);

  //////////////////////////////////////////////////////////////
  //
  //     Methods available only from the selector thread.
  //
 protected:
  // Called by the http server(on selector thread) to handle a http request.
  // This method is registered to a http server, to receive requests.
  void CallbackProcessHttpRequest(http::ServerRequest* req);
  // Called after authentication takes place
  void CallbackProcessHttpRequestAuthorized(http::ServerRequest* req,
      net::UserAuthenticator::Answer auth_answer);

  // Reply to the given request. The request should have been prepared already,
  // containing all the data that needs to be sent back to client.
  void CallbackReplyToHTTPRequest(http::ServerRequest* req,
                                  http::HttpReturnCode rCode);

  //////////////////////////////////////////////////////////////////////
  //
  // Methods available to any external thread (worker threads).
  //
  // create a reply rpc-packet, and queue a closure to send it.
  void WriteReply(uint32 xid,
                  rpc::REPLY_STATUS status,
                  const io::MemoryStream& result);

  //////////////////////////////////////////////////////////////////////
  //
  //           IResultHandler interface methods
  //
  // Called by the execution system (possibly a worker) to announce
  // the result of a query.
  void HandleRPCResult(const rpc::Query& q);

 protected:
  // The http server using this processor.
  http::Server* http_server_;

  // The http url path where we listen for requests.
  string http_path_;

  // Used for authenticating requests. We don't own it.
  net::UserAuthenticator* authenticator_;

  // The rpc services manager.
  rpc::ServicesManager& services_manager_;

  // the query executor. Estabilished in constructor.
  IAsyncQueryExecutor& async_query_executor_;

  // true if we're registered in the executor.
  bool registered_to_query_executor_;

  struct RpcRequest {
    http::ServerRequest* req_;
    rpc::CodecId codec_;
    uint32 xid_;
    RpcRequest(http::ServerRequest* req,
               rpc::CodecId codec,
               uint32 xid)
        : req_(req), codec_(codec), xid_(xid) {}
    ~RpcRequest() {}
  };

  // map of requests being executed: [http request * -> RpcRequest]
  typedef map<uint32, RpcRequest*> MapOfRequests;
  MapOfRequests requests_in_execution_;

  // synchronize access to map requests_in_execution_
  synch::Mutex access_requests_in_execution_;

  // Javascript that needs included in http auto forms
  string auto_forms_js_;
  string auto_forms_log_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(HttpProcessor);
};
}
#endif  // __NET_RPC_LIB_SERVER_RPC_HTTP_PROCESSOR_H__
