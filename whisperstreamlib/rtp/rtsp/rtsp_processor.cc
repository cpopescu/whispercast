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

#include <whisperlib/common/base/date.h>
#include <whisperlib/common/base/scoped_ptr.h>
#include <whisperstreamlib/rtp/rtsp/rtsp_processor.h>
#include <whisperstreamlib/rtp/rtsp/rtsp_common.h>

namespace streaming {
namespace rtsp {

Processor::Processor() {
}
Processor::~Processor() {
}

void Processor::Handle(Connection* conn, const Message* msg) {
  switch ( msg->type() ) {
    case Message::MESSAGE_REQUEST:
      HandleRequest(conn, static_cast<const Request*>(msg));
      return;
    case Message::MESSAGE_RESPONSE:
      HandleResponse(conn, static_cast<const Response*>(msg));
      return;
    case Message::MESSAGE_INTERLEAVED_FRAME:
      HandleInterleavedFrame(conn, static_cast<const InterleavedFrame*>(msg));
      return;
  }
  RTSP_LOG_FATAL << "Illegal Message type: " << msg->type();
}

} // namespace rtsp
} // namespace streaming

