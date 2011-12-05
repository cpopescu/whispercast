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

#include "common/base/errno.h"
#include "common/base/scoped_ptr.h"
#include "net/rpc/lib/client/irpc_client_connection.h"
#include "net/rpc/lib/codec/rpc_codec_factory.h"

namespace rpc {

const char* ConnectionTypeName(rpc::CONNECTION_TYPE connection_type) {
  switch ( connection_type ) {
    case rpc::CONNECTION_TCP:
      return "tcp";
    case rpc::CONNECTION_HTTP:
      return "http";
    default:
      LOG_FATAL << "no such connection_type: " << connection_type;
      return "unknown";
  }
}

IClientConnection::IClientConnection(net::Selector& selector,
                                     rpc::CONNECTION_TYPE connection_type,
                                     rpc::CODEC_ID codec_id)
  : selector_(selector),
    connection_type_(connection_type),
    codec_(rpc::CodecFactory::Create(codec_id)),
    error_(),
    next_xid_(1),
    sync_next_xid_(),
    response_map_(),
    sync_response_map_(),
    timeouter_(&selector_,
               NewPermanentCallback(this, &IClientConnection::NotifyTimeout)),
    done_clear_timeouts_(false, true),
    done_cancel_query_(false, false),
    done_cancel_all_queries_(false, false) {
}

IClientConnection::~IClientConnection() {
  // The implementation(by deriving from us) is the first to be destroyed.
  // So it MUST call NotifyConnectionClosed, triggering the completion
  // of all pending queries.

  /* TODO(cosmin): remove. See comment above.
  // Cancel all pending queries. These are here because the
  // connection may have been closed while waiting for rpc results.
  CancelAllQueries();
  */

  CHECK(response_map_.empty())
    << "#" << response_map_.size() << " calls pending";

  delete codec_;
  codec_ = NULL;
}

uint32 IClientConnection::GenerateNextXID() {
  synch::MutexLocker lock(&sync_next_xid_);
  return next_xid_++;
}

void IClientConnection::SendQuery(uint32 xid,
                                  const std::string& service,
                                  const std::string& method,
                                  io::MemoryStream& params) {
  rpc::Message* const p = new rpc::Message();

  // fill in the header
  rpc::Message::Header& header = p->header_;
  header.xid_ = xid;
  header.msgType_ = RPC_CALL;

  // fill in call body
  rpc::Message::CallBody& body = p->cbody_;
  body.service_ = service;
  body.method_ = method;
  body.params_.AppendStreamNonDestructive(&params);

  Send(p);
}

void IClientConnection::HandleResponse(const rpc::Message* msg) {
  CHECK(selector_.IsInSelectThread());
  CHECK_NOT_NULL(msg);
  scoped_ptr<const rpc::Message> auto_del_msg(msg);

  // check msg type correctness
  if ( msg->header_.msgType_ != RPC_REPLY ) {
    LOG_ERROR << "Received msgType="
              << rpc::MessageTypeName(msg->header_.msgType_)
              << " expected: " << rpc::MessageTypeName(RPC_REPLY);
    return;
  }

  LOG_DEBUG << "Received packet: " << *msg;

  const uint32 xid = msg->header_.xid_;
  const rpc::REPLY_STATUS status =
    static_cast<rpc::REPLY_STATUS>(msg->rbody_.replyStatus_);
  const io::MemoryStream& result = msg->rbody_.result_;
  CompleteQuery(xid, status, result);

  // - the msg is auto-deleted
}

void IClientConnection::NotifySendFailed(const rpc::Message* msg,
                                         rpc::REPLY_STATUS status) {
  CHECK(selector_.IsInSelectThread());
  CHECK_NOT_NULL(msg);
  CHECK_GE(status, 100);  // the status should be a client failure
  scoped_ptr<const rpc::Message> auto_del_msg(msg);

  // check msg type correctness
  CHECK(msg->header_.msgType_ == RPC_CALL)
      << "HandleSendError msgType="
      << rpc::MessageTypeName(msg->header_.msgType_)
      << " expected: " << rpc::MessageTypeName(RPC_CALL);

  const uint32 xid = msg->header_.xid_;

  // Remove the timeout on current xid, to avoid unnecessary HandleTimeout call.
  // However, not removing it is not a bug. The HandleTimeout() may be executing
  // right now.
  ClearTimeout(xid);

  // there should be a ResponseCallback for the failed message
  //
  ResponseCallback * response_callback = PopResponseCallbackSync(xid);
  if ( !response_callback ) {
    LOG_ERROR << "Cannot find a ResponseCallback, xid: " << xid;
    return;
  }

  // Careful not to call response_callback or anything else in
  // ServiceWrapper while keeping the sync_response_map_ locked.
  // Because another thread may try to send something through
  // the ServiceWrapper and then through us.

  // deliver FAIL result through asynchronous callback
  //
  const io::MemoryStream empty_result;
  response_callback->Run(xid, status, empty_result);

  // the response_callback may destroy itself, but that's his business.
}
void IClientConnection::NotifyConnectionClosed() {
  //
  // By default: all pending queries will complete on timeout.
  // But for performance and clarity: go through all pending queries and
  // finish them with error response.
  //
  CompleteAllQueries(RPC_CONN_CLOSED);
}

void IClientConnection::NotifyTimeout(int64 xid) {
  // cancel Send packet in implementation specific layer
  // Not required, this is just an optimization.
  Cancel(xid);

  const io::MemoryStream empty_result;
  CompleteQuery(xid, RPC_QUERY_TIMEOUT, empty_result);
}

void IClientConnection::AddResponseCallbackNoSync(
    uint32 xid, ResponseCallback * response_callback) {
  // WARNING! this method is not SYNCHRONIZED!
  // Use it only with sync_response_map_ LOCKED!
  const bool success = response_map_.insert(
      make_pair(xid, response_callback)).second;
  CHECK(success) << "The client is already waiting a response on xid=" << xid;
}

IClientConnection::ResponseCallback *
IClientConnection::PopResponseCallbackSync(uint32 xid) {
  synch::MutexLocker lock(&sync_response_map_);
  ResponseCallbackMap::iterator it = response_map_.find(xid);
  if ( it == response_map_.end() ) {
    return NULL;
  }
  ResponseCallback * response_callback = it->second;
  response_map_.erase(it);
  return response_callback;
}

void IClientConnection::AddTimeout(int64 xid, int64 timeout) {
  if ( !selector_.IsInSelectThread() ) {
    selector_.RunInSelectLoop(
        NewCallback(this, &IClientConnection::AddTimeout, xid, timeout));
    return;
  }
  timeouter_.SetTimeout(xid, timeout);
}

void IClientConnection::ClearTimeout(int64 xid) {
  if ( !selector_.IsInSelectThread() ) {
    selector_.RunInSelectLoop(
        NewCallback(this, &IClientConnection::ClearTimeout, xid));
    return;
  }
  timeouter_.UnsetTimeout(xid);
}

void IClientConnection::ClearTimeouts() {
  if ( !selector_.IsInSelectThread() ) {
    done_clear_timeouts_.Reset();
    selector_.RunInSelectLoop(
        NewCallback(this, &IClientConnection::ClearTimeouts));
    if ( !done_clear_timeouts_.Wait(10000) ) {
      LOG_ERROR << "Timeout waiting for ClearTimeouts()";
    }
    return;
  }
  timeouter_.UnsetAllTimeouts();
  done_clear_timeouts_.Signal();
}

void __ReceiveSynchronousQueryResult(
    synch::Event* received,
    rpc::REPLY_STATUS* status_out, io::MemoryStream* result_out,
    uint32 qid, rpc::REPLY_STATUS status_in, const io::MemoryStream& result) {
  *status_out = status_in;
  result_out->AppendStreamNonDestructive(&result);
  received->Signal();
}

void IClientConnection::Query(const std::string& service,
                              const std::string& method,
                              io::MemoryStream& params,
                              uint32 timeout,
                              rpc::REPLY_STATUS& status,
                              io::MemoryStream& result) {
  CHECK(result.IsEmpty());
  DLOG_DEBUG << "Query service=" << service
             << " method=" << method
             << " params=" << params.DebugString()
             << " timeout=" << timeout;

  synch::Event received(false, true);
  AsyncQuery(service, method, params, timeout,
      NewCallback(&__ReceiveSynchronousQueryResult,
                  &received, &status, &result));
  received.Wait();
}

uint32 IClientConnection::AsyncQuery(const std::string& service,
                                     const std::string& method,
                                     io::MemoryStream& params,
                                     uint32 timeout,
                                     ResponseCallback* response_callback) {
  CHECK(response_callback);

  LOG_DEBUG << "AsyncQuery service=" << service
            << " method=" << method
            << " params=" << params.DebugString()
            << " timeout=" << timeout;

  // generate transaction ID
  const uint32 xid = GenerateNextXID();
  {
    synch::MutexLocker lock(&sync_response_map_);
    AddResponseCallbackNoSync(xid, response_callback);
    AddTimeout(xid, timeout);
  }

  // WARNING: SendQuery with sync_response_map_ UNLOCKED !
  //          Because SendQuery may call HandleResponse (on FAIL) which locks
  //          sync_response_map_ !
  SendQuery(xid, service, method, params);
  return xid;
}

void IClientConnection::CompleteQuery(uint32 qid,
                                      rpc::REPLY_STATUS status,
                                      const io::MemoryStream & result) {
  // Remove the timeout on current xid, to avoid unnecessary HandleTimeout call.
  // However, not removing it is not a bug. The HandleTimeout() may be executing
  // right now.
  ClearTimeout(qid);

  ResponseCallback * response_callback = PopResponseCallbackSync(qid);
  if ( !response_callback ) {
    LOG_ERROR << "Cannot find a ResponseCallback, qid: " << qid;
    return;
  }

  // deliver given status & result through asynchronous callback
  //
  response_callback->Run(qid, status, result);

  // - the response_callback may auto-delete itself, but that's its business.
}

void IClientConnection::CompleteAllQueries(rpc::REPLY_STATUS status) {
  ClearTimeouts();

  // make a copy of response_map, so that we may call the response handler
  // without locking the sync_response_map_
  //
  ResponseCallbackMap tmp;
  {
    synch::MutexLocker lock(&sync_response_map_);
    tmp = response_map_;
    response_map_.clear();
  }

  if ( !tmp.empty() ) {
    LOG_ERROR << "CompleteAllQueries: completing "
              << tmp.size() << " pending queries with status: "
              << status << "(" << rpc::ReplyStatusName(status) << ")";
  }

  // deliver given status to all pending queries
  //
  const io::MemoryStream empty_result;
  for ( ResponseCallbackMap::iterator it = tmp.begin();
        it != tmp.end(); ++it) {
    uint32 qid = it->first;
    ResponseCallback & response_callback = *it->second;
    response_callback.Run(qid, status, empty_result);
  }
}

// Synchronizes with selector thread.
// Waits for selector thread to call this method, witch means the selector
// is not processing something else.
void WaitForSelectorThread(net::Selector * selector, synch::Event * ev = NULL) {
  if ( !selector->IsInSelectThread() ) {
    synch::Event sync_selector(false, true);
    selector->RunInSelectLoop(
        NewCallback(&WaitForSelectorThread, selector, &sync_selector));
    sync_selector.Wait();
    return;
  }

  if ( ev ) {
    ev->Signal();
  }
}

void IClientConnection::CancelQuery(uint32 qid) {
  ClearTimeout(qid);

  ResponseCallback * response_callback = PopResponseCallbackSync(qid);
  if ( !response_callback ) {
    LOG_ERROR << "Cannot find a ResponseCallback for qid: " << qid;
    WaitForSelectorThread(&selector_);
    return;
  }

  // Careful not to call response_callback or anything else in
  // ServiceWrapper while keeping the sync_response_map_ locked.
  // Because another thread may try to send something through
  // the ServiceWrapper and then through us.

  if ( !response_callback->is_permanent() ) {
    delete response_callback;
  }
}

void IClientConnection::CancelAllQueries() {
  ClearTimeouts();

  synch::MutexLocker lock(&sync_response_map_);

  // remove all ResponseCallbacks, delete only the non-permanent ones
  // Do NOT call ResponseCallback with sync_response_map_ LOCKED!
  //
  for ( ResponseCallbackMap::iterator it = response_map_.begin();
        it != response_map_.end(); ++it) {
    ResponseCallback * response_callback = it->second;
    if ( !response_callback->is_permanent() ) {
      delete response_callback;
    }
  }
  response_map_.clear();

  WaitForSelectorThread(&selector_);
}
}
