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

#include "common/base/types.h"
#include "common/base/log.h"
#include "common/base/system.h"
#include "common/base/errno.h"
#include "common/sync/event.h"
#include "common/sync/system.h"
#include "common/base/scoped_ptr.h"

#define LOG_TEST LOG(-1)

void ProcessTerminatedCallback(int exit_status) {
  LOG_TEST << "ProcessTerminatedCallback exit_status = " << exit_status;
  LOG_TEST << "ProcessTerminatedCallback sleeping 2 sec";
  ::sleep(2);
}

int main(int argc, char** argv) {
  common::Init(argc, argv);

  const string cmd("/bin/sleep 3");

  scoped_ptr<process::System> process(new process::System(cmd));
  process->SetExitCallback(NewCallback(&ProcessTerminatedCallback));
  bool success = process->Start();
  if ( !success ) {
    LOG_ERROR << "failed to start process: " << GetLastSystemErrorDescription();
    common::Exit(-1);
  }

  LOG_TEST << "You should see child process \""
           << cmd << "\" running. Kill it.";
  LOG_TEST << "Waiting for child process to terminate.";

  int exit_status;
  while ( true ) {
    bool success = process->Wait(5000, &exit_status);
    if ( success ) {
      break;
    }
    LOG_TEST << "Timeout... please close the child.";
  }
  LOG_TEST << "Child process terminated. exit_status = " << exit_status;

  LOG_TEST << "Pass";
  common::Exit(0);
}
