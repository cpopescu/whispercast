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
#include "rtmp/rtmp_protocol_data.h"

// Check http://osflash.org/_media/rtmp_spec.jpg
// for protocol format details

// The logic is quite complicated here .. follow it closely and *read* the
// document above ..

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

Coder::Coder(rtmp::ProtocolData* protocol_data,
             int64 memory_limit)
  : memory_limit_(memory_limit),
    protocol_data_(protocol_data),
    last_header_(NULL),
    memory_used_(0) {
}
Coder::~Coder() {
  delete last_header_;
  last_header_ = NULL;
}
AmfUtil::ReadStatus Coder::Decode(io::MemoryStream* in,
                                  AmfUtil::Version version,
                                  rtmp::Event** event) {
  CHECK_EQ(version, AmfUtil::AMF0_VERSION);
  AmfUtil::ReadStatus err;
  *event = NULL;
  do {
    // last_header_ was a header that was probably fully decoded in the last
    // call to Decode, but its *chunk* body was not completed..
    bool reuse_header = false;
    if ( last_header_ == NULL ) {
      last_header_ = new rtmp::Header(protocol_data_);
    } else {
      if ( in->Size() < sizeof(uint8) ) {
        return AmfUtil::READ_NO_DATA;
      }
      // Try to save some time when we have a header continuation
      in->MarkerSet();
      const uint8 header_byte = io::NumStreamer::ReadByte(in);
      const uint8 expected = uint8(((HEADER_CONTINUE << 6) +
                                    last_header_->channel_id()) & 0xff);
      if ( header_byte == expected &&
           last_header_->channel_id() <= 0x3f ) {
        reuse_header = true;
        if ( last_header_->timestamp_ms() >= 0xffffff ) {
          in->Skip(sizeof(uint32));
        }
        const rtmp::Header* const old_header =
            protocol_data_->get_last_read_header(last_header_->channel_id());
        if ( old_header != NULL && old_header != last_header_ ) {
          last_header_->CopyParams(*old_header);
        }
      } else {
        const rtmp::Header* const old_header =
            protocol_data_->get_last_read_header(last_header_->channel_id());
        if ( old_header != last_header_ ) {
          delete last_header_;
        }
        last_header_ = new rtmp::Header(protocol_data_);
        in->MarkerRestore();
      }
    }
    if ( !reuse_header ) {
      in->MarkerSet();
      DLOG_INFO_IF(FLAGS_rtmp_debug_dump_buffers > 0)
        << in->Size()
        << " > ======================== HEADER ================= \n"
        << in->DumpContent(FLAGS_rtmp_debug_dump_buffers);
      AmfUtil::ReadStatus err = last_header_->ReadFromMemoryStream(
        in, version);
      DLOG_INFO_IF(FLAGS_rtmp_debug_dump_buffers > 0)
        << in->Size()
        << " > ============== RETURNED: "
        << AmfUtil::ReadStatusName(err)
        << " ==> " << *last_header_;
      if ( err != AmfUtil::READ_OK ) {
        in->MarkerRestore();
        return err;
      }
      if ( !IsValidEventType(last_header_->event_type()) ) {
        LOG_ERROR << " Invalid header type found: "
                  << *last_header_
                  << " --> " << last_header_->event_type();
        in->MarkerRestore();
        return AmfUtil::READ_CORRUPTED_DATA;
      }
    }
    // Did we receive before some event body for this channel ?
    io::MemoryStream* event_body = protocol_data_->get_partial_body(
      last_header_->channel_id());
    if ( event_body == NULL ) {
      if ( last_header_->channel_id() >=
           rtmp::ProtocolData::kMaxNumChannels ) {
        LOG_ERROR << "Invalid channel received: "
                  << last_header_->channel_id();
        return AmfUtil::READ_TOO_MANY_CHANNELS;
      }
      // TODO(cpopescu): -- too many channels error !!
      event_body = new io::MemoryStream(max(min(last_header_->event_size(),
          static_cast<uint32>(io::DataBlock::kDefaultBufferSize)), 32U));
      protocol_data_->set_partial_body(last_header_->channel_id(), event_body);
    }
    const int32 initial_event_body_size = event_body->Size();
    const int32 to_read = min(
        static_cast<int32>(protocol_data_->read_chunk_size()),
        static_cast<int32>(last_header_->event_size() -
                           initial_event_body_size));
    if ( to_read < 0 ) {
      LOG_ERROR << " Invalid  header to read data: "
                << " chunk size: " << protocol_data_->read_chunk_size()
                << " last_header: " << last_header_->ToString()
                << " initial_event_body_size: " << initial_event_body_size
                << " reuse_header: " << reuse_header;
      in->MarkerRestore();
      return AmfUtil::READ_CORRUPTED_DATA;
    }
    if ( in->Size() < to_read ) {
      in->MarkerRestore();
      return AmfUtil::READ_NO_DATA;
    }
    event_body->AppendStreamNonDestructive(in, to_read);
    const int64 bufsize = event_body->Size();
    memory_used_ += (bufsize - initial_event_body_size);
    if ( memory_used_ > memory_limit_ ) {
      in->MarkerRestore();
      LOG_WARNING << "rtmp::Coder encountered an OOM on header: "
               << *last_header_ << " Reached: " << memory_used_;
      return AmfUtil::READ_OOM;
    }

    in->Skip(to_read);
    // Right now we definitely passed over this chunk - update last_header_
    // and stuff, and advance the marker
    in->MarkerClear();
    rtmp::Header* const old_header =
      protocol_data_->get_last_read_header(last_header_->channel_id());
    if ( old_header != NULL && old_header != last_header_ ) {
      delete old_header;
      protocol_data_->clear_last_read_header(last_header_->channel_id());
    }
    if ( bufsize == last_header_->event_size() ) {
      DCHECK(protocol_data_->get_partial_body(last_header_->channel_id()) ==
             event_body);
      protocol_data_->clear_partial_body(last_header_->channel_id());
      // Now we can compose an event !
      *event = CreateEvent(last_header_);
      // Put a copy of last_header in the protocol state in case it is needed
      protocol_data_->set_last_read_header(last_header_->channel_id(),
                                           new rtmp::Header(protocol_data_,
                                                            *last_header_));
      last_header_ = NULL;   // no need to delete - captured in CreateEvent
      DLOG_INFO_IF(FLAGS_rtmp_debug_dump_buffers > 0)
        <<  "======================== BODY ================= "
        << event_body->DumpContent(FLAGS_rtmp_debug_dump_buffers);

      err = (*event)->ReadFromMemoryStream(event_body, version);
      DLOG_INFO_IF(FLAGS_rtmp_debug_dump_buffers > 0)
        << " ============== RETURNED: "
        << AmfUtil::ReadStatusName(err);
      if ( !event_body->IsEmpty() ) {
        LOG_WARNING << "Events bytes left by decoder: " << event_body->Size()
                    << " for event: " << **event;
      }
      memory_used_ -= bufsize;
      delete event_body;
      return err;
    } else if ( old_header != last_header_ ) {
      DCHECK_LT(bufsize, last_header_->event_size());

      // we still need to read some more chunks - but save the last_header_
      protocol_data_->set_last_read_header(last_header_->channel_id(),
                                           last_header_);
    }
  } while (true);   // next chunk !
}

