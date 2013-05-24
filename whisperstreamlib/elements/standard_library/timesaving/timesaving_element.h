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

#ifndef __MEDIA_BASE_MEDIA_TIMESAVING_SOURCE_H__
#define __MEDIA_BASE_MEDIA_TIMESAVING_SOURCE_H__

#include <whisperlib/common/base/alarm.h>
#include <whisperlib/common/io/checkpoint/state_keeper.h>
#include <whisperstreamlib/base/element.h>
#include <whisperstreamlib/base/filtering_element.h>

//////////////////////////////////////////////////////////////////////
//
//  Media time saving element - this is an element that "remembers" which playin
//  point we reached underneath us, and, if the underneath element allows,
//  restores the play for "lost" connections from the same point it
//  was left last time it played. This is good to protect us on restarts
//  from replaying materials from the beginning, instead to go to a point
//  close to where we were left before the crash.
namespace streaming {

class TimeSavingElement : public FilteringElement {
 public:
  static const char kElementClassName[];

  TimeSavingElement(const string& name,
                    ElementMapper* mapper,
                    net::Selector* selector,
                    io::StateKeepUser* state_keeper);
  virtual ~TimeSavingElement();

 protected:
  //////////////////////////////////////////////////////////////////////
  // FilteringElement interface
  virtual bool Initialize();
  virtual FilteringCallbackData* CreateCallbackData(const string& media_name,
                                                    Request* req);
 private:
  // From time to time we go through all our keys and delete the expired ones.
  void PurgeKeys();

 private:
  static const int32 kPurgeAlarmPeriodMs = 180000;  //  3 minutes
  static const int64 kPurgeTimeMs = 600000;  //  10 minutes

  io::StateKeepUser* const state_keeper_;
  util::Alarm purge_keys_alarm_;
  DISALLOW_EVIL_CONSTRUCTORS(TimeSavingElement);
};
}

#endif  // __MEDIA_BASE_MEDIA_TIMESAVING_SOURCE_H__
