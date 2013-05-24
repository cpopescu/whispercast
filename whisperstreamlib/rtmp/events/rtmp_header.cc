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

#include <whisperlib/common/io/num_streaming.h>
#include <whisperlib/common/base/strutil.h>
#include "rtmp/objects/amf/amf_util.h"
#include "rtmp/objects/amf/amf0_util.h"
#include "rtmp/events/rtmp_header.h"
#include "rtmp/rtmp_consts.h"
#include "rtmp/rtmp_coder.h"

namespace rtmp {

const char* HeaderTypeName(HeaderType ht) {
  switch ( ht ) {
    CONSIDER(HEADER_NEW);
    CONSIDER(HEADER_SAME_SOURCE);
    CONSIDER(HEADER_TIMER_CHANGE);
    CONSIDER(HEADER_CONTINUE);
  }
  LOG_FATAL << "Illegal HeaderType: " << ht;
  return "HEADER_UNKOWN";
}
int32 HeaderLength(HeaderType ht) {
  switch ( ht ) {
    case HEADER_NEW: return 11;
    case HEADER_SAME_SOURCE: return 7;
    case HEADER_TIMER_CHANGE: return 3;
    case HEADER_CONTINUE: return 0;
  }
  LOG_FATAL << "Illegal HeaderType: " << ht;
  return -1;
}

Header::Header()
    : channel_id_(kInvalidChannelId),
      stream_id_(kInvalidStreamId),
      event_type_(EVENT_SHARED_OBJECT),
      timestamp_ms_(0),
      is_timestamp_relative_(true) {
}
Header::Header(uint32 channel_id,
               uint32 stream_id,
               EventType event_type,
               uint32 timestamp_ms,
               bool is_timestamp_relative)
    : channel_id_(channel_id),
      stream_id_(stream_id),
      event_type_(event_type),
      timestamp_ms_(timestamp_ms),
      is_timestamp_relative_(is_timestamp_relative) {
}

Header::~Header() {
}

AmfUtil::ReadStatus Header::Decode(io::MemoryStream* in,
                                   AmfUtil::Version version,
                                   const Coder* coder,
                                   uint32* out_event_size) {
  CHECK_EQ(version, AmfUtil::AMF0_VERSION);
  uint8 header_byte = 0;
  // we need to skip some 0 padding that seem to appear ..
  is_timestamp_relative_ = false;
  do {
    if ( in->Size() < sizeof(uint8) ) {
      return AmfUtil::READ_NO_DATA;
    }
    header_byte = io::NumStreamer::ReadByte(in);
  } while ( header_byte == 0 );
  if ( in->Size() < sizeof(uint16) ) {
    return AmfUtil::READ_NO_DATA;
  }
  const HeaderType header_type = HeaderType(header_byte >> 6);
  const int32 header_length = HeaderLength(header_type);
  if ( (header_byte & 0x3f) == 0 ) {
    // Encoded on two bytes (between 64 and 319)
    channel_id_ = 0x40 + io::NumStreamer::ReadByte(in);
  } else if ( (header_byte & 0x3f) == 1 ) {
    // Encoded on three bytes (between 320 and 320 + 255) - Bigendian reading..
    channel_id_ = (0x40 + io::NumStreamer::ReadByte(in) +
                   (io::NumStreamer::ReadByte(in) << 8));
  } else {
    channel_id_ = header_byte & 0x3f;
  }
  if ( channel_id_ >= Coder::kMaxNumChannels ) {
    LOG_ERROR << "Invalid channel received: " << channel_id_;
    return AmfUtil::READ_TOO_MANY_CHANNELS;
  }
  if ( in->Size() < header_length + sizeof(uint32) ) {
    return AmfUtil::READ_NO_DATA;
  }
  uint32 last_event_size = 0;
  const Header& last_header = coder->last_read_event_header(
      channel_id_, &last_event_size);
  if ( header_type != HEADER_NEW &&
       last_header.channel_id() == kInvalidChannelId ) {
    LOG_ERROR << "Unknown last header for channel: " << channel_id_
              << ", header byte: "
              << strutil::StringPrintf("0x%02x", header_byte);
    return AmfUtil::READ_CORRUPTED_DATA;
  }
  switch ( header_type ) {
  case HEADER_NEW:
    timestamp_ms_ = io::NumStreamer::ReadUInt24(in, common::BIGENDIAN);
    *out_event_size = io::NumStreamer::ReadUInt24(in, common::BIGENDIAN);
    event_type_ = EventType(io::NumStreamer::ReadByte(in));
    if ( !IsValidEventType(event_type_) ) {
      LOG_ERROR << "Invalid event type: " << event_type_;
      return AmfUtil::READ_CORRUPTED_DATA;
    }
    // Strange enough, this is Little Endian
    stream_id_ = static_cast<uint32>(
        io::NumStreamer::ReadInt32(in, common::LILENDIAN));
    if ( timestamp_ms_ >= 0xffffff ) {
      timestamp_ms_ = io::NumStreamer::ReadUInt32(in, common::BIGENDIAN);
    }
    break;
  case HEADER_SAME_SOURCE:
    is_timestamp_relative_ = true;
    timestamp_ms_ = io::NumStreamer::ReadUInt24(in, common::BIGENDIAN);
    *out_event_size = io::NumStreamer::ReadUInt24(in, common::BIGENDIAN);
    event_type_ = EventType(io::NumStreamer::ReadByte(in));
    if ( !IsValidEventType(event_type_) ) {
      LOG_ERROR << "Invalid event type: " << event_type_;
      return AmfUtil::READ_CORRUPTED_DATA;
    }
    stream_id_ = last_header.stream_id();
    if ( timestamp_ms_ >= 0xffffff ) {
      timestamp_ms_ = io::NumStreamer::ReadUInt32(in, common::BIGENDIAN);
    }
    break;
  case HEADER_TIMER_CHANGE:
    is_timestamp_relative_ = true;
    timestamp_ms_ = io::NumStreamer::ReadUInt24(in, common::BIGENDIAN);
    *out_event_size = last_event_size;
    event_type_ = last_header.event_type();
    stream_id_ = last_header.stream_id();
    if ( timestamp_ms_ >= 0xffffff ) {
      timestamp_ms_ = io::NumStreamer::ReadUInt32(in, common::BIGENDIAN);
    }
    break;
  case HEADER_CONTINUE:
    is_timestamp_relative_ = true;
    timestamp_ms_ = last_header.timestamp_ms();
    *out_event_size = last_event_size;
    event_type_ = last_header.event_type();
    stream_id_ = last_header.stream_id();
    if ( timestamp_ms_ >= 0xffffff ) {
      timestamp_ms_ = io::NumStreamer::ReadUInt32(in, common::BIGENDIAN);
    }
    break;
  }
  return AmfUtil::READ_OK;
}

void Header::Encode(AmfUtil::Version version,
                    const Coder* coder,
                    uint32 event_size,
                    bool force_continue,
                    io::MemoryStream* out) const {
  CHECK_EQ(version, AmfUtil::AMF0_VERSION);
  CHECK_LT(channel_id_, Coder::kMaxNumChannels);
  HeaderType header_type = force_continue ? HEADER_CONTINUE : HEADER_NEW;
  uint32 last_event_size = 0;
  const Header& last = coder->last_write_event_header(
      channel_id_, &last_event_size);
  // Start checking how much is it similar w/ last written event..
  // NOTE: Absolute timestamp headers are always HEADER_NEW
  if ( !force_continue && is_timestamp_relative_ &&
       stream_id_ == last.stream_id() ) {
    // Same stream id ..
    header_type = HEADER_SAME_SOURCE;
    if ( event_size == last_event_size &&
         event_type_ == last.event_type() ) {
      // .. and same event..
      header_type = HEADER_TIMER_CHANGE;
      if ( timestamp_ms_ == last.timestamp_ms() ) {
        // .. they fully match ..
        header_type = HEADER_CONTINUE;
      }
    }
  }

  uint32 ts32 = timestamp_ms_; // 32 bit timestamp
  if ( header_type != HEADER_NEW && !is_timestamp_relative_ ) {
    CHECK(last.channel_id() != kInvalidChannelId) << "last: " << last;
    ts32 = timestamp_ms_ > last.timestamp_ms_ ?
           timestamp_ms_ - last.timestamp_ms_ : 0;
  }
  bool is_extended_ts = (ts32 >= 0xffffff);
  uint32 ts24 = is_extended_ts ? 0xffffff : ts32; // 24 bit timestamp
  if ( header_type == HEADER_CONTINUE ) {
    // NOTE: on HEADER_CONTINUE decide by last event timestamp,
    //       whether extended timestamp is needed
    is_extended_ts = (last.timestamp_ms() >= 0xffffff);
  }

  // Write the header..
  if ( channel_id_ <= 0x3f ) {
    io::NumStreamer::WriteByte(out, uint8((header_type << 6) +
                                               channel_id_));
  } else if ( channel_id_ <= (0x40 + 0xff) ) {
    io::NumStreamer::WriteByte(out, uint8((header_type << 6)));
    io::NumStreamer::WriteByte(out, uint8(channel_id_ - 0x40));
  } else {
    io::NumStreamer::WriteByte(out, uint8((header_type << 6)));
    io::NumStreamer::WriteByte(out, uint8((channel_id_ - 0x40) & 0xff));
    io::NumStreamer::WriteByte(out, uint8((channel_id_ - 0x40) >> 8));
  }
  switch (header_type) {
    case HEADER_NEW:
      io::NumStreamer::WriteUInt24(out, ts24, common::BIGENDIAN);
      io::NumStreamer::WriteUInt24(out, event_size, common::BIGENDIAN);
      io::NumStreamer::WriteByte(out, event_type_);
      // Check out this crap - the only LITTLE endian from the whole story
      io::NumStreamer::WriteInt32(out, stream_id_, common::LILENDIAN);
      if ( is_extended_ts ) {
        io::NumStreamer::WriteUInt32(out, ts32, common::BIGENDIAN);
      }
      break;
    case HEADER_SAME_SOURCE:
      io::NumStreamer::WriteUInt24(out, ts24, common::BIGENDIAN);
      io::NumStreamer::WriteUInt24(out, event_size, common::BIGENDIAN);
      io::NumStreamer::WriteByte(out, event_type_);
      if ( is_extended_ts ) {
        io::NumStreamer::WriteUInt32(out, ts32, common::BIGENDIAN);
      }
      break;
    case HEADER_TIMER_CHANGE:
      io::NumStreamer::WriteUInt24(out, ts24, common::BIGENDIAN);
      if ( is_extended_ts ) {
        io::NumStreamer::WriteUInt32(out, ts32, common::BIGENDIAN);
      }
      break;
    case HEADER_CONTINUE:
      if ( is_extended_ts ) {
        io::NumStreamer::WriteUInt32(out, ts32, common::BIGENDIAN);
      }
      break;
  }
}

string Header::ToString() const {
  return strutil::StringPrintf("%s@%8d rtmp::Header[S-%d : C-%d]{%s}",
                               is_timestamp_relative_ ? "REL" : "ABS",
                               timestamp_ms_, stream_id_, channel_id_,
                               EventTypeName(event_type_));
}
}
