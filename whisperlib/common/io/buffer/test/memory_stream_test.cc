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

#include "common/io/buffer/memory_stream.h"
#include "common/io/num_streaming.h"

//////////////////////////////////////////////////////////////////////

DEFINE_int32(rand_seed,
             0,
             "Seed the random with this guy");

//////////////////////////////////////////////////////////////////////

static unsigned int rand_seed;

//
// This tests MemoryStream, and implicily the data_block.{h,cc} classes
//

void TestPipeStyle(int64 initial_ints,     // we write initially these many ints
                   int64 num_ints,         // then we r/w these ints
                   int64 final_ints,       // we read finally these many ints
                   int32 min_write_size,   // in pieces of size
                                           // between these limits
                   int32 max_write_size) {
  CHECK_GT(max_write_size, min_write_size);
  int32* buffer = new int32[max_write_size];
  int32 last_num_read = 0;
  int32 last_num_write = 0;
  io::MemoryStream stream;
  int64 cb = 0;
  int64 expected_size = 0;
  const int64 total_ints = num_ints + initial_ints + final_ints;
  while ( cb < total_ints ) {
    const int32 crt_ints = (min_write_size +
                            random() % (max_write_size - min_write_size));
    const int32 op = random() % 2;
    if ( (op || cb < initial_ints) && cb < num_ints + initial_ints ) {
      // write operation
      for ( int i = 0; i < crt_ints; ++i ) {
        buffer[i] = last_num_write;
        last_num_write++;
      }
      VLOG(10) << "Writing " << crt_ints
               << " from: " << buffer[0] << " to " << buffer[crt_ints-1];
      const int32 cbwrite = stream.Write(buffer, crt_ints * sizeof(*buffer));
      CHECK_EQ(cbwrite, crt_ints * sizeof(*buffer));
      expected_size += crt_ints * sizeof(*buffer);
    } else {
      // read operation
      CHECK_EQ(stream.Size(), expected_size);
      const int32 cbread = stream.Read(buffer, crt_ints * sizeof(*buffer));
      const int32 ints_read = cbread / sizeof(*buffer);
      VLOG(10) << "Read " << crt_ints << " got " << cbread
               << " (" << ints_read << ") "
               << " from: " << buffer[0] << " to " << buffer[ints_read-1];
      CHECK_EQ(cbread, min(static_cast<int64>(sizeof(*buffer) * crt_ints),
                           expected_size));
      expected_size -= cbread;
      CHECK_EQ(stream.Size(), expected_size);
      for ( int i = 0; i < ints_read; ++i ) {
        CHECK_EQ(last_num_read, buffer[i]);
        last_num_read++;
      }
    }
    cb += crt_ints;
  }
}

//////////////////////////////////////////////////////////////////////

int32 GenerateRecord(io::MemoryStream* buf,
                     int32* out_buf,
                     int32 left_ints,
                     int32 max_add_record_size) {
  const int32 rec_size = min(
      left_ints,
      static_cast<int32>(rand_r(&rand_seed) %
                         (max_add_record_size / sizeof(int32))));
  VLOG(10) << " Generating a record of " << rec_size << " int32.";
  int32* p = out_buf;
  for ( int32 i = 0; i < rec_size; ++i ) {
    *p++ = rand_r(&rand_seed);
  }
  buf->Write(out_buf, rec_size * sizeof(*p));
  return rec_size;
}

void CheckRecords(io::MemoryStream* buf,
                  int32* check_buf,
                  int32 left_ints,
                  int32 max_read_record_size) {
  buf->MarkerSet();
  int32* p = check_buf;
  while ( left_ints > 0 ) {
    const int32 rec_size = min(
        left_ints,
        static_cast<int32>(rand_r(&rand_seed) % (max_read_record_size /
                                     sizeof(*check_buf))));
    VLOG(10) << " Checking a record of " << rec_size << " int32.";
    const int32 size = rec_size * sizeof(*check_buf);
    string s;
    buf->ReadString(&s, size);
    CHECK_EQ(s.size(), size);
    CHECK(!memcmp(s.data(), p, size));
    p += rec_size;
    left_ints -= rec_size;
  }
  buf->MarkerRestore();
}

