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

#ifndef __NET_RPC_LIB_CODEC_RPC_TYPENAME_CODEC_H__
#define __NET_RPC_LIB_CODEC_RPC_TYPENAME_CODEC_H__

#include <string.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/io/num_streaming.h>

// when this is defined we encode type along with every object.
//     Provides type check.
// else: does not encode type. Faster.
#undef __PERFORM_RPC_TYPENAME_ENCODING__

#if defined(__PERFORM_RPC_TYPENAME_ENCODING__) && defined(_DEBUG)
#  define RPC_TYPENAME_ENCODE(szTypename)       \
  do {                                          \
    const uint32 size = ::strlen(szTypename);   \
    CHECK_LT(size, 256);                        \
    io::IONumStreamer::WriteByte(out_, 0x0A);   \
    io::IONumStreamer::WriteByte(out_, 0x0B);   \
    io::IONumStreamer::WriteByte(out_, 0x0C);   \
    io::IONumStreamer::WriteByte(out_, size);   \
    out_->Write(szTypename, size);              \
  } while ( false )

// TODO(cpopescu): Attention: this look buggy !!!
#  define RPC_TYPENAME_DECODE_AND_CHECK(szExpectedTypename)             \
  do {                                                                  \
    char szTypename[256] = { 0, };                                      \
    uint32 available = in_.Readable();                                  \
    if ( available < 4 ) {                                              \
      return 0;                                                         \
    }                                                                   \
    if ( io::IONumStreamer::ReadByte(&in_) != 0x0A ||                   \
         io::IONumStreamer::ReadByte(&in_) != 0x0B ||                   \
         io::IONumStreamer::ReadByte(&in_) != 0x0C ) {                  \
      LOG_ERROR << "Typename check failure. Wrong mark.";               \
      return -1;                                                        \
    }                                                                   \
    const uint32 nSize = io::IONumStreamer::ReadByte(&in_);             \
    available -= 4;                                                     \
    if ( available < nSize ) {                                          \
      return 0;                                                         \
    }                                                                   \
    in_.Read(szTypename, nSize);                                        \
    if ( 0 != strcmp(szTypename, szExpectedTypename) ) {                \
      LOG_ERROR << "Typename check failure. Found: '"                   \
                << szTypename << "'  expected: '"                       \
                << szExpectedTypename << "'";                           \
      return -1;                                                        \
    }                                                                   \
  }
#else
#  define RPC_TYPENAME_ENCODE(szTypename)
#  define RPC_TYPENAME_DECODE_AND_CHECK(szExpectedTypename)
#endif   //  defined(__PERFORM_RPC_TYPENAME_ENCODING__) && defined(_DEBUG)

#endif   //  __NET_RPC_LIB_CODEC_RPC_TYPENAME_CODEC_H__
