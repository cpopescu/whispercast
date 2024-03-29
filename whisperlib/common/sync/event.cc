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

#include <errno.h>
#include "common/base/log.h"
#include "common/sync/event.h"
#include "common/base/timer.h"

namespace synch {

Event::Event(bool is_signaled, bool manual_reset)
  : is_signaled_(is_signaled),
    manual_reset_(manual_reset) {
  CHECK_SYS_FUN(pthread_mutex_init(&mutex_, NULL), 0);
  CHECK_SYS_FUN(pthread_cond_init(&cond_, NULL), 0);
}
Event::~Event() {
  CHECK_SYS_FUN(pthread_cond_destroy(&cond_), 0);
  CHECK_SYS_FUN(pthread_mutex_destroy(&mutex_), 0);
}

void Event::Signal() {
  CHECK_SYS_FUN(pthread_mutex_lock(&mutex_), 0);
  is_signaled_ = true;
  if ( manual_reset_ ) {
    // manual reset event => wakes up all waiting thread
    CHECK_SYS_FUN(pthread_cond_broadcast(&cond_), 0);
  } else {
    // auto reset event => wakes up just 1 thread
    CHECK_SYS_FUN(pthread_cond_signal(&cond_), 0);
  }
  CHECK_SYS_FUN(pthread_mutex_unlock(&mutex_), 0);
}

void Event::Reset() {
  DCHECK(manual_reset_);
  CHECK_SYS_FUN(pthread_mutex_lock(&mutex_), 0);
  is_signaled_ = false;
  CHECK_SYS_FUN(pthread_mutex_unlock(&mutex_), 0);
}

void Event::Wait() {
  CHECK_SYS_FUN(pthread_mutex_lock(&mutex_), 0);
  while ( !is_signaled_ ) {
    CHECK_SYS_FUN(pthread_cond_wait(&cond_, &mutex_), 0);
  }
  CHECK(AutoResetLocked());
  CHECK_SYS_FUN(pthread_mutex_unlock(&mutex_), 0);
}

bool Event::Wait(uint32 timeout_in_ms) {
  if ( kInfiniteWait == timeout_in_ms ) {
    Wait();
    return true;
  }
  CHECK_SYS_FUN(pthread_mutex_lock(&mutex_), 0);
  const struct timespec ts = timer::TimespecAbsoluteMsec(timeout_in_ms);
  const int64 ms_start = timer::TicksMsec();
  int64 ms_now = ms_start;
  while ( ms_now - ms_start < timeout_in_ms && !is_signaled_ ) {
    // NOTE: [COSMIN]: normally pthread_cond_timedwait returns ETIMEDOUT
    // after the timestamp expired, NOT sooner.
    // However, there is bug in GDB: a thread waiting in pthread_cond_timedwait
    // immediately returns ETIMEDOUT if another thread is created.
    // The workaround is to wait in a loop, util the timestamp expired
    // or the event becomes signaled.
    //
    const int result = pthread_cond_timedwait(&cond_, &mutex_, &ts);
    CHECK(result == 0 || result == ETIMEDOUT) << " Invalid result: " << result;
    ms_now = timer::TicksMsec();
  }
  bool ret = AutoResetLocked();
  CHECK_SYS_FUN(pthread_mutex_unlock(&mutex_), 0);
  return ret;
}
}
