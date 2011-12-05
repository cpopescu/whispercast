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
//

#include "stats_keeper.h"

namespace streaming {

void StatsKeeper::OpenMediaStats(const StreamBegin& stream_begin_stats,
                                 const string& media_id,
                                 const string& content_id,
                                 int64 media_time_ms,
                                 int64 stream_time_ms) {
  synch::MutexLocker lock(&mutex_);
  if ( media_stats_.find(content_id) != media_stats_.end() ) {
    return;
  }
  MediaBegin* media_begin_stats = new MediaBegin(media_id, timer::Date::Now(),
      stream_begin_stats, content_id, media_time_ms, stream_time_ms);
  MediaEnd* media_end_stats = new MediaEnd(media_id, timer::Date::Now(),
      0, 0, 0, 0, 0, 0, 0, MediaResult("RUNNING"));

  DLOG_DEBUG << "Sending media stats: " << *media_begin_stats;
  media_stats_.insert(make_pair(content_id,
                                make_pair(media_begin_stats,
                                          media_end_stats)));
  stats_collector_->StartStats(media_begin_stats, media_end_stats);
}

void StatsKeeper::CloseMediaStats(const string& content_id,
                                  const string& result,
                                  int64 duration_ms) {
  synch::MutexLocker lock(&mutex_);
  MediaStatsMap::iterator it = media_stats_.find(content_id);
  if ( it == media_stats_.end() ) {
    return;
  }
  CloseMediaStats(it, result, duration_ms);
}

#define UPDATE_ALL_MEDIA_STATS(field, amount) {\
  synch::MutexLocker lock(&mutex_);\
  for ( MediaStatsMap::iterator it = media_stats_.begin();\
        it != media_stats_.end(); ++it ) {\
    MediaEnd* media_end = it->second.second;\
    media_end->field = media_end->field.get() + amount;      \
  }\
}

void StatsKeeper::bytes_up_add(int amount) {
  UPDATE_ALL_MEDIA_STATS(bytes_up_, amount);
}
void StatsKeeper::bytes_down_add(int amount) {
  UPDATE_ALL_MEDIA_STATS(bytes_down_, amount);
}
void StatsKeeper::audio_frames_add(int amount) {
  UPDATE_ALL_MEDIA_STATS(audio_frames_, amount);
}
void StatsKeeper::video_frames_add(int amount) {
  UPDATE_ALL_MEDIA_STATS(video_frames_, amount);
}
void StatsKeeper::audio_frames_dropped_add(int amount) {
  UPDATE_ALL_MEDIA_STATS(audio_frames_dropped_, amount);
}
void StatsKeeper::video_frames_dropped_add(int amount) {
  UPDATE_ALL_MEDIA_STATS(video_frames_dropped_, amount);
}

void StatsKeeper::CloseMediaStats(MediaStatsMap::iterator it,
                                  const string& result,
                                  int64 duration_ms) {
  // mutex_ should be already locked.
  it->second.second->duration_in_ms_ = duration_ms;
  it->second.second->timestamp_utc_ms_ = timer::Date::Now();
  it->second.second->result_ = MediaResult(result);

  DLOG_DEBUG << " Sending media stats: " << *it->second.second;
  stats_collector_->EndStats(it->second.second);
  delete it->second.second;
  delete it->second.first;
  media_stats_.erase(it);
}

}
