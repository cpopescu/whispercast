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

#ifndef __NET_RPC_LIB_RPC_SERVER_EXECUTION_RPC_EXECUTION_SIMPLE__
#define __NET_RPC_LIB_RPC_SERVER_EXECUTION_RPC_EXECUTION_SIMPLE__

#include <whisperlib/net/rpc/lib/server/irpc_async_query_executor.h>
#include <whisperlib/net/rpc/lib/server/rpc_services_manager.h>

namespace rpc {

class ExecutionSimple : public IAsyncQueryExecutor {
 public:
  explicit ExecutionSimple(rpc::ServicesManager& services_manager,
                           uint64 max_concurent_queries = 999)
      : IAsyncQueryExecutor(max_concurent_queries),
        services_manager_(services_manager),
        query_completed_callback_(
            NewPermanentCallback(this, &ExecutionSimple::QueryCompleted)) {
  }

  virtual ~ExecutionSimple() {
    delete query_completed_callback_;
    query_completed_callback_ = NULL;
  }

 private:
  //  Asynchronous called (private thread context) to pass back a
  //  query's result.
  // input:
  //   q: a query launched in execution by QueueRPC. The service implementation
  //      executed the query in the meanwhile (maybe directly on invoke,
  //      maybe asynchronously), and now it returns "q" containing the result.
  //      We need to propagate this query down to the transport layer.
  //
  //      The query q is no longer needed after this call, and should
  //      be automatically deleted.
  void QueryCompleted(const rpc::Query& q) {
    ReturnResult(q);
  }

 public:
  //////////////////////////////////////////////////////////////////////
  //
  // IAsyncQueryExecutor interface:
  //
  // Execute the given query "q", and return it QueryCompleted.
  bool InternalQueueRPC(rpc::Query* q) {
    q->SetCompletionCallback(query_completed_callback_);
    return services_manager_.Call(q);
  }

 protected:
  rpc::ServicesManager& services_manager_;

  // a permanent callback to the local method QueryCompleted to be
  // set in every rpc::Query
  Callback1<const rpc::Query &>* query_completed_callback_;
 private:
  DISALLOW_EVIL_CONSTRUCTORS(ExecutionSimple);
};
}
#endif  // __NET_RPC_LIB_RPC_SERVER_EXECUTION_RPC_EXECUTION_SIMPLE__
