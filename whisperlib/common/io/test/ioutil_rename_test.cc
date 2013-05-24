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
// Author: Cosmin Tudorache

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "common/base/types.h"
#include "common/base/log.h"
#include "common/base/errno.h"
#include "common/base/strutil.h"
#include "common/base/system.h"
#include "common/base/scoped_ptr.h"
#include "common/io/ioutil.h"

//////////////////////////////////////////////////////////////////////

bool CreateFile(const string& path, const string& text) {
  int fd = ::open(path.c_str(), O_CREAT | O_EXCL| O_RDWR, 00660);
  if ( fd == -1 ) {
    LOG_ERROR << "::open path: [" << path << "] failed, error: "
              << GetLastSystemErrorDescription();
    return false;
  }
  int result = ::write(fd, text.c_str(), text.size());
  CHECK_EQ(result, text.size());
  result = ::close(fd);
  if ( result != 0 ) {
    LOG_ERROR << "::close failed for fd: " << fd;
    return false;
  }
  if ( !io::IsReadableFile(path) ) {
    LOG_ERROR << "io::IsReadableFile failed for path: [" << path << "]";
    return false;
  }
  return true;
}

bool ReadFile(const string& path, const string& expected_text) {
  if ( !io::IsReadableFile(path) ) {
    LOG_ERROR << "io::IsReadableFile failed for path: [" << path << "]";
    return false;
  }
  int fd = ::open(path.c_str(), O_CREAT | O_RDONLY, 00440);
  if ( fd == -1 ) {
    LOG_ERROR << "::open path: [" << path << "] failed, error: "
              << GetLastSystemErrorDescription();
    return false;
  }
  scoped_array<char> text(new char[expected_text.size() + 1]);
  int result = ::read(fd, text.get(), expected_text.size());
  CHECK_EQ(result, expected_text.size());
  text[result] = 0;
  if ( expected_text != text.get() ) {
    LOG_ERROR << "ReadTestFile path: [" << path << "] got: ["
              << text.get() << "]";
    return false;
  }
  return true;
}

const uint32 F = 0x01;  // file
const uint32 D = 0x02;  // directory
const uint32 S = 0x04;  // symlink

bool Verify(const string& path, uint32 flags) {
  if ( io::Exists(path) != static_cast<bool>(flags) ) {
    LOG_ERROR << "io::Exists failed for path: [" << path << "]";
    return false;
  }
  if ( io::IsReadableFile(path) != static_cast<bool>(flags & F) ) {
    LOG_ERROR << "io::IsReadableFile failed for path: [" << path << "]";
    return false;
  }
  if ( io::IsDir(path) != static_cast<bool>(flags & D) ) {
    LOG_ERROR << "io::IsDir failed for path: [" << path << "]";
    return false;
  }
  if ( io::IsSymlink(path) != static_cast<bool>(flags & S) ) {
    LOG_ERROR << "io::IsSymlink failed for path: [" << path << "]";
    return false;
  }
  return true;
}

bool CreateSymlink(const string& existing,
                   const string& to_create) {
  uint32 existing_type = (io::IsReadableFile(existing) ? F : 0) |
                         (io::IsDir(existing) ? D : 0) |
                         (io::IsSymlink(existing) ? S : 0);
  if ( ::symlink(existing.c_str(), to_create.c_str()) ) {
    LOG_ERROR << "::symlink failed for existing: ["
              << existing << "], to_create: [" << to_create << "]";
    return false;
  }
  if ( !Verify(to_create, existing_type | S) ) {
    LOG_ERROR << "Verify to_create failed";
    return false;
  }
  return true;
}

bool CreateDir(const string& dir) {
  if ( !io::CreateRecursiveDirs(dir) ) {
    LOG_ERROR << "io::CreateRecursiveDirs failed for dir: [" << dir << "]";
    return false;
  }
  if ( !Verify(dir, D) ) {
    LOG_ERROR << "Verify dir failed";
    return false;
  }
  return true;
}

