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

#include "common/sync/thread_pool.h"

namespace thread {

ThreadPool::ThreadPool(uint32 pool_size, uint32 backlog_size)
  : jobs_(backlog_size),
    threads_(pool_size, NULL) {
  CHECK_GT(pool_size, 0);
  CHECK_GT(backlog_size, pool_size);
}


ThreadPool::~ThreadPool() {
  Stop(true);
}

void ThreadPool::Start() {
  CHECK(threads_[0] == NULL) << "Multiple Start()";
  for ( int i = 0; i < threads_.size(); ++i ) {
    threads_[i] = new Thread(NewCallback(this, &ThreadPool::ThreadRun));
    threads_[i]->SetJoinable();
    threads_[i]->Start();
  }
}

void ThreadPool::Stop(bool cancel_pending) {
  if ( threads_[0] == NULL ) {
    // already stopped
    return;
  }

  // cancel all jobs that were not started yet
  if ( cancel_pending ) {
    while ( true ) {
      Closure* callback = jobs_.Get(0);
      if ( callback == NULL ) {
        break;
      }
      if ( !callback->is_permanent() ) {
        delete callback;
      }
    }
  }

  // the NULL job is a signal for each thread that it should terminate
  jobs_.Put(NULL);
  for ( uint32 i = 0; i < threads_.size(); i++ ) {
    threads_[i]->Join();
    delete threads_[i];
    threads_[i] = NULL;
  }
  Closure* last_job = jobs_.Get(0);
  CHECK_NULL(last_job);
  CHECK(jobs_.IsEmpty());
}


void ThreadPool::ThreadRun() {
  while ( true ) {
    Closure* callback = jobs_.Get();
    if ( callback == NULL ) {
      jobs_.Put(NULL);
      return;
    }
    callback->Run();
  }
}
}
