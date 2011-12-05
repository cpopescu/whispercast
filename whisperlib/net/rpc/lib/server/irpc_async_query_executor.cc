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

#include "net/rpc/lib/server/irpc_async_query_executor.h"

namespace rpc {

IAsyncQueryExecutor::IAsyncQueryExecutor(uint64 max_concurent_queries)
  : handlers_(),
    next_result_handler_id_(0),
    max_concurent_queries_(max_concurent_queries),
    concurent_queries_(0),
    return_result_callback_(
        NewPermanentCallback(this, &IAsyncQueryExecutor::ReturnResult)),
    sync_() {
}
IAsyncQueryExecutor::~IAsyncQueryExecutor() {
  CHECK(handlers_.empty()) << "#" << handlers_.size() << " handlers pending";
  CHECK(concurent_queries_ == 0)
      << "Have " << concurent_queries_ << " pending queries";
  delete return_result_callback_;
  return_result_callback_ = NULL;
}
void IAsyncQueryExecutor::RegisterResultHandler(IResultHandler& result_handler) {
  synch::MutexLocker lock(&sync_);

  // generate a new unique resultHandlerID
  uint64 result_handler_id = (next_result_handler_id_++);

  // set the ID in the handler
  result_handler.SetResultHandlerID(result_handler_id);

  // insert the [ID, {handler..}] pair in map
  CHECK(handlers_.insert(make_pair(result_handler_id, &result_handler)).second)
      << "Duplicate result_handler_id=" << result_handler_id;
}

void IAsyncQueryExecutor::UnregisterResultHandler(
    IResultHandler& result_handler) {
  const uint64 result_handler_id = result_handler.GetResultHandlerID();
  // remove handler from map
  {
    synch::MutexLocker lock(&sync_);
    CHECK(handlers_.erase(result_handler_id))
      << " result_handler_id=" << result_handler_id;
  }
}

bool IAsyncQueryExecutor::QueueRPC(rpc::Query* q) {
  {
    synch::MutexLocker lock(&sync_);

    // one more query in execution
    concurent_queries_++;

    // check we have the query's result handler registered
    #ifdef _DEBUG
    CHECK(handlers_.find(q->rid()) != handlers_.end());
    #endif

    if ( concurent_queries_ >= max_concurent_queries_ ) {
      q->SetCompletionCallback(return_result_callback_);
      q->Complete(RPC_SERVER_BUSY);
      return true;
    }
  }
  // pass this query to executor implementation
  return InternalQueueRPC(q);
}

void IAsyncQueryExecutor::ReturnResult(const rpc::Query& q) {
  // find the local map entry of the result_handler
  synch::MutexLocker lock(&sync_);

  // one less query in execution
  concurent_queries_--;

  MapOfResultHandlers::const_iterator it = handlers_.find(q.rid());
  if ( it == handlers_.end() ) {
    LOG_WARNING << "No connection to deliver completed query: " << q.ToString();
    // the connection who launched this query was closed. Ignore the result.
    return;
  }

  IResultHandler& handler = *(it->second);

  // Deliver the result.
  //  (NOTE: we need to keep the map locked, so that the handler cannot be
  //         unregistered & deleted)
  handler.HandleRPCResult(q);
}
}
