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

#ifndef __COMMON_SYNC_SYSTEM_H__
#define __COMMON_SYNC_SYSTEM_H__

#include <string>
#include <vector>
#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/callback.h>

#include <whisperlib/common/sync/thread.h>
#include <whisperlib/common/sync/event.h>

namespace process {

// A class that executes a shell command or an external process using the shell.
// It uses the ::system(..) function.

class System {
 public:
  static const int kInvalidExitValue = 1000;
  static const int kErrorExitValue   = 1001;

  // constructs a process from a shell command
  explicit System(const char* cmd);
  explicit System(const string& cmd);
  ~System();

  // Starts executing the command or starts the process.
  // Returns success status. On failure, call GetLastSystemError() for
  // errno code.
  bool Start();

  //  Wait for the running process to terminate.
  // Returns:
  //  true: the process terminated. exit_status contains the
  //        process exit status.
  //  false: timeout or the process was not started. Call GetLastSystemError()
  // for errno code.
  bool Wait(uint32 timeout_ms, int* exit_status);

  // The callback parameter is the process exit status.
  typedef Callback1<int> ExitCallback;

  // Sets a callback to be run when the process terminated.
  void SetExitCallback(ExitCallback* exit_callback);

 private:
  // Test if the process is running.
  bool IsRunning() const;

  // Kills the process.
  void Kill();

  // Executor thread.
  void ExecutorRun();

 private:
  const string cmd_;
  int exit_status_;

  ExitCallback* exit_callback_;

  // callback to ExecutorRun
  Closure* executor_run_callback_;
  // The thread which runns the process and wait(blocking)
  // for process termination.
  thread::Thread executor_thread_;
  // Signaled by the inner thread after initialization.
  // Used by the application to wait for thread initialization.
  synch::Event executor_thread_start_;
  // Signaled by the inner thread on termination.
  // Used by the application to wait (with timeout) for thread termination.
  synch::Event executor_thread_stop_;
  // true while the executor_thread_ is running (i.e. the system command is
  // running), false otherwise.
  bool executor_running_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(System);
};
}

#endif  // __COMMON_SYNC_SYSTEM_H__
