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
#define __NET_RPC_LIB_RPC_SERVER_EXECUTION_RPC_EXECUTION_WORKER__

#include <sys/types.h>             // for pthread_t
#include <whisperlib/common/sync/event.h>
#include <whisperlib/net/rpc/lib/server/execution/rpc_execution_pool.h>

namespace rpc {
// This is a worker thread. It is strictly related to the rpc::ExecutionPool.
// It receives rpc::Queries, executes the query by invoking the
// rpc::ServicesManager and may send the rpc::Response back to the
// execution layer.
//
// The service implementation may call the query's completion handler
// right away from the worker's thread context, or it may delay execution
// on a separate thread of its own and call query's completion handler later.

class ExecutionWorker {
 public:
  explicit ExecutionWorker(ExecutionPool& pool);
  ~ExecutionWorker();

  // Start worker thread.
  bool Start();

  // Stop worker thread.
  void Stop();

  // Test if worker thread is running.
  bool IsRunning();

 protected:
  // internal thread ID
  pthread_t tid_;

  // used to signal the internal thread is time to terminate
  bool end_;

  // used to wait internal thread initialization
  synch::Event evThreadInit_;

  // used to wait(with timeout) internal thread termination
  // NOTE: this is necessary because pthread_join does not have a timeout.
  synch::Event evThreadExit_;

  // the execution pool this worker is part of
  rpc::ExecutionPool& pool_;

  static void* ThreadProc(void* param);
  void* Run();

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ExecutionWorker);
};
}
#endif   // __NET_RPC_LIB_RPC_SERVER_EXECUTION_RPC_EXECUTION_WORKER__
