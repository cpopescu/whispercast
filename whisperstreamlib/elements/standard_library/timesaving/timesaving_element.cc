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

#include <whisperlib/common/base/date.h>
#include <whisperlib/common/base/gflags.h>
#include "elements/standard_library/timesaving/timesaving_element.h"


DEFINE_int32(media_timed_state_save_interval_ms, 15000,
             "We save our time media state every so often (in miliseconds) for "
             "each client.");

namespace {

struct TimedMediaState {
  int64 media_ms_;
  int64 utc_ms_;
  TimedMediaState()
    : media_ms_(0),
      utc_ms_(0) {
  }
  string StrEncode() const {
    return strutil::StringPrintf(
        "media_ms_: %"PRId64", "
        "utc_ms_: %"PRId64"",
        media_ms_,
        utc_ms_);
  }
  bool StrDecode(const string& str) {
    vector< pair<string,string> > v;
    strutil::SplitPairs(str, ",", ":", &v);
    if ( v.size() != 2 ) {
      LOG_ERROR << "Invalid string: [" << str << "]";
      return false;
    }
    media_ms_ = ::strtoll(v[0].second.c_str(), NULL, 10);
    utc_ms_ = ::strtoll(v[1].second.c_str(), NULL, 10);
    return true;
  }
  string ToString() const {
    return strutil::StringPrintf("TimedMediaState: {"
        "media_ms_: %"PRId64", utc_ms_: %"PRId64"(%s)}",
        media_ms_, utc_ms_, timer::Date(utc_ms_).ToString().c_str());
  }
};

class TimedMediaData : public streaming::FilteringCallbackData {
 public:
  TimedMediaData(const char* media_name,
                 net::Selector* selector,
                 streaming::Request* req,
                 io::StateKeepUser* state_keeper)
    : streaming::FilteringCallbackData(),
      selector_(selector),
      state_key_(req->info().GetId() + "/" + media_name),
      state_keeper_(state_keeper) {
    string value;
    if ( state_keeper_ == NULL ||
         !state_keeper_->GetValue(state_key_, &value) ||
         !state_.StrDecode(value) ) {
      DLOG_DEBUG << state_key_ << ": Cannot load state, assuming clean start";
      return;
    }

    LOG_INFO << state_key_ << ": Loaded state: " << state_.ToString();
    req->mutable_info()->seek_pos_ms_ = state_.media_ms_;
  }

  virtual ~TimedMediaData() {
    if ( state_keeper_ != NULL ) {
      state_keeper_->DeleteValue(state_key_);
    }
  }

  virtual void FilterTag(const streaming::Tag* tag,
                         int64 timestamp_ms,
                         TagList* out) {
    // we update our internal state, and always FORWARD the tag
    out->push_back(FilteredTag(tag, timestamp_ms));

    if ( state_keeper_ == NULL ) {
      return;
    }

    stream_time_calculator_.ProcessTag(tag, timestamp_ms);

    int64 now = timer::Date::Now();

    // Save the state from time to time
    if ( now - state_.utc_ms_ > FLAGS_media_timed_state_save_interval_ms ) {
      state_.media_ms_ = stream_time_calculator_.media_time_ms();
      state_.utc_ms_ = now;
      state_keeper_->SetValue(state_key_, state_.StrEncode());
      DLOG_DEBUG << state_key_ << ": Save state: " << state_.ToString();
    }
  }


  const string& state_key() const { return state_key_; }

 private:
  net::Selector* const selector_;
  const string state_key_;
  io::StateKeepUser* const state_keeper_;
  TimedMediaState state_;

  streaming::StreamTimeCalculator stream_time_calculator_;

  DISALLOW_EVIL_CONSTRUCTORS(TimedMediaData);
};

}

namespace streaming {

const char TimeSavingElement::kElementClassName[] = "timesaving";

TimeSavingElement::TimeSavingElement(const char* name,
                                     const char* id,
                                     ElementMapper* mapper,
                                     net::Selector* selector,
                                     io::StateKeepUser* state_keeper)
    : streaming::FilteringElement(kElementClassName, name, id,
                                  mapper, selector),
      state_keeper_(state_keeper),
      purge_keys_alarm_(*selector) {
}
TimeSavingElement::~TimeSavingElement() {
}

bool TimeSavingElement::Initialize() {
  purge_keys_alarm_.Set(NewPermanentCallback(this,
      &TimeSavingElement::PurgeKeys), true, kPurgeAlarmPeriodMs, true, true);
  return true;
}

FilteringCallbackData* TimeSavingElement::CreateCallbackData(
    const char* media_name, Request* req) {
  return new TimedMediaData(media_name, selector_, req, state_keeper_);
}

void TimeSavingElement::PurgeKeys() {
  DLOG_INFO << name() << " purging keys.. ";
  CHECK(state_keeper_ != NULL);

  map<string, string>::const_iterator begin, end;
  state_keeper_->GetBounds("", &begin, &end);

  const int64 now = timer::Date::Now();
  uint32 count = 0;
  for ( map<string, string>::const_iterator it = begin; it != end; ++it ) {
    TimedMediaState state;
    if ( !state.StrDecode(it->second) ) {
      LOG_ERROR << "Failed to decode TimedMediaState from: ["
                << it->second << "]";
      state_keeper_->DeleteValue(it->first);
      count++;
      continue;
    }
    if ( now - state.utc_ms_ > kPurgeTimeMs ) {
      state_keeper_->DeleteValue(it->first);
      count++;
    }
  }
  DLOG_INFO << name() << " deleting: #" << count << " entries from the state";
}
}
