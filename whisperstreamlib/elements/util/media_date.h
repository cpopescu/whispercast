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

#ifndef __MEDIA_BASE_MEDIA_DATE_H__
#define __MEDIA_BASE_MEDIA_DATE_H__

#include <whisperlib/common/base/date.h>
#include <whisperstreamlib/elements/auto/factory_types.h>

namespace streaming {
// Given a time specification (see factory.rpc) and a date (may be now)
// it returns when is the next happening of a date in "date" (in ms)
// (from 'now') - it is a delta of time !
// This version reports events in progress! by returning a negative value.
//
// We compute the happening with the possible given error
int64 NextHappening(const TimeSpec& date, const timer::Date& now,
                    int64 error_delta_ms = 10000);  // 10 sec.

// Similar to the above version. The difference is:
// - this version doesn't report events in the past or event in progress;
//   it always returns strictly positive delta, non-zero.
int64 NextFutureHappening(const TimeSpec& date, const timer::Date& now);

// Given a time specification (see factory.rpc) and a date (may be now)
// we compute:
//  d : the soonest previous happening of 'date'. d may be exactly 'now'.
// we return:
//  'now' - 'd' in milliseconds. Positive value, can be zero.
//  Or kMaxInt64 on error.
int64 PrevHappening(const TimeSpec& date, const timer::Date& now);
// Returns milliseconds to GO for given time specification.
// Or -1 if 'date' is not happening 'now'.
int64 CurrentHappening(const TimeSpec& date, const timer::Date& now);
// Helper to obtain an unitialized (but still rpc-valid) TimeSpec
inline TimeSpec EmptyTimeSpec() {
  TimeSpec empty;
  empty.start_minute_ = 0;
  empty.start_hour_ = 0;
  empty.duration_in_seconds_ = -1;
  return empty;
}
}

#endif   // __MEDIA_BASE_MEDIA_DATE_H__
