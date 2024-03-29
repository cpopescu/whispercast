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
#include "common/base/log.h"
#include "common/base/common.h"
#include "common/base/errno.h"

#include "common/io/file/file.h"
#include "common/io/file/fd.h"

namespace io {

const char* File::AccessName(Access access) {
  switch ( access ) {
    CONSIDER(GENERIC_READ);
    CONSIDER(GENERIC_WRITE);
    CONSIDER(GENERIC_READ_WRITE);
  }
  return "UnknownAccess";
}

const char* File::CreationDispositionName(CreationDisposition cd) {
  switch ( cd ) {
    CONSIDER(CREATE_ALWAYS);
    CONSIDER(CREATE_NEW);
    CONSIDER(OPEN_ALWAYS);
    CONSIDER(OPEN_EXISTING);
    CONSIDER(TRUNCATE_EXISTING);
  }
  return "UnknownCreationDisposition";
}

const char* File::MoveMethodName(MoveMethod mm) {
  switch ( mm ) {
    CONSIDER(FILE_SET);
    CONSIDER(FILE_CUR);
    CONSIDER(FILE_END);
  }
  return "UnknownMoveMethod";
}

File::File()
  : filename_(""),
    fd_(INVALID_FD_VALUE),
    size_(0),
    position_(0) {
}

File::~File() {
  Close();
}

File* File::OpenFileOrDie(const char* filename) {
  File* const f = new File();
  CHECK(f->Open(filename, GENERIC_READ, OPEN_EXISTING));
  return f;
}

File* File::TryOpenFile(const char* filename) {
  File* const f = new File();
  if ( !f->Open(filename, GENERIC_READ, OPEN_EXISTING) ) {
    delete f;
    return NULL;
  }
  return f;
}

File* File::CreateFileOrDie(const char* filename) {
  File* const f = new File();
  CHECK(f->Open(filename, GENERIC_READ_WRITE, CREATE_ALWAYS));
  return f;
}
File* File::TryCreateFile(const char* filename) {
  File* const f = new File();
  if ( !f->Open(filename, GENERIC_READ_WRITE, CREATE_ALWAYS) ) {
    delete f;
    return NULL;
  }
  return f;
}


bool File::Open(const string& filename,  Access acc, CreationDisposition cd) {
  CHECK(!is_open());

  int flags = O_NOCTTY | O_LARGEFILE;
  switch ( cd ) {
  case CREATE_ALWAYS:     flags |= O_CREAT | O_TRUNC; break;
  case CREATE_NEW:        flags |= O_CREAT | O_EXCL; break;
  case OPEN_ALWAYS:       flags |= O_CREAT; break;
  case OPEN_EXISTING:     flags |= 0; break;
  case TRUNCATE_EXISTING: flags |= O_TRUNC; break;
  default:
    LOG_FATAL << filename << " - Invalid Disposition: " << cd;
  };

  mode_t mode = 0;
  switch ( acc )  {
  case GENERIC_READ:       flags |= O_RDONLY; mode = 00444; break;
  case GENERIC_WRITE:      flags |= O_WRONLY; mode = 00644; break;
  case GENERIC_READ_WRITE: flags |= O_RDWR;   mode = 00644; break;
  default:
    LOG_FATAL << filename << " Invalid Access: " << acc;
  };
  flags |= O_LARGEFILE;

  const int fd = ::open(filename.c_str(), flags, mode);
  if ( fd == INVALID_FD_VALUE ) {
    LOG_ERROR << "Cannot open file " << filename
              << " using access: " << AccessName(acc)
              << " creationDisposition: " << CreationDispositionName(cd)
              << " reason: " << GetLastSystemErrorDescription();
    return false;
  }

  filename_ = filename;
  fd_ = fd;
  UpdateSize();
  UpdatePosition();

  return true;
}

void File::Close() {
  if ( !is_open() ) {
    return;
  }
  if ( ::close(fd_) < 0 ) {
    LOG_ERROR << filename_ << " - Error closing file."
              << " Reason: " << GetLastSystemErrorDescription();
  }
  fd_ = INVALID_FD_VALUE;
  filename_.clear();
  size_ = 0;
  position_ = 0;
}

int64 File::Size() const {
  CHECK(is_open());
  return size_;
}

int64 File::Position() const {
  CHECK(is_open());
  return position_;
}

int64 File::SetPosition(int64 distance,
                        MoveMethod move_method) {
  CHECK(is_open());
  int whence = SEEK_SET;
  switch ( move_method ) {
  case FILE_SET: whence = SEEK_SET; break;
  case FILE_CUR: whence = SEEK_CUR; break;
  case FILE_END: whence = SEEK_END; break;
  default:
    LOG_FATAL << filename_ << " Invalid MoveMethod : " << move_method;
  }

  const int64 crt = ::lseek64(fd_, distance, whence);
  if ( crt == -1 ) {
    LOG_ERROR << filename_ << " lseek64 failed "
              << GetLastSystemErrorDescription();
    return -1;
  }
  // update cached position_
  position_ = crt;
  return position_;
}

void File::Truncate(int64 pos) {
  if ( pos == -1 )  {
    pos = Position();
  }
  if ( ::ftruncate(fd_, pos) < 0 ) {
    LOG_ERROR << "::ftruncate() failed: " << GetLastSystemErrorDescription();
    return;
  }
  SetPosition(0, FILE_END);
  UpdateSize();
}

int32 File::Read(void* buf, int32 len) {
  CHECK(is_open()) << filename_;
  const int32 cb = ::read(fd_, buf, len);
  if ( cb < 0 ) {
    LOG_ERROR << "::read() failed for file: [" << filename_
              << "], err: " << GetLastSystemErrorDescription();
    return cb;
  }
  position_ += cb;
  if ( size_ < position_ ) {
    // happens when the file gets bigger while we're reading
    UpdateSize();
  }
  return cb;
}

int32 File::Read(io::MemoryStream* out, int32 len) {
  int32 read = 0;
  while ( read < len ) {
    uint8 tmp[512];
    int32 size = min(len - read, (int32)sizeof(tmp));
    int32 r = Read(tmp, size);
    if ( r < 0 ) {
      return r;
    }
    if ( r == 0 ) {
      break;
    }
    out->Write(tmp, r);
    read += r;
  }
  return read;
}

int32 File::Write(const void* buf, int32 len) {
  CHECK(is_open()) << filename_;
  const int32 cb = ::write(fd_, buf, len);
  if ( cb < 0 ) {
    LOG_ERROR << "::write() failed for file: [" << filename_
              << "], err: " << GetLastSystemErrorDescription();
    UpdatePosition();  // don't know where the file pointer ended-up
    return cb;
  }
  position_ += cb;
  size_ = max(size_, position_);
  return cb;
}
int32 File::Write(const string& s) {
  return Write(s.c_str(), s.size());
}
int32 File::Write(io::MemoryStream& ms, int32 len) {
  CHECK(is_open()) << filename_;
  if ( len == -1 ) {
    len = ms.Size();
  }
  int32 write = 0;
  while ( write < len && !ms.IsEmpty() ) {
    uint8 tmp[512];
    int32 size = ms.Read(tmp, min(len-write, (int32)sizeof(tmp)));
    CHECK(size != 0) << " write: " << write << ", len: " << len;
    int32 w = Write(tmp, size);
    if ( w < 0 ) {
      return w;
    }
    if ( w == 0 ) {
      break;
    }
    write += w;
  }
  return write;
}

void File::Flush() {
  CHECK(is_open()) << filename_;
  // Well this should not happen - we are screwed otherwise
  CHECK(::fdatasync(fd_) != -1) << "::fdatasync() failed for file: ["
      << filename_ << "], err: " << GetLastSystemErrorDescription();
}


int64 File::UpdateSize() {
  const int64 crt = ::lseek64(fd_, 0, SEEK_CUR);
  CHECK(crt != -1) << GetLastSystemErrorDescription();
  size_ =  ::lseek64(fd_, 0, SEEK_END);
  CHECK(size_ != -1) << GetLastSystemErrorDescription();
  CHECK(::lseek64(fd_, crt, SEEK_SET) == crt)
      << " crt: " << crt << " - error: " << GetLastSystemErrorDescription();
  return size_;
}

int64 File::UpdatePosition() {
  position_ = ::lseek64(fd_, 0, SEEK_CUR);
  CHECK(position_ != -1) << GetLastSystemErrorDescription();
  return position_;
}
}
