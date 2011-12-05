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

#include "common/base/log.h"
#include "net/rpc/lib/codec/binary/rpc_binary_decoder.h"

namespace rpc {

DECODE_RESULT BinaryDecoder::DecodeItemsCount(uint32& count) {
  uint32 n = 0;
  const DECODE_RESULT result = Decode(n);
  if ( result == DECODE_RESULT_SUCCESS ) {
    count = n;
    items_stack_.push_back(count);
  }
  return result;
}

DECODE_RESULT BinaryDecoder::DecodeStructStart(uint32& num_attributes) {
  return DecodeItemsCount(num_attributes);
}

DECODE_RESULT BinaryDecoder::DecodeStructContinue(bool& more_attributes) {
  DCHECK(!items_stack_.empty());
  more_attributes = (items_stack_.back() != 0);
  // TODO(cpopescu): clarify w/ cosmin the logic here
  if ( !more_attributes ) {
    items_stack_.pop_back();
  }
  return DECODE_RESULT_SUCCESS;
}
DECODE_RESULT BinaryDecoder::DecodeStructAttribStart() {
  DCHECK(!items_stack_.empty());
  DCHECK_GT(items_stack_.back(), 0);
  items_stack_.back()--;
  return DECODE_RESULT_SUCCESS;
}
DECODE_RESULT BinaryDecoder::DecodeStructAttribMiddle() {
  DCHECK(!items_stack_.empty());
  return DECODE_RESULT_SUCCESS;
}
DECODE_RESULT BinaryDecoder::DecodeStructAttribEnd() {
  DCHECK(!items_stack_.empty());
  return DECODE_RESULT_SUCCESS;
}
DECODE_RESULT BinaryDecoder::DecodeArrayStart(uint32& num_elements) {
  return DecodeItemsCount(num_elements);
}
DECODE_RESULT BinaryDecoder::DecodeArrayContinue(bool& more_elements) {
  CHECK(!items_stack_.empty());
  more_elements = (items_stack_.back() != 0);
  if ( more_elements ) {
    items_stack_.back()--;
  } else {
    items_stack_.pop_back();
  }
  return DECODE_RESULT_SUCCESS;
}
DECODE_RESULT BinaryDecoder::DecodeMapStart(uint32& num_pairs) {
  return DecodeItemsCount(num_pairs);
}
DECODE_RESULT BinaryDecoder::DecodeMapContinue(bool& more_pairs) {
  DCHECK(!items_stack_.empty());
  more_pairs = (items_stack_.back() != 0);
  // TODO(cpopescu): clarify w/ cosmin the logic here
  if ( !more_pairs ) {
    items_stack_.pop_back();
  }
  return DECODE_RESULT_SUCCESS;
}
DECODE_RESULT BinaryDecoder::DecodeMapPairStart() {
  DCHECK(!items_stack_.empty());
  DCHECK_GT(items_stack_.back(), 0);
  items_stack_.back()--;
  return DECODE_RESULT_SUCCESS;
}
DECODE_RESULT BinaryDecoder::DecodeMapPairMiddle() {
  DCHECK(!items_stack_.empty());
  return DECODE_RESULT_SUCCESS;
}
DECODE_RESULT BinaryDecoder::DecodeMapPairEnd() {
  DCHECK(!items_stack_.empty());
  return DECODE_RESULT_SUCCESS;
}

DECODE_RESULT BinaryDecoder::DecodeBody(rpc::Void& out) {
  uint8 byte;
  const uint32 read = in_.Read(&byte, 1);
  if ( read < 1 ) {
    return DECODE_RESULT_NOT_ENOUGH_DATA;         // not enough data
  }
  if ( byte != 0xff ) {
    return DECODE_RESULT_ERROR;        // not a rpc::Void object
  }
  return DECODE_RESULT_SUCCESS;
}

DECODE_RESULT BinaryDecoder::DecodeBody(bool& out) {
  uint8 bValue;
  const uint32 read = in_.Read(&bValue, 1);
  if ( read < 1 ) {
    return DECODE_RESULT_NOT_ENOUGH_DATA;         // not enough data
  }
  switch ( bValue ) {
    case 0:
      out = false;
      return DECODE_RESULT_SUCCESS;
    case 1:
      out = true;
      return DECODE_RESULT_SUCCESS;
    default:
      return DECODE_RESULT_ERROR;      // not a bool object
  }
}

DECODE_RESULT BinaryDecoder::DecodeBody(string& out) {
  int32 length = 0;
  if ( in_.Size() < sizeof(length) ) {
    return DECODE_RESULT_NOT_ENOUGH_DATA;     // not enough data
  }
  // decode lenght
  length = io::NumStreamer::ReadInt32(&in_, rpc::kBinaryByteOrder);
  // TODO(cpopescu): some checks on upper size should be in place here, probably
  if ( length < 0 ) {
    return DECODE_RESULT_ERROR;
  }
  if ( in_.Size() < length ) {
    return DECODE_RESULT_NOT_ENOUGH_DATA;                 // not enough data
  }
  if ( length > 0 ) {
    in_.ReadString(&out, length);
    CHECK_EQ(length, out.size());
  }
  return DECODE_RESULT_SUCCESS;
}
}
