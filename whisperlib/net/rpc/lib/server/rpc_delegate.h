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
// Author: Mihai Ianculescu

// TODO(cpopescu): Ask misi if this is still useful

#ifndef __NET_RPC_LIB_SERVER_RPC_DELEGATE_H__
#define __NET_RPC_LIB_SERVER_RPC_DELEGATE_H__

#include <whisperlib/common/sync/event.h>
#include <whisperlib/common/base/callback.h>


namespace rpc {

class AsyncCall {
 public:
  AsyncCall(net::Selector* selector, Closure* callback) :
      selector_(selector),
      callback_(callback),
      event_(false, true) {
  }
  ~RpcAsyncCall() {
    delete callback_;
  }
  void Run() {
    selector_->RunInSelectLoop(callback_);
    event_.Wait();
  }
 private:
  void Call() {
    callback_->Run();
    event_.Signal();
  }
  net::Selector* const selector_;
  Closure* const callback_;

  synch::Event event_;
};
}

#define RPC_DELEGATE_TO_SELECTOR(selector, object, function, call, ...) \
  if ( !(selector)->IsInSelectThread() ) {                              \
    DLOG_INFO << "RPC " <<  __func__ << " IN RPC...";                   \
    selector_->RunInSelectLoop(NewCallback(object, function,            \
                                           call, __VA_ARGS__));         \
    return;                                                             \
  }                                                                     \
  DLOG_INFO << "RPC " << __func__ << " IN SELECTOR...";

#endif   // __NET_RPC_LIB_SERVER_RPC_DELEGATE_H__
