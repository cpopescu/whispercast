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
// Author: Catalin Popescu & Cosmin Tudorache

#include <sys/types.h>

#include "common/io/num_streaming.h"

namespace io {

BitArray::BitArray()
  : data_(NULL), data_size_(0), data_owned_(false),
    head_(0), head_bit_(0) {
}
BitArray::~BitArray() {
  if ( data_owned_ ) {
    delete data_;
    data_ = NULL;
  }
  data_size_ = 0;
  data_owned_ = false;
}

void BitArray::Wrap(const void* data, uint32 size) {
  data_ = static_cast<const uint8*>(data);
  data_size_ = size;
  data_owned_ = false;
  head_ = 0;
  head_bit_ = 7;
}
void BitArray::Put(const void* data, uint32 size) {
  uint8* data_copy = new uint8[size];
  ::memcpy(data_copy, data, size);
  delete data_;
  data_ = data_copy;
  data_size_ = size;
  data_owned_ = true;
  head_ = 0;
  head_bit_ = 7;
}

void BitArray::Skip(uint32 bit_count) {
  if ( bit_count <= head_bit_ ) {
    head_bit_ -= bit_count;
  } else {
    uint32 remaining_bit_count = bit_count - head_bit_ - 1;
    head_bit_ = 7;
    head_++;

    head_ += (remaining_bit_count / 8);
    head_bit_ -= (remaining_bit_count % 8);
  }
}

uint32 BitArray::Size() {
  CHECK_LE(head_bit_, 7);
  CHECK_GE(head_bit_, 0);
  if ( head_ == data_size_ ) {
    CHECK_EQ(head_bit_, 7);
    return 0;
  }
  CHECK_LT(head_, data_size_);
  return head_bit_ + 1 + (data_size_ - head_ - 1) * 8;
}

//static
void BitArray::BitCopy(const void* void_src, uint32 src_bit_index,
                       void* void_dst, uint32 dst_bit_index,
                       uint32 bit_count) {
  const uint8* src = static_cast<const uint8*>(void_src);
  uint8* dst = static_cast<uint8*>(void_dst);

  // advance 's' until src_bit_index is < 8
  src = src + (src_bit_index / 8);
  src_bit_index = src_bit_index % 8;

  // advance 'd' until dst_bit_index is < 8
  dst = dst + (dst_bit_index / 8);
  dst_bit_index = dst_bit_index % 8;

  while ( bit_count != 0 ) {
    // consider the copy: (the '#' should be copied, the rest of '-' should stay the same)
    //         7 6 5 4 3 2 1 0
    //         | | | | | | | |
    //         V V V V V V V V
    //  - src [-,-,-,#,#,#,#,-]
    //  - dst [-,-,-,-,-,-,#,#][#,#,-,-,-,-,-,-]
    // src_bit_index = 4
    // dst_bit_index = 1
    // count: 2 // How many bit do we copy now
    // src_mask: 00011000 // Use it to extract 'count' bits from src
    // dst_mask: 00000011 // Negate it and use it to zero 'count' bits in dst
    // s = *src & src_mask => s = 000##000
    // shift s to same position as dst_bit_index => s = 000000##
    // d = *dst & (~dst_mask) => d = ------00
    // d = d | s => d = ------## // 6 bits from *dst and last 2 bits from *src
    // *dst = d // 'count' bits copied
    // src_bit_index -= count // adjust index after copy
    // dst_bit_index -= count // adjust index after copy
    // bit_count -= count     // adjust remaining 'bits to copy' after copy
    //
    uint32 count = min(min(src_bit_index + 1, dst_bit_index + 1), bit_count);

    uint8 src_mask = (static_cast<uint8>(0xff) >> (7 - src_bit_index)) &
                     (static_cast<uint8>(0xff) << (src_bit_index + 1 - count));
    uint8 dst_mask = (static_cast<uint8>(0xff) >> (7 - dst_bit_index)) &
                     (static_cast<uint8>(0xff) << (dst_bit_index + 1 - count));

    uint8 s = (*src) & src_mask;
    if ( src_bit_index > dst_bit_index ) {
      s = s >> (src_bit_index - dst_bit_index);
    } else {
      s = s << (dst_bit_index - src_bit_index);
    }
    uint8 d = (*dst) & (~dst_mask);
    d = d | s;

    *dst = d;

    if ( src_bit_index >= count ) {
      src_bit_index -= count;
    } else {
      CHECK_EQ(count, src_bit_index + 1);
      src_bit_index = 7;
      src++;
    }
    if ( dst_bit_index >= count ) {
      dst_bit_index -= count;
    } else {
      CHECK_EQ(count, dst_bit_index + 1);
      dst_bit_index = 7;
      dst++;
    }
    CHECK_LE(count, bit_count);
    bit_count -= count;
  }
}
}
