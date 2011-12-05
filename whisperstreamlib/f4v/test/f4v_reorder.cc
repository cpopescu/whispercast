
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
// Transform a f4v file by reordering the frames and modifying the MOOV atom
// accordingly.

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
#include <whisperlib/common/base/timer.h>
#include <whisperlib/common/base/date.h>
#include <whisperlib/common/io/ioutil.h>
#include <whisperlib/common/io/file/file_input_stream.h>
#include <whisperlib/common/io/buffer/memory_stream.h>

#include <whisperstreamlib/f4v/f4v_tag.h>
#include <whisperstreamlib/f4v/f4v_decoder.h>
#include <whisperstreamlib/f4v/f4v_encoder.h>
#include <whisperstreamlib/f4v/f4v_util.h>
#include <whisperstreamlib/f4v/f4v_file_reader.h>
#include <whisperstreamlib/f4v/f4v_file_writer.h>
#include <whisperstreamlib/f4v/atoms/movie/moov_atom.h>


//////////////////////////////////////////////////////////////////////

DEFINE_string(in,
              "",
              "The input .f4v file.");
DEFINE_string(out,
              "",
              "The output .f4v file.");
DEFINE_bool(order_by_timestamp,
            true,
            "If true, orders frames by timestamp."
            " Else the frames are ordered by offset.");

//////////////////////////////////////////////////////////////////////

bool ReorderF4vFile(const string& in_f4v_file, const string& out_f4v_file,
    bool order_by_timestamp) {
  // prepare input file
  streaming::f4v::FileReader file_reader;
  if ( !file_reader.Open(in_f4v_file) ) {
    LOG_ERROR << "Failed to read file: [" << in_f4v_file << "]";
    return false;
  }
  file_reader.decoder().set_order_frames_by_timestamp(order_by_timestamp);

  // prepare the output file
  streaming::f4v::FileWriter file_writer;
  if ( !file_writer.Open(out_f4v_file) ) {
    LOG_ERROR << "Failed to write file: [" << out_f4v_file << "]";
    return false;
  }

  while ( true ) {
    // 1. read f4v_tag
    scoped_ref<streaming::F4vTag> f4v_tag;
    streaming::f4v::TagDecodeStatus result = file_reader.Read(&f4v_tag);
    if ( result == streaming::f4v::TAG_DECODE_NO_DATA ) {
      // EOS reached complete
      break;
    }
    if ( result == streaming::f4v::TAG_DECODE_ERROR ) {
      LOG_ERROR << "Error reading f4v tag";
      return false;
    }
    CHECK_EQ(result, streaming::f4v::TAG_DECODE_SUCCESS);
    CHECK_NOT_NULL(f4v_tag.get());

    LOG_DEBUG << "Read F4vTag: " << f4v_tag->ToString();

    // 2. maybe print media info (from MOOV atom)
    if ( f4v_tag->is_atom() &&
         f4v_tag->atom()->type() == streaming::f4v::ATOM_MOOV ) {
      const streaming::f4v::MoovAtom& moov =
        static_cast<const streaming::f4v::MoovAtom&>(*f4v_tag->atom());
      streaming::f4v::util::MediaInfo media_info;
      streaming::f4v::util::ExtractMediaInfo(moov, &media_info);
      LOG_INFO << "F4V MediaInfo: " << media_info.ToString();
    }

    // 3. write f4v_tag
    file_writer.Write(*f4v_tag);
    LOG_DEBUG << "Wrote F4vTag: " << f4v_tag->ToString();
  }

  if ( file_reader.Remaining() > 0 ) {
    LOG_ERROR << "Useless bytes at file end: " << file_reader.Remaining()
              << " bytes, at file offset: " << file_reader.Position()
              << ", in file: [" << in_f4v_file << "]";
  }

  LOG_INFO << "Conversion succeeded. " << file_reader.stats().ToString();

  return true;
}

int main(int argc, char* argv[]) {
  common::Init(argc, argv);

  if ( !io::IsReadableFile(FLAGS_in) ) {
    LOG_ERROR << "No such input file: [" << FLAGS_in << "]";
    common::Exit(1);
  }
  if ( FLAGS_in == FLAGS_out ) {
    LOG_ERROR << "Input and output files cannot be identical";
    common::Exit(1);
  }

  LOG_WARNING << "Analyzing file: [" << FLAGS_in << "]"
                 ", size: " << io::GetFileSize(FLAGS_in) << " bytes.";

  if ( !ReorderF4vFile(FLAGS_in, FLAGS_out, FLAGS_order_by_timestamp) ) {
    LOG_ERROR << "Conversion failed";
    common::Exit(1);
  }

  LOG_WARNING << "Success. Output file: [" << FLAGS_out << "]";
  common::Exit(0);
}
