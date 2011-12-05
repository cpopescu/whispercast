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

#ifndef __MEDIA_RTP_RTSP_COMMON_H__
#define __MEDIA_RTP_RTSP_COMMON_H__

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/gflags.h>

// Flags defined in rtsp_common.cc
DECLARE_int32(rtsp_log_level);
DECLARE_string(rtsp_server_name);

#define RTSP_LOG_DEBUG                   \
  if ( FLAGS_rtsp_log_level < LDEBUG ) ; \
  else LOG_DEBUG << "[rtsp] "
#define RTSP_LOG_INFO                           \
  if ( FLAGS_rtsp_log_level < LINFO ) ;         \
  else LOG_INFO << "[rtsp] "
#define RTSP_LOG_WARNING                                                \
  if ( FLAGS_rtsp_log_level < LWARNING ) ;                              \
  else LOG_WARNING << "[rtsp] "
#define RTSP_LOG_ERROR                                                  \
  if ( FLAGS_rtsp_log_level < LERROR ) ;                                \
  else LOG_ERROR << "[rtsp] "

#define RTSP_LOG_FATAL LOG_FATAL << "[rtsp] "


namespace streaming {
namespace rtsp {

enum DecodeStatus {
  DECODE_SUCCESS, // RTSP request or response read successfully
  DECODE_NO_DATA, // not enough data to read request
  DECODE_ERROR,   // bad data in stream. You should abort reading RTSP request.
};
const string& DecodeStatusName(DecodeStatus status);

} // namespace rtsp
} // namespace streaming

#endif // __MEDIA_RTP_RTSP_COMMON_H__