bool IsEmptyDir(const string& dir) {
  vector<string> names;
  if ( !io::DirList(dir, io::LIST_FILES | io::LIST_DIRS, NULL, &names) ) {
    LOG_ERROR << "io::DirList failed for dir: [" << dir << "]";
    return false;
  }
  if ( !names.empty() ) {
    LOG_ERROR << "Directory: [" << dir << "] not empty: "
              << strutil::ToString(names);
    return false;
  }
  return true;
}

bool Remove(const string& path) {
  if ( !io::Rm(path) ) {
    LOG_ERROR << "io::Rm failed for path: [" << path << "]";
    return false;
  }
  if ( !Verify(path, 0) ) {
    LOG_ERROR << "Verify failed for path: [" << path << "]";
    return false;
  }
  return true;
}

int main(int argc, char* argv[]) {
  common::Init(argc, argv);

  const string test_dir = "/tmp/ioutil_test";

  CHECK(Remove(test_dir));
  CHECK(CreateDir(test_dir));

  #define PATH(a) strutil::JoinPaths(test_dir, a)

  // negative tests
  do {
    CHECK(CreateSymlink(PATH("d"), PATH("sd")));
    CHECK(CreateSymlink(PATH("f"), PATH("sf")));
    CHECK(CreateDir(PATH("d")));
    CHECK(CreateFile(PATH("f"), "ftext"));
    CHECK(!io::Rename(PATH("d"), PATH("f"), true));
    CHECK(!io::Rename(PATH("d"), PATH("sf"), true));
    CHECK(!io::Rename(PATH("f"), PATH("d"), true));
    CHECK(!io::Rename(PATH("f"), PATH("sd"), true));
    CHECK(Remove(PATH("d")));
    CHECK(Remove(PATH("f")));
    CHECK(Remove(PATH("sd")));
    CHECK(Remove(PATH("sf")));
  } while ( false );
  CHECK(IsEmptyDir(test_dir));

  // rename empty dir
  {
    CHECK(CreateDir(PATH("d0")));
    CHECK(CreateDir(PATH("d1")));
    CHECK(io::Rename(PATH("d0"), PATH("d1"), false));
    CHECK(Verify(PATH("d0"), 0));
    CHECK(IsEmptyDir(PATH("d1")));
    CHECK(Remove(PATH("d1")));
  }
  CHECK(IsEmptyDir(test_dir));

  // rename file
  {
    CHECK(CreateFile(PATH("f0"), "file text"));
    CHECK(io::Rename(PATH("f0"), PATH("f1"), false));
    CHECK(Verify(PATH("f0"), 0));
    CHECK(ReadFile(PATH("f1"), "file text"));
    CHECK(Remove(PATH("f1")));
  }
  CHECK(IsEmptyDir(test_dir));

  // rename symbolic link to dir
  {
    CHECK(CreateDir(PATH("d0")));
    CHECK(CreateSymlink(PATH("d0"), PATH("s0")));
    CHECK(io::Rename(PATH("s0"), PATH("s1"), false));
    CHECK(Verify(PATH("s0"), 0));
    CHECK(Verify(PATH("s1"), S|D));
    CHECK(io::Rm(PATH("d0")));
    CHECK(Verify(PATH("s1"), S));   // s1 is dangling
    CHECK(io::Rm(PATH("s1")));
  }
  CHECK(IsEmptyDir(test_dir));

  // rename symbolic link to file
  {
    CHECK(CreateFile(PATH("f0"), "f0"));
    CHECK(CreateSymlink(PATH("f0"), PATH("s0")));
    CHECK(io::Rename(PATH("s0"), PATH("s1"), false));
    CHECK(Verify(PATH("s0"), 0));
    CHECK(Verify(PATH("s1"), S|F));
    CHECK(ReadFile(PATH("s1"), "f0"));
    CHECK(Remove(PATH("s1")));
    CHECK(ReadFile(PATH("f0"), "f0"));
    CHECK(Remove(PATH("f0")));
  }
  CHECK(IsEmptyDir(test_dir));

  // rename integrate directory, WITHOUT overwrite
  {
    CHECK(CreateDir(PATH("d0")));
    CHECK(CreateDir(PATH("d0/t")));
    CHECK(CreateFile(PATH("d0/t/f.txt"), "f0 text"));
    CHECK(CreateSymlink(PATH("d0/t/f.txt"), PATH("d0/s0")));
    CHECK(CreateSymlink(PATH("d0/t/missing"), PATH("d0/dang")));

    CHECK(CreateDir(PATH("d1")));
    CHECK(CreateDir(PATH("d1/t")));
    CHECK(CreateFile(PATH("d1/t/f.txt"), "f1 text"));
    CHECK(CreateSymlink(PATH("d1/t/f.txt"), PATH("d1/s1")));

    // cannot overwrite: "d0/t/f.txt" -> "d1/t/f.txt"
    CHECK(!io::Rename(PATH("d0"), PATH("d1"), false));

    CHECK(Verify(PATH("d0"), D));
    CHECK(Verify(PATH("d0/t"), D));
    CHECK(ReadFile(PATH("d0/t/f.txt"), "f0 text"));
    CHECK(Verify(PATH("d0/s0"), 0));
    CHECK(Verify(PATH("d1"), D));
    CHECK(Verify(PATH("d1/t"), D));
    CHECK(ReadFile(PATH("d1/t/f.txt"), "f1 text"));
    CHECK(Verify(PATH("d1/s1"), S|F));
    CHECK(ReadFile(PATH("d1/s1"), "f1 text"));
    CHECK(Verify(PATH("d1/s0"), S|F));
    CHECK(ReadFile(PATH("d1/s0"), "f0 text"));
    CHECK(Verify(PATH("d1/dang"), S));

    CHECK(Remove(PATH("d0")));
    CHECK(Remove(PATH("d1")));
  }
  CHECK(IsEmptyDir(test_dir));

  // rename integrate directory, WITH overwrite
  {
    CHECK(CreateDir(PATH("d0")));
    CHECK(CreateDir(PATH("d0/t")));
    CHECK(CreateDir(PATH("d0/empty")));
    CHECK(CreateFile(PATH("d0/t/f.txt"), "f0 text"));
    CHECK(CreateSymlink(PATH("d0/t/f.txt"), PATH("d0/s0")));
    CHECK(CreateSymlink(PATH("d0/t/missing"), PATH("d0/dang")));

    CHECK(CreateDir(PATH("d1")));
    CHECK(CreateDir(PATH("d1/t")));
    CHECK(CreateFile(PATH("d1/t/f.txt"), "f1 text"));
    CHECK(CreateSymlink(PATH("d1/t/f.txt"), PATH("d1/s1")));

    // file "f.txt" has been overwritten
    CHECK(io::Rename(PATH("d0"), PATH("d1"), true));

    CHECK(Verify(PATH("d0"), 0));
    CHECK(Verify(PATH("d1"), D));
    CHECK(Verify(PATH("d1/empty"), D));
    CHECK(Verify(PATH("d1/t"), D));
    CHECK(ReadFile(PATH("d1/t/f.txt"), "f0 text"));
    CHECK(Verify(PATH("d1/s1"), S|F));
    CHECK(ReadFile(PATH("d1/s1"), "f0 text"));
    CHECK(Verify(PATH("d1/s0"), S));    // became dangling
    CHECK(Verify(PATH("d1/dang"), S));  // was/is dangling
    CHECK(Remove(PATH("d1")));
  }
  CHECK(IsEmptyDir(test_dir));

  LOG_INFO << "Pass";

  common::Exit(0);
}
