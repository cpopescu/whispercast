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

#include <stdio.h>

#include "common/base/types.h"
#include "common/base/log.h"
#include "common/base/timer.h"
#include "common/base/system.h"
#include "common/base/gflags.h"

#include "common/io/ioutil.h"
#include "common/io/file/file_input_stream.h"
#include "common/io/file/file_output_stream.h"

//////////////////////////////////////////////////////////////////////

DEFINE_string(d,
              ".",
              "List this directory");

DEFINE_bool(r,
            false,
            "Recursive list");

DEFINE_string(f,
              "",
              "Match this when listing the specified directory.");

//////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
  common::Init(argc, argv);
  CHECK(!FLAGS_d.empty()) << " Specify a directory!";
  CHECK(io::IsDir(FLAGS_d.c_str())) << " Not a directory: " << FLAGS_d;

  //////////////////////////////////////////////////////////////////////

  map<string, int> tofind;
  tofind["/ala/bala/gigi/marga"] = 1;
  tofind["/ala/bala/gigi/targa"] = 2;
  tofind["/ala/bala/"] = 3;

  string s;

  s = "/ala/bala/gigi/marga";
  CHECK_EQ(io::FindPathBased(&tofind, s), 1);
  CHECK_EQ(s, "/ala/bala/gigi/marga");

  s = "/ala/bala/gigi/marga/";
  CHECK_EQ(io::FindPathBased(&tofind, s), 1);
  CHECK_EQ(s, "/ala/bala/gigi/marga");

  s = "/ala/bala/gigi/targa/";
  CHECK_EQ(io::FindPathBased(&tofind, s), 2);
  CHECK_EQ(s, "/ala/bala/gigi/targa");

  s = "/ala/bala/gigi/";
  CHECK_EQ(io::FindPathBased(&tofind, s), 3);
  CHECK_EQ(s, "/ala/bala/");

  s = "/ala";
  CHECK_EQ(io::FindPathBased(&tofind, s), 0);
  CHECK_EQ(s, "/ala");

  map<string, int> xres;
  s = "/ala/bala/gigi/marga";
  CHECK_EQ(io::FindAllPathBased(&tofind, s, &xres), 2);
  CHECK_EQ(xres.size(), 2);
  CHECK_EQ(xres["/ala/bala/gigi/marga"], 1);
  CHECK_EQ(xres["/ala/bala/"], 3);

  //////////////////////////////////////////////////////////////////////

  vector<string> res;
  re::RE regex(FLAGS_f.empty() ? ".*" : FLAGS_f.c_str());
  io::DirList(FLAGS_d, io::LIST_FILES | (FLAGS_r ? io::LIST_RECURSIVE : 0),
      &regex, &res);
  for ( int i = 0; i < res.size(); i++ ) {
    printf("   %s\n", res[i].c_str());
  }

  CHECK(io::CreateRecursiveDirs("/tmp/gigi/marga/king/of/targa"));
  CHECK(io::IsDir("/tmp/gigi/marga/king/of/targa"));
  CHECK(io::CreateRecursiveDirs("/tmp/gigi/marga/king/"));
  CHECK(!io::IsDir("/usr/blah"));
  CHECK(!io::CreateRecursiveDirs("/usr/blah"));

  const string touch_test_file = "/tmp/ioutil_touch_test.txt";
  CHECK(io::Touch(touch_test_file));
  CHECK(io::Exists(touch_test_file));
  CHECK(io::FileOutputStream::TryWriteFile(touch_test_file.c_str(), "abc"));
  CHECK(io::Touch(touch_test_file));
  string touch_test_content;
  CHECK(io::FileInputStream::TryReadFile(touch_test_file.c_str(),
      &touch_test_content));
  CHECK_EQ(touch_test_content, "abc");
}
