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

#ifndef __NET_RPC_LIB_RPC_SERVER_IRPC_ASYNC_QUERY_EXECUTOR_H__
#define __NET_RPC_LIB_RPC_SERVER_IRPC_ASYNC_QUERY_EXECUTOR_H__

#include <map>
#include <whisperlib/common/base/types.h>
#include <whisperlib/common/sync/mutex.h>
#include <whisperlib/net/rpc/lib/server/rpc_core_types.h>
#include <whisperlib/net/rpc/lib/server/irpc_result_handler.h>

// Interface for rpc::Query executor. The execution layer implements
// this interface to receive queries from the rpc::ServerConnection.

namespace rpc {

class IAsyncQueryExecutor {
 public:
  IAsyncQueryExecutor(uint64 max_concurent_queries);
  virtual ~IAsyncQueryExecutor();

  // Registers the given result handler.
  // From now on the handler can receive results.
  void RegisterResultHandler(IResultHandler& resultHandler);

  // Removes the given result handler.
  // The handler will no longer receive results
  // (pending queries results will be ignored).
  void UnregisterResultHandler(IResultHandler& resultHandler);

  //  Queue the given query for asynchronous execution.
  //  The result will go to the corresponding result handler.
  // input:
  //  q: the query to be executed. Dynamically allocated, will be automatically
  //     deleted.
  // returns:
  //  Should always return true. If an error happens the query should be
  //  completed with an error suitable status.
  bool QueueRPC(rpc::Query * q);

 protected:
  //  Implementation specific, asynchronously executes the given query.
  //  When the execution finishes, the result is passed to ReturnResult(..).
  // input:
  //  q: the query to be executed. Dynamically allocated, will be automatically
  //     deleted.
  // returns:
  //  Should always return true. If an error happens the query should be
  //  completed with an error suitable status.
  virtual bool InternalQueueRPC(rpc::Query * q) = 0;

  //  Called by the superclass to send back the result of a query to
  //  the a specific IResultHandler. If that handler unregistered in
  //  the meanwhile, the result is discarded.
  // input:
  //   q: executed query, containing the result
  void ReturnResult(const rpc::Query& q);

 private:
  // map resultHandlerID -> IResultHandler*
  typedef map<uint64, IResultHandler*> MapOfResultHandlers;
  MapOfResultHandlers handlers_;

  // used to generate unique result handler id s
  uint64 next_result_handler_id_;

  // The maximum number of parallel executing queries.
  // Beyond this number we reject new queries.
  const uint64 max_concurent_queries_;
  // the number of queries in execution at any given moment
  uint64 concurent_queries_;

  // permanent callback to ReturnResult
  Callback1<const rpc::Query&>* return_result_callback_;

  // synchronize access to handlers_
  synch::Mutex sync_;

  DISALLOW_EVIL_CONSTRUCTORS(IAsyncQueryExecutor);
};
}
#endif   //  __NET_RPC_LIB_RPC_SERVER_IRPC_ASYNC_QUERY_EXECUTOR_H__
