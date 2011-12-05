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

#include <whisperlib/common/io/num_streaming.h>
#include "rtmp/objects/rtmp_objects.h"
#include "rtmp/events/rtmp_event_ping.h"
#include "rtmp/objects/amf/amf_util.h"
#include "rtmp/objects/amf/amf0_util.h"

namespace rtmp {

const char* EventPing::TypeName(Type type_id) {
  switch ( type_id ) {
    CONSIDER(STREAM_CLEAR);
    CONSIDER(STREAM_CLEAR_BUFFER);

    CONSIDER(CLIENT_BUFFER);
    CONSIDER(STREAM_RESET);

    CONSIDER(PING_CLIENT);
    CONSIDER(PONG_SERVER);

    CONSIDER(SWF_VERIFY_REQUEST);
    CONSIDER(SWF_VERIFY_RESPONSE);
  }
  return "UNKNOWN";
}

AmfUtil::ReadStatus EventPing::ReadFromMemoryStream(
  io::MemoryStream* in, AmfUtil::Version version) {
  CHECK_EQ(version, AmfUtil::AMF0_VERSION);

  if ( in->Size() < header()->event_size() )
    return AmfUtil::READ_NO_DATA;

  if ( in->Size() < 6 ) {
    return AmfUtil::READ_NO_DATA;
  }
  const uint16 type = io::NumStreamer::ReadInt16(in, common::BIGENDIAN);
  if ( !IsValidType(type) ) {
    LOG_ERROR << "Invalid rtmp::EventPing::Type: " << type;
  }
  set_ping_type(Type(type));
  set_value2(io::NumStreamer::ReadInt32(in, common::BIGENDIAN));
  if ( in->Size() >= sizeof(value3_ ) ) {
    set_value3(io::NumStreamer::ReadInt32(in, common::BIGENDIAN));
    if ( in->Size() >= sizeof(value4_ ) ) {
      set_value4(io::NumStreamer::ReadInt32(in, common::BIGENDIAN));
      if ( in->Size() >= sizeof(value5_ ) ) {
        set_value5(io::NumStreamer::ReadInt32(in, common::BIGENDIAN));
      }
    }
  }
  return AmfUtil::READ_OK;
}

void EventPing::WriteToMemoryStream(
  io::MemoryStream* out, AmfUtil::Version version) const {
  io::NumStreamer::WriteInt16(out, int16(ping_type_), common::BIGENDIAN);
  io::NumStreamer::WriteInt32(out, value2_, common::BIGENDIAN);
  if ( value3_ != -1 ) {
    io::NumStreamer::WriteInt32(out, value3_, common::BIGENDIAN);
    if ( value4_ != -1 ) {
      io::NumStreamer::WriteInt32(out, value4_, common::BIGENDIAN);
      if ( value5_ != -1 )
        io::NumStreamer::WriteInt32(out, value5_, common::BIGENDIAN);
    }
  }
}
}
