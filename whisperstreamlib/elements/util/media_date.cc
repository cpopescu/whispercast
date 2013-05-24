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

#include <set>
#include <bitset>
#include <whisperlib/common/base/types.h>
#include "elements/util/media_date.h"

namespace streaming {

int64 NextHappening(const TimeSpec& date, const timer::Date& now,
                    int64 error_delta_ms) {
  int64 min_dif = 366LL*12*31*24*60*60*1000; // 1 YEAR in milliseconds
  if ( date.weekdays_.is_set() ) {
    set<int> weekdays;
    for ( int i = 0; i < date.weekdays_.size(); i++ ) {
      int weekday = date.weekdays_[i];
      if ( weekday == 7 ) weekday = 0;
      if ( weekday < 0 || weekday > 6 ) continue;
      weekdays.insert(weekday);
    }
    if ( weekdays.empty() ) return 0;
    timer::Date to_check(now.GetYear(), now.GetMonth(), now.GetDay(),
                         date.start_hour_,
                         date.start_minute_,
                         (date.start_second_.is_set() &&
                          date.start_second_ > 0
                          ? date.start_second_
                          : 0),
                         0, now.IsUTC());
    // Start from before - catch also ongoing stuff..
    to_check = timer::Date::ShiftDay(to_check.GetTime(), -1, now.IsUTC());
    //to_check.SetTime(to_check.GetTime() -
    //                 1000 * 3600 * 24);  // one day in ms :) - in advance
    for ( int i = 0; i < 9; i++ ) {
      if ( to_check.HasErrors() ) {
        LOG_ERROR << " Error in time specification: " << date.ToString();
        continue;
      }
      if ( weekdays.find(to_check.GetDayOfTheWeek()) != weekdays.end() ) {
        const int64 crt_time = to_check.GetTime();
        const int64 crt_time_end =
          crt_time + date.duration_in_seconds_ * 1000LL;
        const int64 crt_dif = crt_time - now.GetTime();
        if ( crt_dif > 0 && crt_dif < min_dif ) {
          min_dif = crt_dif;
        } else if ( crt_time_end > now.GetTime() + error_delta_ms &&
                    crt_time - error_delta_ms < now.GetTime() ) {
          min_dif = crt_time - now.GetTime();
        }
      }
      to_check.SetTime(to_check.GetTime() +
                       1000 * 3600 * 24);  // one day in ms :)
    }
  }
  if ( date.dates_.is_set() ) {
    for ( int i = 0; i < date.dates_.size(); i++ ) {
      timer::Date to_check(date.dates_[i].year_,
                           date.dates_[i].month_ - 1,
                           date.dates_[i].day_,
                           date.start_hour_,
                           date.start_minute_,
                           (date.start_second_.is_set() &&
                            date.start_second_ > 0
                            ? date.start_second_
                            : 0),
                           0, now.IsUTC());
      if ( to_check.HasErrors() ) {
        LOG_ERROR << " Error in time specification: " << date.ToString();
        continue;
      }
      const int64 crt_time = to_check.GetTime();
      const int64 crt_time_end =
        crt_time + date.duration_in_seconds_ * 1000LL;
      const int64 crt_dif = crt_time - now.GetTime();
      if ( crt_dif > 0 && crt_dif < min_dif ) {
        min_dif = crt_dif;
      } else if ( crt_time_end > now.GetTime() + error_delta_ms &&
                  crt_time - error_delta_ms < now.GetTime() ) {  // 10 sec delay
        min_dif = crt_time - now.GetTime();
      }
    }
  }
  return min_dif;
}
int64 NextFutureHappening(const TimeSpec& date, const timer::Date& now) {
  int64 delay = 366LL*12*31*24*60*60*1000; // 1 YEAR in milliseconds
  const int64 now_ts = now.GetTime();

  if ( date.weekdays_.is_set() ) {
    bitset<7> weekdays;
    for ( int i = 0; i < date.weekdays_.size(); i++ ) {
      int weekday = date.weekdays_[i];
      if ( weekday == 7 ) weekday = 0;
      if ( weekday < 0 || weekday > 6 ) {
        LOG_ERROR << "Illegal weekday: " << weekday;
        continue;
      }
      weekdays[weekday] = true;
    }
    timer::Date start(now.GetYear(), now.GetMonth(), now.GetDay(),
                      date.start_hour_,
                      date.start_minute_,
                      (date.start_second_.is_set() &&
                       date.start_second_ > 0
                       ? date.start_second_
                       : 0),
                      0, now.IsUTC());
    if ( start.HasErrors() ) {
      LOG_ERROR << " Error in time specification: " << date.ToString();
      return delay;
    }
    int64 start_ts = start.GetTime();

    for ( int i = 0; i < 8; i++ ) {
      int64 to_check_ts = timer::Date::ShiftDay(start_ts, i, now.IsUTC());
      if ( to_check_ts <= now_ts ) {
        //timer::Date to_check(to_check_ts, now.IsUTC());
        //LOG_WARNING << "In the past: " << to_check
        //            << " - " << to_check.GetDayOfTheWeekName();
        continue;
      }
      timer::Date to_check(to_check_ts, now.IsUTC());
      if ( !weekdays[to_check.GetDayOfTheWeek()] ) {
        //LOG_WARNING << "Not a valid weekday: " << to_check
        //            << " - " << to_check.GetDayOfTheWeekName();
        continue;
      }
      delay = to_check_ts - now_ts;
      //LOG_WARNING << "Good: " << to_check
      //            << " - " << to_check.GetDayOfTheWeekName();
      break;
    }
  }
  if ( date.dates_.is_set() ) {
    for ( int i = 0; i < date.dates_.size(); i++ ) {
      timer::Date to_check(date.dates_[i].year_,
                           date.dates_[i].month_ - 1,
                           date.dates_[i].day_,
                           date.start_hour_,
                           date.start_minute_,
                           (date.start_second_.is_set() &&
                            date.start_second_ > 0
                            ? date.start_second_
                            : 0),
                           0, now.IsUTC());
      if ( to_check.HasErrors() ) {
        LOG_ERROR << "Error in time specification: " << date.ToString();
        continue;
      }
      const int64 to_check_ts = to_check.GetTime();
      if ( to_check_ts <= now_ts ) {
        //LOG_WARNING << "In the past: " << to_check;
        continue;
      }
      int64 to_check_delay = to_check_ts - now_ts;
      if ( to_check_delay > delay ) {
        //LOG_WARNING << "Too far: " << to_check;
        continue;
      }
      delay = to_check_delay;
      //LOG_WARNING << "Good: " << to_check << " delay=" << delay;
    }
  }
  return delay;
}
int64 PrevHappening(const TimeSpec& date, const timer::Date& now) {
  int64 prev_delay =           // positive: milliseconds in the past
    366LL*12*31*24*60*60*1000; // 1 YEAR in milliseconds
  const int64 now_ts = now.GetTime();
  if ( date.weekdays_.is_set() ) {
    bitset<7> weekdays(0);
    for ( int i = 0; i < date.weekdays_.size(); i++ ) {
      int weekday = date.weekdays_[i];
      if ( weekday == 7 ) weekday = 0;
      if ( weekday < 0 || weekday > 6 ) {
        LOG_ERROR << "Illegal weekday: " << weekday;
        continue;
      }
      //LOG_WARNING << "Found weekday: " << weekday
      //            << " - " << timer::Date::DayOfTheWeekName(weekday);
      weekdays[weekday] = true;
    }
    timer::Date start(now.GetYear(), now.GetMonth(), now.GetDay(),
                      date.start_hour_,
                      date.start_minute_,
                      (date.start_second_.is_set() &&
                       date.start_second_ > 0
                       ? date.start_second_
                       : 0),
                      0, now.IsUTC());
    if ( start.HasErrors() ) {
      LOG_ERROR << "Error in time specification: " << start;
      return kMaxInt64;
    }

    int64 start_ts = start.GetTime();
    for ( int i = 0; i < 8; i++ ) { // check until 8 days before
      int64 to_check_ts = timer::Date::ShiftDay(start_ts, -i, now.IsUTC());
      if ( to_check_ts > now_ts ) {
        //timer::Date to_check(to_check_ts, now.IsUTC());
        //LOG_WARNING << "In the future: " << to_check
        //            << " - " << to_check.GetDayOfTheWeekName();
        continue;
      }
      timer::Date to_check(to_check_ts, now.IsUTC());
      if ( !weekdays[to_check.GetDayOfTheWeek()] ) {
        //LOG_WARNING << "Not a valid weekday: " << to_check
        //            << " - " << to_check.GetDayOfTheWeekName();
        continue;
      }
      prev_delay = now_ts - to_check_ts;
      //LOG_WARNING << "Good: " << to_check
      //            << " - " << to_check.GetDayOfTheWeekName();
      break;
    }
  }
  if ( date.dates_.is_set() ) {
    for ( int i = 0; i < date.dates_.size(); i++ ) {
      timer::Date to_check(date.dates_[i].year_,
                           date.dates_[i].month_ - 1,
                           date.dates_[i].day_,
                           date.start_hour_,
                           date.start_minute_,
                           (date.start_second_.is_set() &&
                            date.start_second_ > 0
                            ? date.start_second_
                            : 0),
                           0, now.IsUTC());
      if ( to_check.HasErrors() ) {
        //LOG_ERROR << "Error in time specification: " << date.ToString();
        continue;
      }
      const int64 to_check_ts = to_check.GetTime();
      if ( to_check_ts > now_ts ) {
        //LOG_WARNING << "In the future: " << to_check;
        continue;
      }
      int64 to_check_delay = now_ts - to_check_ts;
      if ( to_check_delay > prev_delay ) {
        //LOG_WARNING << "Too far: " << to_check;
        continue;
      }
      prev_delay = to_check_delay;
      //LOG_WARNING << "Good: " << to_check << " prev_delay=" << prev_delay;
    }
  }
  return prev_delay;
}
int64 CurrentHappening(const TimeSpec& date, const timer::Date& now) {
  if ( date.duration_in_seconds_ == -1 ) {
    return -1;
  }
  timer::Date oldest_possible_start(
      now.GetTime() - date.duration_in_seconds_ * 1000, now.IsUTC());
  int64 delay = NextHappening(date, oldest_possible_start);
  if ( delay > date.duration_in_seconds_ * 1000 ) {
    return -1;
  }
  return date.duration_in_seconds_ * 1000 - delay;
}

}
