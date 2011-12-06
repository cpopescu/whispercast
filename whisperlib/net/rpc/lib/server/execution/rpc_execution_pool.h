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

#ifndef __NET_RPC_LIB_RPC_SERVER_EXECUTION_RPC_EXECUTION_POOL__
#define __NET_RPC_LIB_RPC_SERVER_EXECUTION_RPC_EXECUTION_POOL__

#include <list>
#include <whisperlib/common/base/types.h>
#include <whisperlib/common/sync/mutex.h>
#include <whisperlib/common/sync/event.h>
#include <whisperlib/net/rpc/lib/server/irpc_async_query_executor.h>
#include <whisperlib/net/rpc/lib/server/irpc_result_handler.h>
#include <whisperlib/net/rpc/lib/server/rpc_services_manager.h>

namespace rpc {

// This is the execution layer. It receives rpc::Querys, executes them and
// passes the rpc::Response to the IResponseHandler.
class ExecutionWorker;
class ExecutionPool : public IAsyncQueryExecutor {
 public:
  ExecutionPool(rpc::ServicesManager& servicesManager,
                uint64 max_concurent_queries = 999);
  virtual ~ExecutionPool();

  rpc::ServicesManager& GetServicesManager();

  //  Start the execution pool: start all worker threads.
  //  returns: success status. Call GetLastError() for additional information.
  bool Start(uint32 nWorkers);

  // Stop the execution pool: stop all worker threads.
  void Stop();

  // Test if the execution pool is running.
  bool IsRunning();

  //  Asynchronous called by every worker, to obtain a new query.
  //  Gets a new query from the queries_ queue. If the queue is empty, the call
  //  will wait untill a new query is queued or the timeout expires.
  //  The worker will execute the obtained query and the result will comeback
  //  through the completion handler QueryCompleted(..) in either way:
  //   1. The implementation completes the query right away,
  //      calling the completion handler from the worker context.
  //   2. The implementation delays query execution (probably using a
  //      private thread) and the completion handler is called later from
  //      a private context.
  // returns:
  //   - non-NULL = the query to be executed.
  //   - NULL = timeout. No query available.
  rpc::Query* WaitForQuery(uint32 timeout);

  //  Asynchronous called (private thread context) to pass back a query's
  //  result.
  // input:
  //   q: a query previously obtained by WaitForQuery(..),
  //      executed in the meanwhile, and now containing the result.
  //      We need to propagate this query down to the transport layer.
  //
  //      The query q is no longer needed after this call, and should
  //      be automatically deleted.
  void QueryCompleted(const rpc::Query& q);

  //////////////////////////////////////////////////////////////////////
  //         rpc::IQueryExecutor interface methods
  //
  //  Queue the given query "q" for later execution, and return fast.
  //  When execution is completed the query is passed to QueryCompleted.
  bool InternalQueueRPC(rpc::Query* q);

 protected:
  ServicesManager& servicesManager_;

  list<rpc::Query*>queries_;    // queue of queries waiting execution
  synch::Mutex sync_queries_;   // synchronize access to the queue of queries

  // Signals new queries.
  // The QueueRPC method appends a new query and signals the event.
  // The WaitForQuery method waits for this event and dequeues a query.
  synch::Event new_query_;

  // the list of internal workers
  list<rpc::ExecutionWorker*> workers_;

  // a permanent callback to the local method QueryCompleted to be set in
  // every rpc::Query
  Callback1<const rpc::Query&>* completion_callback_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ExecutionPool);
};
}

#endif  // __NET_RPC_LIB_RPC_SERVER_EXECUTION_RPC_EXECUTION_POOL__