void MoveData(io::MemoryStream* out,
              io::DataBlockPointer* reader,
              int size) {
  // TODO(cosmin): Why do we allocate a new DataBlock?
  //               Instead of using the simple Write() which copies data?
  //               size is usually small: 1 byte.
  char* buffer = new char[size];
  const int32 len = reader->ReadData(buffer, size);
  out->AppendRaw(buffer, len);
}

int Coder::EncodeWithAuxBuffer(io::MemoryStream* out,
                               AmfUtil::Version version,
                               rtmp::Event* event,
                               const io::MemoryStream* event_stream) const {
  CHECK_EQ(version, AmfUtil::AMF0_VERSION);

  // Then prepare chunks to be written to out ..
  io::MemoryStream chunk_stream(protocol_data_->write_chunk_size() + 12);

  // Update the size of the stream header ..
  event->mutable_header()->set_event_size(event_stream->Size());
  CHECK_EQ(event->event_type(), event->header()->event_type());
#ifdef _DEBUG
  if ( FLAGS_rtmp_debug_dump_sent_events ) {
    if ( FLAGS_rtmp_debug_dump_only_control ) {
      if ( event->event_type() != EVENT_MEDIA_DATA &&
           event->event_type() != EVENT_AUDIO_DATA &&
           event->event_type() != EVENT_VIDEO_DATA ) {
        DLOG_INFO
          << " RTMP send event: " << *event->header() << " - " << *event;
      }
    } else {
        DLOG_INFO
          << " RTMP send event: " << *event->header() << " - " << *event;
    }
    DLOG(10) << " Encoding an event : \n" << *event
             << "\nHeader \n" << *event->header()
             << "\nSize: " << event_stream->Size();
  }
#endif
  // Logic:
  //  - write the header at the beginning of the chunk
  //  - fill the chunk w/ event_stream bytes.
  //  - append the chunk to out
  //  - while there are some bytes to be sent in event_stream..
  int num_chunks = 0;
  const rtmp::Header* previous =
    protocol_data_->get_last_write_header(event->header()->channel_id());

  io::DataBlockPointer es_reader = event_stream->GetReadPointer();
  int32 es_size = event_stream->Size();

  int32 leftover = 0;
  CHECK_GT(protocol_data_->write_chunk_size(), 0);
  do {
    event->header()->WriteToMemoryStream(out, version,
                                         num_chunks > 0);
    if ( previous ) {
      delete previous;
      previous = NULL;
    }
    if ( num_chunks == 0 ) {
      protocol_data_->set_last_write_header(
          event->header()->channel_id(),
          new rtmp::Header(protocol_data_, *event->header()));
    }
    leftover = min(es_size,
                   protocol_data_->write_chunk_size());

    if (leftover > 0) {
      MoveData(out, &es_reader, leftover);

      es_size -= leftover;
      num_chunks++;
    }

    /*
    char* buffer = new char[leftover];
    const int32 len = es_reader.ReadData(buffer, leftover);
    out->AppendRaw(buffer, len);
    */
    /*
      Alternate:

      io::DataBlockPointer es_end(es_reader);
      es_end.Advance(leftover);
      out->AppendStreamNonDestructive(&es_reader, &es_end);
      es_reader = es_end;
    */
  } while ( es_size > 0 && leftover > 0 );

  // protocol_data_->clear_last_write_header(event->header()->channel_id());
  return num_chunks;
}

