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

#include "common/base/types.h"
#include "common/base/log.h"
#include "common/base/timer.h"
#include "common/base/system.h"

#include "common/io/num_streaming.h"

//////////////////////////////////////////////////////////////////////

template <typename T>
void TestBitArray() {
  io::BitArray ba;
  ba.Put("\xaa\xaa\xaa\xaa", 4);
  uint32 r = 0;
  r = ba.Read<T>(1);
  CHECK_EQ(r, 0x01);
  r = ba.Read<T>(2);
  CHECK_EQ(r, 0x01);
  r = ba.Read<T>(3);
  CHECK_EQ(r, 0x02);
  r = ba.Read<T>(3);
  CHECK_EQ(r, 0x05);
  r = ba.Read<T>(4);
  CHECK_EQ(r, 0x05);
  r = ba.Read<T>(5);
  CHECK_EQ(r, 0x0a);
  r = ba.Read<T>(6);
  CHECK_EQ(r, 0x2a);
  r = ba.Read<T>(7);
  CHECK_EQ(r, 0x55);
  r = ba.Read<T>(1);
  CHECK_EQ(r, 0x00);
  CHECK_EQ(ba.Size(), 0);
}

int main(int argc, char* argv[]) {
  common::Init(argc, argv);

  //////////////////////////////////////////////////////////////////////

  {
    //                 7 6 5 4 3 2 1 0
    //                 | | | | | | | |
    //                 V V V V V V V V
    uint8 a = 0x55; // 0 1 0 1 0 1 0 1
    uint8 b = 0x00; // 0 0 0 0 0 0 0 0

    b = 0;
    io::BitArray::BitCopy(&a, 7, &b, 7, 1);
    CHECK((uint32)b == 0x00) << ", b: " << strutil::ToBinary(b);

    b = 0;
    io::BitArray::BitCopy(&a, 7, &b, 7, 8);
    CHECK((uint32)b == 0x55) << ", b: " << strutil::ToBinary(b);

    b = 0;
    io::BitArray::BitCopy(&a, 7, &b, 7, 2);
    CHECK((uint32)b == 0x40) << ", b: " << strutil::ToBinary(b);

    b = 0;
    io::BitArray::BitCopy(&a, 4, &b, 7, 2);
    CHECK((uint32)b == 0x80) << ", b: " << strutil::ToBinary(b);

    b = 0;
    io::BitArray::BitCopy(&a, 3, &b, 2, 2);
    CHECK((uint32)b == 0x02) << ", b: " << strutil::ToBinary(b);

    b = 0;
    io::BitArray::BitCopy(&a, 2, &b, 2, 3);
    CHECK((uint32)b == 0x05) << ", b: " << strutil::ToBinary(b);

    b = 0;
    io::BitArray::BitCopy(&a, 0, &b, 3, 1);
    CHECK((uint32)b == 0x08) << ", b: " << strutil::ToBinary(b);

    b = 0;
    io::BitArray::BitCopy(&a, 6, &b, 0, 1);
    CHECK((uint32)b == 0x01) << ", b: " << strutil::ToBinary(b);
  }

  {
    //                    7 6 5 4 3 2 1 0   15 14 13 12 11 10 9  8
    //                    | | | | | | | |   |  |  |  |  |  |  |  |
    //                    V V V V V V V V   V  V  V  V  V  V  V  V
    uint16 a = 0xaaaa; // 1 0 1 0 1 0 1 0   1  0  1  0  1  0  1  0
    uint16 b = 0x0001; // 0 0 0 0 0 0 0 1   0  0  0  0  0  0  0  0

    b = 0;
    io::BitArray::BitCopy(&a, 7, &b, 7, 16);
    CHECK(b == 0xaaaa) << ", b: " << strutil::ToBinary(b);

    b = 0;
    io::BitArray::BitCopy(&a, 5, &b, 15, 8);
    CHECK(b == 0xaa00) << ", b: " << strutil::ToBinary(b);

    b = 0;
    io::BitArray::BitCopy(&a, 13, &b, 0, 2);
    CHECK(b == 0x0001) << ", b: " << strutil::ToBinary(b);
  }

  TestBitArray<uint8>();
  TestBitArray<uint16>();
  TestBitArray<uint32>();

  io::BitArray ba;
  ba.Put("\xaa\xaa\xaa\xaa", 4);
  uint32 r = 0;
  r = ba.Read<uint32>(3);
  CHECK_EQ(r, 0x05);
  r = ba.Read<uint32>(6);
  CHECK_EQ(r, 0x15);
  r = ba.Read<uint32>(22);
  CHECK_EQ(r, 0x155555);
  CHECK_EQ(ba.Size(), 1);
  ba.Skip(1);
  CHECK_EQ(ba.Size(), 0);

  LOG_INFO << "Pass";
  common::Exit(0);
}
