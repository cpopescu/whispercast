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


#ifndef __MEDIA_STATS2_STATS_KEEPER_H__
#define __MEDIA_STATS2_STATS_KEEPER_H__

#include <map>

#include <whisperstreamlib/stats2/stats_collector.h>

namespace streaming {

class StatsKeeper {
 public:
  StatsKeeper(streaming::StatsCollector* stats_collector)
      : stats_collector_(stats_collector),
        media_stats_() {
  }
  ~StatsKeeper() {
  }
  // e.g. Open "concert"
  //      Open "part1"
  //      Open "song1"
  void OpenMediaStats(const StreamBegin& stream_begin_stats,
                      const string& media_id,
                      const string& content_id,
                      int64 media_time_ms,
                      int64 stream_time_ms);
  void CloseMediaStats(const string& content_id,
                       const string& result,
                       int64 duration_ms);

  // Each of these updates all media_stats_
  void bytes_up_add(int amount);
  void bytes_down_add(int amount);
  void audio_frames_add(int amount);
  void video_frames_add(int amount);
  void audio_frames_dropped_add(int amount);
  void video_frames_dropped_add(int amount);

  // map: content id -> stats
  // This streams are like the chapters / paragraphs of a book.
  //  e.g. start Show
  //         start Part 1
  //           start Clip 1
  //           ...            <--- (example time)
  //           end Clip 1
  //         ...
  //
  //       At (example time) the map contains:
  //         "Show" -> stats for show
  //         "Part 1" -> stats for part 1
  //         "Clip 1" -> stats for clip 1
  typedef map< string, pair<MediaBegin*, MediaEnd*> > MediaStatsMap;

 private:
  void CloseMediaStats(MediaStatsMap::iterator it,
                       const string& result,
                       int64 duration_ms);

  StatsCollector* stats_collector_;
  MediaStatsMap media_stats_;
  // Synchronize access to media_stats_ map.
  // Usually OpenMediaStats()/CloseMediaStats() executes in media thread,
  // while bytes_up_add() executes in network thread.
  synch::Mutex mutex_;

  DISALLOW_EVIL_CONSTRUCTORS(StatsKeeper);
};

}

#endif
