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

#ifndef __NET_RTMP_RTMP_CODER_H__
#define __NET_RTMP_RTMP_CODER_H__

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperstreamlib/rtmp/objects/rtmp_objects.h>
#include <whisperstreamlib/rtmp/events/rtmp_event.h>
#include <whisperstreamlib/rtmp/rtmp_consts.h>

// This defines the rtmp::Coder - a class involved in reading and writing
// data (events) of an rtmp conversation.
// We use an rtmp::ProtocolData to hold the current state of a conversation.
// The conversation happens in chunks of a default size
// (from rtmp::ProtocolData::read_chunk_size for reading and
// write_chunk_size for writing).
// Each chunk has some header (rtmp::Header), which can be fully specified in
// the chunk or specified relative to a header from a previous chunk.
// One event can (and should) be encoded on multiple chunks if they are larger
// then a chunk size. Once all chunks forming an event are restored, the full
// event can be decoded (and is decoded).
// Multiple conversations can be multiplexed on the same connection and
// their chunks intercallated. The channel_id members of the headers are used
// to classigy the conversations.
// The chunk sizes can be changed using the ChunkSize event (this affects the
// chunks transmitted by one side of the connection.

namespace rtmp {
class ProtocolData;
class Coder {
 public:
  Coder(int64 memory_limit);
  virtual ~Coder();

  // Reads (and creates) the next event found in the 'in' memory stream.
  // The read pointer in 'in' is left such that the next call to Decode will
  // be meaningful (i.e. don't mess with it in between).
  // If the *event is created (not NULL) we are at the end of all chunks in
  // that event.
  // If READ_OK is returned all things are fine and *event is valid.
  // On READ_NO_DATA - we need more data to decode an object (i.e try later)
  //  -- however the read pointer in 'in' can be advanced from the initial
  //  position (i.e. we ate some data..)
  // If READ_CORRUPTED_DATA, READ_STRUCT_TOO_LONG or READ_OOM is returned
  // you should probably torn down the connection that reached that state.
  // Else (READ_NOT_IMPLEMENTED) - we found an object / event that we did
  // not implement in this version - and probably is invalid (you should
  // silently discard it and LOG the exception, but do not tear down the
  // connection)

  // TODO(cpopescu):  Attention: watch out for the internal memory buildup
  //                  per protocol !!
  AmfUtil::ReadStatus Decode(io::MemoryStream* in,
                             AmfUtil::Version version,
                             scoped_ref<rtmp::Event>* out);

  // Writes an event to the given output stream, splitting it (possibly)
  // into multiple chunks. 'event' is not const because we update
  // event->header()->event_size() in the process of writing.
  void Encode(const rtmp::Event& event,
              AmfUtil::Version version,
              io::MemoryStream* out);

  // Another version of encode, where you have the encoded event (no header)
  // in event stream..
  void EncodeWithAuxBuffer(const rtmp::Event& event,
                           const io::MemoryStream* event_stream,
                           AmfUtil::Version version,
                           io::MemoryStream* out);

  // Factory method for events
  static rtmp::Event* CreateEvent(const rtmp::Header& header);


  const rtmp::Header& last_read_event_header(uint32 channel_id,
      uint32* out_last_read_event_size) const {
    DCHECK(channel_id < kMaxNumChannels);
    *out_last_read_event_size = last_read_event_size_[channel_id];
    return last_read_event_header_[channel_id];
  }
  const rtmp::Header& last_write_event_header(uint32 channel_id,
      uint32* out_last_write_event_size) const {
    DCHECK(channel_id < kMaxNumChannels);
    *out_last_write_event_size = last_write_event_size_[channel_id];
    return last_write_event_header_[channel_id];
  }
  int32 read_chunk_size() const {
    return read_chunk_size_;
  }

  static const uint32 kMaxNumChannels = 32;
  static const uint32 kMaxChunkSize = 65536;
 private:
  // in bytes. The upper limit for simultaneous partial events on all channels.
  const int64 memory_limit_;

  rtmp::Header last_read_event_header_[kMaxNumChannels];
  uint32 last_read_event_size_[kMaxNumChannels];
  rtmp::Header last_write_event_header_[kMaxNumChannels];
  uint32 last_write_event_size_[kMaxNumChannels];
  io::MemoryStream partial_bodies_[kMaxNumChannels];

  // the data that comes from the client, comes in chunks of this size:
  int32 read_chunk_size_;
  // the data that goes to the client, goes in chunks of this size:
  int32 write_chunk_size_;

  // We keep track of the used memory (size of uncompleted events - events
  // that received a bunch of chunks w/ a total memory too big)
  int64 memory_used_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(Coder);
};
}

#endif  // __NET_RTMP_RTMP_CODER_H__
