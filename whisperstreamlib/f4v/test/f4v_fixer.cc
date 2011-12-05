
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
// Utility to fix the structure of a f4v file: by moving the MOOV from the
// end to the beginning of file.

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
#include <whisperstreamlib/f4v/atoms/movie/moov_atom.h>


//////////////////////////////////////////////////////////////////////

DEFINE_string(in,
              "",
              "The input .f4v file.");
DEFINE_string(out,
              "",
              "The output .f4v file.");

//////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
  common::Init(argc, argv);

  if ( !io::IsReadableFile(FLAGS_in) ) {
    LOG_ERROR << "No such input file: [" << FLAGS_in << "]";
    common::Exit(1);
  }
  if ( FLAGS_out == "" ) {
    LOG_ERROR << "No output file specified.";
    common::Exit(1);
  }
  if ( FLAGS_in == FLAGS_out ) {
    LOG_ERROR << "Input and output files cannot be identical";
    common::Exit(1);
  }

  streaming::f4v::util::FixResult result =
      streaming::f4v::util::FixFileStructure(FLAGS_in, FLAGS_out, true);
  if ( result == streaming::f4v::util::FIX_ERROR ) {
    LOG_ERROR << "Error fixing file: [" << FLAGS_in << "]";
    common::Exit(1);
  }

  LOG_WARNING << "Success. Output file: [" << FLAGS_out << "]";
  common::Exit(0);
}
