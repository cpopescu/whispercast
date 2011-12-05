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

#ifndef __MEDIA_STATS2_STATS_COLLECTOR_H__
#define __MEDIA_STATS2_STATS_COLLECTOR_H__

#include <list>
#include <set>

#include <whisperlib/common/base/types.h>
#include WHISPER_HASH_MAP_HEADER

#include <whisperlib/common/sync/thread.h>
#include <whisperlib/common/sync/mutex.h>

#include <whisperlib/common/sync/producer_consumer_queue.h>
#include <whisperlib/net/base/selector.h>
#include <whisperstreamlib/stats2/stats_saver.h>
#include <whisperstreamlib/stats2/auto/media_stats_invokers.h>

namespace streaming {

class StatsCollector : public ServiceInvokerMediaStats {
 public:
  StatsCollector(net::Selector* selector,
                 const string& server_id,
                 int64 server_instance,
                 vector<StatsSaver*>* savers);
  virtual ~StatsCollector();

  virtual void GetStreamsStats(rpc::CallContext<MediaStreamsStats>* call,
                               const vector<string>& stream_ids);
  virtual void GetAllStreamIds(
      rpc::CallContext< map<string, int32> >* call);
  virtual void GetDetailedMediaStats(
      rpc::CallContext< map<string, MediaBeginEnd> >* call,
      int32 start, int32 limit);

 private:
  // StatsCollector own thread. This thread invokes the savers to actually
  // save the statistics. The savers can take their time processing the stats.
  void ThreadProc();

 public:
  // save_interval: miliseconds between consecutive saves/updates.
  bool Start();
  void Stop();
  bool IsRunning() const;

 public:
  void RegisterStatsSaver(StatsSaver* saver);
  void UnregisterStatsSaver(StatsSaver* saver);

  // Utilities for creating a MediaStatEvent.
  // Returns a newly allocated MediaStatEvent! You should delete it!
  MediaStatEvent* MakeMediaStatEvent() const;
  MediaStatEvent* MakeMediaStatEvent(const StreamBegin* stat) const;
  MediaStatEvent* MakeMediaStatEvent(const ConnectionBegin* stat) const;
  MediaStatEvent* MakeMediaStatEvent(const MediaBegin* stat) const;
  MediaStatEvent* MakeMediaStatEvent(const StreamEnd* stat) const;
  MediaStatEvent* MakeMediaStatEvent(const ConnectionEnd* stat) const;
  MediaStatEvent* MakeMediaStatEvent(const MediaEnd* stat) const;

  // Stats are dynamically allocated by the application on specific
  // events(new connection, new stream, new media) and continuously kept up
  // to date until finally the "stats" are set inactive.
  // We (StatsCollector) save the stats from time to time while they are active,
  // and delete the inactive stats.
  // [These functions are always called made from selector thread context.]
  void StartStats(const StreamBegin* begin, const StreamEnd* end);
  void StartStats(const ConnectionBegin* begin, const ConnectionEnd* end);
  void StartStats(const MediaBegin* begin, const MediaEnd* end);
  void EndStats(const StreamEnd* end);
  void EndStats(const ConnectionEnd* end);
  void EndStats(const MediaEnd* end);

 private:
  net::Selector* const selector_;
  const string server_id_;
  const int64 server_instance_;
  // inner statistics saver thread
  thread::Thread* thread_;

  // we send commands to the collecting therad_ through this.
  synch::ProducerConsumerQueue<MediaStatEvent*>* queue_;

  // statistics savers (file savers, db savers, rpc savers, ..)
  vector<StatsSaver*> savers_;

  /////////////////////////////////////////////////////////////////////////
  // The application statistics - Start/EndStats are inserting/erasing on these.
  // Used by GetStreamsStats to deliver live stats.
  // NOTE:
  //  The ConnectionBegin, StreamBegin, MediaBegin are constants.
  //  The ConnectionEnd, StreamEnd, MediaEnd are continuously updating.
  //  #Synchronization: you can safely read them from selector thread context.
  //  #Lifetime: they are kept alive until EndStats is called.

  template <typename BeginStats, typename EndStats>
  struct GenericStats {
    const BeginStats* begin_;
    const EndStats* end_;
    GenericStats(const BeginStats* begin, const EndStats* end)
      : begin_(begin), end_(end) {
    }
    GenericStats(const GenericStats<BeginStats, EndStats>& copy)
      : begin_(copy.begin_), end_(copy.end_) {
    }
  };
  typedef GenericStats<ConnectionBegin, ConnectionEnd> ConnectionStats;
  typedef GenericStats<StreamBegin, StreamEnd> StreamStats;
  typedef GenericStats<MediaBegin, MediaEnd> MediaStats;

  typedef hash_map<string, ConnectionStats > ConnectionStatsMap;
  typedef hash_map<string, StreamStats > StreamStatsMap;
  typedef hash_map<string, MediaStats > MediaStatsMap;

  // map connection_id -> ConnectionBegin, ConnectionEnd
  ConnectionStatsMap connection_stats_;
  // map stream_id -> StreamBegin, StreamEnd
  StreamStatsMap stream_stats_;
  // map media_id -> MediaBegin, MediaEnd
  MediaStatsMap media_stats_;

  // Because sometimes StartStats()/EndStats() are called on secondary
  // network thread.
  // e.g. rtmp_connection.cc -> StartStats(ConnectionBegin..)
  //      rtmp_connection.cc -> EndStats(ConnectionEnd..)
  synch::Mutex sync_connection_stats_;

  DISALLOW_EVIL_CONSTRUCTORS(StatsCollector);
};
}

#endif  //  __MEDIA_STATS2_STATS_COLLECTOR_H__
