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

// Showcase for compressing / decompressing a file

#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#include "common/base/types.h"
#include "common/base/log.h"
#include "common/base/system.h"
#include "common/base/gflags.h"
#include "common/base/errno.h"

#include "common/io/zlib/zlibwrapper.h"
#include "common/io/file/file.h"

//////////////////////////////////////////////////////////////////////

DEFINE_string(infile,
              "",
              "The input file");
DEFINE_string(outfile,
              "",
              "The outputput file");
DEFINE_bool(compress,
            true,
            "True -> compress / else decompress");

//////////////////////////////////////////////////////////////////////

void ProcessCompressFileInput(io::ZlibDeflateWrapper* zwrapper,
                              int32* size,
                              io::File* fout,
                              io::MemoryStream* in) {
  io::MemoryStream out;
  LOG_INFO << " Got to process: " << in->Size();
  if ( !zwrapper->DeflateSize(in, &out, size) ) {
    LOG_FATAL << " Error in compression - damn.. ";
  }
  if ( !out.IsEmpty() ) {
    LOG_INFO << "Process - left: " << *size << " got : " << out.Size();
    string s;
    out.ReadString(&s);
    LOG_INFO << " Writing: " << s.size();
    fout->Write(s.data(), s.size());
  }
}

void ProcessDecompressFileInput(io::ZlibInflateWrapper* zwrapper,
                                int32* size,
                                io::File* fout,
                                io::MemoryStream* in) {
  io::MemoryStream out;
  LOG_INFO << " Got to process: " << in->Size();
  int err = zwrapper->InflateSize(in, &out, size);
  if ( err == Z_STREAM_END ) {
    LOG_INFO << " Decompress done !!!";
  } else if ( err != Z_OK ) {
    LOG_FATAL << " Error in decompression !" << err;
  }
  if ( !out.IsEmpty() ) {
    LOG_INFO << "Process - left: " << *size << " got : " << out.Size();
    string s;
    out.ReadString(&s);
    LOG_INFO << " Writing: " << s.size();
    fout->Write(s.data(), s.size());
  }
}

int main(int argc, char* argv[]) {
  common::Init(argc, argv);
  CHECK(!FLAGS_infile.empty())  << " Please specify an input file !";
  CHECK(!FLAGS_outfile.empty()) << " Please specify an output file !";

  io::File* fin = io::File::OpenFileOrDie(FLAGS_infile.c_str());
  int32 file_size = fin->Size();
  io::File* fout = io::File::CreateFileOrDie(FLAGS_outfile.c_str());
  io::ZlibInflateWrapper decompress_wrapper;
  io::ZlibDeflateWrapper compress_wrapper;
  char buffer[65536];
  int cb = 0;
  io::MemoryStream in;
  while ( (cb = fin->Read(buffer, sizeof(buffer))) > 0 ) {
    in.Write(buffer, cb);
    if ( FLAGS_compress ) {
      ProcessCompressFileInput(&compress_wrapper, &file_size, fout, &in);
    } else {
      ProcessDecompressFileInput(&decompress_wrapper, &file_size, fout, &in);
    }
  }
  delete fin;
  delete fout;
}
