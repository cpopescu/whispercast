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

class PublishStream : public Stream, public streaming::Publisher {
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
  virtual bool ProcessEvent(const Event* event, int64 timestamp_ms);
  virtual void NotifyConnectionClosed() {
    InternalStop(true, true);
  }
 public:
  // called from ServerConnection::InvokeDeleteStream
  virtual void Close() {
    InternalStop(true, true);
  }

 private:
  bool InvokePublish(const Event* event, int invoke_id);
  bool InvokeUnpublish(const Event* event, int invoke_id);

  void StartPublishing(streaming::AuthorizerRequest auth_req,
                       bool dec_ref = false);

  // internally stop everything, close network connection
  void InternalStop(bool send_eos, bool forced, bool dec_ref = false);

  void SendTag(scoped_ref<streaming::Tag> tag, int64 timestamp_ms, bool dec_ref = false);

  ///////////////////////////////////////////////////////////////////////////
  // Publisher methods
  virtual void StartCompleted(bool success);
  virtual void Stop() {
    CHECK(connection_->media_selector()->IsInSelectThread());
    // stop sending tags downstream
    importer_ = NULL;
    InternalStop(false, true);
  }
  virtual const streaming::MediaInfo& GetMediaInfo() const {
    return media_info_;
  }

 private:
  streaming::ElementMapper* element_mapper_;
  streaming::RtmpImporter* importer_;

  // Between importer->Start() and asynchronous StartCompleted() we mark
  // pending_start_ == true. Because a malicious client may send TAGs before
  // Publish completes, and we don't want to break the Importer.
  bool pending_start_;

  // parameters from the 'publish' request
  // command: can be "live", "record" or "append"
  string publish_command_;
  // channel_id: needed to send publish response back to client
  int publish_channel_id_;
  // invoke_id: needed to send publish response back to client
  uint32 publish_invoke_id_;

  // stream properties, learned from metadata
  bool has_audio_;
  bool has_video_;
  // we need the first audio + video + metadata tag to extract media info
  scoped_ref<streaming::FlvTag> first_audio_;
  scoped_ref<streaming::FlvTag> first_video_;
  scoped_ref<streaming::FlvTag> first_metadata_;
  streaming::MediaInfo media_info_;
  bool media_info_extracted_;

  DISALLOW_EVIL_CONSTRUCTORS(PublishStream);
};


} // namespace rtmp

#endif  // __NET_RTMP_RTMP_PUBLISH_STREAM_H__
