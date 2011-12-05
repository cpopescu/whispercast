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

#include "net/rpc/lib/codec/rpc_encoder.h"

namespace rpc {

void rpc::Encoder::EncodePacket(const rpc::Message& p) {
  // encode body first in temporary buffer tmp_, to evaluate it's size
  tmp_.Clear();

  // switch output
  io::MemoryStream* originalOut = out_;
  out_ = &tmp_;

  // encode body
  if ( p.header_.msgType_ == RPC_CALL ) {
    const rpc::Message::CallBody& call = p.cbody_;
    Encode(call.service_);
    Encode(call.method_);
    tmp_.AppendStreamNonDestructive(&call.params_);
  } else if ( p.header_.msgType_ == RPC_REPLY ) {
    const rpc::Message::ReplyBody& reply = p.rbody_;
    Encode(reply.replyStatus_);
    tmp_.AppendStreamNonDestructive(&reply.result_);
  } else {
    LOG_FATAL << "Bad msgType: " << p.header_.msgType_;
  }

  // switch back to original output
  out_ = originalOut;

  const uint32 bodySize = tmp_.Size();

  // encode the header
  const rpc::Message::Header& header = p.header_;
  out_->Write(rpc::kMessageMark, sizeof(rpc::kMessageMark));
  Encode(header.xid_);
  Encode(header.msgType_);
  const int32 rpcBodySize(bodySize);
  Encode(rpcBodySize);

  // write the body (direct copy from the temporary buffer)
  //
  out_->AppendStream(&tmp_);
}
}
