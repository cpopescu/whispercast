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
// Author: Catalin Popescu

#ifndef __NET_RTMP_RTMP_FLAGS_H__
#define __NET_RTMP_RTMP_FLAGS_H__

#include <string>
#include <whisperlib/common/base/types.h>

namespace rtmp {

struct ProtocolFlags {
  // maximum number of concurrent connections on a port
  int32 max_num_connections_;
  // maximum number of streams per connection
  int32 max_num_connection_streams_;

  // we close the connection over this limit of the sending buffer
  int32 max_outbuf_size_;
  // we do not send data bellow this threshold, unless forced
  int32 min_outbuf_size_to_send_;

  // we keep the RTMP stream at most this much ahead of the real time
  int32 max_write_ahead_ms_;
  // we keep the RTMP stream this much ahead of the real time by default
  int32 default_write_ahead_ms_;

  // we close connection after this much time in pause (or inactivity)
  int32 pause_timeout_ms_;

  // The internal TCP write buffer size for the RTMP connections
  int32 send_buffer_size_;
  // Write timeout for RTMP server connections
  int32 write_timeout_ms_;

  // we limit the size of the objects received from client to this size
  int32 decoder_mem_limit_;

  // Use thhis chunk size when talking with the clients.
  int32 chunk_size_;

  // We delay the seek processing by this amount of time to manage seek bursts
  int32 seek_processing_delay_ms_;

  // When sending h264 we send Media chunks when they reach this size
  int64 media_chunk_size_ms_;

  // If valid files, we use SSL on accepted connections.
  // Else we use regular TCP on accepted connections.
  string ssl_certificate_;
  string ssl_key_;

  // log level for this protocol
  int32 log_level_;

  // wait these milliseconds before sending 'StreamNotFound'
  int32 reject_delay_ms_;

  // expiration ms for the rtmp::ServerAcceptor::missing_stream_cache_
  int64 missing_stream_cache_expiration_ms_;
};

void GetProtocolFlags(ProtocolFlags* flags);
}

#endif
