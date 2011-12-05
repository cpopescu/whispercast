
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
// Transform a f4v file into a flv file. Only the container is changed,
// the binary audio & video data remains unchanged.

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/system.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/base/date.h>
#include <whisperlib/common/io/ioutil.h>

#include <whisperstreamlib/f4v/f4v_to_flv.h>

//////////////////////////////////////////////////////////////////////

DEFINE_string(in,
              "",
              "The input .f4v file.");
DEFINE_string(out,
              "",
              "The output .flv file. If not specified we use '<in>.flv'.");

//////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
  common::Init(argc, argv);

  if ( !io::IsReadableFile(FLAGS_in) ) {
    LOG_ERROR << "No such input file: [" << FLAGS_in << "]";
    common::Exit(1);
  }
  if ( FLAGS_out == "" ) {
    FLAGS_out = strutil::CutExtension(FLAGS_in) + ".flv";
    LOG_WARNING << "Using implicit output: [" << FLAGS_out << "]";
  }
  if ( FLAGS_in == FLAGS_out ) {
    LOG_ERROR << "Input and output files cannot be identical";
    common::Exit(1);
  }

  LOG_WARNING << "Analyzing file: [" << FLAGS_in << "]"
                 ", size: " << io::GetFileSize(FLAGS_in) << " bytes.";

  if ( !streaming::ConvertF4vToFlv(FLAGS_in, FLAGS_out) ) {
    LOG_ERROR << "Conversion failed";
    common::Exit(1);
  }

  LOG_WARNING << "Success. Output file: [" << FLAGS_out << "]";
  common::Exit(0);
}
