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

#ifndef __NET_RTMP_RTMP_PROTOCOL_DATA_H__
#define __NET_RTMP_RTMP_PROTOCOL_DATA_H__

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperstreamlib/rtmp/rtmp_consts.h>
#include <whisperstreamlib/rtmp/events/rtmp_header.h>

namespace rtmp {

class ProtocolData {
 public:
  ProtocolData()
    : amf_version_(AmfUtil::AMF0_VERSION),
      read_chunk_size_(kDefaultReadChunkSize),
      write_chunk_size_(kDefaultWriteChunkSize) {
    bzero(last_read_headers_, sizeof(last_read_headers_));
    bzero(last_write_headers_, sizeof(last_write_headers_));
    bzero(partial_bodies_, sizeof(partial_bodies_));
  }
  ~ProtocolData() {
    for ( int i = 0; i < kMaxNumChannels; ++i ) {
      delete last_read_headers_[i];
      delete last_write_headers_[i];
      delete partial_bodies_[i];
    }
  }

  //////////

  AmfUtil::Version amf_version() const { return amf_version_; }

  rtmp::Header* get_last_read_header(uint32 channel_id) const {
    if ( channel_id >= kMaxNumChannels ) return NULL;
    return last_read_headers_[channel_id];
  }
  // This function takes control of the rtmp::Header* header object until you
  // clear it ..
  void set_last_read_header(uint32 channel_id, rtmp::Header* header) {
    DCHECK(header != NULL);
    DCHECK_EQ(channel_id, header->channel_id());
    if ( channel_id < kMaxNumChannels ) {
      last_read_headers_[channel_id] = header;
    }
  }
  void clear_last_read_header(uint32 channel_id) {
    if ( channel_id < kMaxNumChannels ) {
      last_read_headers_[channel_id] = NULL;
    }
  }

  //////////

  io::MemoryStream* get_partial_body(uint32 channel_id) const {
    if ( channel_id >= kMaxNumChannels ) return NULL;
    return partial_bodies_[channel_id];
  }
  // This function takes control of the MemoryStream* object until you
  // clear it ..
  void set_partial_body(uint32 channel_id, io::MemoryStream* in) {
    DCHECK(in != NULL);
    if ( channel_id < kMaxNumChannels ) {
      partial_bodies_[channel_id] = in;
    }
  }
  void clear_partial_body(uint32 channel_id) {
    if ( channel_id < kMaxNumChannels ) {
      partial_bodies_[channel_id] = NULL;
    }
  }

  //////////

  const rtmp::Header* get_last_write_header(uint32 channel_id) {
    if ( channel_id >= kMaxNumChannels ) return NULL;
    return last_write_headers_[channel_id];
  }
  // This function does not take control of the rtmp::Header object
  void set_last_write_header(uint32 channel_id, const rtmp::Header* header) {
    DCHECK(header != NULL);
    if ( channel_id < kMaxNumChannels ) {
      last_write_headers_[channel_id] = header;
    }
  }
  void clear_last_write_header(uint32 channel_id) {
    if ( channel_id < kMaxNumChannels ) {
      last_write_headers_[channel_id] = NULL;
    }
  }

  //////////

  int32 read_chunk_size() const {
    return read_chunk_size_;
  }
  int32 write_chunk_size() const {
    return write_chunk_size_;
  }
  void set_read_chunk_size(int32 read_chunk_size) {
    read_chunk_size_ = read_chunk_size;
  }
  void set_write_chunk_size(int32 write_chunk_size) {
    write_chunk_size_ = write_chunk_size;
  }

  static const int kMaxNumChannels = 32;

 private:
  // We read and write w/ this AMF version (defaulted to AMF0)
  const AmfUtil::Version amf_version_;

  rtmp::Header* last_read_headers_[kMaxNumChannels];
  const rtmp::Header* last_write_headers_[kMaxNumChannels];
  io::MemoryStream* partial_bodies_[kMaxNumChannels];

  // the data that comes from the client, comes in chunks of this size:
  int32 read_chunk_size_;
  // the data that goes to the  client, goes in chunks of this size:
  int32 write_chunk_size_;

  DISALLOW_EVIL_CONSTRUCTORS(ProtocolData);
};
}

#endif
