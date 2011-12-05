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
#include "common/base/errno.h"
#include "common/base/timer.h"
#include "common/base/scoped_ptr.h"
#include "common/sync/process.h"

// #declared in unistd.h
// Points to an array of strings called the 'environment'.
// By convention these strings have the form 'name=value'.

// """
// is initialized as a pointer to an array of character pointers to
// the environment strings. The argv and environ arrays are each
// terminated by a null pointer. The null pointer terminating  the
// argv array is not counted in argc.
// """

#ifdef __APPLE__
#include <crt_externs.h>
#define environ (*_NSGetEnviron())
#else
extern char** environ;
#endif
#include <stdarg.h>
#include <whisperlib/common/base/strutil.h>

namespace process {

void MakeStringVector(const char* const argv[], vector<string>* out) {
  if ( argv ) {
    for ( int i = 0; argv[i] != NULL; i++ ) {
      out->push_back(string(argv[i]));
    }
  }
}

const pid_t Process::kInvalidPid = pid_t(-1);
const int Process::kInvalidExitValue = 1000;
const int Process::kErrorExitValue = 1001;
const int Process::kDetachedExitValue = 1002;

const uint32 Process::kStartupWaitMs = 5000;
const uint32 Process::kKillWaitMs = 5000;

Process::Process(const char* path, const char* arg, ...)
  : path_(path),
    bind_pid_(kInvalidPid),
    pid_(kInvalidPid),
    exit_status_(kInvalidExitValue),
    exit_callback_(NULL),
    executor_thread_(NewCallback(this, &Process::ExecutorRun)),
    executor_thread_start_(false, true),
    executor_thread_stop_(false, true),
    executor_end_(false, true) {
  va_list ap;
  ::va_start(ap, arg);
  const char* crt = arg;
  while ( crt ) {
    argv_.push_back(crt);
    crt = va_arg(ap, const char*);
  }
  ::va_end(ap);
  MakeStringVector(environ, &envp_);
}

Process::Process(const char* path, char* const argv[], char* const envp[])
  : path_(path),
    bind_pid_(kInvalidPid),
    pid_(kInvalidPid),
    exit_status_(kInvalidExitValue),
    exit_callback_(NULL),
    executor_thread_(NewCallback(this, &Process::ExecutorRun)),
    executor_thread_start_(false, true),
    executor_thread_stop_(false, true),
    executor_end_(false, true) {
  MakeStringVector(argv, &argv_);
  MakeStringVector(envp, &envp_);
}

Process::Process(const string& path, const vector<string>& argv)
  : path_(path),
    argv_(argv),
    bind_pid_(kInvalidPid),
    pid_(kInvalidPid),
    exit_status_(kInvalidExitValue),
    exit_callback_(NULL),
    executor_thread_(NewCallback(this, &Process::ExecutorRun)),
    executor_thread_start_(false, true),
    executor_thread_stop_(false, true),
    executor_end_(false, true) {
  MakeStringVector(environ, &envp_);
}

Process::Process(const string& path,
                 const vector<string>& argv,
                 const vector<string>& envp)
  : path_(path),
    argv_(argv),
    envp_(envp),
    bind_pid_(kInvalidPid),
    pid_(kInvalidPid),
    exit_status_(kInvalidExitValue),
    exit_callback_(NULL),
    executor_thread_(NewCallback(this, &Process::ExecutorRun)),
    executor_thread_start_(false, true),
    executor_thread_stop_(false, true),
    executor_end_(false, true) {
}

Process::Process(pid_t pid)
  : path_(),
    argv_(),
    envp_(),
    bind_pid_(pid),
    pid_(kInvalidPid),
    exit_status_(kInvalidExitValue),
    exit_callback_(NULL),
    executor_thread_(NewCallback(this, &Process::ExecutorRun)),
    executor_thread_start_(false, true),
    executor_thread_stop_(false, true),
    executor_end_(false, true) {
}

Process::~Process() {
  int result;
  if ( !Wait(kKillWaitMs, &result) ) {
    Kill();
  }
  CHECK(!IsRunning());
}

const string& Process::path() const {
  return path_;
}
const vector<string>& Process::argv() const {
  return argv_;
}

bool Process::Start() {
  if ( IsStarted() ) {
    return false;
  }
  if ( !executor_thread_.Start() ) {
    LOG_ERROR << "Failed to start executor thread: "
              << GetLastSystemErrorDescription();
    return false;
  }
  // 5 seconds should be enough
  if ( !executor_thread_start_.Wait(kStartupWaitMs) ) {
    LOG_ERROR << "Timeout waiting for executor thread startup";
    executor_thread_.Kill();
    executor_thread_start_.Reset();
    executor_thread_stop_.Reset();
    pid_ = kInvalidPid;
    return false;
  }
  return pid_ != kInvalidPid;
}

// Returns the running process pid.
// If the process is not started, returns kInvalidPid.
pid_t Process::Pid() const {
  return pid_;
}

// Sends a signal to the running process.
// Returns success status. On failure, call GetLastSystemError() for errno code.
bool Process::Signal(int signum) {
  if ( !IsStarted() ) {
    return false;
  }
  if ( ::kill(Pid(), signum) ) {
    LOG_ERROR << "::kill failed for pid_=" << Pid()
              << ", signal=" << signum
              << ", error=" << GetLastSystemErrorDescription();
    return false;
  }
  return true;
}

// Kills the process.
void Process::Kill() {
  if ( !IsRunning() ) {
    return;
  }

  // detach
  executor_end_.Signal();
  executor_thread_stop_.Wait(kKillWaitMs);

  // kill
  if ( ::kill(Pid(), SIGKILL) ) {
    LOG_ERROR << "::kill failed for pid_=" << Pid()
              << ", signal=" << SIGKILL
              << ", error=" << GetLastSystemErrorDescription();
  } else {
    LOG_WARNING << "Process [pid=" << Pid() << "] killed.";
  }
}

void Process::Detach() {
  if ( !IsRunning() ) {
    return;
  }
  // detach
  executor_end_.Signal();
  executor_thread_stop_.Wait(kKillWaitMs);
}

bool Process::Wait(uint32 timeout_ms, int* exit_status) {
  bool success = executor_thread_stop_.Wait(timeout_ms);
  if ( !success ) {
    // Timeout, the process is still running.
    return false;
  }
  *exit_status = exit_status_;
  return true;
}

void Process::SetExitCallback(ExitCallback* exit_callback) {
  exit_callback_ = exit_callback;
}

bool Process::IsStarted() const {
  return pid_ != kInvalidPid;
}

bool Process::IsRunning() const {
  return IsStarted() &&                      // already started
         exit_status_ == kInvalidExitValue;  // not yet terminated
}

//////////////////////////////////////////////////////////////////
//
//                  execute using execve
//
void Process::ExecutorRun() {
  if ( bind_pid_ == kInvalidPid ) {
    LOG_WARNING << "Starting process: path_=" << path_
                << " argv_=" << strutil::ToString(argv_)
                << " envp_=" << strutil::ToString(envp_);

    const char* path = path_.c_str();

    vector<string>::const_iterator it;
    uint32 i;

    scoped_array<char*> argv(new char*[argv_.size() + 2]);
    argv[0] = const_cast<char*>(path);
    for ( it = argv_.begin(), i = 1; it != argv_.end(); ++it, ++i ) {
      argv[i] = const_cast<char*>(it->c_str());
    }
    argv[argv_.size() + 1] = NULL;

    scoped_array<char*> envp(new char*[envp_.size() + 1]);
    for ( it = envp_.begin(), i = 0; it != envp_.end(); ++it, ++i ) {
      envp[i] = const_cast<char*>(it->c_str());
    }
    envp[envp_.size()] = NULL;

    pid_t pid = ::fork();
    if ( pid == 0 ) {
      // child process here
      int result = ::execve(path, argv.get(), envp.get());
      ::_exit(result);
    }
    pid_ = pid;
    LOG_WARNING << "Process [pid=" << pid_ << "] running.";
  } else {
    // test if bind_pid_ exists
    int result = ::kill(bind_pid_, 0);
    if ( result == -1 ) {
      switch ( GetLastSystemError() ) {
        case ESRCH:
          LOG_ERROR << "Process [pid=" << pid_ << "] does not exist.";
          break;
        case EPERM:
          LOG_ERROR << "Process [pid=" << pid_ << "]. You don't have "
                    << " permission to send signals to this process.";
          break;
        default:
          LOG_ERROR << "::kill failed for pid=" << pid_ << " with error="
                    << GetLastSystemErrorDescription();
      }
      exit_status_ = kErrorExitValue;
      executor_thread_start_.Signal();
      executor_thread_stop_.Signal();
      return;
    }
    pid_ = bind_pid_;
  }

  // parent thread [the child is running in background]
  executor_thread_start_.Signal();
  while ( true ) {
    int status;
    const pid_t result = ::waitpid(pid_, &status, WNOHANG);
    if ( result == -1 ) {
      LOG_ERROR << "::waitpid [pid=" << pid_ << "] failed: "
                << GetLastSystemErrorDescription();
      exit_status_ = kErrorExitValue;
      break;
    }
    if ( result == 0 ) {
      // timeout, sleep for 1 sec then try again ::waitpid
      bool success = executor_end_.Wait(1000);
      if ( success ) {
        exit_status_ = kDetachedExitValue;
        break;
      } else {
        continue;
      }
    }
    CHECK_EQ(result, pid_);
    exit_status_ = WEXITSTATUS(status);
    LOG_WARNING << "Process [pid=" << pid_ << "] terminated: path_=\""
                << path_ << "\", exit_status_=" << exit_status_;
    break;
  };
  CHECK_NE(exit_status_, kInvalidExitValue);

  if ( exit_callback_ ) {
    exit_callback_->Run(exit_status_);
  }

  executor_thread_stop_.Signal();
}
}
