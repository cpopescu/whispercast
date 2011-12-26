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


#include <whisperstreamlib/base/request.h>
#include <whisperstreamlib/base/importer.h>
#include <whisperstreamlib/rtmp/rtmp_stream.h>
#include <whisperstreamlib/rtmp/rtmp_connection.h>
#include <whisperstreamlib/rtmp/events/rtmp_event_invoke.h>
#include <whisperstreamlib/rtmp/events/rtmp_event_notify.h>

namespace rtmp {

//////////////////////////////////////////////////////////////////////
//
// Publishing stream
//
class PublishStream : public Stream {
 public:
  PublishStream(const StreamParams& params,
                ServerConnection* connection,
                streaming::ElementMapper* element_mapper);
  virtual ~PublishStream();

  ////////////////////////////////////////////////////////////////////////
  // Stream methods
 protected:
  virtual void NotifyOutbufEmpty(int32 outbuf_size) {
  }
  virtual bool ProcessEvent(Event* event, int64 timestamp_ms);
  virtual void NotifyConnectionClosed() {
    Stop(true, true);
  }
 public:
  // called from ServerConnection::InvokeDeleteStream
  virtual void Close() {
    Stop(true, true);
  }

 private:
  bool InvokePublish(EventInvoke* invoke);
  bool InvokeUnpublish(EventInvoke* invoke);

  void StartPublishing(int channel_id, uint32 invoke_id,
                       streaming::AuthorizerRequest auth_req,
                       string command, bool dec_ref = false);
  // called by the importer
  void StartPublishingCompleted(int channel_id, uint32 invoke_id,
                                streaming::ProcessingCallback* callback);
  // called by the importer
  void StopPublisher();

  // internally stop everything, close network connection
  void Stop(bool send_eos, bool forced, bool dec_ref = false);

  void SendTag(scoped_ref<streaming::Tag> tag, int64 timestamp_ms, bool dec_ref = false);

 private:
  streaming::ElementMapper* element_mapper_;

  streaming::RtmpImporter* importer_;
  streaming::ProcessingCallback* callback_;

  // permanent callback to StopPublisher()
  Closure* stop_publisher_callback_;

  DISALLOW_EVIL_CONSTRUCTORS(PublishStream);
};


} // namespace rtmp

#endif  // __NET_RTMP_RTMP_PUBLISH_STREAM_H__