void TestRandomAppends(bool mix_operations,
                       int32 block_size1,
                       int32 block_size2,
                       int32 total_size,
                       int32 max_add_record_size,
                       int32 max_append_record_size,
                       int32 max_read_record_size) {
  int32 num_ints = total_size / sizeof(int32);
  int32* out_buf = new int32[num_ints];
  int32* p = out_buf;
  io::MemoryStream buf1(block_size1);
  io::MemoryStream buf2(block_size2);

  LOG_INFO << "Generating buffers... ";
  int32 size = 0;
  int32 expected_size  = 0;
  while ( num_ints > size ) {
    if ( mix_operations && ((rand_r(&rand_seed) % 5) == 0) ) {
      while ( buf1.Size() > 0 ) {
        const int32 rec_size = min((int32)buf1.Size(),
                                   rand_r(&rand_seed) % max_append_record_size);
        expected_size += rec_size;
        buf2.AppendStream(&buf1, rec_size);
        CHECK(buf2.Size() == expected_size)
            << " expected_size: " << expected_size
            << " Got: " << buf2.DetailedContent();
      }
    } else {
      const int32 written = GenerateRecord(&buf1, p, num_ints - size,
                                           max_add_record_size);
      p += written;
      size += written;
    }
  }

  if ( !mix_operations ) {
    LOG_INFO << "First check buf1... ";
    CHECK_EQ(num_ints * sizeof(*out_buf), buf1.Size());
    CheckRecords(&buf1, out_buf, num_ints, max_read_record_size);
  }
  LOG_INFO << "Appending... ";
  buf1.MarkerSet();
  while ( buf1.Size() > 0 ) {
    const int32 rec_size = min((int32)buf1.Size(),
                               rand_r(&rand_seed) % max_append_record_size);
    expected_size += rec_size;
    buf2.AppendStream(&buf1, rec_size);
    CHECK(buf2.Size() == expected_size)
        << " expected_size: " << expected_size
        << " Got: " << buf2.DetailedContent();
  }
  buf1.MarkerRestore();

  if ( !mix_operations ) {
    LOG_INFO << "Second check buf1... ";
    CheckRecords(&buf2, out_buf, num_ints, max_read_record_size);
    CHECK_EQ(num_ints * sizeof(*out_buf), buf1.Size());
  }
  LOG_INFO << "First check buf2... ";
  CHECK_EQ(num_ints * sizeof(*out_buf), buf2.Size());
  CheckRecords(&buf2, out_buf, num_ints, max_read_record_size);
  delete[] out_buf;
}

