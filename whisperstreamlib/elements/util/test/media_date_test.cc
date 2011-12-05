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

#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/strutil.h>
#include <whisperlib/common/base/system.h>
#include "elements/util/media_date.h"

int main(int argc, char* argv[]) {
  common::Init(argc, argv);

  {
    TimeSpec ts;
    ts.start_hour_.ref() = 23;
    ts.start_minute_.ref() = 52;
    ts.duration_in_seconds_ = 75 * 60;

    ts.dates_.ref().push_back(DaySpec(2008, 9, 29));
    ts.dates_.ref().push_back(DaySpec(2008, 10, 1));

    CHECK_EQ(streaming::NextHappening(ts,
                                      timer::Date(2008, 8, 29, 23, 50, 0, 0)),
             120 * 1000LL);
    timer::Date d1(2008, 8, 30, 0, 30, 0, 0);
    CHECK_EQ(streaming::NextHappening(ts, d1),
             -30 * 60 * 1000 - 8 * 60 * 1000);
    CHECK_EQ(streaming::NextHappening(
                 ts, timer::Date(2008, 8, 30, 2, 30, 0, 0)),
             23 * 3600000LL + 52 * 60000LL + 21 * 3600000LL + 30 * 60000LL);
    //////////////////////////////////////////////////////////////////////
    int64 ph = streaming::PrevHappening(ts, timer::Date(2008, 8, 30, 0, 0, 0, 0));
    CHECK_EQ(ph, 8LL * 60000);

    ph = streaming::PrevHappening(ts, timer::Date(2008, 8, 29, 23, 52, 0, 0));
    CHECK_EQ(ph, 0LL);

    ph = streaming::PrevHappening(ts, timer::Date(2008, 8, 30, 11, 12, 7, 0));
    CHECK_EQ(ph, 8LL * 60000 + 11LL * 3600000 + 12LL * 60000 + 7LL * 1000);

    ph = streaming::PrevHappening(ts, timer::Date(2008, 9, 1, 0, 0, 0, 0));
    CHECK_EQ(ph, 8LL * 60000 + 24LL * 3600000);

    ph = streaming::PrevHappening(ts, timer::Date(2008, 9, 1, 23, 51, 13, 0));
    CHECK_EQ(ph, 8LL * 60000 + 24LL * 3600000
                 + 23LL * 3600000 + 51LL * 60000 + 13LL * 1000);

    ph = streaming::PrevHappening(ts, timer::Date(2008, 9, 1, 23, 55, 0, 0));
    CHECK_EQ(ph, 3LL * 60000);

    ph = streaming::PrevHappening(ts, timer::Date(2008, 9, 2, 0, 1, 0, 0));
    CHECK_EQ(ph, 8LL * 60000 + 1LL * 60000);

    ph = streaming::PrevHappening(ts, timer::Date(2008, 9, 1, 23, 52, 0, 0));
    CHECK_EQ(ph, 0LL);
    //////////////////////////////////////////////////////////////////////
    {
      TimeSpec ts;
      ts.start_hour_.ref() = 23;
      ts.start_minute_.ref() = 52;
      ts.duration_in_seconds_ = 75 * 60;
      ts.dates_.ref().push_back(DaySpec(2008, 9, 29));
      ts.dates_.ref().push_back(DaySpec(2008, 10, 1));

      int64 nh = 0;

      nh = streaming::NextFutureHappening(ts, timer::Date(2008, 8, 29,
                                                          23, 51, 30, 0));
      CHECK_EQ(nh, 30LL * 1000);

      nh = streaming::NextFutureHappening(ts, timer::Date(2008, 8, 29,
                                                          23, 53, 0, 0));
      CHECK_EQ(nh, 2LL * 24 * 3600000 - 1LL * 60000);
    }
  }
  {
    TimeSpec ts;
    ts.start_hour_.ref() = 23;
    ts.start_minute_.ref() = 52;
    ts.duration_in_seconds_ = 75 * 60;

    ts.weekdays_.ref().push_back(1);
    ts.weekdays_.ref().push_back(3);
    CHECK_EQ(streaming::NextHappening(
                 ts, timer::Date(2008, 8, 29, 23, 50, 0, 0)),
             120 * 1000LL);
    timer::Date d1(timer::Date(2008, 8, 30, 0, 30, 0, 0));
    CHECK_EQ(streaming::NextHappening(ts, d1),
             -30 * 60 * 1000 - 8 * 60 * 1000);
    CHECK_EQ(streaming::NextHappening(
                 ts, timer::Date(2008, 8, 30, 2, 30, 0, 0)),
             23 * 3600000LL + 52 * 60000LL + 21 * 3600000LL + 30 * 60000LL);
    //////////////////////////////////////////////////////////////////////
    int64 ph = streaming::PrevHappening(ts, timer::Date(2009, 9, 2, 21, 10, 0, 0));
    CHECK_EQ(ph, 8LL * 60000 + 24LL * 3600000 + 21LL * 3600000 + 10LL * 60000);

    ph = streaming::PrevHappening(ts, timer::Date(2009, 8, 30, 23, 52, 0, 1));
    CHECK_EQ(ph, 1LL);

    // PrevHappening can return 0.
    ph = streaming::PrevHappening(ts, timer::Date(2009, 8, 30, 23, 52, 0, 0));
    CHECK_EQ(ph, 0LL);

    ph = streaming::PrevHappening(ts, timer::Date(2009, 8, 30, 23, 51, 7, 0));
    CHECK_EQ(ph, 8LL * 60000 + 24LL * 3600000
                 + 23LL * 3600000 + 51LL * 60000 + 7LL * 1000);

    ph = streaming::PrevHappening(ts, timer::Date(2009, 8, 28, 23, 53, 17, 0));
    CHECK_EQ(ph, 1LL * 60000 + 17LL * 1000);

    ph = streaming::PrevHappening(ts, timer::Date(2009, 8, 28, 23, 52, 0, 0));
    CHECK_EQ(ph, 0LL);
    //////////////////////////////////////////////////////////////////////
    {
      TimeSpec ts;
      ts.start_hour_.ref() = 23;
      ts.start_minute_.ref() = 52;
      ts.duration_in_seconds_ = 75 * 60;
      ts.weekdays_.ref().push_back(1);

      int64 nh = 0;

      nh = streaming::NextFutureHappening(ts, timer::Date(2008, 8, 29,
                                                          23, 51, 30, 0));
      CHECK_EQ(nh, 30LL * 1000);

      // NextFutureHappening cannot return 0. It returns strictly future events.
      nh = streaming::NextFutureHappening(ts, timer::Date(2008, 8, 29,
                                                          23, 52, 0, 0));
      CHECK_EQ(nh, 7 * 24LL * 3600000); // 7 days

      nh = streaming::NextFutureHappening(ts, timer::Date(2008, 8, 29,
                                                          23, 53, 0, 0));
      CHECK_EQ(nh, 7LL * 24 * 3600000 - 1LL * 60000);
    }
  }
  {
    TimeSpec ts;
    ts.start_minute_.ref() = 00;
    ts.start_hour_.ref() = 13;
    ts.duration_in_seconds_ = 3600;

    ts.weekdays_.ref().push_back(1);
    ts.weekdays_.ref().push_back(2);
    ts.weekdays_.ref().push_back(3);
    ts.weekdays_.ref().push_back(4);
    CHECK_EQ(streaming::NextHappening(ts,
                                  timer::Date(2009, 0, 7, 13, 02, 0, 0)),
             -2 * 60 * 1000);
  }
}
