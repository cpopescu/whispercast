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

#include "common/base/types.h"
#include "common/base/log.h"
#include "common/base/timer.h"
#include "common/base/system.h"
#include "common/base/gflags.h"

#include "common/io/file/file_output_stream.h"
#include "common/io/file/file_input_stream.h"


DEFINE_string(test_tmp_dir, "/tmp", "Where to write temporarely test files");

void TestFile(int32 num_ints,
              int32 min_write_size,
              int32 max_write_size) {
  int32* buffer = new int32[max_write_size];
  int64 expected_size = 0;
  string testfile(FLAGS_test_tmp_dir + "/test_file_stream");
  // Start writing
  {
    io::FileOutputStream out(io::File::CreateFileOrDie(testfile.c_str()));
    int32 cb = 0;
    int32 last_num_write = 0;
    while ( cb < num_ints ) {
      const int32 crt_ints = (min_write_size +
                              random() % (max_write_size - min_write_size));
      for ( int i = 0; i < crt_ints; ++i ) {
        buffer[i] = last_num_write;
        last_num_write++;
      }
      LOG(10) << "Writing " << crt_ints
              << " from: " << buffer[0] << " to " << buffer[crt_ints-1];
      const int32 cbwrite = out.Write(buffer, crt_ints * sizeof(*buffer));
      CHECK_EQ(cbwrite, crt_ints * sizeof(*buffer));
      expected_size += crt_ints * sizeof(*buffer);
      cb += crt_ints;
    }
  }
  LOG_INFO << " Wrote " << expected_size << " bytes.";
  // Start reading
  {
    io::FileInputStream in(io::File::OpenFileOrDie(testfile.c_str()));
    int32 cb = 0;
    int32 last_num_read = 0;
    while ( cb < num_ints ) {
      const int32 crt_ints = (min_write_size +
                              random() % (max_write_size - min_write_size));
      CHECK_EQ(in.Readable(), expected_size);
      const int32 cbread = in.Read(buffer, crt_ints * sizeof(*buffer));
      const int32 ints_read = cbread / sizeof(*buffer);
      LOG(10) << "Read " << crt_ints << " got " << cbread
              << " (" << ints_read << ") "
              << " from: " << buffer[0] << " to " << buffer[ints_read-1];
      CHECK_EQ(cbread, min(static_cast<int64>(sizeof(*buffer) * crt_ints),
                           expected_size));
      expected_size -= cbread;
      for ( int i = 0; i < ints_read; ++i ) {
        CHECK_EQ(last_num_read, buffer[i]);
        last_num_read++;
      }
      cb += crt_ints;
    }
    CHECK_EQ(in.Readable(), expected_size);
  }
  LOG_INFO << " Read until " << expected_size << " bytes.";
  delete []buffer;
}

int main(int argc, char* argv[]) {
  common::Init(argc, argv);
  LOG_INFO << "Test 0";
  TestFile(1000, 10, 20);
  LOG_INFO << "Test 1";
  TestFile(1000000, 100, 2000);
  LOG_INFO << "Test 3";
  TestFile(1000000, 1, 10);
  LOG_INFO << "PASS";
  common::Exit(0);
}
