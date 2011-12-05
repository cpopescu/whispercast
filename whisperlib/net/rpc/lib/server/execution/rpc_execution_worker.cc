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

#include <pthread.h>

#include <memory>

#include "common/base/log.h"
#include "common/base/errno.h"
#include "net/rpc/lib/server/execution/rpc_execution_worker.h"
#include "net/rpc/lib/server/rpc_services_manager.h"

// TODO(cosmin): use thread::Thread..

namespace rpc {

void* rpc::ExecutionWorker::ThreadProc(void* param) {
  CHECK(param);
  rpc::ExecutionWorker* This = (rpc::ExecutionWorker*)param;
  return This->Run();
}

void* rpc::ExecutionWorker::Run() {
  tid_ = pthread_self();
  // thread initialized
  evThreadInit_.Signal();
  while ( !end_ ) {
    // dequeue a new query
    rpc::Query* q = pool_.WaitForQuery(100);
    if ( !q ) {
      // no queries available, try again
      continue;
    }
    // execute the query
    if ( !pool_.GetServicesManager().Call(q) ) {
      q->Complete(RPC_SYSTEM_ERR,
                  GetLastSystemErrorDescription());
      continue;
    }
  }

  tid_ = 0;
  evThreadExit_.Signal();
  LOG_DEBUG << "Terminated [tid="
#ifdef __APPLE__
            << reinterpret_cast<long long unsigned int>(::pthread_self()) << "]."
#else
            << static_cast<uint32>(::pthread_self()) << "]."
#endif
      ;

  return NULL;
}

rpc::ExecutionWorker::ExecutionWorker(rpc::ExecutionPool& pool)
    : tid_(0),
      end_(false),
      evThreadInit_(false, true),
      evThreadExit_(false, true),
      pool_(pool) {
}
rpc::ExecutionWorker::~ExecutionWorker() {
  Stop();
}

bool rpc::ExecutionWorker::Start() {
  if ( IsRunning() ) {
    return false;
  }
  evThreadInit_.Reset();

  pthread_attr_t attr;
  pthread_attr_init(&attr);
  pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);

  pthread_t tid;
  const int result = pthread_create(&tid,
                                    NULL,
                                    rpc::ExecutionWorker::ThreadProc,
                                    this);
  pthread_attr_destroy(&attr);
  if ( result != 0 ) {
    LOG_ERROR  << "Cannot start thread: " << GetLastSystemErrorDescription();
    return false;
  }

  // wait 5 secs for internal thread initialization
  if ( !evThreadInit_.Wait(5000) ) {
    LOG_ERROR  << "timeout waiting for worker's thread initialization";
    pthread_cancel(tid);
    return false;
  }
  CHECK(IsRunning());
  CHECK_EQ(tid, tid_);

  return true;
}

void rpc::ExecutionWorker::Stop() {
  if ( !IsRunning() ) {
    return;
  }

  // remember tid; the executor thread will reset it on exit.
  pthread_t tid = tid_;
  // set end signal
  end_ = true;
  // wait for internal thread termination
  evThreadExit_.Wait(5000);

  // still running ?
  if ( IsRunning() ) {
    LOG_ERROR << "Failed to stop rpc worker nicely [tid=" << pthread_self()
              << "]. Thread not responding; force terminate.";
    pthread_cancel(tid);  // kill'im
    tid_ = 0;
  } else {
    void* ret;
    int result = ::pthread_join(tid, &ret);
    if ( result != 0 && result != ESRCH ) {
      // if the thread already exit we get ESRCH
      // (No thread could be found corresponding to that specified by
      // the given thread ID.)
      LOG_ERROR << "pthread_join failed for tid: " << tid << " error: "
                << GetSystemErrorDescription(result);
    }
    CHECK(tid_ == 0);
  }
}

bool rpc::ExecutionWorker::IsRunning() {
  return tid_ != 0;
}
}
