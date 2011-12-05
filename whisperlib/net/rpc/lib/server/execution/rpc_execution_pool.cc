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
#include "net/rpc/lib/server/execution/rpc_execution_pool.h"
#include "net/rpc/lib/server/execution/rpc_execution_worker.h"

namespace rpc {

rpc::ExecutionPool::ExecutionPool(rpc::ServicesManager& servicesManager,
                                  uint64 max_concurent_queries)
    : IAsyncQueryExecutor(max_concurent_queries),
      servicesManager_(servicesManager),
      queries_(),
      sync_queries_(),
      new_query_(false, true),
      workers_(),
      completion_callback_(
          NewPermanentCallback(this, &rpc::ExecutionPool::QueryCompleted)) {
}

rpc::ExecutionPool::~ExecutionPool() {
  Stop();
  while ( !queries_.empty() ) {
    rpc::Query* q = queries_.front();
    queries_.pop_front();
    delete q;
  }
}

rpc::ServicesManager& rpc::ExecutionPool::GetServicesManager() {
  return servicesManager_;
}

bool rpc::ExecutionPool::Start(uint32 nWorkers) {
  if ( IsRunning() ) {
    return false;
  }
  for ( uint32 i = 0; i < nWorkers; i++ ) {
    rpc::ExecutionWorker* const w = new rpc::ExecutionWorker(*this);
    if ( !w->Start() ) {
      LOG_ERROR << "Failed to start worker #" << i
                << " reason: " << GetLastSystemErrorDescription();
      delete w;
      return false;
    }
    workers_.push_back(w);
  }
  LOG_INFO << "Started. " << nWorkers << " workers running...";
  return true;
}

void rpc::ExecutionPool::Stop() {
  if ( !IsRunning() ) {
    // if we're not running => do nothing
    return;
  }
  // stop & destroy all workers
  LOG_INFO << "Stopping... (" << workers_.size() << " workers active)";
  while ( !workers_.empty() ) {
    rpc::ExecutionWorker* const w = workers_.front();
    workers_.pop_front();
    w->Stop();
    delete w;
  }
  LOG_INFO << "Stopped.";
}

bool rpc::ExecutionPool::IsRunning() {
  return !workers_.empty();
}

rpc::Query* rpc::ExecutionPool::WaitForQuery(uint32 timeout) {
  // wait for a new query
  if ( !new_query_.Wait(timeout) ) {
    // timeout = no query, or system error (check GetLastError())
    return NULL;
  }
  // dequeue the new query and return it
  synch::MutexLocker lock(&sync_queries_);
  if ( queries_.empty() ) {
    new_query_.Reset();
    return NULL;
  }
  rpc::Query* const q = queries_.front();
  queries_.pop_front();
  if ( queries_.empty() ) {
    new_query_.Reset();
  }
  return q;
}

void rpc::ExecutionPool::QueryCompleted(const rpc::Query& q) {
  ReturnResult(q);
}

bool rpc::ExecutionPool::InternalQueueRPC(rpc::Query* q) {
  uint32 nQueries = 0;
  // queue & signal the new job
  {
    synch::MutexLocker lock(&sync_queries_);
    q->SetCompletionCallback(completion_callback_);
    queries_.push_back(q);
    nQueries = queries_.size();
    new_query_.Signal();
  }

  // TODO(cosmin): load limit is performed in IRPCAsyncQueryExecutor::QueueRPC.
  /*
  if ( nQueries > 100 ) {
    // TODO(cpopescu): Usleep is really BAD
    ::usleep(nQueries * 1);
  }
  */
  return true;
}
}
