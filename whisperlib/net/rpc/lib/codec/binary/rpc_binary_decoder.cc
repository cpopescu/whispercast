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

#include <whisperlib/common/base/log.h>
#include <whisperlib/net/rpc/lib/codec/binary/rpc_binary_decoder.h>

namespace rpc {

DECODE_RESULT BinaryDecoder::DecodeItemCount(io::MemoryStream& in) {
  uint32 n = 0;
  const DECODE_RESULT err = Decode(in, &n);
  if ( err != DECODE_RESULT_SUCCESS ) {
    LOG_ERROR << "DecodeItemCount failed, err: " << DecodeResultName(err);
    return err;
  }
  items_stack_.push_back(n);
  return DECODE_RESULT_SUCCESS;
}

DECODE_RESULT BinaryDecoder::DecodeStructStart(io::MemoryStream& in) {
  return DecodeItemCount(in);
}

DECODE_RESULT BinaryDecoder::DecodeStructContinue(io::MemoryStream& in, bool* more_attributes) {
  DCHECK(!items_stack_.empty());
  *more_attributes = (items_stack_.back() > 0);
  if ( !(*more_attributes) ) {
    items_stack_.pop_back();
  }
  return DECODE_RESULT_SUCCESS;
}
DECODE_RESULT BinaryDecoder::DecodeStructAttribStart(io::MemoryStream& in) {
  DCHECK(!items_stack_.empty());
  DCHECK_GT(items_stack_.back(), 0);
  items_stack_.back()--;
  return DECODE_RESULT_SUCCESS;
}
DECODE_RESULT BinaryDecoder::DecodeStructAttribMiddle(io::MemoryStream& in) {
  DCHECK(!items_stack_.empty());
  return DECODE_RESULT_SUCCESS;
}
DECODE_RESULT BinaryDecoder::DecodeStructAttribEnd(io::MemoryStream& in) {
  DCHECK(!items_stack_.empty());
  return DECODE_RESULT_SUCCESS;
}
DECODE_RESULT BinaryDecoder::DecodeArrayStart(io::MemoryStream& in) {
  return DecodeItemCount(in);
}
DECODE_RESULT BinaryDecoder::DecodeArrayContinue(io::MemoryStream& in, bool* more_elements) {
  CHECK(!items_stack_.empty());
  *more_elements = (items_stack_.back() > 0);
  if ( *more_elements ) {
    items_stack_.back()--;
  } else {
    items_stack_.pop_back();
  }
  return DECODE_RESULT_SUCCESS;
}
DECODE_RESULT BinaryDecoder::DecodeMapStart(io::MemoryStream& in) {
  return DecodeItemCount(in);
}
DECODE_RESULT BinaryDecoder::DecodeMapContinue(io::MemoryStream& in, bool* more_pairs) {
  DCHECK(!items_stack_.empty());
  *more_pairs = (items_stack_.back() > 0);
  if ( !(*more_pairs) ) {
    items_stack_.pop_back();
  }
  return DECODE_RESULT_SUCCESS;
}
DECODE_RESULT BinaryDecoder::DecodeMapPairStart(io::MemoryStream& in) {
  DCHECK(!items_stack_.empty());
  DCHECK_GT(items_stack_.back(), 0);
  items_stack_.back()--;
  return DECODE_RESULT_SUCCESS;
}
DECODE_RESULT BinaryDecoder::DecodeMapPairMiddle(io::MemoryStream& in) {
  DCHECK(!items_stack_.empty());
  return DECODE_RESULT_SUCCESS;
}
DECODE_RESULT BinaryDecoder::DecodeMapPairEnd(io::MemoryStream& in) {
  DCHECK(!items_stack_.empty());
  return DECODE_RESULT_SUCCESS;
}

DECODE_RESULT BinaryDecoder::Decode(io::MemoryStream& in, rpc::Void* out) {
  uint8 byte = 0;
  const uint32 read = in.Read(&byte, 1);
  if ( read < 1 ) {
    return DECODE_RESULT_NOT_ENOUGH_DATA;
  }
  if ( byte != 0xff ) {
    LOG_ERROR << "Failed to read rpc::Void, expected 0xff, got: "
              << strutil::StringPrintf("0x%02x", byte);
    return DECODE_RESULT_ERROR;
  }
  return DECODE_RESULT_SUCCESS;
}

DECODE_RESULT BinaryDecoder::Decode(io::MemoryStream& in, bool* out) {
  uint8 bValue = 0;
  const uint32 read = in.Read(&bValue, 1);
  if ( read < 1 ) {
    return DECODE_RESULT_NOT_ENOUGH_DATA;
  }
  switch ( bValue ) {
    case 0:
      *out = false;
      return DECODE_RESULT_SUCCESS;
    case 1:
      *out = true;
      return DECODE_RESULT_SUCCESS;
    default:
      LOG_ERROR << "Failed to read rpc::Bool, expected 1/0, got: "
                << strutil::StringPrintf("0x%02x", bValue);
      return DECODE_RESULT_ERROR;
  }
}

DECODE_RESULT BinaryDecoder::Decode(io::MemoryStream& in, string* out) {
  int32 length = 0;
  if ( in.Size() < sizeof(length) ) {
    return DECODE_RESULT_NOT_ENOUGH_DATA;     // not enough data
  }
  // decode lenght
  length = io::NumStreamer::ReadInt32(&in, rpc::kBinaryByteOrder);
  // TODO(cpopescu): some checks on upper size should be in place here, probably
  if ( length < 0 ) {
    return DECODE_RESULT_ERROR;
  }
  if ( in.Size() < length ) {
    return DECODE_RESULT_NOT_ENOUGH_DATA;                 // not enough data
  }
  if ( length > 0 ) {
    in.ReadString(out, length);
    CHECK_EQ(length, out->size());
  }
  return DECODE_RESULT_SUCCESS;
}

DECODE_RESULT BinaryDecoder::ReadRawObject(io::MemoryStream& in, io::MemoryStream* out) {
  uint32 size = 0;
  DECODE_VERIFY(DecodeNumeric(in, &size));
  if ( in.Size() < size ) {
    return DECODE_RESULT_NOT_ENOUGH_DATA;
  }
  out->AppendStream(&in, size);
  return DECODE_RESULT_SUCCESS;
}

}
