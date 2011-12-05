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
// Author: Cosmin Tudorache & Catalin Popescu

#ifndef __NET_RTMP_EVENTS_RTMP_EVENT_H__
#define __NET_RTMP_EVENTS_RTMP_EVENT_H__

#include <string>
#include <vector>

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperlib/common/io/num_streaming.h>

#include <whisperstreamlib/rtmp/objects/rtmp_objects.h>
#include <whisperstreamlib/rtmp/rtmp_consts.h>
#include <whisperstreamlib/rtmp/events/rtmp_header.h>
#include <whisperstreamlib/rtmp/objects/amf/amf0_util.h>

namespace rtmp {

// EventNotify, EventPing and EventInvoke in their different
// files, all other events are here.

class Event {
 public:
  // Constructs an event based on a type and a header. (Which comes under
  // our control..)
  Event(EventType event_type, EventSubType event_subtype, Header* header)
      : event_type_(event_type),
        event_subtype_(event_subtype),
        header_(header) {
  }
  Event(EventType event_type, EventSubType event_subtype,
        ProtocolData* protocol_data, uint32 channel_id, uint32 stream_id)
      : event_type_(event_type),
        event_subtype_(event_subtype),
        header_(new Header(protocol_data)) {
    header_->set_channel_id(channel_id);
    header_->set_stream_id(stream_id);
    header_->set_event_type(event_type);
  }
  virtual ~Event() {
    delete header_;
  }

  EventType event_type() const {
    return event_type_;
  }
  const char* event_type_name() const {
    return EventTypeName(event_type_);
  }
  EventSubType event_subtype() const {
    return event_subtype_;
  }
  const char* event_subtype_name() const {
    return EventSubTypeName(event_subtype_);
  }
  const Header* header() const {
    return header_;
  }
  Header* mutable_header() {
    return header_;
  }

  virtual bool Equals(const Event* event) const {
    return ( event->event_type() == event_type() &&
             event->event_subtype() == event_subtype());
  }
  // Returns a human readable string
  string ToString() const {
    return strutil::StringPrintf("Event{type: %s:%s, header: %s, %s}",
        event_type_name(), event_subtype_name(), header_->ToString().c_str(),
        ToStringAttr().c_str());
  }
  // Reading / writting
  virtual AmfUtil::ReadStatus ReadFromMemoryStream(
      io::MemoryStream* in, AmfUtil::Version version) {
    LOG_ERROR << " ReadFromMemoryStream not implemented for " << ToString();
    return AmfUtil::READ_NOT_IMPLEMENTED;
  }
  virtual void WriteToMemoryStream(
      io::MemoryStream* out, AmfUtil::Version version) const {
    LOG_ERROR << " ReadFromMemoryStream not implemented for " << ToString();
  }
 private:
  // Subclasses must serialize their attributes in human readable form.
  virtual string ToStringAttr() const = 0;

 private:
  const EventType event_type_;
  const EventSubType event_subtype_;
  Header* const header_;
 private:
  DISALLOW_EVIL_CONSTRUCTORS(Event);
};

inline ostream& operator<<(ostream& os, const Event& obj) {
  return os << obj.ToString();
}

//////////////////////////////////////////////////////////////////////

//  An event that has a bulk data member
//  Extremely important - All ints in memory streams are big endian !!
class BulkDataEvent : public Event {
 public:
  BulkDataEvent(EventType event_type, EventSubType event_subtype,
                Header* header,
                int block_size = io::DataBlock::kDefaultBufferSize)
      : Event(event_type, event_subtype, header),
        data_(block_size) {
  }
  BulkDataEvent(EventType event_type, EventSubType event_subtype,
                ProtocolData* protocol_data,
                uint32 channel_id, uint32 stream_id,
                int block_size = io::DataBlock::kDefaultBufferSize)
      : Event(event_type, event_subtype,
              protocol_data, channel_id, stream_id),
        data_(block_size) {
  }

  virtual ~BulkDataEvent() {
  }

