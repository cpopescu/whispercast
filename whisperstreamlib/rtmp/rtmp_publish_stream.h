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

#ifndef __NET_RTMP_RTMP_PUBLISH_STREAM_H__
#define __NET_RTMP_RTMP_PUBLISH_STREAM_H__

#include <whisperstreamlib/rtmp/rtmp_stream.h>
#include <whisperstreamlib/rtmp/rtmp_protocol.h>

#include <whisperstreamlib/flv/flv_consts.h>
#include <whisperstreamlib/flv/flv_tag.h>

#include <whisperstreamlib/rtmp/events/rtmp_event_invoke.h>
#include <whisperstreamlib/rtmp/events/rtmp_event_notify.h>

#include <whisperstreamlib/rtmp/rtmp_manager.h>

namespace rtmp {

//////////////////////////////////////////////////////////////////////
//
// Publishing stream - basically we operate in media stream alltogether..
//
class StreamManager;

class PublishStream : public Stream {
 public:
  PublishStream(StreamManager* manager,
           const StreamParams& params,
           Protocol* const protocol);
  virtual ~PublishStream();

  virtual void NotifyOutbufEmpty(int32 outbuf_size) {
  }

  virtual bool ProcessEvent(rtmp::Event* event, int64 timestamp_ms);

  virtual void Close();

  virtual bool IsPublishing(const string& stream_name) {
    return callback_ != NULL && stream_name == stream_name_;
  }

 private:
  bool InvokePublish(rtmp::EventInvoke* invoke);
  bool InvokeUnpublish(rtmp::EventInvoke* invoke);

  void CanPublishCompleted(int channel_id,
                           uint32 invoke_id,
                           bool success);

  void Stop(bool forced);

  bool SetMetadata(rtmp::EventNotify* n, int64 timestamp_ms);
  bool ProcessData(rtmp::BulkDataEvent* bd,
                   streaming::FlvFrameType data_type,
                   int64 timestamp_ms);

  bool SendTag(io::MemoryStream* tag_content,
               streaming::FlvFrameType data_type,
               const rtmp::Event* event, int64 timestamp_ms);

  StreamManager* const manager_;
  streaming::ProcessingCallback* callback_;
  int64 stream_offset_;
  bool closed_;
  streaming::FlvTagSerializer serializer_;

  DISALLOW_EVIL_CONSTRUCTORS(PublishStream);
};


} // namespace rtmp

#endif  // __NET_RTMP_RTMP_PUBLISH_STREAM_H__
