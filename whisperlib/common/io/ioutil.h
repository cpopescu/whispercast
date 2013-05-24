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

#ifndef __IO_IOUTIL_H__
#define __IO_IOUTIL_H__

#if defined(__APPLE__) && __WORDSIZE == 64
#define _DARWIN_USE_64_BIT_INODE
#endif

#include <sys/stat.h>
#include <vector>
#include <string>
#include <map>
#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/re.h>

namespace io {

bool IsDir(const char* name);
bool IsDir(const string& name);
bool IsReadableFile(const char* name);
bool IsReadableFile(const string& name);
bool IsSymlink(const string& path);
bool Exists(const char* path);
bool Exists(const string& path);
int64 GetFileSize(const char* name);
int64 GetFileSize(const string& name);
// returns ms from Epoch
int64 GetFileModificationTime(const string& filename);
// List a directory, possibly looking into subdirectories, filter by regex.
// Symlinks are not followed, and completely ignored.
// list_attr: a combinations of 1 or more DirListAttributes
// out: Returned entries are relative to 'dir' (they do not contain the 'dir').
// Returns success.
enum DirListAttributes {
  // return regular files
  LIST_FILES = 0x01,
  // return directories
  LIST_DIRS = 0x02,
  // return symlinks (no matter what they point to)
  LIST_SYMLINKS = 0x04,
  // return everything (files, dirs, symlinks, sockets, pipes,..)
  LIST_EVERYTHING = 0x0f,
  // look into subdirectories
  LIST_RECURSIVE = 0x80,
};
bool DirList(const string& dir, uint32 list_attr, re::RE* regex,
    vector<string>* out);

bool CreateRecursiveDirs(
  const char* dirname,
  mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
bool CreateRecursiveDirs(
  const string& dirname,
  mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);


// Utility for finding based on a path in a map of paths and structures.
// We return C() when not found, and we update path to reflect the path
// that triggerd the found objest
template<class C>
C FindPathBased(const map<string, C>* data, string& path) {
  if ( data->empty() ) {
    return C();
  }
  string to_search(path);
  while ( !to_search.empty() ) {
    const typename map<string, C>::const_iterator it = data->find(to_search);
    if ( it != data->end() ) {
      path = to_search;
      return it->second;
    }
    if ( !to_search.empty() && to_search[to_search.size() - 1] == '/' ) {
      to_search.resize(to_search.size() - 1);
    } else {
      const size_t pos_slash = to_search.rfind("/");
      if ( pos_slash != string::npos ) {
        to_search.resize(pos_slash + 1);
      } else {
        to_search.resize(0);
      }
    }
  }
  const typename map<string, C>::const_iterator it = data->find(to_search);
  if ( it != data->end() ) {
    path = to_search;
    return it->second;
  }
  return C();
}

// Same as above, but adds all matching paths
template<class C>
int FindAllPathBased(const map<string, C>* data,
                     const string& path,
                     map<string, C>* matches) {
  if ( data->empty() ) {
    return 0;
  }
  string to_search(path);
  int num_found = 0;
  while ( !to_search.empty() ) {
    const typename map<string, C>::const_iterator it = data->find(to_search);
    if ( it != data->end() ) {
      ++num_found;
      matches->insert(make_pair(to_search, it->second));
    }
    if ( !to_search.empty() && to_search[to_search.size() - 1] == '/' ) {
      to_search.resize(to_search.size() - 1);
    } else {
      const size_t pos_slash = to_search.rfind("/");
      if ( pos_slash != string::npos ) {
        to_search.resize(pos_slash + 1);
      } else {
        to_search.resize(0);
      }
    }
  }
  const typename map<string, C>::const_iterator it = data->find(to_search);
  if ( it != data->end() ) {
    ++num_found;
    matches->insert(make_pair(to_search, it->second));
  }
  return num_found;
}


// Returns the last of a set of numbered files placed in the specified
// directory. The number of the file can be strtoul from the last
// file_num_size chars of the filename.
// Returns:
//   -2 : error : cannot open directory, or file does not end in number
//   -1 : error : no file found
//  >=0 : success
int32 GetLastNumberedFile(const string& dir, re::RE* re, int32 file_num_size);

// A tool to easy remove files or folders (even non empty folders).
// Returns success state. Failure reason can be found in GetLastSystemError().
bool Rm(const string& path);

// Easy remove empty directory.
// Fails if the target directory is not empty.
bool Rmdir(const string& path);

// Move file or directory to destination directory.
// If a directory with the same name already exists, the source directory
// will be integrated in the destionation directory. See function Rename(..) .
//
// e.g. path = "/tmp/abc.avi"
//      dir = "/home/my_folder"
//      Will move file "abc.avi" to "/home/my_folder/abc.avi"
//      and "/tmp/abc.avi" no longer exists.
bool Mv(const string& path, const string& dir, bool overwrite);

// Rename file or directory moving it between directories if required.
// The parent of new_path MUST exist, otherwise this function FAILS.
// Symlinks (just like files) are treated as a whole.
//
// If old_path is a file(or symlink):
//   - if new_path is a directory => FAIL.
//   - if new_path does not exist => ATOMICAL move.
//   - if new_path is a file(or a symlink):
//       if overwrite == true => ATOMICAL move.
//       else => FAIL.
// If old_path is a directory:
//   - if new_path is a file(or symlink) => FAIL.
//   - if new_path does not exist => ATOMICAL move.
//   - if new_path is an existing directory:
//     then all the content of old_path is recusively copied to new_path
//     according to "overwrite" flag. This function returns true if all
//     the content of old_path has been moved to new_path. Otherwise this
//     function does its best to move as much as possible and returns false.
//
// e.g. old_path = "/tmp/abc.avi"
//      new_path = "/home/my_folder/123.xyz"
//      Will move file "abc.avi" to "/home/my_folder/123.xyz"
//      and "/tmp/abc.avi" no longer exists.
bool Rename(const string& old_path,
            const string& new_path,
            bool overwrite);

// Creates a directory on disk.
// recursive: if true => creates all directories on path "dir"
//            if false => creates only "dir"; it's parent must exist.
bool Mkdir(const string& dir,
           bool recursive = false,
           mode_t mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);

// Prepends the current working directory to the provided path, if
// necessary, to obtain a fully qualified path
string MakeAbsolutePath(const char* path);

// Create empty file with given name. Does nothing and returns true if file
// already exists.
// Returns success.
bool Touch(const string& filename);
}

#endif  // __IO_IOUTIL_H__
