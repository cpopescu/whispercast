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
// Authors: Catalin Popescu
//
// A simple utility to join a bunch of flv input file, one after another,
// in a larger flv file.
//
// Precondition: all flv-s should be encoded with the same coding parameters
// (else results a mess)
//
#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/system.h>
#include <whisperlib/common/base/gflags.h>

#include "flv/flv_joiner.h"

//////////////////////////////////////////////////////////////////////

DEFINE_string(files,
              "",
              "Comma-separated files to join");
DEFINE_string(out,
              "",
              "We write the output to this file");
DEFINE_int32(cue_ms_time,
             5000,
             "We write these many cue points in the output file");

//////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
  common::Init(argc, argv);
  CHECK(!FLAGS_files.empty()) << "Please specify input files!";
  CHECK(!FLAGS_out.empty()) << "Please specify an output file!";
  CHECK(FLAGS_cue_ms_time >= 0) << "Invalid cue_ms_time: " << FLAGS_cue_ms_time;

  vector<string> files;
  strutil::SplitString(FLAGS_files, ",", &files);
  int64 duration = streaming::JoinFilesIntoFlv(files, FLAGS_out, FLAGS_cue_ms_time);
  return duration > 0 ? 0 : 1;
}
