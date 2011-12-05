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
//
// Performance test for JsonEncode
//
//

#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#include "common/base/log.h"
#include "common/base/strutil.h"
#include "common/base/system.h"
#include "common/base/gflags.h"

DEFINE_string(infile,
              "",
              "Convert text from this file");
DEFINE_int32(piece_size,
             100,
             "Convert in pieces of this size");

DEFINE_bool(dump,
            false,
            "Dump what we read ?");

int main(int argc, char* argv[]) {
  common::Init(argc, argv);
  const int fd = ::open(FLAGS_infile.c_str(), O_RDONLY | O_LARGEFILE);
  CHECK(fd >= 0) << " Cannot open: " << FLAGS_infile;

  char* const buffer = new char[FLAGS_piece_size];
  do {
    const int cb = ::read(fd, buffer, FLAGS_piece_size - 1);
    buffer[cb] = '\0';
    if ( cb <= 0 ) {
      return 0;
    }
    string out(strutil::JsonStrEscape(buffer, cb));
    if ( FLAGS_dump ) {
      printf("%s\n", out.c_str());
    }
  } while ( true );
  return 1;
}
