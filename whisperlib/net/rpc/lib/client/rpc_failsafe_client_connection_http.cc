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

#include "common/base/strutil.h"
#include "common/base/common.h"
#include "common/base/scoped_ptr.h"
#include "net/rpc/lib/codec/rpc_codec.h"
#include "net/rpc/lib/client/rpc_failsafe_client_connection_http.h"
#include "net/rpc/lib/rpc_constants.h"

namespace rpc {

FailsafeClientConnectionHTTP::FailsafeClientConnectionHTTP(
    net::Selector* selector,
    rpc::CodecId codec,
    http::FailSafeClient* failsafe_client,
    const string& http_request_path,
    const string& auth_user,
    const string& auth_pass)
    : IClientConnection(*selector, rpc::CONNECTION_FAILSAFE_HTTP, codec),
      failsafe_client_(failsafe_client),
      http_request_path_(http_request_path),
      auth_user_(auth_user),
      auth_pass_(auth_pass) {
}

FailsafeClientConnectionHTTP::~FailsafeClientConnectionHTTP() {
  Close();
  delete failsafe_client_;
  CHECK(queries_.empty());
}

const char* FailsafeClientConnectionHTTP::Error() const {
  return "no_error";
}

void FailsafeClientConnectionHTTP::Close() {
  delete failsafe_client_;
  failsafe_client_ = NULL;

  CHECK(queries_.empty()) << "#" << queries_.size() << " queries pending.";
}
string FailsafeClientConnectionHTTP::ToString() const {
  return strutil::StringPrintf(
      "FailsafeClientConnectionHTTP{"
      "failsafe_client_: 0x%p, "
      "http_request_path_: %s, "
      "auth_user_: %s, "
      "auth_pass_: %s, "
      "queries_: #%zu queries}",
      failsafe_client_,
      http_request_path_.c_str(),
      auth_user_.c_str(),
      auth_pass_.c_str(),
      queries_.size());
}

void FailsafeClientConnectionHTTP::Send(const rpc::Message* p) {
  // Create a http request containing:
  //  - the codec id
  //  - the encoded RPC message
  //

  DLOG_DEBUG << "Sending request on http path: [" << http_request_path_ << "]";
  // req will be deleted on CallbackRequestDone
  http::ClientRequest* req = new http::ClientRequest(
      http::METHOD_POST, http_request_path_);
  //strutil::StringPrintf("%s/%s/%s",
  //                           http_request_path_.c_str(),
  //                          p->cbody_.service_.StdStr().c_str(),
  //                          p->cbody_.method_.StdStr().c_str()));
  CHECK_NOT_NULL(req);

  // write codec ID
  req->request()->client_header()->AddField(
      kHttpFieldCodec,
      CodecName(codec()), false);

  if ( !auth_user_.empty() ) {
    req->request()->client_header()->SetAuthorizationField(auth_user_,
                                                           auth_pass_);
  }

  // write RPC message
  EncodeBy(codec(), *p, req->request()->client_data());

  QueryStruct* qs = new QueryStruct(req, p);
  queries_.insert(make_pair(qs->xid_, qs));

  // NOTE is essential to interpose a selector loop before calling -
  //  async RPC callbacks should not return immediatly
  Closure* const req_done_callback =
      NewCallback(
          this,
          &rpc::FailsafeClientConnectionHTTP::CallbackRequestDone,
          qs);
  selector_.RunInSelectLoop(
      NewCallback(failsafe_client_,
                  &http::FailSafeClient::StartRequest,
                  qs->req_, req_done_callback));
}

void FailsafeClientConnectionHTTP::Cancel(uint32 xid) {
  QueryMap::const_iterator it = queries_.find(xid);
  if ( it != queries_.end() ) {
    it->second->cancelled_ = true;
  }
}

void FailsafeClientConnectionHTTP::CallbackRequestDone(QueryStruct* qs) {
  DCHECK(selector_.IsInSelectThread());
  CHECK(queries_.erase(qs->xid_));
  if ( !qs->cancelled_ ) {
    const http::ClientError cli_error = qs->req_->error();
    const http::HttpReturnCode ret_code =
        qs->req_->request()->server_header()->status_code();
    if ( cli_error != http::CONN_OK || ret_code != http::OK ) {
      LOG_ERROR << "Error: req[" << qs->req_->name() << ": "
                << http::ClientErrorName(cli_error)
                << " http: " << http::GetHttpReturnCodeName(ret_code);
      NotifySendFailed(qs->msg_, RPC_CONN_ERROR);
    } else {
      // wrap request buffer
      io::MemoryStream* const in = qs->req_->request()->server_data();
      in->MarkerSet(); // to be able to restore data on error & print
                       // it for debug
      // decode RPC message from inside http request
      rpc::Message* msg = new rpc::Message();
      DECODE_RESULT result = DecodeBy(codec(), *in, msg);
      if ( result == DECODE_RESULT_ERROR ) {
        LOG_ERROR << "Error in RPC encountered: "
                  << qs->msg_->cbody().service() << "::"
                  << qs->msg_->cbody().method();
        in->MarkerRestore();
        DLOG_DEBUG << "Received message: " << in->DebugString();
        delete msg;
      } else if ( result == DECODE_RESULT_NOT_ENOUGH_DATA ) {
        LOG_ERROR << "RPC Decode Message insufficient data, in http reply for "
                  << qs->msg_->cbody().service() << "::"
                  << qs->msg_->cbody().method();
        in->MarkerRestore();
        DLOG_DEBUG << "Received message: " << in;
        delete msg;
      } else {
        DCHECK_EQ(result, DECODE_RESULT_SUCCESS);
        in->MarkerClear();
        // send msg to the IConnection which will send RPC response back to
        // application
        HandleResponse(msg);
      }
    }
  }
  delete qs;
}
}
