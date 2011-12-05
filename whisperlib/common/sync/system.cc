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

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include "common/sync/system.h"
#include "common/base/errno.h"

namespace process {

System::System(const char* cmd)
  : cmd_(cmd),
    exit_status_(kInvalidExitValue),
    exit_callback_(NULL),
    executor_run_callback_(NewPermanentCallback(this, &System::ExecutorRun)),
    executor_thread_(executor_run_callback_),
    executor_thread_start_(false, true),
    executor_thread_stop_(false, true),
    executor_running_(false) {
}
System::System(const string& cmd)
  : cmd_(cmd),
    exit_status_(kInvalidExitValue),
    exit_callback_(NULL),
    executor_run_callback_(NewPermanentCallback(this, &System::ExecutorRun)),
    executor_thread_(executor_run_callback_),
    executor_thread_start_(false, true),
    executor_thread_stop_(false, true),
    executor_running_(false) {
}
System::~System() {
  CHECK(!IsRunning());
  delete executor_run_callback_;
  executor_run_callback_ = NULL;
}

bool System::Start() {
  if ( IsRunning() ) {
    return false;
  }
  executor_thread_start_.Reset();
  executor_thread_stop_.Reset();
  executor_running_ = false;

  if ( !executor_thread_.Start() ) {
    LOG_ERROR << "Failed to start executor thread: "
              << GetLastSystemErrorDescription();
    return false;
  }

  if ( !executor_thread_start_.Wait(5000) ) {
    LOG_ERROR << "Timeout waiting for executor thread startup";
    executor_thread_.Kill();
    executor_thread_start_.Reset();
    executor_thread_stop_.Reset();
    executor_running_ = false;
    return false;
  }
  return true;
}

bool System::Wait(uint32 timeout_ms, int* exit_status) {
  if ( !executor_thread_stop_.Wait(timeout_ms) ) {
    // Timeout, the process is still running.
    return false;
  }
  *exit_status = exit_status_;
  return true;
}

void System::SetExitCallback(ExitCallback* exit_callback) {
  exit_callback_ = exit_callback;
}

bool System::IsRunning() const {
  return executor_running_;
}

void System::Kill() {
  LOG_FATAL << "Cannot kill a ::system command";
}

void System::ExecutorRun() {
  executor_running_ = true;
  executor_thread_start_.Signal();

  LOG_WARNING << "Executing system command: " << cmd_;

  int result = ::system(cmd_.c_str());
  if ( result == -1 ) {
    LOG_ERROR << "System command failed for cmd: " << cmd_ << " error: "
              << GetLastSystemErrorDescription();
    exit_status_ = kErrorExitValue;
  } else {
    exit_status_ = WEXITSTATUS(result);
    LOG_WARNING << "System command terminated: exit_status_=" << exit_status_;
    if (WIFSIGNALED(result) && (WTERMSIG(result) == SIGINT ||
                                WTERMSIG(result) == SIGQUIT)) {
      // A SIGINT or SIGQUIT was received while running the child.
      // Send ourselves SIGINT.
      ::kill(::getpid(), SIGINT);
    }
    if ( exit_status_ == 127 ) {
      LOG_WARNING << "System command may have failed with: '/bin/sh' "
                  << "could not be executed";
    }
  }

  if ( exit_callback_ ) {
    exit_callback_->Run(exit_status_);
  }

  executor_running_ = false;
  executor_thread_stop_.Signal();
}
}
