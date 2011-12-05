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
//
// Utility that prints all tags from an f4v file.

#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/system.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/base/scoped_ptr.h>
#include <whisperlib/common/io/file/file_input_stream.h>
#include <whisperlib/common/io/buffer/memory_stream.h>

#include <whisperstreamlib/f4v/f4v_tag_splitter.h>
#include <whisperstreamlib/f4v/f4v_tag.h>
#include <whisperstreamlib/f4v/f4v_file_reader.h>

//////////////////////////////////////////////////////////////////////

DEFINE_string(f4v_path,
              "",
              "The file to print");
DEFINE_bool(f4v_just_moov,
            false,
            "Stop after MOOV atom.");
DEFINE_bool(f4v_order_frames_by_timestamp,
            false,
            "Order frames by timestamp. However, the MOOV atom containing"
            " the frame table won't be changed!");
DEFINE_bool(f4v_split_raw_frames,
            false,
            "Read raw frames instead of decoding regular frames."
            " Use this when the file has no MOOV in the beginning.");
DEFINE_bool(f4v_print_frame_data,
            false,
            "If true, prints the frame content in hex.");

//////////////////////////////////////////////////////////////////////

namespace {
class NoHeadLogLine {
public:
  NoHeadLogLine() : oss_() {}
  ~NoHeadLogLine() { cout << oss_.str() << endl; }
  ostream& stream() { return oss_; }
private:
  ostringstream oss_;
};
}

#define LOG_TEST NoHeadLogLine().stream()

int main(int argc, char* argv[]) {
  common::Init(argc, argv);
  if ( FLAGS_f4v_path.empty() ) {
    LOG_ERROR << " Please specify f4v_path parameter !";
    common::Exit(1, false);
  }
  LOG_INFO << "Analyzing file: [" << FLAGS_f4v_path << "]";

  streaming::f4v::FileReader file_reader;
  file_reader.decoder().set_order_frames_by_timestamp(
      FLAGS_f4v_order_frames_by_timestamp);
  file_reader.decoder().set_split_raw_frames(
      FLAGS_f4v_split_raw_frames);
  if ( !file_reader.Open(FLAGS_f4v_path) ) {
    LOG_ERROR << "Failed to open file: [" << FLAGS_f4v_path << "]";
    common::Exit(1, false);
  }

  while ( true ) {
    scoped_ref<streaming::f4v::Tag> f4v_tag;
    streaming::f4v::TagDecodeStatus result = file_reader.Read(&f4v_tag);
    if ( result == streaming::f4v::TAG_DECODE_ERROR ) {
      CHECK_NULL(f4v_tag.get());
      LOG_ERROR << "Error decoding tag at file pos: " << file_reader.Position();
      break;
    }
    if ( result == streaming::f4v::TAG_DECODE_NO_DATA ) {
      CHECK_NULL(f4v_tag.get());
      LOG_TEST << "EOF @" << file_reader.Position();
      if ( file_reader.Remaining() > 0 ) {
        LOG_ERROR << "Useless bytes at file end: " << file_reader.Remaining();
      }
      break;
    }
    CHECK_NOT_NULL(f4v_tag.get());
    LOG_TEST << "@" << file_reader.Position() << " TAG : @"
             << f4v_tag->timestamp_ms() << " " << f4v_tag->ToString();
    if ( FLAGS_f4v_print_frame_data && f4v_tag->is_frame() ) {
      LOG_TEST << "FrameData: " << f4v_tag->frame()->data().DumpContentHex();
    }
    if ( FLAGS_f4v_just_moov &&
         f4v_tag->is_atom() &&
         f4v_tag->atom()->type() == streaming::f4v::ATOM_MOOV ) {
      LOG_TEST << "Stopping after MOOV";
      common::Exit(1, true);
    }

  }
  LOG_TEST << "STATISTICS: " << endl
           << file_reader.stats().ToString();
}
