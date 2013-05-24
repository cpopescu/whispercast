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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <whisperlib/common/base/timer.h>

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/gflags.h>
#include "rtmp/objects/rtmp_objects.h"
#include "rtmp/events/rtmp_event.h"
#include "rtmp/events/rtmp_event_invoke.h"
#include "rtmp/events/rtmp_event_notify.h"
#include "rtmp/events/rtmp_event_ping.h"
#include "rtmp/objects/amf/amf0_util.h"
#include "rtmp/objects/amf/amf_util.h"
#include "rtmp/rtmp_coder.h"

// Check http://osflash.org/_media/rtmp_spec.jpg for protocol format details

//////////////////////////////////////////////////////////////////////

DEFINE_int32(rtmp_debug_dump_buffers,
             0,
             "For deep debugging, dumps to log the buffers before "
             "decoding");
DEFINE_bool(rtmp_debug_dump_sent_events,
            false,
            "For debugging, dump all sent events");
DEFINE_bool(rtmp_debug_dump_only_control,
            false,
            "For debugging, dump just the control events (no audio/video)");

//////////////////////////////////////////////////////////////////////

namespace rtmp {

Coder::Coder(int64 memory_limit)
  : memory_limit_(memory_limit),
    read_chunk_size_(kDefaultReadChunkSize),
    write_chunk_size_(kDefaultWriteChunkSize),
    memory_used_(0) {
}
Coder::~Coder() {
}
AmfUtil::ReadStatus Coder::Decode(io::MemoryStream* in,
                                  AmfUtil::Version version,
                                  scoped_ref<rtmp::Event>* out) {
  CHECK_EQ(version, AmfUtil::AMF0_VERSION);
  *out = NULL;
  while ( true ) { // multiple chunks
    Header header;
    uint32 event_size = 0;

    in->MarkerSet();
    AmfUtil::ReadStatus err = header.Decode(in, version, this, &event_size);
    if ( err != AmfUtil::READ_OK ) {
      in->MarkerRestore();
      return err;
    }
    CHECK(IsValidEventType(header.event_type())); // header.Decode() should have checked this

    // persistent event body for this channel, filled with each chunk.
    // When the event body is completely received, the event gets decoded.
    io::MemoryStream& event_body = partial_bodies_[header.channel_id()];
    const int32 initial_event_body_size = event_body.Size();
    const int32 to_read = min(
        static_cast<int32>(read_chunk_size_),
        static_cast<int32>(event_size - initial_event_body_size));
    if ( to_read < 0 ) {
      LOG_ERROR << " Invalid  header to read data: "
                << " chunk size: " << read_chunk_size_
                << " header: " << header.ToString()
                << " initial_event_body_size: " << initial_event_body_size;
      in->MarkerRestore();
      return AmfUtil::READ_CORRUPTED_DATA;
    }
    if ( in->Size() < to_read ) {
      in->MarkerRestore();
      return AmfUtil::READ_NO_DATA;
    }
    event_body.AppendStream(in, to_read);
    memory_used_ += (event_body.Size() - initial_event_body_size);
    if ( memory_used_ > memory_limit_ ) {
      in->MarkerRestore();
      LOG_WARNING << "rtmp::Coder encountered an OOM on header: " << header
                  << " Reached: " << memory_used_;
      return AmfUtil::READ_OOM;
    }

    // Right now we definitely passed over this chunk - advance the marker
    in->MarkerClear();
    if ( initial_event_body_size == 0 ) {
      // first header in a chunk series: update last read event header/size
      last_read_event_header_[header.channel_id()] = header;
      last_read_event_size_[header.channel_id()] = event_size;
    }
    if ( event_body.Size() < event_size ) {
      // we still need to read some more chunks
      continue;
    }

    // Now we can compose an event !
    *out = CreateEvent(header);
    // All the event_body is going to be consumed right away
    memory_used_ -= event_body.Size();
    err = (*out)->DecodeBody(&event_body, version);
    if ( err != AmfUtil::READ_OK ) {
      LOG_ERROR << "Failed to decode event: " << (*out)->ToString()
                << ", err: " << AmfUtil::ReadStatusName(err);
      return err;
    }
    if ( !event_body.IsEmpty() ) {
      LOG_ERROR << "Event bytes left by decoder: " << event_body.Size()
                << " bytes for event: " << (*out)->ToString();
    }
    // Done with this event.
    event_body.Clear();

    // maybe update read_chunk_size_
    if ( (*out)->event_type() == EVENT_CHUNK_SIZE ) {
      int chunk_size = static_cast<EventChunkSize*>(out->get())->chunk_size();
      if ( chunk_size > kMaxChunkSize ) {
        LOG_ERROR << "Refusing to set chunk size: " << chunk_size;
      } else {
        LOG_INFO << "Setting chunk size to: " << chunk_size;
        read_chunk_size_ = chunk_size;
      }
    }

    return err;
  }
}

void Coder::Encode(const rtmp::Event& event,
                   AmfUtil::Version version,
                   io::MemoryStream* out) {
  CHECK_EQ(version, AmfUtil::AMF0_VERSION);
  io::MemoryStream event_stream;
  event.EncodeBody(&event_stream, version);
  EncodeWithAuxBuffer(event, &event_stream, version, out);
}

void MoveData(io::MemoryStream* out, io::DataBlockPointer* reader, int size) {
}
void Coder::EncodeWithAuxBuffer(const rtmp::Event& event,
                                const io::MemoryStream* event_stream,
                                AmfUtil::Version version,
                                io::MemoryStream* out) {
  CHECK_EQ(version, AmfUtil::AMF0_VERSION);

  // maybe update write_chunk_size_
  if ( event.event_type() == EVENT_CHUNK_SIZE ) {
    write_chunk_size_ = static_cast<const EventChunkSize&>(event).chunk_size();
    CHECK_GT(write_chunk_size_, 0);
  }

#ifdef _DEBUG
  if ( FLAGS_rtmp_debug_dump_sent_events ) {
    if ( FLAGS_rtmp_debug_dump_only_control ) {
      LOG_INFO_IF(event.event_type() != EVENT_MEDIA_DATA &&
                  event.event_type() != EVENT_AUDIO_DATA &&
                  event.event_type() != EVENT_VIDEO_DATA)
                 << " RTMP encode event: " << event;
    } else {
        LOG_INFO << " RTMP encode event: " << event;
    }
    VLOG(10) << "Encoding an event : " << event;
  }
#endif

  // NOTE: event_stream is actually tag data, which is shared between multiple
  //       network threads! Simple Read/Write ops are not thread safe and they
  //       corrupt the stream! That's why DataBlockPointer/AppendRaw are used.
  io::DataBlockPointer es_reader = event_stream->GetReadPointer();
  int32 es_size = event_stream->Size();
  uint32 num_chunks = 0;
  // NOTE: If the event_stream is empty, a 0 sized event still needs to be sent
  //       This is why we use do {} while;
  do {
    int chunk_size = min(es_size, write_chunk_size_);
    event.header().Encode(version, this, es_size, num_chunks > 0, out);
    if ( num_chunks == 0 ) {
      last_write_event_header_[event.header().channel_id()] = event.header();
      last_write_event_size_[event.header().channel_id()] = es_size;
    }

    if ( chunk_size > 0 ) {
      // copy data from event_stream to out
      char* buffer = new char[chunk_size];
      const int32 len = es_reader.ReadData(buffer, chunk_size);
      CHECK_EQ(len, chunk_size);
      out->AppendRaw(buffer, len);

      // update remaining event_stream size, and num_chunks
      es_size -= len;
      num_chunks++;
    }
  } while ( es_size > 0 );
}

rtmp::Event* Coder::CreateEvent(const rtmp::Header& header) {
  switch ( header.event_type() ) {
  case EVENT_CHUNK_SIZE:
    return new rtmp::EventChunkSize(header, 0);
  case EVENT_INVOKE:
    return new rtmp::EventInvoke(header, NULL);
  case EVENT_NOTIFY:
    return new rtmp::EventNotify(header);
  case EVENT_PING:
    return new rtmp::EventPing(header, EventPing::STREAM_CLEAR, -1, -1, -1);
  case EVENT_BYTES_READ:
    return new rtmp::EventBytesRead(header, 0);
  case EVENT_AUDIO_DATA:
    return new rtmp::EventAudioData(header);
  case EVENT_VIDEO_DATA:
    return new rtmp::EventVideoData(header);
  case EVENT_FLEX_SHARED_OBJECT:
    return new rtmp::EventFlexSharedObject(header);
  case EVENT_SHARED_OBJECT:
    return new rtmp::EventSharedObject(header);
  case EVENT_SERVER_BANDWIDTH:
    return new rtmp::EventServerBW(header, 0);
  case EVENT_CLIENT_BANDWIDTH:
    return new rtmp::EventClientBW(header, 0, 0);
  case EVENT_FLEX_MESSAGE:
    return new rtmp::EventFlexMessage(header);
  case EVENT_MEDIA_DATA:
    return new rtmp::MediaDataEvent(header);
  }
  LOG_FATAL << "Illegal event type: " << header.event_type();
  return NULL;
}
}
