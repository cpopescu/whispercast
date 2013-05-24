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

#ifndef __COMMON_SYNC_MUTEX_H__
#define __COMMON_SYNC_MUTEX_H__

#include <pthread.h>
#include <whisperlib/common/base/log.h>

namespace synch {

class Mutex {
 protected:
  pthread_mutex_t mutex_;
  pthread_mutexattr_t attr_;
 public:
  explicit Mutex(bool is_reentrant = false) {
    pthread_mutexattr_init(&attr_);
    if ( is_reentrant ) {
      CHECK_SYS_FUN(pthread_mutexattr_settype(&attr_, PTHREAD_MUTEX_RECURSIVE),
                    0);
    } else {
      CHECK_SYS_FUN(pthread_mutexattr_settype(&attr_, PTHREAD_MUTEX_NORMAL), 0);
    }
    CHECK_SYS_FUN(pthread_mutex_init(&mutex_, &attr_), 0);
  }
  virtual ~Mutex() {
    pthread_mutexattr_destroy(&attr_);
    CHECK_SYS_FUN(pthread_mutex_destroy(&mutex_), 0);
  }

  void Lock() {
    CHECK_SYS_FUN(pthread_mutex_lock(&mutex_), 0);
  }
  void Unlock() {
    CHECK_SYS_FUN(pthread_mutex_unlock(&mutex_), 0);
  }

  // Classic named Lock
  void P() {  Lock(); }
  // Classic named Unlock
  void V() {  Unlock(); }

 private:
  DISALLOW_EVIL_CONSTRUCTORS(Mutex);
};

// Utility class for scope protected locking
class MutexLocker {
 public:
  explicit MutexLocker(Mutex* mutex) : mutex_(mutex) {
    if ( mutex_ != NULL ) {
      mutex_->Lock();
    }
  }
  ~MutexLocker() {
    if ( mutex_ != NULL ) {
      mutex_->Unlock();
    }
  }
 protected:
  Mutex* const mutex_;

  DISALLOW_EVIL_CONSTRUCTORS(MutexLocker);
};
}

#endif  // __COMMON_SYNC_MUTEX_H__
