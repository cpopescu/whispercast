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

#ifndef __NET_RTMP_EVENTS_RTMP_CONSTS_H__
#define __NET_RTMP_EVENTS_RTMP_CONSTS_H__

#include <whisperlib/common/base/types.h>

namespace rtmp {

// Handshaking stuff
static const int kHandshakeSize = 1536;   // protocol specific
static const uint8 kHandshakeLeadByte = 0x03;
extern const uint8 kServerV1HandshakeBlock[kHandshakeSize];
extern const uint8 kServerV3HandshakeBlock[kHandshakeSize];

//////////////////////////////////////////////////////////////////////
//
// Event Specific stuff
//

static const int kDefaultChunkSize = 128;
static const int kDefaultReadChunkSize = kDefaultChunkSize;
static const int kDefaultWriteChunkSize = kDefaultChunkSize;

enum EventType {
  // Signals ths size of a chunk
  EVENT_CHUNK_SIZE = 0x01,
  // Unknown: 0x02

  // An event Sent every x bytes by both sides during read
  EVENT_BYTES_READ = 0x03,
  // Ping is a stream control message (has subtypes)
  EVENT_PING = 0x04,
  // Server (downstream) bandwidth marker
  EVENT_SERVER_BANDWIDTH = 0x05,
  // Client (upstream) bandwidth marker
  EVENT_CLIENT_BANDWIDTH = 0x06,

  // Unknown: 0x07

  // Marks audio data
  EVENT_AUDIO_DATA = 0x08,
  // Marks video data
  EVENT_VIDEO_DATA = 0x09,
  // Unknowns: 0x0A-0x0F

  // Marks an AMF3 shared object
  EVENT_FLEX_SHARED_OBJECT = 0x10,
  // Marks an AMF3 message
  EVENT_FLEX_MESSAGE = 0x11,
  // Notification is an invocation without response
  EVENT_NOTIFY = 0x12,
  // Stream metadata. Same ID as EVENT_NOTIFY !
  EVENT_STREAM_METAEVENT = 0x12,
  // Shared Object marker
  EVENT_SHARED_OBJECT = 0x13,
  // Invoke operation (remoting call but also used for streaming) marker
  EVENT_INVOKE = 0x14,

  // Unknown - -- most probably used for mixed media H.264 encoded data
  EVENT_MEDIA_DATA = 0x16,

  // Unknown: everything else
  // Not an event type. This constant is used to mark an erroneus EVENT_TYPE
  EVENT_INVALID = 0xFF,
};
inline bool IsValidEventType(EventType ev) {
  return ev <= EVENT_MEDIA_DATA;
}

const char* EventTypeName(EventType ev);

// TODO(cosmin):  document
enum EventSubType {
  SUBTYPE_SYSTEM         = 0x00,
  SUBTYPE_STATUS         = 0x01,
  SUBTYPE_SERVICE_CALL   = 0x02,
  SUBTYPE_SHARED_OBJECT  = 0x03,
  SUBTYPE_STREAM_CONTROL = 0x04,
  SUBTYPE_STREAM_DATA    = 0x05,
  SUBTYPE_CLIENT         = 0x06,
  SUBTYPE_SERVER         = 0x07,
};
const char* EventSubTypeName(EventSubType st);

}

#endif  // __NET_RTMP_EVENTS_RTMP_CONSTS_H__
