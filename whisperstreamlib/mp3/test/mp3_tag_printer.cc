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
// Simple utility that prints all the tags from a given mp3 file.
//

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

#include <whisperstreamlib/base/media_file_reader.h>

//////////////////////////////////////////////////////////////////////

DEFINE_string(mp3_path,
              "",
              "The file to print");

//////////////////////////////////////////////////////////////////////

void PrintTag(const streaming::Tag* media_tag, uint64 file_pos) {
  if ( media_tag->type() != streaming::Tag::TYPE_MP3 ) {
    printf("@%"PRIu64" NON MP3: %s\n", file_pos, media_tag->ToString().c_str());
    return;
  }
  printf("@%"PRIu64": %s\n", file_pos, media_tag->ToString().c_str());
}
int main(int argc, char* argv[]) {
  common::Init(argc, argv);

  streaming::MediaFileReader reader;
  CHECK(reader.Open(FLAGS_mp3_path)) << " Cannot open: ["
                                     << FLAGS_mp3_path << "]";

  while ( true ) {
    scoped_ref<streaming::Tag> tag;
    uint64 pos = reader.Position();
    streaming::TagReadStatus result = reader.Read(&tag);
    if ( result == streaming::READ_EOF ) {
      printf("EOF @%"PRIu64"\n", reader.Position());
      printf("Useless bytes at file end: %"PRIu64"\n", reader.Remaining());
      break;
    }
    if ( result != streaming::READ_OK ) {
      printf("Failed to read next tag, result: %s.\n",
          streaming::TagReadStatusName(result));
      break;
    }
    PrintTag(tag.get(), pos);
  }
  printf("STATISTICS:\n%s\n", reader.splitter()->stats().ToString().c_str());
}
