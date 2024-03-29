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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>

#include "common/base/log.h"
#include "common/base/errno.h"
#include "common/base/strutil.h"

#include "common/base/date.h"

namespace timer {

const char* Date::MonthName(int month_number) {
  switch ( month_number ) {
  case 0:  return "January";
  case 1:  return "February";
  case 2:  return "March";
  case 3:  return "April";
  case 4:  return "May";
  case 5:  return "June";
  case 6:  return "July";
  case 7:  return "August";
  case 8:  return "September";
  case 9:  return "October";
  case 10: return "November";
  case 11: return "December";
  default:
    return NULL;
  };
}

const char* Date::DayOfTheWeekName(int day_of_the_week_number) {
  switch ( day_of_the_week_number ) {
  case 0: return "Sunday";
  case 1: return "Monday";
  case 2: return "Tuesday";
  case 3: return "Wednesday";
  case 4: return "Thursday";
  case 5: return "Friday";
  case 6: return "Saturday";
  default:
    return NULL;
  };
}

int64 Date::Now() {
  struct timeval tv;
  struct timezone tz;

  int result = ::gettimeofday(&tv, &tz);
  if ( result != 0 ) {
    LOG_ERROR << "gettimeofday failed: " << GetLastSystemErrorDescription();
    return 0;
  }

  return ((static_cast<int64>(tv.tv_sec)) * static_cast<int64>(1000) +
          tv.tv_usec / static_cast<int64>(1000));
}
//static
int64 Date::ShiftDay(int64 time, int32 days, bool is_utc) {
  int64 shift_time = time + 24LL * 3600 * 1000 * days;
  if ( is_utc ) {
    return shift_time;
  }
  Date d(time, is_utc);
  Date s(shift_time, is_utc);
  if ( s.GetHour() == d.GetHour() ) {
    // no daylight saving change in the 'days' interval
    return shift_time;
  }
  // daylight saving change, we need to restore the hour
  s.Set(s.GetYear(),
        s.GetMonth(),
        s.GetDay(),
        d.GetHour(),
        s.GetMinute(),
        s.GetSecond(),
        s.GetMilisecond());
  return s.GetTime();
}

Date::Date(bool use_utc)
  : time_(0),
    broken_down_milisecond_(0),
    is_utc_(use_utc),
    has_errors_(false) {
  memset(&broken_down_time_, 0, sizeof(broken_down_time_));
  SetNow();
}

Date::Date(int64 time, bool use_utc)
  : time_(0),
    broken_down_milisecond_(0),
    is_utc_(use_utc),
    has_errors_(false) {
  SetTime(time);
}
#if __WORDSIZE != 64
Date::Date(time_t time, bool use_utc)
  : time_(0),
    broken_down_milisecond_(0),
    is_utc_(use_utc),
    has_errors_(false) {
  SetTime(1000LL * time);
}
#endif
Date::Date(int year, int month, int day, int hour,
           int minute, int second, int milisecond, bool use_utc)
  : time_(0),
    broken_down_milisecond_(0),
    is_utc_(false),
    has_errors_(false) {
  has_errors_ = !Set(year, month, day,
                     hour, minute, second, milisecond,
                     use_utc);
}

Date::Date(const Date& date)
  : time_(date.GetTime()),
    is_utc_(date.IsUTC()),
    has_errors_(date.HasErrors()) {
  SetTime(date.GetTime());
}

///////////////////////////////////////////////////////////////////////

bool Date::Set(int year, int month, int day,
               int hour, int minute, int second, int milisecond,
               bool is_utc) {
  if (year < 1900) {
    LOG_ERROR << "invalid year " << year << ". Should be greater than 1900.";
    return false;
  }
  if ( month > 11 || month < 0 ) {
    LOG_ERROR << "invalid month " << month << ". Should be in range 0...11.";
    return false;
  }
  if ( day > 31 || day < 1 ) {
    LOG_ERROR << "invalid day " << day << ". Should be in range 1...31.";
    return false;
  }
  if ( hour > 23 ||  hour < 0 ) {
    LOG_ERROR << "invalid hour " << hour << ". Should be in range 0...23.";
    return false;
  }
  if ( minute > 59 || minute < 0 ) {
    LOG_ERROR << "invalid minute " << minute << ". Should be in range 0...59.";
    return false;
  }
  if ( second > 59 || second < 0 ) {
    LOG_ERROR << "invalid second " << second << ". Should be in range 0...60.";
    return false;
  }
  if ( milisecond > 999 || milisecond < 0 ) {
    LOG_ERROR << "invalid milisecond " << milisecond
              << ". Should be in range 0...999.";
    return false;
  }

  struct tm t = {0, };
  t.tm_year = year - 1900;
  t.tm_mon = month;
  t.tm_mday = day;
  t.tm_hour = hour;
  t.tm_min = minute;
  t.tm_sec = second;
  t.tm_isdst = -1;
  time_t tt = ::mktime(&t);  // mktime always treats t as localtime
  if ( tt == -1 ) {
    LOG_ERROR << "mktime() failed: " << GetLastSystemErrorDescription();
    return false;
  }
  // tt = seconds since Epoch until t as localtime

  // If we need seconds until t as UTC we need to shift tt by gmt offset.
  if ( is_utc ) {
    struct tm asLocalTime;
    struct tm * result = ::localtime_r(&tt, &asLocalTime);
    if ( result == NULL ) {
      LOG_ERROR << "localtime_r() failed: "
                << GetLastSystemErrorDescription();
      return false;
    }

    tt += asLocalTime.tm_gmtoff;
  }
  const int64 milis_since_epoch =
      (static_cast<int64>(tt)) * static_cast<int64>(1000) + milisecond;

  SetUTC(is_utc);
  SetTime(milis_since_epoch);

  // now check that we obtained the same year/month/day/.. values
  DCHECK_EQ(year, GetYear());
  DCHECK_EQ(month, GetMonth());
  DCHECK_EQ(day, GetDay());
  if ( hour != GetHour() ) {
    // happens when the daylight saving time changes
    // e.g.  localtime(mktime(...03:00...)) => ...4:00...
    DCHECK_EQ(::abs(hour - GetHour()), 1);
  }
  DCHECK_EQ(minute, GetMinute());
  DCHECK_EQ(second, GetSecond());
  DCHECK_EQ(milisecond, GetMilisecond());

  return true;
}

// date: the milliseconds since January 1, 1970, 00:00:00 GMT.
void Date::SetTime(int64 epoch_ms) {
  time_ = epoch_ms;
  broken_down_milisecond_ = static_cast<int>(epoch_ms % 1000);
  time_t tt = time_t(epoch_ms / 1000);
  struct tm * result = IsUTC() ? ::gmtime_r(&tt, &broken_down_time_) :
                                 ::localtime_r(&tt, &broken_down_time_);
  if ( result == NULL ) {
    has_errors_ = true;
    LOG_ERROR << (IsUTC() ? "gmtime_r()" : "localtime_r()") << " failed: "
              << GetLastSystemErrorDescription();
  } else {
    has_errors_ = false;
  }

  // check time_
  CHECK(has_errors_ || GetTime() == epoch_ms)
    << " has_errors_: " << has_errors_
    << " GetTime(): " << GetTime()
    << " epoch_ms: " << epoch_ms;
}

void Date::SetNow() {
  const int64 milis_since_epoch = Now();
  SetTime(milis_since_epoch);
}

void Date::SetUTC(bool use_utc) {
  is_utc_ = use_utc;
  SetTime(time_);  // updates broken down time
}

Date& Date::operator=(const Date& dt) {
  time_ = dt.time_;
  broken_down_time_ = dt.broken_down_time_;
  broken_down_milisecond_ = dt.broken_down_milisecond_;
  is_utc_ = dt.is_utc_;
  return *this;
}

bool Date::operator==(const Date& dt) const {
  return time_ == dt.time_;
}

bool Date::FromShortString(const string& s, bool is_utc) {
  if ( s.size() != 15 && s.size() != 19 ) {
    LOG_ERROR << "Illegal size: [" << s << "]";
    return false;
  }
  for ( uint32 i = 0; i < 8; i++ ) {
    if ( s[i] < '0' || s[i] > '9' ) {
      LOG_ERROR << "Illegal character: " << s[i] << ", expected digit"
                   ", at index: " << i << " in: [" << s << "]";
      return false;
    }
  }
  if ( s[8] != '-' ) {
    LOG_ERROR << "Illegal character: " << s[8] << ", expected '-'"
                 ", at index 8 in: [" << s << "]";
    return false;
  }
  for ( uint32 i = 9; i < 15; i++ ) {
    if ( s[i] < '0' || s[i] > '9' ) {
      LOG_ERROR << "Illegal character: " << s[i] << ", expected digit"
                   ", at index: " << i << " in: [" << s << "]";
      return false;
    }
  }
  int ms = 0;
  if ( s.size() == 19 ) {
    if ( s[15] != '-' ) {
      LOG_ERROR << "Illegal character: " << s[15] << ", expected '-'"
                   ", at index 15 in: [" << s << "]";
      return false;
    }
    for ( uint32 i = 16; i < 19; i++ ) {
      if ( s[i] < '0' || s[i] > '9' ) {
        LOG_ERROR << "Illegal character: " << s[i] << ", expected digit"
                     ", at index: " << i << " in: [" << s << "]";
        return false;
      }
    }
    ms = (s[16]-'0') * 100 + (s[17]-'0') * 10 + (s[18]-'0');
  }
  Set((s[0]-'0') * 1000 + (s[1]-'0')*100 + (s[2]-'0')*10 + (s[3]-'0'),
      (s[4]-'0') * 10 + (s[5]-'0') - 1,
      (s[6]-'0') * 10 + (s[7]-'0'),
      (s[9]-'0') * 10 + (s[10]-'0'),
      (s[11]-'0') * 10 + (s[12]-'0'),
      (s[13]-'0') * 10 + (s[14]-'0'),
      ms,
      is_utc);
  return true;
}

string Date::ToShortString() const {
  return strutil::StringPrintf("%04d%02d%02d-%02d%02d%02d-%03d",
                               static_cast<int32>(GetYear()),
                               static_cast<int32>(GetMonth() + 1),
                               static_cast<int32>(GetDay()),
                               static_cast<int32>(GetHour()),
                               static_cast<int32>(GetMinute()),
                               static_cast<int32>(GetSecond()),
                               static_cast<int32>(GetMilisecond()));
}

string Date::ToString() const {
  return strutil::StringPrintf("%s %d.%s.%d %02d:%02d:%02d.%03d %s",
                               GetDayOfTheWeekName(),
                               static_cast<int32>(GetDay()),
                               GetMonthName(),
                               static_cast<int32>(GetYear()),
                               static_cast<int32>(GetHour()),
                               static_cast<int32>(GetMinute()),
                               static_cast<int32>(GetSecond()),
                               static_cast<int32>(GetMilisecond()),
                               GetTimezoneName());
}

ostream& operator<<(ostream& os, const Date& date) {
  return os << date.ToString();
}
}
