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

#include <signal.h>
#include "common/base/types.h"
#include "common/base/log.h"
#include "common/base/system.h"
#include "common/base/errno.h"
#include "common/sync/event.h"
#include "common/sync/process.h"
#include "common/base/scoped_ptr.h"

#define LOG_TEST LOG(INFO)

void ProcessTerminatedCallback(int exit_status) {
  LOG_TEST << "ProcessTerminatedCallback exit_status = " << exit_status;
}

int main(int argc, char** argv) {
  common::Init(argc, argv);

  const char* path = "/bin/sleep";

  LOG_TEST << "Testing run & wait with timeout.";
  {
    scoped_ptr<process::Process> process(new process::Process(
                                           path, "3", NULL));
    process->SetExitCallback(NewCallback(&ProcessTerminatedCallback));
    if ( !process->Start() ) {
      LOG_FATAL << "failed to start process: "
                << GetLastSystemErrorDescription();
    }
    LOG_TEST << "You should see child process path=\""
             << path << "\" , pid=" << process->Pid() << " running.";
    LOG_TEST << "Close the child process whenever you want.";

    int exit_status;
    while ( true ) {
      if ( process->Wait(5000, &exit_status) ) {
        break;
      }
      LOG_TEST << "Timeout... please close the child.";
    }
    LOG_TEST << "Child process terminated. exit_status = " << exit_status;
  }
  LOG_TEST << "Pass.";

  LOG_TEST << "Testing run & kill on timeout.";
  {
    scoped_ptr<process::Process> process(new process::Process(
                                           path, "10", NULL));
    process->SetExitCallback(NewCallback(&ProcessTerminatedCallback));
    if ( !process->Start() ) {
      LOG_FATAL << "failed to start process: "
                << GetLastSystemErrorDescription();
    }
    LOG_TEST << "You should see child process path=\""
             << path << "\" , pid=" << process->Pid() << " running.";
    LOG_TEST << "Do not kill the child. Waiting 5 seconds.";

    int exit_status;
    if ( process->Wait(5000, &exit_status) ) {
      LOG_FATAL << "Child process terminated unexpectedly. exit_status = "
               << exit_status;
    }

    LOG_TEST << "Timeout... killing the child.";
    process->Kill();
    if ( !process->Wait(5000, &exit_status) ) {
      LOG_FATAL << "Failed to kill the child.";
    }
  }
  LOG_TEST << "Pass.";

  LOG_TEST << "Testing run & detach & wait with timeout.";
  {
    pid_t pid = 0;
    {
      scoped_ptr<process::Process> process(new process::Process(
                                             path, "5", NULL));
      process->SetExitCallback(NewCallback(&ProcessTerminatedCallback));
      if ( !process->Start() ) {
        LOG_FATAL << "failed to start process: "
                  << GetLastSystemErrorDescription();
      }
      pid = process->Pid();
      LOG_TEST << "You should see child process path=\""
               << path << "\" , pid=" << process->Pid() << " running.";
      process->Detach();
      LOG_TEST << "Detached from child process.";
    }
    {
      scoped_ptr<process::Process> process(new process::Process(pid));
      process->SetExitCallback(NewCallback(&ProcessTerminatedCallback));
      if ( !process->Start() ) {
        LOG_FATAL << "failed to start process: "
                  << GetLastSystemErrorDescription();
      }
      LOG_TEST << "Attached to process pid=" << process->Pid();
      LOG_TEST << "Close the child process whenever you want.";

      int exit_status;
      while ( true ) {
        if ( process->Wait(5000, &exit_status) ) {
          break;
        }
        LOG_TEST << "Timeout... please close the child.";
      }
      LOG_TEST << "Child process terminated. exit_status = " << exit_status;
    }
  }
  LOG_TEST << "Pass.";

  LOG_TEST << "Testing run & detach & kill on timeout.";
  {
    pid_t pid = 0;
    {
      scoped_ptr<process::Process> process(new process::Process(
                                             path, "20", NULL));
      process->SetExitCallback(NewCallback(&ProcessTerminatedCallback));
      if ( !process->Start() ) {
        LOG_FATAL << "failed to start process: "
                  << GetLastSystemErrorDescription();
      }
      pid = process->Pid();
      LOG_TEST << "You should see child process path=\""
               << path << "\" , pid=" << process->Pid() << " running.";
      process->Detach();
      LOG_TEST << "Detached from child process.";
    }
    {
      scoped_ptr<process::Process> process(new process::Process(pid));
      process->SetExitCallback(NewCallback(&ProcessTerminatedCallback));
      if ( !process->Start() ) {
        LOG_FATAL << "failed to start process: "
                  << GetLastSystemErrorDescription();
      }
      LOG_TEST << "Attached to process pid=" << process->Pid();
      LOG_TEST << "Waiting 5 seconds before killing the child.";

      int exit_status;
      if ( process->Wait(5000, &exit_status) ) {
        LOG_FATAL << "Child process terminated unexpectedly. exit_status = "
                 << exit_status;
      }
      LOG_TEST << "Timeout... killing the child.";
      process->Kill();
    }
  }
  LOG_TEST << "Pass.";

  common::Exit(0);
}
