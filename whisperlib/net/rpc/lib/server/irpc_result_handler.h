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

#ifndef __NET_RPC_LIB_RPC_SERVER_IRPC_RESULT_HANDLER_H__
#define __NET_RPC_LIB_RPC_SERVER_IRPC_RESULT_HANDLER_H__

#include <whisperlib/common/base/types.h>
#include <whisperlib/net/rpc/lib/server/rpc_core_types.h>

// This is an interface for an rpc::Query result receiver.
// (implemented by rpc::ServerConnection)
namespace rpc {
class IResultHandler {
 public:
  IResultHandler()
      : resultHandlerID_(0) {
  }
  virtual ~IResultHandler() {
  }

  uint32 GetResultHandlerID() const {
    return resultHandlerID_;
  }

  // Do something with the given query result.
  virtual void HandleRPCResult(const rpc::Query & query) = 0;

 protected:
  void SetResultHandlerID(uint64 id) {
    resultHandlerID_ = id;
  }

  // Generated & used by the query executor
  uint64 resultHandlerID_;

  friend class IAsyncQueryExecutor;  // sets the resultHandlerID

  DISALLOW_EVIL_CONSTRUCTORS(IResultHandler);
};
}

#endif  // __NET_RPC_LIB_RPC_SERVER_IRPC_RESULT_HANDLER_H__
