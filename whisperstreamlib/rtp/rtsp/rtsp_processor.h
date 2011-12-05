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

#ifndef __MEDIA_RTP_RTSP_PROCESSOR_H__
#define __MEDIA_RTP_RTSP_PROCESSOR_H__

#include <whisperlib/common/base/types.h>
#include <whisperstreamlib/rtp/rtsp/types/rtsp_request.h>
#include <whisperstreamlib/rtp/rtsp/types/rtsp_response.h>
#include <whisperstreamlib/rtp/rtsp/types/rtsp_interleaved_frame.h>

namespace streaming {
namespace rtsp {

class Connection;

// Generic RTSP request / response handler
class Processor {
public:
  Processor();
  virtual ~Processor();

  // conn: the connection who received the message
  // msg: the message received
  void Handle(Connection* conn, const Message* msg);

  // The underlying connection:
  // - connected successfully
  virtual void HandleConnectionConnected(Connection* conn) = 0;
  // - received a rtsp request
  virtual void HandleRequest(Connection* conn, const Request* req) = 0;
  // - received a rtsp response
  virtual void HandleResponse(Connection* conn, const Response* rsp) = 0;
  // - received a rtsp interleaved frame
  virtual void HandleInterleavedFrame(Connection* conn,
      const InterleavedFrame* frame) = 0;
  // - closed successfully
  virtual void HandleConnectionClose(Connection* conn) = 0;
};

} // namespace rtsp
} // namespace streaming

#endif // __MEDIA_RTP_RTSP_PROCESSOR_H__
