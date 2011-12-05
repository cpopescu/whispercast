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

#include <whisperstreamlib/base/media_info.h>
#include <whisperstreamlib/rtp/rtp_sdp.h>
#include <whisperstreamlib/f4v/atoms/movie/moov_atom.h>
#include <whisperstreamlib/flv/flv_tag.h>
#include <whisperstreamlib/mp3/mp3_frame.h>

// Defined in rtp_util.cc
DECLARE_int32(rtp_log_level);
#define RTP_LOG_DEBUG if ( FLAGS_rtp_log_level < LDEBUG ) {} \
                      else LOG_DEBUG << "[rtp] "
#define RTP_LOG_INFO if ( FLAGS_rtp_log_level < LINFO ) {}\
                     else LOG_INFO << "[rtp] "
#define RTP_LOG_WARNING if ( FLAGS_rtp_log_level < LWARNING ) {}\
                        else LOG_WARNING << "[rtp] "
#define RTP_LOG_ERROR if ( FLAGS_rtp_log_level < LERROR ) {}\
                      else LOG_ERROR << "[rtp] "
#define RTP_LOG_FATAL LOG_FATAL << "[rtp] "

namespace streaming {
namespace rtp {
namespace util {

// NOTE: SDP extractors gather only codec information, you have to set yourself
// other SDP parameters such as: session name, session info, phone, email, ... .

// Extract SDP codec information from a MediaInfo structure.
// Audio & video ports are filled in 'out' Sdp.
void ExtractSdpFromMediaInfo(uint16 audio_dst_port, uint16 video_dst_port,
    const MediaInfo& info, Sdp* out);

// file -> MediaInfo -> Sdp
bool ExtractSdpFromFile(uint16 audio_dst_port, uint16 video_dst_port,
    const string& filename, Sdp* out);

}
}
}
