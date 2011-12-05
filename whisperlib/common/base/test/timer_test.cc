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

#include <stdlib.h>

#include "common/base/types.h"
#include "common/base/log.h"
#include "common/base/timer.h"
#include "common/base/system.h"

int main(int argc, char* argv[]) {
  common::Init(argc, argv);

  struct timespec tc = timer::TimespecFromMsec(0LL);
  CHECK_EQ(tc.tv_sec, 0);
  CHECK_EQ(tc.tv_nsec, 0);

  struct timespec ts_i1 = timer::TimespecAbsoluteMsec(0LL);
  struct timespec ts_i2 = timer::TimespecAbsoluteMsec(100LL);
  CHECK_EQ(timer::TimespecToMsec(ts_i1) + 100, timer::TimespecToMsec(ts_i2));


  const int64 ms = timer::TicksMsec();
  const int64 us = timer::TicksUsec();
  const int64 ns = timer::TicksNsec();

  CHECK_LE(ms * 1000, us);
  CHECK_LE(us * 1000, ns);
  CHECK_GE((ms + 1) * 1000, us);
  CHECK_GE((us + 1000) * 1000, ns);

  timer::SleepMsec(100);
  const int64 ms2 = timer::TicksMsec();
  CHECK_GE(ms2, ms + 100);
  CHECK_LT(ms2, ms + 120);

  timer::SleepMsec(1100);
  const int64 ms3 = timer::TicksMsec();
  CHECK_GE(ms3, ms + 1200);
  CHECK_LT(ms3, ms + 1220);

  struct timespec ts1 = timer::TimespecFromMsec(ms);
  struct timespec ts2 = timer::TimespecFromMsec(ms2);
  struct timespec ts3 = timer::TimespecFromMsec(ms3);
  CHECK_EQ(ms, timer::TimespecToMsec(ts1));
  CHECK_EQ(ms2, timer::TimespecToMsec(ts2));
  CHECK_EQ(ms3, timer::TimespecToMsec(ts3));

  LOG_INFO << "PASS";
  common::Exit(0);
}
