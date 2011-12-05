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

#include "common/base/strutil.h"
#include "common/base/common.h"
#include "common/base/scoped_ptr.h"
#include "net/rpc/lib/client/rpc_client_connection_http.h"
#include "net/rpc/lib/rpc_constants.h"

namespace rpc {

ClientConnectionHTTP::ClientConnectionHTTP(
    net::Selector& selector,
    net::NetFactory& net_factory,
    net::PROTOCOL net_protocol,
    const http::ClientParams& params,
    const net::HostPort& addr,
    rpc::CODEC_ID codec_id,
    const string& http_request_path)
    : IClientConnection(selector, rpc::CONNECTION_HTTP, codec_id),
      selector_(selector),
      net_factory_(net_factory),
      net_protocol_(net_protocol),
      remote_addr_(addr),
      http_params_(new http::ClientParams(params)),
      http_protocol_(new http::ClientProtocol(
                         http_params_,
                         new http::SimpleClientConnection(&selector_,
                                                          net_factory_,
                                                          net_protocol),
                         remote_addr_)),
      http_request_path_(http_request_path),
      http_auth_user_(),
      http_auth_pswd_(),
      disconnect_completed_(false, true),
      queries_() {
}

ClientConnectionHTTP::~ClientConnectionHTTP() {
  Close();
  delete http_params_;
  http_params_ = NULL;
  CHECK(queries_.empty());
}
void ClientConnectionHTTP::SetHttpAuthentication(
    const string& user, const string& pswd) {
  http_auth_user_ = user;
  http_auth_pswd_ = pswd;
}
const char* ClientConnectionHTTP::Error() const {
  // http::ClientErrorName(http_protocol_->conn_error_);
  return "no_error";
}

void ClientConnectionHTTP::Close() {
  // Run this function no mater what. Maybe the selector just closed the
  // connection, or a HandleError is in progress.. it doesn't matter.
  // Testing for socket alive does not assure that the selector is not executing
  // something in here.
  //  e.g. [selector] -> rpc::ClientConnectionTCP::Close which closes fd but
  //                     does not finish execution of Close()
  //       [external] -> destructor -> Close -> test fd_ is invalid
  //                     -> go on with destructor
  //       [selector] -> still in rpc::ClientConnectionTCP::Close ==> Crash
  //
  if ( !selector_.IsInSelectThread() && !selector_.IsExiting() ) {
    disconnect_completed_.Reset();
    selector_.RunInSelectLoop(
      NewCallback(this, &rpc::ClientConnectionHTTP::Close));
    disconnect_completed_.Wait();
    return;
  }

  // Notify the HTTP layer, to complete all pending queries with error.
  http_protocol_->Clear();
  delete http_protocol_;
  http_protocol_ = NULL;

  CHECK(queries_.empty()) << "#" << queries_.size() << " queries pending.";

  LOG_INFO << "Disconnect from " << remote_address() << " completed";
  disconnect_completed_.Signal();
}

//////////////////////////////////////////////////////////////////////
//
//         IClientConnection interface methods
//
void ClientConnectionHTTP::Send(const rpc::Message* p) {
  // Keep this method [THREAD SAFE] !

  // Create a http request containing:
  //  - the codec id
  //  - the encoded RPC message
  //

  LOG_INFO << "Sending request on http path: [" << http_request_path_ << "]";
  // req will be deleted on CallbackRequestDone
  http::ClientRequest* req = new http::ClientRequest(http::METHOD_POST,
                                                     http_request_path_);
  CHECK_NOT_NULL(req);

  // write codec ID
  bool success = req->request()->client_header()->AddField(
    string(RPC_HTTP_FIELD_CODEC_ID),
    strutil::StringPrintf("%d", GetCodec().GetCodecID()), false);
  CHECK(success);

  // write authentication header
  req->request()->client_header()->SetAuthorizationField(
      http_auth_user_, http_auth_pswd_);

  // write RPC message
  GetCodec().EncodePacket(*req->request()->client_data(), *p);

  // IMPORTANT:
  //  The query result (or failure) MUST be delivered on a different call !
  //  Many times we synchronize the Call and HandleCallResult, so the Call
  //  should not call HandleCallResult right away.
  //  We also defend against stack-overflow:
  //   - call async function
  //     - call completion handler
  //       - call another async function
  //         - ...

  // Send the http request (through a callback from the selector thread)
  //
  selector_.RunInSelectLoop(
    NewCallback(this, &rpc::ClientConnectionHTTP::SendRequest, req, p));
}

void ClientConnectionHTTP::Cancel(uint32 xid) {
  // cannot cancel http request
  UNUSED_ALWAYS(xid);
}

//////////////////////////////////////////////////////////////////////

void ClientConnectionHTTP::SendRequest(http::ClientRequest* req,
                                       const rpc::Message* p) {
  if ( !selector_.IsInSelectThread() ) {
    selector_.RunInSelectLoop(
      NewCallback(this, &rpc::ClientConnectionHTTP::SendRequest, req, p));
    return;
  }
  if ( !http_protocol_->IsAlive() ) {
    LOG_DEBUG << "http_protocol_ " << (http_protocol_ ? "DEAD" : "NULL")
              << " , recreating http::ClientProtocol";
    delete http_protocol_;
    http_protocol_ = new http::ClientProtocol(
      http_params_,
      new http::SimpleClientConnection(&selector_, net_factory_,
                                       net_protocol_),
      remote_addr_);
  }
  queries_.insert(make_pair(req, p));
  http_protocol_->SendRequest(
    req, NewCallback(this,
                     &rpc::ClientConnectionHTTP::CallbackRequestDone,
                     req));
}

void ClientConnectionHTTP::CallbackRequestDone(http::ClientRequest* req) {
  CHECK(selector_.IsInSelectThread());
  CHECK_NOT_NULL(req);
  scoped_ptr<http::ClientRequest> auto_del_request(req);

  // No need to synchornize queries_. All changes happen in selector thread!
  QueryMap::iterator it = queries_.find(req);
  if ( it == queries_.end() ) {
    LOG_ERROR << "Cannot find active query for http ClientRequest: "
              << req->name();
    return;
  }
  CHECK_EQ(it->first, req);
  scoped_ptr<const rpc::Message> q(it->second);
  queries_.erase(it);

  const http::ClientError cli_error = req->error();
  const http::HttpReturnCode ret_code =
    req->request()->server_header()->status_code();
  if ( cli_error != http::CONN_OK || ret_code != http::OK ) {
    const string error =
      string("client_error=") + http::ClientErrorName(cli_error) +
      string(" http_return_code=") + http::GetHttpReturnCodeName(ret_code);
    LOG_ERROR << "Error: req[0x" << std::hex << req << std::dec << "]" << error;
    NotifySendFailed(q.release(), RPC_CONN_ERROR);
    return;
  }

  // wrap request buffer
  io::MemoryStream* const in = req->request()->server_data();
  in->MarkerSet(); // to be able to restore data on error & print it for debug

  // decode RPC message from inside http request
  rpc::Message* msg = new rpc::Message();
  DECODE_RESULT result = GetCodec().DecodePacket(*in, *msg);
  if ( result == DECODE_RESULT_ERROR ) {
    in->MarkerRestore();
    LOG_ERROR << "RPC Decode Message error, in http reply.";
    delete msg;
    return;
  }

  if ( result == DECODE_RESULT_NOT_ENOUGH_DATA ) {
    in->MarkerRestore();
    LOG_ERROR << "RPC Decode Message insufficient data, in http reply.";
    delete msg;
    return;
  }
  CHECK_EQ(result, DECODE_RESULT_SUCCESS);
  in->MarkerClear();

  // send msg to the IConnection which will send RPC response back to
  // application
  HandleResponse(msg);
}
}
