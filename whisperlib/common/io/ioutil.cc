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

#include <sys/types.h>

#include <dirent.h>
#include <limits.h>
#include <stddef.h>
#include <unistd.h>
#include <utime.h>
#include <stdio.h>

#include "common/io/ioutil.h"
#include "common/io/file/file.h"
#include "common/io/file/file_output_stream.h"
#include "common/base/strutil.h"
#include "common/base/log.h"
#include "common/base/errno.h"

#if defined(__APPLE__) && __WORDSIZE == 64
#define stat64 stat
#define lstat64 lstat
#endif

namespace io {

bool IsDir(const char* name) {
  struct stat64 st;
  if ( 0 != stat64(name, &st) ) {
    return false;
  }
  return S_ISDIR(st.st_mode);
}
bool IsDir(const string& name) {
  return IsDir(name.c_str());
}

bool IsReadableFile(const char* name) {
  struct stat64 st;
  if ( 0 != stat64(name, &st) ) {
    return false;
  }
  return S_ISREG(st.st_mode);
}
bool IsReadableFile(const string& name) {
  return IsReadableFile(name.c_str());
}
bool IsSymlink(const string& path) {
  struct stat64 st;
  // NOTE: ::stat queries the file referenced by link, not the link itself.
  if ( 0 != lstat64(path.c_str(), &st) ) {
    return false;
  }
  return S_ISLNK(st.st_mode);
}
bool Exists(const char* path) {
  struct stat64 st;
  if ( 0 != lstat64(path, &st) ) {
    return false;
  }
  return true;
}
bool Exists(const string& path) {
  return Exists(path.c_str());
}
int64 GetFileSize(const char* name) {
  struct stat64 st;
  if ( 0 != stat64(name, &st) ) {
    return -1;
  }
  return st.st_size;
}
int64 GetFileSize(const string& name) {
  return GetFileSize(name.c_str());
}

// Per:  http://womble.decadentplace.org.uk/readdir_r-advisory.html
// * Calculate the required buffer size (in bytes) for directory       *
// * entries read from the given directory handle.  Return -1 if this  *
// * this cannot be done.                                              *
// *                                                                   *
// * This code does not trust values of NAME_MAX that are less than    *
// * 255, since some systems (including at least HP-UX) incorrectly    *
// * define it to be a smaller value.                                  *
// *                                                                   *
// * If you use autoconf, include fpathconf and dirfd in your          *
// * AC_CHECK_FUNCS list.  Otherwise use some other method to detect   *
// * and use them where available.                                     *
size_t dirent_buf_size(DIR* dirp) {
    long name_max;
    size_t name_end;
#   if defined(HAVE_FPATHCONF) && defined(HAVE_DIRFD) \
       && defined(_PC_NAME_MAX)
        name_max = fpathconf(dirfd(dirp), _PC_NAME_MAX);
        if (name_max == -1)
#           if defined(NAME_MAX)
                name_max = (NAME_MAX > 255) ? NAME_MAX : 255;
#           else
                return (size_t)(-1);
#           endif
#   else
#       if defined(NAME_MAX)
            name_max = (NAME_MAX > 255) ? NAME_MAX : 255;
#       else
#           error "buffer size for readdir_r cannot be determined"
#       endif
#   endif
    name_end = (size_t)offsetof(struct dirent, d_name) + name_max + 1;
    return (name_end > sizeof(struct dirent)
            ? name_end : sizeof(struct dirent));
}
bool DirList(const string& to_list,
             vector<string>* names,
             bool list_dirs,
             re::RE* regex) {
  DIR* dirp = opendir(to_list.c_str());
  if ( NULL == dirp ) {
    LOG_ERROR << "::opendir failed for dir: [" << to_list << "] error: "
              << GetLastSystemErrorDescription();
    return false;
  }
  size_t size = dirent_buf_size(dirp);
  if ( size == -1 ) {
    closedir(dirp);
    LOG_ERROR << "error dirent_buf_size for dir: [" << to_list << "]";
    return false;
  }
  struct dirent* entry;
  struct dirent* buf = reinterpret_cast<struct dirent *>(malloc(size));
  int error;
  while ((error = readdir_r(dirp, buf, &entry)) == 0 && entry != NULL) {
    struct stat64 st;
    // Skip dots
    if ( '.' == entry->d_name[0] &&
         ('\0' == entry->d_name[1] ||
          ('.' == entry->d_name[1] && '\0' == entry->d_name[2])) ) {
      continue;
    }
    const string file = strutil::JoinPaths(to_list, entry->d_name);
    if ( 0 != lstat64(file.c_str(), &st) ) {
      LOG_ERROR << "lstat64 failed for file: [" << file << "]";
      continue;
    }
    if ( S_ISDIR(st.st_mode) ) {
      if ( list_dirs && (regex == NULL || regex->Matches(entry->d_name)) ) {
        names->push_back(entry->d_name);
      }
    } else if ( regex == NULL || regex->Matches(entry->d_name) ) {
      names->push_back(entry->d_name);
    }
  }
  free(buf);
  if ( error ) {
    closedir(dirp);
    LOG_ERROR << " error in readdir_r for dir: [" << to_list << "] error: "
              << GetLastSystemErrorDescription();
    return false;
  }
  closedir(dirp);
  return true;
}


bool RecursiveListing(const string& dir,
                      vector<string>* dir_names,
                      re::RE* regex) {
  vector<string> local_list;
  if ( !DirList(dir, &local_list, true) ) {
    return false;
  }
  vector<string> dirs;
  for ( size_t i = 0; i < local_list.size(); ++i ) {
    const string crt(strutil::JoinPaths(dir, local_list[i]));
    if ( (regex == NULL || regex->Matches(local_list[i]) ) ) {
      if ( io::IsReadableFile(crt.c_str()) ) {
        dir_names->push_back(crt);
        continue;
      }
    }
    if ( IsDir(crt) ) {
      dirs.push_back(crt);
    }
  }
  for ( size_t i = 0; i < dirs.size(); ++i ) {
    if ( !RecursiveListing(dirs[i], dir_names, regex) ) {
      return false;
    }
  }
  return true;
}

bool CreateRecursiveDirs(const char* dirname, mode_t mode) {
  string crt_dir(strutil::NormalizePath(dirname));
  if ( crt_dir.empty() ) return true;
  if ( crt_dir[crt_dir.size() - 1] == '/' ) {
    crt_dir.resize(crt_dir.size() - 1);   // cut any trailing '/'
  }
  vector<string> to_create;
  while ( !crt_dir.empty() ) {
    if ( io::IsDir(crt_dir.c_str()) ) {
      break;
    }
    if ( io::IsReadableFile(crt_dir.c_str()) ) {
      return false;
    }
    to_create.push_back(crt_dir);
    const size_t pos_slash = crt_dir.find_last_of('/');
    if ( pos_slash == string::npos ) {
      break;
    }
    crt_dir = crt_dir.substr(0, pos_slash);
  }
  for ( int i = to_create.size() - 1; i >= 0; --i ) {
    if ( ::mkdir(to_create[i].c_str(), mode) ) {
      LOG_ERROR << "Error creating directory: " << to_create[i]
                << " : " << GetSystemErrorDescription(errno);
      return false;
    }
  }
  return true;
}
bool CreateRecursiveDirs(const string& dirname, mode_t mode) {
  return CreateRecursiveDirs(dirname.c_str(), mode);
}


int32 GetLastNumberedFile(const string& dir,
                          re::RE* re,
                          int32 file_num_size) {
  vector<string> files;
  if ( !DirList(dir + "/", &files, false, re) ) {
    return -2;
  }
  if ( files.empty() ) {
    return -1;
  }
  sort(files.begin(), files.end());
  CHECK_GT(files.back().size(), 10);
  errno = 0;  // essential as strtol would not set a 0 errno
  const int32 file_num = strtol(
    files.back().c_str() + files.back().size() - file_num_size, NULL, 10);
  if ( errno || file_num < 0 ) {
    LOG_ERROR << " Invalid file found: " << files.back()
              << " under: " << dir;
    return -2;
  }
  return file_num;
}

bool Rm(const string& path) {
  struct stat64 s;
  // NOTE: if path is a symbolic link:
  //       - lstat() doesn't follow symbolic links. It returns stats for
  //                 the link itself.
  //       - stat() follows symbolic links and returns stats for the linked file.
  if ( ::lstat64(path.c_str(), &s) ) {
    if ( GetLastSystemError() == ENOENT ) {
      return true;
    }
    LOG_ERROR << "lstat failed for path: " << path
              << " error: " << GetLastSystemErrorDescription();
    return false;
  }
  if ( S_ISREG(s.st_mode) || S_ISLNK(s.st_mode) ) {
    LOG_DEBUG << "Removing " << (S_ISREG(s.st_mode) ? "file" : "symlink")
             << ": [" << path << "]";
    if ( ::unlink(path.c_str()) ) {
      LOG_ERROR << "unlink failed for path: " << path
                << " error: " << GetLastSystemErrorDescription();
      return false;
    }
    return true;
  }
  if ( S_ISDIR(s.st_mode) ) {
    struct dirent** namelist;
    int n = ::scandir(path.c_str(), &namelist, 0, &alphasort);
    if ( n < 0 ) {
      LOG_ERROR << "scandir failed for path: ["
                << path << "] error: " << GetLastSystemErrorDescription();
      return false;
    }
    for ( int i = 0; i < n; i++ ) {
      if ( (namelist[i]->d_name[0] == '.' && namelist[i]->d_name[1] == 0) ||
           (namelist[i]->d_name[0] == '.' &&
            namelist[i]->d_name[1] == '.' &&
            namelist[i]->d_name[2] == 0) ) {
        continue;
      }
      Rm(path + "/" + namelist[i]->d_name);
      ::free(namelist[i]);
    }
    LOG_DEBUG << "Removing dir: [" << path << "]";
    ::rmdir(path.c_str());
    ::free(namelist);
    return true;
  }
  LOG_ERROR << "cannot remove file: [" << path
            << "] unhandled mode: " << s.st_mode;
  return false;
}
bool Rmdir(const string& path) {
  int result = ::rmdir(path.c_str());
  if ( result != 0 ) {
    LOG_ERROR << "::rmdir failed for path: [" << path << "]"
                 " error: " << GetLastSystemErrorDescription();
    return false;
  }
  return true;
}

bool Mv(const string& path, const string& dir, bool overwrite) {
  return Rename(path,
                strutil::JoinPaths(dir, strutil::Basename(path)),
                overwrite);
}

bool Rename(const string& old_path,
            const string& new_path,
            bool overwrite) {
  const bool old_exists = io::Exists(old_path);
  const bool old_is_file = io::IsReadableFile(old_path);
  const bool old_is_dir = io::IsDir(old_path);
  const bool old_is_symlink = io::IsSymlink(old_path);
  const bool old_is_single = old_is_file || old_is_symlink;
  const string old_type = (old_is_symlink ? "symlink" :
                           old_is_file ? "file" :
                           old_is_dir ? "directory" :
                           "unknown");

  const bool new_exists = io::Exists(new_path);
  const bool new_is_file = io::IsReadableFile(new_path);
  const bool new_is_dir = io::IsDir(new_path);
  const bool new_is_symlink = io::IsSymlink(new_path);
  const bool new_is_single = new_is_file || new_is_symlink;
  const string new_type = (new_is_symlink ? "symlink" :
                           new_is_file ? "file" :
                           new_is_dir ? "directory" :
                           "unknown");


  if ( !old_exists ) {
    LOG_ERROR << "Rename old_path: [" << old_path << "] does not exist";
    return false;
  }
  if ( (old_is_single && new_is_dir) ||
       (old_is_dir && new_is_single) ) {
    LOG_ERROR << "Rename old_path: [" << old_path << "](" << old_type
              << "), new_path: [" << new_path << "](" << new_type
              << ") incompatible types";
    return false;
  }
  if ( new_exists && new_is_single && !overwrite ) {
    LOG_ERROR << "Rename old_path: [" << old_path << "](" << old_type
              << ") , new_path: [" << new_path << "](" << new_type
              << ") cannot overwrite";
    return false;
  }

  // - move file or directory over empty_path
  // - or move file over file
  //
  //  ! Atomically !
  //
  if ( !new_exists || (old_is_single && new_is_single) ) {
    LOG_DEBUG << "Renaming [" << old_path << "] to [" << new_path << "]";
    if ( ::rename(old_path.c_str(), new_path.c_str()) ) {
      LOG_ERROR << "::rename failed, old_path: [" << old_path << "]"
                << ", new_path: [" << new_path << "]"
                << ", error: " << GetLastSystemErrorDescription();
      return false;
    }
    return true;
  }

  // move directory over existing directory; we have to integrate
  // the content of old directory into the existing path
  //
  // !  NOT Atomically  !

  CHECK(old_exists);
  CHECK(old_is_dir);
  CHECK(new_exists);
  CHECK(new_is_dir);

  vector<string> old_entries;
  if ( !io::DirList(old_path, &old_entries, true, NULL) ) {
    LOG_ERROR << "io::DirList failed for old_path: [" << old_path
              << "] error: " << GetLastSystemErrorDescription();
    return false;
  }
  bool rename_all_success = true;
  for ( vector<string>::const_iterator it = old_entries.begin();
        it != old_entries.end(); ++it ) {
    const string& old_name = *it;
    const string old_entry = strutil::JoinPaths(old_path, old_name);
    const string new_entry = strutil::JoinPaths(new_path, old_name);
    bool success = Rename(old_entry, new_entry, overwrite);
    rename_all_success = rename_all_success && success;
  }
  if ( rename_all_success ) {
    Rm(old_path);
  }
  return rename_all_success;
}

bool Mkdir(const string& str_dir, bool recursive, mode_t mode) {
  if ( recursive ) {
    return io::CreateRecursiveDirs(str_dir.c_str(), mode);
  }
  const string dir(strutil::NormalizePath(str_dir));
  const int result = ::mkdir(dir.c_str(), mode);
  if ( result != 0 && GetLastSystemError() != EEXIST ) {
    LOG_ERROR << "Failed to create dir: [" << dir << "] error: "
              << GetLastSystemErrorDescription();
    return false;
  }
  return true;
}

string MakeAbsolutePath(const char* path) {
  const string strPath(strutil::StrTrim(string(path)));

  // if "path" is already absolute, then there's nothing more to do
  if ( strPath[0] == '/' ) {
    return strPath;
  }

  // "path" is relative.

  // find current working directory
  char cwd[1024] = { 0, };
  char* result = getcwd(cwd, sizeof(cwd));
  if ( !result ) {
    LOG_ERROR << " getcwd(..) failed: "
              << GetLastSystemErrorDescription();
    return strPath;
  }

  // append relative "path" to current working directory
  const string full_path(strutil::NormalizePath(
                             string(cwd) + "/" + strPath));
  return full_path;
}
bool Touch(const string& filename) {
  if ( io::Exists(filename) ) {
    // update file timestamp
    if ( 0 != ::utime(filename.c_str(), NULL) ) {
      LOG_ERROR << "::utime failed for file: [" << filename << "]"
                   ", error: " << GetLastSystemErrorDescription();
      return false;
    }
    return true;
  }
  if ( !FileOutputStream::TryWriteFile(filename.c_str(), "") ) {
    LOG_ERROR << "TryWriteFile failed for file: [" << filename << "]"
                 ", error: " << GetLastSystemErrorDescription();
    return false;
  }
  return true;
}

}
