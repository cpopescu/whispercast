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

#include <pthread.h>
#include <limits.h>

#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/base/timer.h>
#include "stats2/stats_collector.h"

namespace streaming {
StatsCollector::StatsCollector(net::Selector* selector,
                               const string& server_id,
                               int64 server_instance,
                               vector<StatsSaver*>* savers)
    : ServiceInvokerMediaStats(ServiceInvokerMediaStats::GetClassName()),
      selector_(selector),
      server_id_(server_id),
      server_instance_(server_instance),
      thread_(NULL),
      queue_(NULL),
      savers_(savers->size()){
  copy(savers->begin(), savers->end(), savers_.begin());
  CHECK_NOT_NULL(selector_);
}

StatsCollector::~StatsCollector() {
  LOG_INFO_IF(!connection_stats_.empty() ||
              !stream_stats_.empty() ||
              !media_stats_.empty() )
      << "WARINING: the application should go down first, "
      << "closing all statistics"
      << " connection_stats_.size: " << connection_stats_.size()
      << " stream_stats_.size: " << stream_stats_.size()
      << " media_stats_.size: " << media_stats_.size();
  CHECK_NULL(thread_);
  for ( int i = 0; i < savers_.size(); ++i ) {
    delete savers_[i];
  }
}

void StatsCollector::ThreadProc()  {
  LOG_INFO << "Stats collector thread running with "
           << " server_id_: [" << server_id_ << "]"
           << " server_instance_: " << server_instance_;
  while ( true ) {
    MediaStatEvent* event = queue_->Get();
    if ( event == NULL ) {
      break;
    }
    //LOG_DEBUG << "Broadcasting stat: " << *event;
    for ( vector<StatsSaver*>::iterator it = savers_.begin();
          it != savers_.end(); ++it ) {
      (*it)->Save(event);
    }
    delete event;
  }
}

// save_interval: miliseconds between consecutive saves/updates.
bool StatsCollector::Start() {
  CHECK_NULL(thread_);
  // just one command outstanding
  queue_ = new synch::ProducerConsumerQueue<MediaStatEvent*>(2000);
  thread_ = new thread::Thread(NewCallback(this, &StatsCollector::ThreadProc));
  const size_t stack_size = PTHREAD_STACK_MIN + 65536;
  CHECK(thread_->SetStackSize(stack_size));
  CHECK(thread_->SetJoinable());
  CHECK(thread_->Start());

  return true;
}

void StatsCollector::Stop() {
  CHECK_NOT_NULL(thread_);
  LOG_WARNING << "Stopping the stats collector..";

  queue_->Put(NULL);  // we command w/ no wait..
  thread_->Join();

  delete thread_;
  thread_ = NULL;

  delete queue_;
  queue_ = NULL;

  LOG_WARNING << "Stopped  the stats collector..";
}

MediaStatEvent* StatsCollector::MakeMediaStatEvent() const {
  MediaStatEvent* ev = new MediaStatEvent();
  ev->server_id_ = server_id_;
  ev->server_instance_ = server_instance_;
  return ev;
}
MediaStatEvent* StatsCollector::MakeMediaStatEvent(
    const StreamBegin* stat) const {
  MediaStatEvent* ev = MakeMediaStatEvent();
  ev->stream_begin_ = *stat;
  return ev;
}
MediaStatEvent* StatsCollector::MakeMediaStatEvent(
    const ConnectionBegin* stat) const {
  MediaStatEvent* ev = MakeMediaStatEvent();
  ev->connection_begin_ = *stat;
  return ev;
}
MediaStatEvent* StatsCollector::MakeMediaStatEvent(
    const MediaBegin* stat) const {
  MediaStatEvent* ev = MakeMediaStatEvent();
  ev->media_begin_ = *stat;
  return ev;
}
MediaStatEvent* StatsCollector::MakeMediaStatEvent(
    const StreamEnd* stat) const {
  MediaStatEvent* ev = MakeMediaStatEvent();
  ev->stream_end_ = *stat;
  return ev;
}
MediaStatEvent* StatsCollector::MakeMediaStatEvent(
    const ConnectionEnd* stat) const {
  MediaStatEvent* ev = MakeMediaStatEvent();
  ev->connection_end_ = *stat;
  return ev;
}
MediaStatEvent* StatsCollector::MakeMediaStatEvent(
    const MediaEnd* stat) const {
  MediaStatEvent* ev = MakeMediaStatEvent();
  ev->media_end_ = *stat;
  return ev;
}

void StatsCollector::StartStats(const ConnectionBegin* begin,
                                const ConnectionEnd* end) {
  //CHECK(selector_->IsInSelectThread());
  synch::MutexLocker lock(&sync_connection_stats_);
  MediaStatEvent* ev = MakeMediaStatEvent(begin);
  if ( !queue_->Put(ev, 0) ) {
    LOG_ERROR << "Stats work too slow.. cannot record ConnectionBegin";
    delete ev;
    return;
  }
  pair<ConnectionStatsMap::iterator, bool> result =
      connection_stats_.insert(make_pair(begin->connection_id_,
          ConnectionStats(begin, end)));
  if ( !result.second ) {
    LOG_ERROR << "Duplicate connection stats:" << endl
              << " old: " << *(result.first->second.begin_) << endl
              << " new: " << *begin;
  }
  LOG_DEBUG << "StartStats enqueued new stat: " << *begin;
}
void StatsCollector::StartStats(const StreamBegin* begin,
                                const StreamEnd* end) {
  CHECK(selector_->IsInSelectThread());
  MediaStatEvent* ev = MakeMediaStatEvent(begin);
  if ( !queue_->Put(ev, 0) ) {
    LOG_ERROR << "Stats work too slow.. cannot record StreamBegin";
    delete ev;
    return;
  }
  pair<StreamStatsMap::iterator, bool> result =
      stream_stats_.insert(make_pair(begin->stream_id_,
          StreamStats(begin, end)));
  if ( !result.second ) {
    LOG_ERROR << "Duplicate stream stats:" << endl
              << " old: " << *(result.first->second.begin_) << endl
              << " new: " << *begin;
  }
  LOG_DEBUG << "StartStats enqueued new stat: " << *begin;
}
void StatsCollector::StartStats(const MediaBegin* begin,
                                const MediaEnd* end) {
  CHECK(selector_->IsInSelectThread());
  MediaStatEvent* ev = MakeMediaStatEvent(begin);
  if ( !queue_->Put(ev, 0) ) {
    LOG_ERROR << "Stats work too slow.. cannot record MediaBegin";
    delete ev;
    return;
  }
  pair<MediaStatsMap::iterator, bool> result =
      media_stats_.insert(make_pair(begin->media_id_,
          MediaStats(begin, end)));
  if ( !result.second ) {
    LOG_ERROR << "Duplicate media stats:" << endl
              << " old: " << *(result.first->second.begin_) << endl
              << " new: " << *begin;
  }
  LOG_DEBUG << "StartStats enqueued new stat: " << *begin;
}

/*
void StatsCollector::StartStats(MediaStatEvent* stats) {
  stats->server_id_ = server_id_;
  stats->server_instance_ = server_instance_;
  stats->timestamp_utc_ms_ = timer::Date::Now();

  MediaStatEvent* const copy =
      reinterpret_cast<MediaStatEvent*>(stats->Clone());
  if ( !queue_->Put(copy, 0) ) {
    LOG_ERROR << server_id_ << ":  Stats work too slow.. cannot record reader";
    delete copy;
    return;
  }
  DCHECK(!stats->connection_end_.is_set() &&
         !stats->stream_end_.is_set() &&
         !stats->media_end_.is_set());
  if ( stats->connection_begin_.is_set() ) {
    connection_stats_.insert(make_pair(stats, timer::Date::Now()));
  } else if ( stats->stream_begin_.is_set() ) {
    stream_stats_.insert(make_pair(stats, timer::Date::Now()));
  } else {
    CHECK(stats->media_begin_.is_set());
    media_stats_.insert(make_pair(stats, timer::Date::Now()));
  }
}
*/

void StatsCollector::EndStats(const ConnectionEnd* stats) {
  //CHECK(selector_->IsInSelectThread());
  synch::MutexLocker lock(&sync_connection_stats_);
  if ( !connection_stats_.erase(stats->connection_id_) ) {
    LOG_ERROR << "ConnectionEnd w/o begin: " << *stats;
    return;
  }
  MediaStatEvent* ev = MakeMediaStatEvent(stats);
  if ( !queue_->Put(ev, 0) ) {
    LOG_ERROR << server_id_ << ":  Stats work too slow -  "
              << " we could not register ConnectionEnd ";
    delete ev;
  }
  //LOG_DEBUG << "EndStats enqueued new stat: " << *stats;
}
void StatsCollector::EndStats(const StreamEnd* stats) {
  CHECK(selector_->IsInSelectThread());
  if ( !stream_stats_.erase(stats->stream_id_) ) {
    LOG_ERROR << "StreamEnd w/o begin: " << *stats;
    return;
  }
  MediaStatEvent* ev = MakeMediaStatEvent(stats);
  if ( !queue_->Put(ev, 0) ) {
    LOG_ERROR << server_id_ << ":  Stats work too slow -  "
              << " we could not register StreamEnd ";
    delete ev;
  }
  //LOG_DEBUG << "EndStats enqueued new stat: " << *stats;
}
void StatsCollector::EndStats(const MediaEnd* stats) {
  CHECK(selector_->IsInSelectThread());
  if ( !media_stats_.erase(stats->media_id_) ) {
    LOG_ERROR << "MediaEnd w/o begin: " << *stats;
    return;
  }
  MediaStatEvent* ev = MakeMediaStatEvent(stats);
  if ( !queue_->Put(ev, 0) ) {
    LOG_ERROR << server_id_ << ":  Stats work too slow -  "
              << " we could not register MediaEnd ";
    delete ev;
  }
  //LOG_DEBUG << "EndStats enqueued new stat: " << *stats;
}

/*
void StatsCollector::EndStats(MediaStatEvent* stats) {
  DCHECK_EQ(stats->server_id_, server_id_)
      << " -> " << stats->ToString()
      << " \n" << common::GetBacktrace();
  DCHECK(!stats->connection_begin_.is_set() &&
         !stats->stream_begin_.is_set() &&
         !stats->media_begin_.is_set());
  bool erased = false;
  if ( stats->connection_end_.is_set() ) {
    erased = connection_stats_.erase(stats);
  } else if ( stats->stream_end_.is_set() ) {
    erased = stream_stats_.erase(stats);
  } else {
    CHECK(stats->media_end_.is_set());
    erased = media_stats_.erase(stats);
  }
  if ( erased ) {
    MediaStatEvent* const copy =
        reinterpret_cast<MediaStatEvent*>(stats->Clone());
    if ( !queue_->Put(copy, 0) ) {
      LOG_ERROR << server_id_ << ":  Stats work too slow -  "
                << " we could not register end of stats ";
      delete copy;
    }
    //LOG_DEBUG << "EndStats enqueued new stat: " << *stats;
  } else {
    LOG_ERROR << "EndStats w/o begin: " << *stats;
  }
}
*/

bool StatsCollector::IsRunning() const {
  return thread_ != NULL;
}

typedef hash_map<string, MediaStreamStats*> StatsMap;
void  StatsCollector::GetStreamsStats(
    rpc::CallContext<MediaStreamsStats>* call,
    const vector<string>& stream_ids) {
  MediaStreamsStats ret;
  ret.count_ = 0;
  ret.bandwidth_up_avg_ = 0;
  ret.bandwidth_down_avg_ = 0;
  ret.duration_avg_ = 0;
  ret.streams_.ref();  // force is-set on this field

  StatsMap ids;
  for ( int i = 0; i < stream_ids.size(); ++i ) {
    MediaStreamStats* const s = new MediaStreamStats(0,0,0,0,0,0,0,0,0,0,0,0);
    if ( !ids.insert(make_pair(stream_ids[i], s)).second ) {
      LOG_WARNING << "Duplicate stream_id: " << stream_ids[i];
      delete s;
    }
  }

  const int64 now = timer::Date::Now();
  for ( MediaStatsMap::const_iterator it = media_stats_.begin();
        it != media_stats_.end(); ++it ) {
    const MediaBegin& media_begin = *(it->second.begin_);
    const MediaEnd& media_end = *(it->second.end_);

    const string& stream_id = media_begin.content_id_;

    const StatsMap::iterator it_stats = ids.find(stream_id);
    if ( it_stats == ids.end() ) {
      continue;
    }
    MediaStreamStats& s = *it_stats->second;

    const int64 stat_creation_time = media_begin.timestamp_utc_ms_;
    CHECK_GE(now, stat_creation_time);
    const double duration = (now - stat_creation_time) / 1000.0;   // seconds
    const double bandwidth_down = media_end.bytes_down_
                                  / duration * 8;    // bps
    const double bandwidth_up = media_end.bytes_up_ / duration * 8; // bps

    s.count_ = s.count_ + 1;

    s.bandwidth_up_ = s.bandwidth_up_ + bandwidth_up;
    if ( s.bandwidth_up_min_.get() == 0 ||
         bandwidth_up < s.bandwidth_up_min_ ) {
      s.bandwidth_up_min_ = bandwidth_up;
    }
    if ( bandwidth_up > s.bandwidth_up_max_ ) {
      s.bandwidth_up_max_ = bandwidth_up;
    }
    s.bandwidth_up_avg_ = s.bandwidth_up_avg_ + bandwidth_up;

    s.bandwidth_down_ = s.bandwidth_down_ + bandwidth_down;
    if ( s.bandwidth_down_min_.get() == 0 ||
         bandwidth_down < s.bandwidth_down_min_ ) {
      s.bandwidth_down_min_ = bandwidth_down;
    }
    if ( bandwidth_down > s.bandwidth_down_max_ ) {
      s.bandwidth_down_max_ = bandwidth_down;
    }
    s.bandwidth_down_avg_ = s.bandwidth_down_avg_ + bandwidth_down;

    if ( duration < s.duration_min_.get() ||
         s.duration_min_.get() == 0 ) {
      s.duration_min_.set(static_cast<int64>(duration));
    }
    if ( duration > s.duration_max_ ) {
      s.duration_max_.set(static_cast<int64>(duration));
    }
    s.duration_avg_.set(static_cast<int64>(s.duration_avg_ + duration));
  }
  for ( StatsMap::iterator it = ids.begin(); it != ids.end(); ++it ) {
    const string& stream_id = it->first;
    MediaStreamStats* const s = it->second;
    if ( s->count_ > 0 ) {
      s->duration_avg_ = s->duration_avg_ / s->count_;
      s->bandwidth_up_avg_ = s->bandwidth_up_ / s->count_;
      s->bandwidth_down_avg_ = s->bandwidth_down_ / s->count_;
    }
    ret.count_ = ret.count_ + s->count_;
    ret.bandwidth_up_avg_ = ret.bandwidth_up_avg_ +
                            s->count_ * s->bandwidth_up_avg_;
    ret.bandwidth_down_avg_ = ret.bandwidth_down_avg_ +
                              s->count_ * s->bandwidth_down_avg_;
    ret.duration_avg_ = ret.duration_avg_ +
                        s->count_ * s->duration_avg_;
    ret.streams_.ref().insert(make_pair(stream_id, *s));
    delete s;
  }
  ids.clear();

  if ( ret.count_ != 0 ) {
    ret.bandwidth_up_avg_ = ret.bandwidth_up_avg_ / ret.count_;
    ret.bandwidth_down_avg_ = ret.bandwidth_down_avg_ / ret.count_;
    ret.duration_avg_ = ret.duration_avg_ / ret.count_;
  }
  call->Complete(ret);
}
void StatsCollector::GetAllStreamIds(
    rpc::CallContext< map<string, int32> >* call) {
  map<string, int> stats;
  for ( MediaStatsMap::const_iterator it = media_stats_.begin();
        it != media_stats_.end(); ++it ) {
    const MediaBegin& media_begin = *(it->second.begin_);
    const MediaEnd& media_end = *(it->second.end_);

    const string& stream_id = media_begin.content_id_;
    UNUSED_ALWAYS(media_end);

    if ( stats.find(stream_id) == stats.end() ) {
      stats.insert(make_pair(stream_id, 0));
    }
    stats[stream_id]++;
  }
  call->Complete(stats);
}
void StatsCollector::GetDetailedMediaStats(
    rpc::CallContext< map<string, MediaBeginEnd> >* call,
    int32 start, int32 limit) {
  map<string, MediaBeginEnd> ret;
  MediaStatsMap::const_iterator it = media_stats_.begin();
  for ( int32 i = 0; i < start && it != media_stats_.end(); ++i, ++it ){}
  for ( int32 i = 0; i < limit && it != media_stats_.end(); ++i, ++it ) {
    const string& id = it->first;
    const MediaStats& media = it->second;
    ret.insert(make_pair(id, MediaBeginEnd(*media.begin_, *media.end_)));
  }
  call->Complete(ret);
}

}
