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
// Authors: Cosmin Tudorache & Catalin Popescu

#include "net/base/selectable_filereader.h"

#include <unistd.h>
#include <fcntl.h>
#include "common/base/errno.h"

namespace net {

SelectableFilereader::SelectableFilereader(Selector* selector, int32 block_size)
    : Selectable(),
      selector_(selector),
      fd_(INVALID_FD_VALUE),
      inbuf_(block_size),
      data_callback_(NULL),
      auto_delete_data_callback_(false),
      close_callback_(NULL) {
  CHECK_NOT_NULL(selector);
}
SelectableFilereader::~SelectableFilereader() {
  CHECK(close_callback_ == NULL);
}

bool SelectableFilereader::InitializeFd(int fd,
                                        DataCallback* data_callback,
                                        bool auto_delete_data_callback,
                                        Closure* close_callback) {
  CHECK_EQ(fd_, INVALID_FD_VALUE);
  CHECK_NE(fd, INVALID_FD_VALUE);
  CHECK_NULL(data_callback_);
  CHECK_NOT_NULL(data_callback);
  CHECK(data_callback->is_permanent());

  const int flags = fcntl(fd, F_GETFL, 0);
  if ( flags < 0 ) {
    LOG_ERROR << " Error in fnctl for " << fd << " : "
              << GetLastSystemErrorDescription();
    goto Error;
  }
  if ( fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0 ) {
    LOG_ERROR << " Error in fnctl with noblock " << fd << " : "
              << GetLastSystemErrorDescription()
              << " Flags: " << flags;
    goto Error;
  }
  fd_ = fd;
  data_callback_ = data_callback;
  auto_delete_data_callback_ = auto_delete_data_callback;
  close_callback_ = close_callback;
  if ( !selector_->Register(this) ) {
    goto Error;
  }
  selector_->EnableReadCallback(this, true);
  selector_->EnableWriteCallback(this, false);
  return true;
 Error:
  ::close(fd);
  fd_ = INVALID_FD_VALUE;
  return false;
}

bool SelectableFilereader::HandleReadEvent(const SelectorEventData& event) {
  int32 cb = Selectable::Read(&inbuf_);
  if ( cb < 0 ) {
    LOG_ERROR << "Error in read for: " << GetFd();
    Close();
    return false;
  }
  if ( cb == 0 ) {
    Close();
    return false;
  }
  data_callback_->Run(&inbuf_);
  return true;
}

bool SelectableFilereader::HandleWriteEvent(const SelectorEventData& event) {
  LOG_FATAL << " SelectableFilereader should not have write enabled !!";
  return true;
}
bool SelectableFilereader::HandleErrorEvent(const SelectorEventData& event) {
  Close();
  return false;
}

void SelectableFilereader::Close() {
  if ( fd_ == INVALID_FD_VALUE ) {
    // already closed
    return;
  }
  selector_->Unregister(this);
  ::close(fd_);
  fd_ = INVALID_FD_VALUE;

  if ( auto_delete_data_callback_ ) {
    delete data_callback_;
  }
  data_callback_ = NULL;

  if ( close_callback_ != NULL ) {
    close_callback_->Run();
    close_callback_ = NULL;
  }
}
}
