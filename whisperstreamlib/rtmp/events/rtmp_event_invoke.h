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

#ifndef __NET_RTMP_EVENTS_RTMP_EVENT_INVOKE_H__
#define __NET_RTMP_EVENTS_RTMP_EVENT_INVOKE_H__

#include <string>
#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperstreamlib/rtmp/events/rtmp_event.h>
#include <whisperstreamlib/rtmp/events/rtmp_call.h>

namespace rtmp {

class EventInvoke : public Event {
 public:
  explicit EventInvoke(Header* header)
      : Event(EVENT_INVOKE, SUBTYPE_SERVICE_CALL, header),
        rtmp_pending_call_(NULL) {
  }
  EventInvoke(ProtocolData* protocol_data,
              uint32 channel_id, uint32 stream_id)
      : Event(EVENT_INVOKE, SUBTYPE_SERVICE_CALL,
              protocol_data, channel_id, stream_id),
        rtmp_pending_call_(NULL) {
  }
  virtual ~EventInvoke() {
    delete rtmp_pending_call_;
  }

  const PendingCall* call() const {
    return rtmp_pending_call_;
  }
  PendingCall* mutable_call() const {
    return rtmp_pending_call_;
  }
  // Setter for call (we take control of the provided object)
  void set_call(PendingCall* rtmp_call) {
    delete rtmp_pending_call_;
    rtmp_pending_call_ = rtmp_call;
  }
  // Transfers the call to another EventInvoke object
  void move_call_to(EventInvoke* event) {
    event->rtmp_pending_call_ = rtmp_pending_call_;
    rtmp_pending_call_ = NULL;
  }

  const CObject* connection_params() const {
    if ( rtmp_pending_call_ == NULL ) return NULL;
    return rtmp_pending_call_->connection_params();
  }
  // Setter for connection_params_ (we take control of the provided object)
  void set_connection_params(CObject* connection_params) {
    CHECK(rtmp_pending_call_ != NULL);
    rtmp_pending_call_->set_connection_params(connection_params);
  }
  uint32 invoke_id() const {
    if ( rtmp_pending_call_ == NULL ) return 0;
    return rtmp_pending_call_->invoke_id();
  }
  void set_invoke_id(uint32 invoke_id) {
    if ( rtmp_pending_call_ == NULL ) return;
    return rtmp_pending_call_->set_invoke_id(invoke_id);
  }

  virtual string ToStringAttr() const {
    if ( rtmp_pending_call_ == NULL ) {
      return "NULL CALL";
    }
    return "Call: " + rtmp_pending_call_->ToString();
  }

  // Reading / writting
  virtual AmfUtil::ReadStatus ReadFromMemoryStream(io::MemoryStream* in,
                                                   AmfUtil::Version version);
  virtual void WriteToMemoryStream(io::MemoryStream* out,
                                   AmfUtil::Version version) const;

 protected:
  PendingCall* rtmp_pending_call_;

  void Clear() {
    delete rtmp_pending_call_;
    rtmp_pending_call_ = NULL;
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(EventInvoke);
};
}

#endif /*RTMP_EVENT_INVOKE_H_*/
