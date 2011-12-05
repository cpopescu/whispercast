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

#include <list>
#include <string>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include "common/base/types.h"
#include "common/base/errno.h"
#include "common/base/log.h"
#include "common/sync/thread.h"
#include "common/base/system.h"
#include "common/base/gflags.h"
#include "net/base/address.h"

DEFINE_int32(nThreads, 1,
             "The number of threads to use in the test.");
DEFINE_int32(nSeconds, 5,
             "The number of seconds to run the test.");


namespace test_http_sync {
void run(unsigned nThreads, unsigned nSeconds,
         const net::HostPort& serverAddress);
};
namespace test_http_async {
void run(unsigned nThreads, unsigned nSeconds,
         const net::HostPort& serverAddress);
};

int main(int argc, char ** argv) {
  common::Init(argc, argv);

  unsigned nThreads = (unsigned)FLAGS_nThreads;
  unsigned nSeconds = (unsigned)FLAGS_nSeconds;
  if (nThreads == 0 || nSeconds == 0) {
    LOG_ERROR << "Bad arguments." << std::endl;
    return -1;
  }

  net::HostPort serverAddress("127.0.0.1:8123");

  test_http_sync::run(nThreads, nSeconds, serverAddress);
  test_http_async::run(nThreads, nSeconds, serverAddress);

  common::Exit(0);
  return 0;
}
