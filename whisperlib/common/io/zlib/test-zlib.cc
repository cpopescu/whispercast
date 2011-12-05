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
// Authors: Cosmin Tudorache

#include "common/base/log.h"
#include "common/base/errno.h"
#include "common/io/buffer/memory_stream.h"
#include "common/io/buffer/io_memory_stream.h"
#include "common/io/zlib/zlibwrapper.h"
#include "common/base/gflags.h"

//////////////////////////////////////////////////////////////////////

DEFINE_int32(rand_seed,
             3274,
             "Seed the random number generator w/ this");

//////////////////////////////////////////////////////////////////////

static unsigned int rand_seed;

// original data
uint8* g_data = NULL;
uint32 g_data_len = 0;

// zipped data
uint8* g_zdata = NULL;
uint32 g_zdata_len = 0;

void TestZDecode(uint32 step) {
  CHECK_GT(step, 0);
  CHECK_GT(g_zdata_len, 0);
  CHECK_LE(step, g_zdata_len);

  io::MemoryStream in;
  io::MemoryStream out;
  io::ZlibGzipDecodeWrapper decoder;

  // decode data step by step
  //
  uint32 w = 0;
  for ( ; w < g_zdata_len - step; w += step ) {
    in.Write(g_zdata + w, step);
    const int result = decoder.Decode(&in, &out);
    CHECK(result == Z_OK) << " result: " << result
                          << " with step: " << step << " at index: " << w;
  };
  in.Write(g_zdata + w, g_zdata_len - w);
  const int result = decoder.Decode(&in, &out);
  CHECK_EQ(result, Z_STREAM_END);

  // check decoded data correctness
  //
  CHECK_EQ(out.Size(), g_data_len);
  for ( uint32 i = 0; i < out.Size(); ) {
    uint8 tmp[128];
    int32 read = out.Read(tmp, sizeof(tmp));
    CHECK_EQ(0, ::memcmp(g_data + i, tmp, read));
    i += read;
  }
}

int main(int argc, char** argv) {
  common::Init(argc, argv);

  rand_seed = FLAGS_rand_seed;
  srand(rand_seed);

  // create original data
  //
  g_data_len = 1 << 20;
  g_data = new uint8[g_data_len];
  for ( uint32 i = 1; i < g_data_len; i++ ) {
    g_data[i] = (uint8)rand_r(&rand_seed);
  }

  LOG_INFO << "Original data size: " << g_data_len;

  // encode original data
  //
  {
    io::MemoryStream in;
    const int32 write = in.Write(g_data, g_data_len);
    CHECK_EQ(write, g_data_len);

    io::MemoryStream out;
    io::ZlibGzipEncodeWrapper encoder;
    encoder.Encode(&in, &out);

    g_zdata_len = out.Size();
    g_zdata = new uint8[g_zdata_len];
    const int32 read = out.Read(g_zdata, g_zdata_len);
    CHECK_EQ(read, g_zdata_len);

    // test encoding was ok
    //
    {
      io::ZlibGzipDecodeWrapper decoder;
      io::MemoryStream in;
      io::MemoryStream out;
      const int32 write = in.Write(g_zdata, g_zdata_len);
      CHECK_EQ(write, g_zdata_len);
      const int result = decoder.Decode(&in, &out);
      CHECK_EQ(result, Z_STREAM_END);
      for ( uint32 i = 0; i < out.Size(); ) {
        uint8 tmp[128];
        const int32 read = out.Read(tmp, sizeof(tmp));
        CHECK_EQ(0, ::memcmp(g_data + i, tmp, read));
        i += read;
      }
    }
  }

  LOG_INFO << "Encoded data size: " << g_zdata_len;

  // Decode data in steps of 1, 2, 3 ...
  //
  for ( uint32 i = 1; i < 100; i++ ) {
    TestZDecode(i);
    cerr << ".";
  }
  for ( uint32 i = 100; i < 10000; i += 13 ) {
    TestZDecode(i);
    cerr << ".";
  }
  for ( uint32 i = 10000; i < g_zdata_len / 3; i += 169 ) {
    TestZDecode(i);
    cerr << ".";
  }
  cerr << endl;

  delete [] g_data;
  g_data = NULL;
  g_data_len = 0;

  delete [] g_zdata;
  g_zdata = NULL;
  g_zdata_len = 0;

  LOG_INFO << "Pass";

  common::Exit(0);
  return 0;
}
