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

#include "common/base/errno.h"
#include "common/base/log.h"
#include "net/rpc/lib/codec/rpc_decoder.h"
#include "net/rpc/lib/codec/rpc_typename_codec.h"
#include "net/rpc/lib/types/rpc_message.h"

namespace rpc {

DECODE_RESULT Decoder::DecodePacket(Message& out) {
  // Decode  message header
  char mark[sizeof(kMessageMark)] = { 0, };
  if ( in_.Read(mark, sizeof(mark)) < sizeof(mark) ) {
    return DECODE_RESULT_NOT_ENOUGH_DATA;
  }
  if ( memcmp(mark, kMessageMark, sizeof(mark)) ) {
    LOG_ERROR << "RPC bad header marker. Found: "
              << strutil::PrintableDataBuffer((const uint8*)mark, sizeof(mark));
    return DECODE_RESULT_ERROR;
  }

  Message::Header& header = out.header_;
  DECODE_RESULT result = Decode(header.xid_);
  if ( result != DECODE_RESULT_SUCCESS ) {
    return result;     // error on Decode or not enough data
  }

  result = Decode(header.msgType_);
  if ( result != DECODE_RESULT_SUCCESS ) {
    return result;     // error on Decode or not enough data
  }

  int32 body_size = 0;
  result = Decode(body_size);
  if ( result != DECODE_RESULT_SUCCESS ) {
    return result;     // error on Decode or not enough data
  }

  if ( in_.Size() < body_size ) {
    return DECODE_RESULT_NOT_ENOUGH_DATA;
  }

  // Decode RPC message body
  if ( header.msgType_ == RPC_CALL ) {
    const uint32 r1 = in_.Size();       // bytes at body begin
    Message::CallBody& call = out.cbody_;
    result = Decode(call.service_);
    if ( result != DECODE_RESULT_SUCCESS ) {
      return result;
    }
    result = Decode(call.method_);
    if ( result != DECODE_RESULT_SUCCESS ) {
      return result;
    }
    const uint32 r2 = in_.Size();       // bytes at params begin

    DCHECK_LT(r2, r1);
    // r1 - r2 = service + method length
    const uint32 params_size = body_size - (r1 - r2);
    DCHECK_LE(params_size, r2);

    call.params_.Clear();
    call.params_.AppendStream(&in_, params_size);
  } else if ( header.msgType_ == RPC_REPLY ) {
    const uint32 r1 = in_.Size();                   // bytes at body begin
    Message::ReplyBody & reply = out.rbody_;

    result = Decode(reply.replyStatus_);
    if ( result != DECODE_RESULT_SUCCESS ) {
      return result;
    }
    const uint32 r2 = in_.Size();                   // bytes at result begin

    DCHECK_LT(r2, r1);
    // r1 - r2 = replyStatus length
    uint32 result_size = body_size - (r1 - r2);
    DCHECK_LE(result_size, r2);

    reply.result_.Clear();
    reply.result_.AppendStream(&in_, result_size);
  } else {
    LOG_ERROR << "RPC Bad message type: " << header.msgType_;
    return DECODE_RESULT_ERROR;
  }
  return DECODE_RESULT_SUCCESS;
}
}
