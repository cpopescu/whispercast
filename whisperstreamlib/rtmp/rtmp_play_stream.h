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
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author: Catalin Popescu

#ifndef __NET_RTMP_RTMP_PLAY_STREAM_H__
#define __NET_RTMP_RTMP_PLAY_STREAM_H__

#include <whisperstreamlib/rtmp/rtmp_stream.h>
#include <whisperstreamlib/rtmp/rtmp_connection.h>

#include <whisperstreamlib/stats2/stats_keeper.h>

#include <whisperstreamlib/flv/flv_tag.h>
#include <whisperstreamlib/f4v/f4v_tag.h>
#include <whisperstreamlib/f4v/f4v_to_flv.h>

#include <whisperstreamlib/rtmp/events/rtmp_event_invoke.h>
#include <whisperstreamlib/rtmp/events/rtmp_event_notify.h>

#include <whisperstreamlib/base/exporter.h>

namespace rtmp {

//////////////////////////////////////////////////////////////////////

class PlayStream : public Stream, protected streaming::Exporter {
 public:
  PlayStream(const StreamParams& params,
             ServerConnection* connection,
             streaming::ElementMapper* element_mapper,
             streaming::StatsCollector* stats_collector);
  virtual ~PlayStream();

  ////////////////////////////////////////////////////////////////////
  // Stream methods

  // net selector
  virtual void NotifyOutbufEmpty(int32 outbuf_size);

  // net selector
  virtual bool ProcessEvent(Event* event, int64 timestamp_ms);

  // called from ServerConnection::InvokeDeleteStream
  virtual void Close() {
    CloseInternal();
  }

  // net selector
  virtual void NotifyConnectionClosed() {
    CloseInternal();
  }

 private:
  bool InvokePlay(const EventInvoke* invoke);
  bool FlexPlay(const EventFlexMessage* flex);

  bool InvokePause(const EventInvoke* invoke);
  bool FlexPause(const EventFlexMessage* flex);

  bool InvokeSeek(const EventInvoke* invoke);
  bool FlexSeek(const EventFlexMessage* flex);

  bool ProcessPlay(const string& stream_name, int64 seek_time_ms);
  bool ProcessPause(bool pause, int64 stream_time_ms);
  bool ProcessSeek(int64 seek_time_ms);

  //////////////////////////////////////////////////////////////////////

  void HandlePlay(string stream_name, int64 seek_time_ms, bool dec_ref = false);
  void HandlePause(bool pause, bool dec_ref = false);
  void HandleSeek(int64 seek_time_ms, bool dec_ref = false);

  //////////////////////////////////////////////////////////////////////
  //
  // ExporterT implementation
  //

 protected:
  virtual void IncRef() {
    Stream::IncRef();
  }
  virtual void DecRef() {
    Stream::DecRef();
  }

  virtual synch::Mutex* mutex() const {
    return &mutex_;
  }

  virtual bool is_closed() const {
    return connection_->is_closed();
  }

  virtual const ConnectionBegin& connection_begin_stats() const {
    return connection_->connection_begin_stats();
  }
  virtual const ConnectionEnd& connection_end_stats() const {
    return connection_->connection_end_stats();
  }

  virtual void OnStreamNotFound();
  virtual void OnTooManyClients();
  virtual void OnAuthorizationFailed(bool is_reauthorization);
  virtual void OnAddRequestFailed();
  virtual void OnPlay();

  virtual bool CanSendTag() const;
  virtual void SetNotifyReady();
  virtual void SendTag(const streaming::Tag* tag, int64 tag_timestamp_ms);

 private:
  void SendSimpleTag(const streaming::Tag* tag, int64 tag_timestamp_ms);
  void SendFlvTag(const streaming::FlvTag* tag, int64 tag_timestamp_ms);

  // accumulate this tag into a large media event
  bool AppendToMediaTag(const streaming::FlvTag* flv_tag,
                        int64 tag_timestamp_ms);
  // send that large media event
  void SendMediaTag();

  void CloseInternal(bool dec_ref = false);

 protected:
  //////////////////////////////////////////////////////////////////////
  //
  // MEDIA/NET THREAD members
  //

  // we are seeking
  bool seeking_;

  //////////////////////////////////////////////////////////////////////
  //
  // MEDIA THREAD members
  //

  // Enables us to process seek a little after we receive it.
  // Helps with multiple seeks in a short period of time.
  util::Alarm seek_alarm_;

  // F4V related members
  streaming::F4vToFlvConverter f4v2flv_;
  uint32 f4v_cue_point_number_;

  //////////////////////////////////////////////////////////////////////
  //
  // NET THREAD members
  //

  // If we want are in building media aggregated tags we use these:
  enum MediaBuildState {
    WAITING_VIDEO,     // we decide that we build mediat tags, so we wait for
                       // next video tag
    WAITING_AUDIO,     // waiting for an audio tag in WAITING_VIDEO state
    BUILDING_MEDIA,    // we accumulate in collapsed_media_tag_.
  };

  streaming::FlvFlagVideoCodec video_codec_;
  streaming::FlvFlagVideoFrameType video_frame_type_;

  MediaBuildState media_build_state_;

  // The timestamp of the first video tag in WAITING_VIDEO state
  int32 composed_media_data_tag_delta_;

  scoped_ref<MediaDataEvent> media_event_;
  int32 media_event_size_;
  int64 media_event_timestamp_ms_;
  int64 media_event_duration_ms_;

  // If true then we are in the middle of the bootstrap
  bool bootstrapping_;

  // If true we need to send send reset/clear pings before the first media tag
  bool send_reset_audio_;
  bool send_reset_pings_;
  bool send_switch_media_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(PlayStream);
};

} // namespace rtmp

#endif  // __NET_RTMP_RTMP_PLAY_STREAM_H__