//////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
  common::Init(argc, argv);
  rand_seed = FLAGS_rand_seed;
  if ( FLAGS_rand_seed > 0 ) {
    srand(FLAGS_rand_seed);
  }

  common::ByteOrder bos[2] = {common::BIGENDIAN, common::LILENDIAN};
  for ( int i = 0; i < 2; i++ ) {
    common::ByteOrder bo = bos[i];
    LOG_INFO << "Test number " << common::ByteOrderName(bo);
    io::MemoryStream ms;

    io::NumStreamer::WriteByte(&ms, 0xab);
    io::NumStreamer::WriteInt16(&ms, 0xabcd, bo);
    io::NumStreamer::WriteUInt24(&ms, 0x1234abcd, bo);
    io::NumStreamer::WriteInt32(&ms, 0x12345678, bo);
    io::NumStreamer::WriteInt64(&ms, 0x123456789abcdfULL, bo);
    io::NumStreamer::WriteDouble(&ms, 0.987654321, bo);
    io::NumStreamer::WriteFloat(&ms, 1.23456789, bo);

    CHECK_EQ(io::NumStreamer::ReadByte(&ms), 0xab);
    CHECK_EQ(io::NumStreamer::ReadInt16(&ms, bo), int16(0xabcd));
    CHECK_EQ(io::NumStreamer::ReadUInt24(&ms, bo), 0x34abcd);
    CHECK_EQ(io::NumStreamer::ReadInt32(&ms, bo), 0x12345678);
    CHECK_EQ(io::NumStreamer::ReadInt64(&ms, bo), 0x123456789abcdfULL);
    CHECK_EQ(io::NumStreamer::ReadDouble(&ms, bo), 0.987654321);
    CHECK_EQ(io::NumStreamer::ReadFloat(&ms, bo), static_cast<float>(1.23456789));

    CHECK(ms.IsEmpty());
  }

  {
    io::MemoryStream a, b;
    a.Write("abcdefg");
    a.Write("123456");
    b.AppendStreamNonDestructive(&a);
    string s1, s2;
    a.ReadString(&s1);
    CHECK_EQ(s1, "abcdefg123456");
    b.ReadString(&s2);
    CHECK_EQ(b.Size(), 0);
    CHECK_EQ(s2, "abcdefg123456");

    a.Write("abcdefg");
    a.Write("123456");
    b.AppendStreamNonDestructive(&a, 0, 6);
    b.ReadString(&s2);
    CHECK_EQ(s2, "abcdef");
    a.Skip(7);
    b.AppendStreamNonDestructive(&a, 0, 6);
    a.ReadString(&s1);
    b.ReadString(&s2);
    CHECK_EQ(s1, "123456");
    CHECK_EQ(s2, "123456");
  }

  {
    io::MemoryStream a(12);
    a.Write("1234567890\r\n"
            "abcdef\r\n"
            "123456789012345\r\n"
            "abcdefg\r\n"
            "\r\n"
            "abcd");
    string s;
    CHECK(a.ReadLine(&s));
    CHECK_EQ(s, "1234567890");
    CHECK(a.ReadLine(&s));
    CHECK_EQ(s, "abcdef");
    CHECK(a.ReadLine(&s));
    CHECK_EQ(s, "123456789012345");
    CHECK(a.ReadLine(&s));
    CHECK_EQ(s, "abcdefg");
    CHECK(a.ReadLine(&s));
    CHECK_EQ(s, "");
    CHECK(!a.ReadLine(&s));
  }
  LOG_INFO << "Test Tokens";
  {
    io::MemoryStream a(12);
    a.Write("{ ala bala \"porto'ca\\nla}\" xbuc'  \n  \\t' *");
    string s;
    CHECK_EQ(a.ReadNextAsciiToken(&s), io::TOKEN_SEP_OK);
    CHECK_EQ(s, "{");
    CHECK_EQ(a.ReadNextAsciiToken(&s), io::TOKEN_OK);
    CHECK_EQ(s, "ala");
    CHECK_EQ(a.ReadNextAsciiToken(&s), io::TOKEN_OK);
    CHECK_EQ(s, "bala");
    CHECK_EQ(a.ReadNextAsciiToken(&s), io::TOKEN_QUOTED_OK);
    CHECK_EQ(s, "\"porto'ca\\nla}\"");
    CHECK_EQ(a.ReadNextAsciiToken(&s), io::TOKEN_OK);
    CHECK_EQ(s, "xbuc");
    CHECK_EQ(a.ReadNextAsciiToken(&s), io::TOKEN_QUOTED_OK);
    CHECK_EQ(s, "'  \n  \\t'");
    CHECK_EQ(a.ReadNextAsciiToken(&s), io::TOKEN_SEP_OK);
    CHECK_EQ(s, "*");
    CHECK_EQ(a.ReadNextAsciiToken(&s), io::TOKEN_NO_DATA);
  }

  LOG_INFO << "TestRandomAppends /false/1";
  TestRandomAppends(false, 16384, 16384, 10000000, 2048, 2048, 2048);
  LOG_INFO << "TestRandomAppends /false/2";
  TestRandomAppends(false, 1627, 2591, 10000000, 4663, 5303, 3943);
  LOG_INFO << "TestRandomAppends /false/3";
  TestRandomAppends(false, 5303, 3943, 100000000, 1627, 2591, 5333);
  LOG_INFO << "TestRandomAppends /false/4";
  TestRandomAppends(false, 16384, 12000, 10000000, 128, 160, 256);

  LOG_INFO << "TestRandomAppends /true/1";
  TestRandomAppends(true, 16384, 16384, 10000000, 2048, 2048, 2048);
  LOG_INFO << "TestRandomAppends /true/2";
  TestRandomAppends(true, 1627, 2591, 10000000, 4663, 5303, 3943);
  LOG_INFO << "TestRandomAppends /true/3";
  TestRandomAppends(true, 5303, 3943, 10000000, 1627, 2591, 5333);
  LOG_INFO << "TestRandomAppends /true/4";
  TestRandomAppends(true, 16384, 12000, 10000000, 128, 160, 256);

  LOG_INFO << "Test 0";
  const int two_block_plus3 = 4096 * 2 + 3;
  TestPipeStyle(two_block_plus3, 0, two_block_plus3,
                two_block_plus3, two_block_plus3 + 1);

  LOG_INFO << "Test 1";
  TestPipeStyle(600, 0, 600, 100, 300);
  LOG_INFO << "Test 2";
  TestPipeStyle(10000, 10000000, 5000, 100, 300);
  LOG_INFO << "Test 3";
  TestPipeStyle(0, 100000000, 5000, 500, 1000);
  LOG_INFO << "Test 4";
  // TestPipeStyle(10000, 1000000000, 5000, 1, 1000);
  LOG_INFO << "PASS";
  common::Exit(0);
}
