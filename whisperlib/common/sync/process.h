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

#ifndef __COMMON_SYNC_PROCESS_H__
#define __COMMON_SYNC_PROCESS_H__

#include <string>
#include <vector>

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/callback.h>
#include <whisperlib/common/base/common.h>
#include <whisperlib/common/sync/thread.h>
#include <whisperlib/common/sync/event.h>

namespace process {

class Process {
 public:
  static const pid_t kInvalidPid;

  // the process is still running
  static const int kInvalidExitValue;
  // error retrieving process exit code
  static const int kErrorExitValue;
  // detached from process
  static const int kDetachedExitValue;

  // How long to wait for a process to start
  static const uint32 kStartupWaitMs;
  // How long to wait for a process to stop before killing it
  static const uint32 kKillWaitMs;

  // constructs a process from executable image and parameters
  // arg[0] should be your first argument, and NOT the program name
  // The constructors which don't specify "envp" will use current "environ".
  //
  // IMPORTANT NOTE: You MUST set the last argument as NULL in all cases
  //   e.g.
  //     new Process("/usr/sleep", "10", NULL);
  //   or with no arguments:
  //     new Process("/usr/bin/uname", NULL, NULL);
  //     // The first NULL is enough for the call to behave correctly,
  //     // the second NULL exists just to satisfy
  //     // the G_GNUC_NULL_TERMINATED sentinel attribute.
  //
  Process(const char* path, const char* arg, ...) G_GNUC_NULL_TERMINATED;
  Process(const char* path, char* const argv[], char* const envp[]);
  Process(const string& path, const vector<string>& argv);
  Process(const string& path, const
          vector<string>& argv,
          const vector<string>& envp);
  // binds to an existing process. You still have to call Start() to bind.
  explicit Process(pid_t pid);
  ~Process();

  const string& path() const;
  const vector<string>& argv() const;

  // Either starts the process or binds to an existing process (if you
  // specified a pid).
  // Returns success status. On failure, call GetLastSystemError() for
  // errno code.
  bool Start();

  // Returns the running process pid.
  // If the process is not started, returns INVALID_PID_VALUE.
  pid_t Pid() const;

  // Sends a signal to the running process.
  // Returns success status. On failure, call GetLastSystemError() for
  // errno code.
  bool Signal(int signum);

  // Kills the process.
  void Kill();

  // Detaches from the executing process. So you can delete this
  // object and the process keeps running.
  void Detach();

  //  Wait for the running process to terminate.
  // Returns:
  //  true: the process terminated.
  //        exit_status contains the process exit status.
  //  false: timeout or the process was not started. Call GetLastSystemError()
  //         for errno code.
  bool Wait(uint32 timeout_ms, int* exit_status);

  // The callback parameter is the process exit status.
  typedef Callback1<int> ExitCallback;

  // Sets a callback to be run when the process terminated.
  void SetExitCallback(ExitCallback* exit_callback);

 private:
  // Test if the process has been started once.
  bool IsStarted() const;

  // Tests if the executor thread is running (i.e. the external process
  // is running).
  bool IsRunning() const;

  // Executor thread.
  void ExecutorRun();

 private:
  const string path_;
  vector<string> argv_;
  vector<string> envp_;
  const pid_t bind_pid_;
  pid_t pid_;
  int exit_status_;

  ExitCallback* exit_callback_;

  // The thread which runns the process and wait(blocking)
  // for process termination.
  thread::Thread executor_thread_;
  // Signaled by the inner thread after initialization.
  // Used by the application to wait for thread initialization.
  synch::Event executor_thread_start_;
  // Signaled by the inner thread on termination.
  // Used by the application to wait (with timeout) for thread termination.
  synch::Event executor_thread_stop_;
  // when signaled, tells the executor thread it should terminate
  synch::Event executor_end_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(Process);
};
}

#endif  //  __COMMON_SYNC_PROCESS_H__