int Coder::Encode(io::MemoryStream* out,
                  AmfUtil::Version version,
                  rtmp::Event* event) const {
  CHECK_EQ(version, AmfUtil::AMF0_VERSION);
  // First encode the event in one buffer ..
  io::MemoryStream event_stream;
  event->WriteToMemoryStream(&event_stream, version);
  return EncodeWithAuxBuffer(out, version, event, &event_stream);
}


rtmp::Event* Coder::CreateEvent(rtmp::Header* header) {
  switch ( header->event_type() ) {
  case EVENT_CHUNK_SIZE:
    return new rtmp::EventChunkSize(header);
  case EVENT_INVOKE:
    return new rtmp::EventInvoke(header);
  case EVENT_NOTIFY:
    return new rtmp::EventNotify(header);
  case EVENT_PING:
    return new rtmp::EventPing(header);
  case EVENT_BYTES_READ:
    return new rtmp::EventBytesRead(header);
  case EVENT_AUDIO_DATA:
    return new rtmp::EventAudioData(header);
  case EVENT_VIDEO_DATA:
    return new rtmp::EventVideoData(header);
  case EVENT_FLEX_SHARED_OBJECT:
    return new rtmp::EventFlexSharedObject(header);
  case EVENT_SHARED_OBJECT:
    return new rtmp::EventSharedObject(header);
  case EVENT_SERVER_BANDWIDTH:
    return new rtmp::EventServerBW(header);
  case EVENT_CLIENT_BANDWIDTH:
    return new rtmp::EventClientBW(header);
  case EVENT_FLEX_MESSAGE:
    return new rtmp::EventFlexMessage(header);
  case EVENT_MEDIA_DATA:
    return new rtmp::MediaDataEvent(header);
  case EVENT_INVALID:
    LOG_ERROR << " Invalid event found: " <<  header->event_type();
    return new rtmp::EventUnknown(header);
  }
  LOG_ERROR << " Unknown event found: " <<  header->event_type();
  return new rtmp::EventUnknown(header);
}
}
