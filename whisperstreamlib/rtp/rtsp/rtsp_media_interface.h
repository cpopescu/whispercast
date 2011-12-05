
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

#ifndef __MEDIA_RTP_RTSP_MEDIA_INTERFACE_H__
#define __MEDIA_RTP_RTSP_MEDIA_INTERFACE_H__

#include <whisperlib/common/base/types.h>
#include <whisperstreamlib/base/media_info.h>
#include <whisperstreamlib/rtp/rtp_broadcaster.h>
#include <whisperstreamlib/rtp/rtp_sdp.h>
#include <whisperstreamlib/rtp/rtsp/rtsp_consts.h>
#include <whisperstreamlib/rtp/rtsp/types/rtsp_request.h>

namespace streaming {
namespace rtsp {

// This interface connects Broadcasters to actual media sources.
// There are 2 implementations:
// 1. ElementMapperMediaInterface: connects Broadcaster through MediaMapper
//    to a whispercast element.
// 2. FileMediaInterface: connects Broadcaster to a simple file reader.
//    Used in non-whispercast application (such as rtsp_simple_server).
class MediaInterface {
public:
  MediaInterface() {}
  virtual ~MediaInterface() {}

  // Asynchronously returns media description.
  // Returns: false -> there was an error, the callback won't be called
  //          true -> the callback will be called to indicate a 'MediaInfo'.
  typedef Callback1<const MediaInfo*> MediaInfoCallback;
  virtual bool Describe(const string& media, MediaInfoCallback* callback) = 0;

  // Links the broadcaster to the specified media. The stream is initially
  // paused. After Play(true) the media source will start sending tags to
  // the broadcaster at stream rate.
  virtual bool Attach(rtp::Broadcaster* broadcaster, const string& media) = 0;
  virtual void Detach(rtp::Broadcaster* broadcaster) = 0;

  // Play/Pause media to the given broadcaster.
  virtual void Play(rtp::Broadcaster* broadcaster, bool play) = 0;
};

} // namespace rtsp
} // namespace streaming

#endif // __MEDIA_RTP_RTSP_MEDIA_INTERFACE_H__
