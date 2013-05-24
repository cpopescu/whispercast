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
#include <whisperlib/common/base/ref_counted.h>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperlib/common/io/num_streaming.h>

#include <whisperstreamlib/rtmp/objects/rtmp_objects.h>
#include <whisperstreamlib/rtmp/rtmp_consts.h>
#include <whisperstreamlib/rtmp/events/rtmp_header.h>
#include <whisperstreamlib/rtmp/objects/amf/amf0_util.h>
#include <whisperstreamlib/flv/flv_tag.h>

namespace rtmp {

// EventNotify, EventPing and EventInvoke in their different
// files, all other events are here.

class Event : public RefCounted {
 public:
  // Constructs an event based on a type and a header.
  Event(const Header& header, EventType event_type, EventSubType event_subtype)
      : header_(header),
        event_type_(event_type),
        event_subtype_(event_subtype) {
    CHECK_EQ(header.event_type(), event_type);
  }
  virtual ~Event() {
  }

  EventType event_type() const {
    return event_type_;
  }
  const char* event_type_name() const {
    return EventTypeName(event_type());
  }
  EventSubType event_subtype() const {
    return event_subtype_;
  }
  const char* event_subtype_name() const {
    return EventSubTypeName(event_subtype_);
  }
  const Header& header() const {
    return header_;
  }
  Header* mutable_header() {
    return &header_;
  }

  virtual bool Equals(const Event* event) const {
    return (event->event_type() == event_type() &&
            event->event_subtype() == event_subtype());
  }
  virtual AmfUtil::ReadStatus DecodeBody(io::MemoryStream* in,
      AmfUtil::Version version) = 0;
  virtual void EncodeBody(io::MemoryStream* out,
      AmfUtil::Version version) const = 0;

  string ToString() const {
    return strutil::StringPrintf("Event{type: %s:%s, header: %s, %s}",
        event_type_name(), event_subtype_name(), header_.ToString().c_str(),
        ToStringAttr().c_str());
  }
 private:
  // Subclasses must serialize their attributes in human readable form.
  virtual string ToStringAttr() const = 0;

 private:
  Header header_;
  const EventType event_type_;
  const EventSubType event_subtype_;
 private:
  DISALLOW_EVIL_CONSTRUCTORS(Event);
};

inline ostream& operator<<(ostream& os, const Event& obj) {
  return os << obj.ToString();
}

//////////////////////////////////////////////////////////////////////

//  An event that has a bulk data member
//  Extremely important: all ints in memory streams are big endian !!
class BulkDataEvent : public Event {
 public:
  BulkDataEvent(const Header& header,
                EventType event_type,
                EventSubType event_subtype,
                int block_size = io::DataBlock::kDefaultBufferSize)
      : Event(header, event_type, event_subtype),
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
  }
  // append data from in to our data buffer
  void append_data(io::MemoryStream* in) {
    data_.AppendStream(in);
  }
  void append_data(const char* data, int32 size, Closure* disposer = NULL) {
    data_.AppendRaw(data, size, disposer);
  }
  // copies data without destroying in
  void copy_data(const io::MemoryStream* in) {
    data_.Clear();
    data_.AppendStreamNonDestructive(in);
  }
  virtual bool Equals(const Event* obj) const {
    if ( !Event::Equals(obj) ) return false;
    const BulkDataEvent* event = static_cast<const BulkDataEvent*>(obj);
    return data_.Equals(event->data());
  }
  virtual string ToStringAttr() const {
    return strutil::StringPrintf("data.size: %d", data_.Size());
  }
  virtual AmfUtil::ReadStatus DecodeBody(io::MemoryStream* in,
      AmfUtil::Version version) {
    data_.AppendStream(in, in->Size());
    return AmfUtil::READ_OK;
  }
  virtual void EncodeBody(io::MemoryStream* out,
      AmfUtil::Version version) const {
    out->AppendStreamNonDestructive(&data_);
  }
 protected:
  io::MemoryStream data_;
  DISALLOW_EVIL_CONSTRUCTORS(BulkDataEvent);
};

//////////////////////////////////////////////////////////////////////

// contains a single Video Tag
class EventVideoData : public BulkDataEvent {
 public:
  explicit EventVideoData(const Header& header)
      : BulkDataEvent(header, EVENT_VIDEO_DATA, SUBTYPE_STREAM_DATA) {
  }
  // NO EncodeTag(), because Coder::EncodeWithAuxBuffer() offers a nice
  // performance gain by encoding tag->data directly to Network stream

  // Extracts 1 Video FlvTag from inside our body.
  void DecodeTag(int64 timestamp_ms,
                 scoped_ref<streaming::FlvTag>* out) const;
 private:
  DISALLOW_EVIL_CONSTRUCTORS(EventVideoData);
};

// contains a single Audio Tag
class EventAudioData : public BulkDataEvent {
 public:
  explicit EventAudioData(const Header& header)
      : BulkDataEvent(header, EVENT_AUDIO_DATA, SUBTYPE_STREAM_DATA) {
  }
  // NO EncodeTag(), because Coder::EncodeWithAuxBuffer() offers a nice
  // performance gain by encoding tag->data directly to Network stream

  // Extracts 1 Audio FlvTag from inside our body.
  void DecodeTag(int64 timestamp_ms,
                 scoped_ref<streaming::FlvTag>* out) const;
 private:
  DISALLOW_EVIL_CONSTRUCTORS(EventAudioData);
};