  // Cannot do much w/ this getter .. but whatever..
  const io::MemoryStream& data() const {
    return data_;
  }
  // the meat..
  io::MemoryStream* mutable_data() {
    return &data_;
  }
  // moves data from in to our data buffer (clearing what is set before)
  void set_data(io::MemoryStream* in) {
    data_.Clear();
    data_.AppendStream(in);
    mutable_header()->set_event_size(data_.Size());
  }
  // append data from in to our data buffer
  void append_data(io::MemoryStream* in) {
    data_.AppendStream(in);
    mutable_header()->set_event_size(data_.Size());
  }
  void append_data(const char* data, int32 size, Closure* disposer = NULL) {
    data_.AppendRaw(data, size, disposer);
    mutable_header()->set_event_size(data_.Size());
  }
  // copies data without destroying in
  void copy_data(const io::MemoryStream* in) {
    data_.Clear();
    data_.AppendStreamNonDestructive(in);
    mutable_header()->set_event_size(data_.Size());
  }
  virtual bool Equals(const Event* obj) const {
    if ( !Event::Equals(obj) ) return false;
    const BulkDataEvent* event =
        reinterpret_cast<const BulkDataEvent*>(obj);
    return event->data().Size() == data_.Size();
  }
  virtual string ToStringAttr() const {
    return strutil::StringPrintf("data.size: %d", data_.Size());
  }
  virtual AmfUtil::ReadStatus ReadFromMemoryStream(
      io::MemoryStream* in, AmfUtil::Version version) {
    data_.AppendStream(in, header()->event_size());
    mutable_header()->set_event_size(data_.Size());
    return AmfUtil::READ_OK;
  }
  virtual void WriteToMemoryStream(
      io::MemoryStream* out, AmfUtil::Version version) const {
    out->AppendStreamNonDestructive(&data_);
  }
 private:
  io::MemoryStream data_;
  DISALLOW_EVIL_CONSTRUCTORS(BulkDataEvent);
};

//////////////////////////////////////////////////////////////////////

class EventUnknown : public BulkDataEvent {
 public:
  explicit EventUnknown(Header* header)
      : BulkDataEvent(EVENT_INVALID, SUBTYPE_SYSTEM, header) {
  }
  EventUnknown(ProtocolData* protocol_data,
               uint32 channel_id, uint32 stream_id)
      : BulkDataEvent(EVENT_INVALID, SUBTYPE_SYSTEM,
                      protocol_data, channel_id, stream_id) {
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(EventUnknown);
};

class EventVideoData : public BulkDataEvent {
 public:
  explicit EventVideoData(Header* header)
      : BulkDataEvent(EVENT_VIDEO_DATA, SUBTYPE_STREAM_DATA, header) {
  }
  EventVideoData(ProtocolData* protocol_data,
                 uint32 channel_id, uint32 stream_id)
      : BulkDataEvent(EVENT_VIDEO_DATA, SUBTYPE_STREAM_DATA,
                      protocol_data, channel_id, stream_id) {
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(EventVideoData);
};

class EventAudioData : public BulkDataEvent {
 public:
  explicit EventAudioData(Header* header)
      : BulkDataEvent(EVENT_AUDIO_DATA, SUBTYPE_STREAM_DATA, header) {
  }
  EventAudioData(ProtocolData* protocol_data,
                 uint32 channel_id, uint32 stream_id)
      : BulkDataEvent(EVENT_AUDIO_DATA, SUBTYPE_STREAM_DATA,
                      protocol_data, channel_id, stream_id) {
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(EventAudioData);
};

//////////////////////////////////////////////////////////////////////

// NOTE: this is some kind of tricky stuff .. basically we use this
// *just to write* things that we know are metadata, and we don't
// want to split arround.
// They are read as notification events !

class EventStreamMetadata : public BulkDataEvent {
 public:
  explicit EventStreamMetadata(Header* header)
      : BulkDataEvent(EVENT_NOTIFY, SUBTYPE_SERVICE_CALL, header) {
  }
  EventStreamMetadata(ProtocolData* protocol_data,
                      uint32 channel_id, uint32 stream_id)
      : BulkDataEvent(EVENT_NOTIFY, SUBTYPE_SERVICE_CALL,
                      protocol_data, channel_id, stream_id) {
  }
  virtual string ToStringAttr() const {
    return strutil::StringPrintf("data.size: %d", data().Size());
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(EventStreamMetadata);
};

//////////////////////////////////////////////////////////////////////

// Events that have just one uint32 included
#define DECLARE_UINT32_EVENT(classname, evtype, evsubtype, member)      \
  class classname : public Event {                                      \
    public:                                                             \
    explicit classname(Header* header)                                  \
        : Event(evtype, evsubtype, header), member##_(0) {}             \
    classname(uint32 member, Header* header)                            \
        : Event(evtype, evsubtype, header), member##_(member) {}        \
    classname(uint32 member,                                            \
              ProtocolData* protocol_data,                              \
              uint32 channel_id, uint32 stream_id)                      \
        : Event(evtype, evsubtype, protocol_data, channel_id, stream_id), \
          member##_(member) {}                                          \
    uint32 member() const { return member##_; }                         \
    void set_##member(uint32 member) { member##_ = member; }            \
    virtual bool Equals(const Event* obj) const {                       \
      if ( !Event::Equals(obj) ) return false;                          \
      const classname* event = reinterpret_cast<const classname*>(obj); \
      return event->member() == member();                               \
    }                                                                   \
    virtual string ToStringAttr() const {                               \
      return strutil::StringPrintf(#member ": %d", member##_);          \
    }                                                                   \
    virtual AmfUtil::ReadStatus                                         \
    ReadFromMemoryStream(io::MemoryStream* in,                          \
                         AmfUtil::Version version) {                    \
      if ( in->Size() < sizeof(member##_) ) {                           \
        return AmfUtil::READ_NO_DATA;                                   \
      }                                                                 \
      member##_ = static_cast<uint32>(io::NumStreamer::ReadInt32(in,    \
                                      common::BIGENDIAN));              \
      return AmfUtil::READ_OK;                                          \
    }                                                                   \
    virtual void WriteToMemoryStream(io::MemoryStream* out,             \
                                     AmfUtil::Version version) const {  \
      io::NumStreamer::WriteInt32(out,                                  \
                                  static_cast<int32>(member##_),        \
                                  common::BIGENDIAN);                   \
    }                                                                   \
    private:                                                            \
    uint32 member##_;                                                   \
    private:                                                            \
    DISALLOW_EVIL_CONSTRUCTORS(classname);                              \
  }


// An event Sent every x bytes by both sides during read
DECLARE_UINT32_EVENT(EventBytesRead,
                     EVENT_BYTES_READ, SUBTYPE_STREAM_CONTROL,
                     bytes_read);

// Sets the protocol chunk size..
DECLARE_UINT32_EVENT(EventChunkSize,
                     EVENT_CHUNK_SIZE, SUBTYPE_SYSTEM,
                     chunk_size);

// This message is transmitted between client and server, it's meaning is
// unknown really (in terms of what bandwidth is expressed bps, Bps, etc),
// and in fact we don't really use it..

// see org.red5.server.net.rtmp.event.ServerBW
DECLARE_UINT32_EVENT(EventServerBW,
                     EVENT_SERVER_BANDWIDTH, SUBTYPE_STREAM_CONTROL,
                     bandwidth);

// This message is transmitted between client and server, it's meaning is
// unknown really (in terms of what bandwidth is expressed bps, Bps, etc),
// and in fact we don't really use it..

// see org.red5.server.net.rtmp.event.ClientBW
class EventClientBW : public Event {
 public:
  explicit EventClientBW(Header* header)
      : Event(EVENT_CLIENT_BANDWIDTH, SUBTYPE_STREAM_CONTROL, header),
        bandwidth_(0), value2_(0) {
  }
  EventClientBW(uint32 bandwidth, uint8 value2, Header* header)
      : Event(EVENT_CLIENT_BANDWIDTH, SUBTYPE_STREAM_CONTROL, header),
        bandwidth_(bandwidth), value2_(value2) {
  }
  EventClientBW(uint32 bandwidth, uint8 value2,
                ProtocolData* protocol_data,
                uint32 channel_id, uint32 stream_id)
      : Event(EVENT_CLIENT_BANDWIDTH, SUBTYPE_STREAM_CONTROL,
              protocol_data, channel_id, stream_id),
        bandwidth_(bandwidth), value2_(value2) {
  }
  uint32 bandwidth() const { return bandwidth_; }
  void set_bandwidth(uint32 bandwidth) { bandwidth_ = bandwidth; }
  uint8 value2() const { return value2_; }
  void set_value2(uint8 value2) { value2_ = value2; }

  virtual bool Equals(const Event* obj) const {
    if ( !Event::Equals(obj) ) return false;
    const EventClientBW* event =
        reinterpret_cast<const EventClientBW*>(obj);
    return (event->bandwidth() == bandwidth() &&
            event->value2() == value2());
  }
  virtual string ToStringAttr() const {
    return strutil::StringPrintf("bandwidth_: %u, value2_: %hhu",
                                  bandwidth_, value2_);
  }
  virtual AmfUtil::ReadStatus ReadFromMemoryStream(
      io::MemoryStream* in, AmfUtil::Version version) {
    if ( in->Size() < sizeof(bandwidth_) + sizeof(value2_) ) {
      return AmfUtil::READ_NO_DATA;
    }
    bandwidth_ = static_cast<uint32>(io::NumStreamer::ReadInt32(in,
        common::BIGENDIAN));
    value2_ = uint8(io::NumStreamer::ReadByte(in));
    return AmfUtil::READ_OK;
  }
  virtual void WriteToMemoryStream(
      io::MemoryStream* out, AmfUtil::Version version) const {
    io::NumStreamer::WriteInt32(out, bandwidth_, common::BIGENDIAN);
    io::NumStreamer::WriteByte(out, value2_);
  }
 private:
  uint32 bandwidth_;
  uint8  value2_;       // Unclear what this means ...

  DISALLOW_EVIL_CONSTRUCTORS(EventClientBW);
};


// Flex message - seems to be just a vector of events
class EventFlexMessage : public Event {
 public:
  explicit EventFlexMessage(Header* header)
      : Event(EVENT_FLEX_MESSAGE, SUBTYPE_SERVICE_CALL, header),
        unknown_(0) {
  }
  EventFlexMessage(ProtocolData* protocol_data,
                   uint32 channel_id, uint32 stream_id)
      : Event(EVENT_FLEX_MESSAGE, SUBTYPE_SERVICE_CALL,
              protocol_data, channel_id, stream_id),
        unknown_(0) {
  }
  ~EventFlexMessage() {
    Clear();
  }

  // Returns a human readable string
  virtual string ToStringAttr() const {
    string s = "\n";
    for ( int i = 0; i < data_.size(); ++i ) {
      s += strutil::StringPrintf("%5d: %s\n", i, data_[i]->ToString().c_str());
    }
    return s;
  }

  // Reading / writting
  virtual AmfUtil::ReadStatus ReadFromMemoryStream(
      io::MemoryStream* in, AmfUtil::Version version) {
    if ( in->Size() < sizeof(uint8) ) {
      return AmfUtil::READ_NO_DATA;
    }
    Clear();
    in->MarkerSet();
    unknown_ = io::NumStreamer::ReadByte(in);
    AmfUtil::ReadStatus err = AmfUtil::READ_NO_DATA;
    while ( !in->IsEmpty() ) {
      CObject* obj = NULL;
      err = Amf0Util::ReadNextObject(in, &obj);
      if ( err == AmfUtil::READ_OK ) {
        data_.push_back(obj);
      } else {
        in->MarkerRestore();
        Clear();
        return err;
      }
    }
    in->MarkerClear();
    return err;
  }
  virtual void WriteToMemoryStream(
      io::MemoryStream* out, AmfUtil::Version version) const {
    io::NumStreamer::WriteByte(out, unknown_);
    for ( int i = 0; i < data_.size(); ++i ) {
      data_[i]->WriteToMemoryStream(out, version);
    }
  }

  const vector<CObject*>& data() const { return data_; }
  vector<CObject*>* mutable_data() { return &data_; }

  void Clear() {
    for ( int i = 0; i < data_.size(); ++i ) {
      delete data_[i];
    }
    data_.clear();
  }

 private:
  uint8 unknown_;
  vector<CObject*> data_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(EventFlexMessage);
};

//////////////////////////////////////////////////////////////////////

// The really stupid events - basically we don't know what they mean ..


class EventFlexSharedObject : public BulkDataEvent {
 public:
  explicit EventFlexSharedObject(Header* header)
      : BulkDataEvent(EVENT_FLEX_SHARED_OBJECT, SUBTYPE_SYSTEM, header) {
  }
  EventFlexSharedObject(ProtocolData* protocol_data,
                        uint32 channel_id, uint32 stream_id)
      : BulkDataEvent(EVENT_FLEX_SHARED_OBJECT, SUBTYPE_SYSTEM,
                      protocol_data, channel_id, stream_id) {
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(EventFlexSharedObject);
};

class EventSharedObject : public BulkDataEvent {
 public:
  explicit EventSharedObject(Header* header)
      : BulkDataEvent(EVENT_SHARED_OBJECT, SUBTYPE_SYSTEM, header) {
  }
  EventSharedObject(ProtocolData* protocol_data,
                    uint32 channel_id, uint32 stream_id)
      : BulkDataEvent(EVENT_SHARED_OBJECT, SUBTYPE_SYSTEM,
                      protocol_data, channel_id, stream_id) {
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(EventSharedObject);
};

class MediaDataEvent : public BulkDataEvent {
 public:
  explicit MediaDataEvent(Header* header)
      : BulkDataEvent(EVENT_MEDIA_DATA, SUBTYPE_STREAM_DATA, header) {
  }
  MediaDataEvent(ProtocolData* protocol_data,
                 uint32 channel_id, uint32 stream_id)
      : BulkDataEvent(EVENT_MEDIA_DATA, SUBTYPE_STREAM_DATA,
                      protocol_data, channel_id, stream_id) {
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(MediaDataEvent);
};
//////////////////////////////////////////////////////////////////////
}

#endif  // __NET_RTMP_EVENTS_RTMP_EVENT_H__
