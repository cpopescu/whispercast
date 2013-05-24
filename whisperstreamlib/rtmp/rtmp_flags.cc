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

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/gflags.h>
#include "rtmp/rtmp_flags.h"

// All sort of flags for configuring the rtmp protocol

DEFINE_int32(rtmp_connection_max_outbuf_size,
             2*65536,
             "We close the connection over this limit of the sending buffer");
DEFINE_int32(rtmp_connection_min_outbuf_size_to_send,
             8192,
             "We do not send to an RTMP connection under this size, "
             "unless forced");
DEFINE_int32(rtmp_connection_send_buffer_size,
             -1,
             "The internal TCP write buffer size for the RTMP connections, "
             "-1 is the OS default");
DEFINE_int32(rtmp_connection_write_timeout_ms,
             20000,
             "Write timeout for RTMP server connections");
DEFINE_int32(rtmp_max_write_ahead_ms,
             5000,
             "We keep the RTMP stream "
             "at most this much ahead of the real time");
DEFINE_int32(rtmp_default_write_ahead_ms,
             1000,
             "We keep the RTMP stream this much ahead of the real time "
             "by default");
DEFINE_int32(rtmp_connection_pause_timeout_ms,
             30000,
             "After how long we close a connection in pause / no activity ?");
DEFINE_int32(rtmp_connection_log_level,
             2,
             "The log level of messages for RTMP connections");
DEFINE_int32(rtmp_max_num_connections,
             1000,
             "Accept at most these many concurrent RTMP connections");
DEFINE_int32(rtmp_max_num_connection_streams,
             10,
             "Maximum number of streams per RTMP connection");
DEFINE_int32(rtmp_decoder_mem_limit,
             1 << 17,            // 128 K
             "we limit the size of the objects received from client "
             "to this size");
DEFINE_int32(rtmp_connection_chunk_size,
             4096,
             "Use thhis chunk size when talking with the clients");
DEFINE_int32(rtmp_connection_seek_processing_delay_ms,
             100,
             "We delay the seek processing by this amount of time to "
             "manage seek bursts");
DEFINE_int64(rtmp_connection_media_chunk_size_ms,
             250,
             "When sending reaching this time in accumulated media chunks "
             "we send them");
DEFINE_string(rtmp_ssl_certificate,
              "",
              "Path to ssl certificate file. We need both ssl key & certificate"
              " in order to use SSL on rtmp accepted connections.");
DEFINE_string(rtmp_ssl_key,
              "",
              "Path to ssl key file. We need both ssl key & certificate"
              " in order to use SSL on rtmp accepted connections.");
DEFINE_int64(rtmp_missing_stream_cache_expiration_ms,
             45000,
             "We have a cache for missing stream names, to be able to rapidly"
             " reject them. This is the cache expiration timeout"
             " in milliseconds.");
DEFINE_int64(rtmp_reject_delay_ms,
             7000,
             "Milliseconds delay before sending 'StreamNotFound' to client."
             " Helps with fast reconnecting clients on a missing stream.");

// This thing is needed because FlvPlayback objects from Flash does some nasty
// url breakup and processing.
//
// TODO(cpopescu): change the default value to false
DEFINE_bool(rtmp_fancy_escape,
            true,
            "TODO **** change the default value to false ***"
            "If true, decodes escaped stream names ('='->'/' and '#' -> '.')");


namespace rtmp {
void GetProtocolFlags(ProtocolFlags* f) {
  f->max_num_connections_ =
      FLAGS_rtmp_max_num_connections;
  f->max_num_connection_streams_ =
      FLAGS_rtmp_max_num_connection_streams;
  f->max_outbuf_size_ =
      FLAGS_rtmp_connection_max_outbuf_size;
  f->min_outbuf_size_to_send_ =
      FLAGS_rtmp_connection_min_outbuf_size_to_send;
  f->max_write_ahead_ms_ =
      FLAGS_rtmp_max_write_ahead_ms;
  f->default_write_ahead_ms_ =
      FLAGS_rtmp_default_write_ahead_ms;
  f->pause_timeout_ms_ =
      FLAGS_rtmp_connection_pause_timeout_ms;
  f->log_level_ =
      FLAGS_rtmp_connection_log_level;
  f->decoder_mem_limit_ =
      FLAGS_rtmp_decoder_mem_limit;
  f->chunk_size_ =
      FLAGS_rtmp_connection_chunk_size;
  f->seek_processing_delay_ms_ =
      FLAGS_rtmp_connection_seek_processing_delay_ms;
  f->media_chunk_size_ms_ =
      FLAGS_rtmp_connection_media_chunk_size_ms;
  f->send_buffer_size_ =
      FLAGS_rtmp_connection_send_buffer_size;
  f->write_timeout_ms_ =
      FLAGS_rtmp_connection_write_timeout_ms;
  f->ssl_certificate_ =
      FLAGS_rtmp_ssl_certificate;
  f->ssl_key_ =
      FLAGS_rtmp_ssl_key;
  f->reject_delay_ms_ =
      FLAGS_rtmp_reject_delay_ms;
  f->missing_stream_cache_expiration_ms_ =
      FLAGS_rtmp_missing_stream_cache_expiration_ms;
}
}