// contains multiple Audio & Video Tags
class MediaDataEvent : public BulkDataEvent {
 public:
  explicit MediaDataEvent(const Header& header)
      : BulkDataEvent(header, EVENT_MEDIA_DATA, SUBTYPE_STREAM_DATA),
        first_tag_ts_(-1),
        previous_tag_size_(0),
        duration_ms_(0) {
  }
  int64 first_tag_ts() const { return first_tag_ts_; }
  int64 duration_ms() const { return duration_ms_; }
  // tag: tag to encode inside our body
  // timestamp_ms: tag's timestamp (does not have to start on 0)
  void EncodeAddTag(const streaming::FlvTag& tag, int64 timestamp_ms);
  // After several EncodeAddTag(), call this method to finalize encoding
  void EncodeFinalize();
  // Decode all the tags inside our body.
  // Offset tags timestamp by 'timestamp_ms'
  void DecodeTags(int64 timestamp_ms,
      vector<scoped_ref<streaming::FlvTag> >* out) const;

 private:
  int64 first_tag_ts_;
  uint32 previous_tag_size_;
  // accumulated tags duration (based on their timestamp)
  int64 duration_ms_;
  DISALLOW_EVIL_CONSTRUCTORS(MediaDataEvent);
};

//////////////////////////////////////////////////////////////////////

// NOTE: this is some kind of tricky stuff .. basically we use this
// *just to write* things that we know are metadata, and we don't
// want to split arround.
// They are read as notification events !

class EventStreamMetadata : public BulkDataEvent {
 public:
  explicit EventStreamMetadata(const Header& header)
      : BulkDataEvent(header, EVENT_NOTIFY, SUBTYPE_SERVICE_CALL) {
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
   public:                                                              \
    classname(const Header& header, uint32 member)                      \
        : Event(header, evtype, evsubtype), member##_(member) {}        \
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
    virtual AmfUtil::ReadStatus DecodeBody(io::MemoryStream* in,        \
                                           AmfUtil::Version version) {  \
      if ( in->Size() < sizeof(member##_) ) {                           \
        return AmfUtil::READ_NO_DATA;                                   \
      }                                                                 \
      member##_ = static_cast<uint32>(io::NumStreamer::ReadInt32(in,    \
                                      common::BIGENDIAN));              \
      return AmfUtil::READ_OK;                                          \
    }                                                                   \
    virtual void EncodeBody(io::MemoryStream* out,                      \
                                     AmfUtil::Version version) const {  \
      io::NumStreamer::WriteInt32(out,                                  \
                                  static_cast<int32>(member##_),        \
                                  common::BIGENDIAN);                   \
    }                                                                   \
   private:                                                             \
    uint32 member##_;                                                   \
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
  explicit EventClientBW(const Header& header, uint32 bandwidth, uint8 value2)
      : Event(header, EVENT_CLIENT_BANDWIDTH, SUBTYPE_STREAM_CONTROL),
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
  virtual AmfUtil::ReadStatus DecodeBody(io::MemoryStream* in,
      AmfUtil::Version version) {
    if ( in->Size() < sizeof(bandwidth_) + sizeof(value2_) ) {
      return AmfUtil::READ_NO_DATA;
    }
    bandwidth_ = static_cast<uint32>(io::NumStreamer::ReadInt32(in,
        common::BIGENDIAN));
    value2_ = uint8(io::NumStreamer::ReadByte(in));
    return AmfUtil::READ_OK;
  }
  virtual void EncodeBody(io::MemoryStream* out,
      AmfUtil::Version version) const {
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
  explicit EventFlexMessage(const Header& header)
      : Event(header, EVENT_FLEX_MESSAGE, SUBTYPE_SERVICE_CALL),
        unknown_(0) {
  }
  ~EventFlexMessage() {
    Clear();
  }

  // Returns a human readable string
  virtual string ToStringAttr() const {
    string s = "\n";
    for ( uint32 i = 0; i < data_.size(); ++i ) {
      s += strutil::StringPrintf("%5d: %s\n", i, data_[i]->ToString().c_str());
    }
    return s;
  }

  virtual AmfUtil::ReadStatus DecodeBody(io::MemoryStream* in,
      AmfUtil::Version version) {
    if ( in->Size() < 1 ) {
      return AmfUtil::READ_NO_DATA;
    }
    Clear();
    unknown_ = io::NumStreamer::ReadByte(in);
    while ( !in->IsEmpty() ) {
      CObject* obj = NULL;
      AmfUtil::ReadStatus err = Amf0Util::ReadNextObject(in, &obj);
      if ( err != AmfUtil::READ_OK ) {
        return err;
      }
      data_.push_back(obj);
    }
    return AmfUtil::READ_OK;
  }
  virtual void EncodeBody(
      io::MemoryStream* out, AmfUtil::Version version) const {
    io::NumStreamer::WriteByte(out, unknown_);
    for ( uint32 i = 0; i < data_.size(); ++i ) {
      data_[i]->Encode(out, version);
    }
  }

  const vector<CObject*>& data() const { return data_; }
  vector<CObject*>* mutable_data() { return &data_; }

  void Clear() {
    for ( uint32 i = 0; i < data_.size(); ++i ) {
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
  explicit EventFlexSharedObject(const Header& header)
      : BulkDataEvent(header, EVENT_FLEX_SHARED_OBJECT, SUBTYPE_SYSTEM) {
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(EventFlexSharedObject);
};

class EventSharedObject : public BulkDataEvent {
 public:
  explicit EventSharedObject(const Header& header)
      : BulkDataEvent(header, EVENT_SHARED_OBJECT, SUBTYPE_SYSTEM) {
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(EventSharedObject);
};

//////////////////////////////////////////////////////////////////////
}

#endif  // __NET_RTMP_EVENTS_RTMP_EVENT_H__
