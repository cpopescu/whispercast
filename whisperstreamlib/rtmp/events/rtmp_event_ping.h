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

#ifndef __NET_RTMP_EVENTS_RTMP_EVENT_PING_H__
#define __NET_RTMP_EVENTS_RTMP_EVENT_PING_H__

#include <string>
#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperstreamlib/rtmp/events/rtmp_event.h>

namespace rtmp {

class EventPing : public Event {
 public:
  enum Type {
    STREAM_CLEAR             = 0,
    STREAM_CLEAR_BUFFER      = 1,

    CLIENT_BUFFER            = 3,
    STREAM_RESET             = 4,

    PING_CLIENT              = 6,
    PONG_SERVER              = 7,

    SWF_VERIFY_REQUEST       = 26,
    SWF_VERIFY_RESPONSE      = 27
  };
  static const char* TypeName(Type type_id);

  // return: test if the given value correponds to a TYPE constant
  static bool IsValidType(uint16 val) {
    switch (val) {
      case STREAM_CLEAR:
      case STREAM_CLEAR_BUFFER:

      case CLIENT_BUFFER:
      case STREAM_RESET:

      case PING_CLIENT:
      case PONG_SERVER:

      case SWF_VERIFY_REQUEST:
      case SWF_VERIFY_RESPONSE:
        return true;
    }
    return false;
  }

 public:
  explicit EventPing(Header* header)
      : Event(EVENT_PING, SUBTYPE_SYSTEM, header),
        ping_type_(PONG_SERVER), value2_(0), value3_(-1),
        value4_(-1), value5_(-1) {
  }
  EventPing(ProtocolData* protocol_data,
            uint32 channel_id, uint32 stream_id)
      : Event(EVENT_PING, SUBTYPE_SYSTEM, protocol_data,
              channel_id, stream_id),
        ping_type_(PONG_SERVER), value2_(0), value3_(-1),
        value4_(-1), value5_(-1) {
  }

  EventPing(Type value1, int32 value2, Header* header)
      : Event(EVENT_PING, SUBTYPE_SYSTEM, header),
        ping_type_(value1), value2_(value2), value3_(-1),
        value4_(-1), value5_(-1) {
  }
  EventPing(Type value1, int32 value2,
            ProtocolData* protocol_data,
            uint32 channel_id, uint32 stream_id)
      : Event(EVENT_PING, SUBTYPE_SYSTEM, protocol_data,
              channel_id, stream_id),
        ping_type_(value1), value2_(value2), value3_(-1),
        value4_(-1), value5_(-1) {
  }
  EventPing(Type value1, int32 value2, int32 value3,
            ProtocolData* protocol_data,
            uint32 channel_id, uint32 stream_id)
      : Event(EVENT_PING, SUBTYPE_SYSTEM, protocol_data,
              channel_id, stream_id),
        ping_type_(value1), value2_(value2), value3_(value3),
        value4_(-1), value5_(-1) {
  }
  EventPing(Type value1, int32 value2, int32 value3, int32 value4,
            ProtocolData* protocol_data,
            uint32 channel_id, uint32 stream_id)
      : Event(EVENT_PING, SUBTYPE_SYSTEM, protocol_data,
              channel_id, stream_id),
        ping_type_(value1), value2_(value2), value3_(value3),
        value4_(value4), value5_(-1) {
  }

  EventPing(Type value1, int32 value2, int32 value3, Header* header)
      : Event(EVENT_PING, SUBTYPE_SYSTEM, header),
        ping_type_(value1), value2_(value2), value3_(value3),
        value4_(-1), value5_(-1) {
  }
  EventPing(Type value1, int32 value2, int32 value3, int32 value4,
            Header* header)
      : Event(EVENT_PING, SUBTYPE_SYSTEM, header),
        ping_type_(value1), value2_(value2), value3_(value3),
        value4_(value4), value5_(-1) {
  }
  virtual ~EventPing() {
  }

  EventPing::Type ping_type() const {
    return ping_type_;
  }
  int32 value2() const {
    return value2_;
  }
  int32 value3() const {
    return value3_;
  }
  int32 value4() const {
    return value4_;
  }
  int32 value5() const {
    return value5_;
  }
  const char* GetPingTypeName() const {
    return TypeName(ping_type_);
  }

  void set_ping_type(EventPing::Type ping_type) {
    ping_type_ = ping_type;
  }
  void set_value2(int32 value2)  {
    value2_ = value2;
  }
  void set_value3(int32 value3)  {
    value3_ = value3;
  }
  void set_value4(int32 value4)  {
    value4_ = value4;
  }
  void set_value5(int32 value5)  {
    value5_ = value5;
  }
  virtual bool Equals(const Event* obj) const {
    if ( !Event::Equals(obj) ) return false;
    const EventPing* event =
        reinterpret_cast<const EventPing*>(obj);
    return (event->ping_type() == ping_type() &&
            event->value2() == value2() &&
            event->value3() == value3() &&
            event->value4() == value4() &&
            event->value5() == value5());
  }
  virtual string ToStringAttr() const {
    return strutil::StringPrintf("ping_type_: %s, value2: %d, value3: %d"
        ", value4: %d, value5: %d",
        GetPingTypeName(), value2_, value3_, value4_, value5_);
  }
  virtual AmfUtil::ReadStatus ReadFromMemoryStream(
      io::MemoryStream* in, AmfUtil::Version version);
  virtual void WriteToMemoryStream(
      io::MemoryStream* out, AmfUtil::Version version) const;

 protected:
  Type  ping_type_;
  int32 value2_;       // XXX: can someone suggest better names?
  int32 value3_;
  int32 value4_;
  int32 value5_;
};
}
#endif  // __NET_RTMP_EVENTS_RTMP_EVENT_PING_H__
