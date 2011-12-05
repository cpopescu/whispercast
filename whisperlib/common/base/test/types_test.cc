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
// Author: Catalin Popescu
#include <stdlib.h>

#include "common/base/types.h"
#include "common/base/log.h"
#include "common/base/system.h"

int main(int argc, char* argv[]) {
  common::Init(argc, argv);

  COMPILE_ASSERT(1 == sizeof(int8), Invalid_int8_size);
  CHECK_EQ(1, sizeof(int8));
  COMPILE_ASSERT(1 == sizeof(uint8), Invalid_uint8_size);
  CHECK_EQ(1, sizeof(uint8));
  COMPILE_ASSERT(2 == sizeof(int16), Invalid_int16_size);
  CHECK_EQ(2, sizeof(int16));
  COMPILE_ASSERT(2 == sizeof(uint16), Invalid_uint16_size);
  CHECK_EQ(2, sizeof(uint16));
  COMPILE_ASSERT(4 == sizeof(int32), Invalid_int32_size);
  CHECK_EQ(4, sizeof(int32));
  COMPILE_ASSERT(4 == sizeof(uint32), Invalid_uint32_size);
  CHECK_EQ(4, sizeof(uint32));
  COMPILE_ASSERT(8 == sizeof(int64), Invalid_int64_size);
  CHECK_EQ(8, sizeof(int64));
  COMPILE_ASSERT(8 == sizeof(uint64), Invalid_uint64_size);
  CHECK_EQ(8, sizeof(uint64));
  LOG_INFO << "PASS";

  common::Exit(0);
}
