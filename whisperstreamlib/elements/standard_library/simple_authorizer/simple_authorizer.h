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
//

#ifndef __MEDIA_STANDARD_LIBRARY_SIMPLE_AUTHORIZER_H__
#define __MEDIA_STANDARD_LIBRARY_SIMPLE_AUTHORIZER_H__

#include <string>
#include <whisperstreamlib/base/stream_auth.h>
#include <whisperlib/net/base/user_authenticator.h>
#include <whisperlib/net/rpc/lib/server/rpc_http_server.h>
#include <whisperlib/common/io/checkpoint/state_keeper.h>

#include <whisperstreamlib/elements/standard_library/auto/standard_library_invokers.h>
#include <whisperstreamlib/elements/standard_library/simple_authorizer/simple_authorizer.h>

namespace streaming {

// This is an authorizer that says yes to all operations requested by
// an authenticated user.
class SimpleAuthorizer
    : public Authorizer,
      public ServiceInvokerSimpleAuthorizerService {
 public:
  static const char kAuthorizerClassName[];
  SimpleAuthorizer(const string& name,
                   int32 time_limit_ms,
                   io::StateKeepUser* state_keeper,
                   const string& rpc_path,
                   rpc::HttpServer* rpc_server);
  virtual ~SimpleAuthorizer();

  /////////////////////////////////////////
  // Authorizer methods
  virtual bool Initialize();
  virtual void Authorize(const AuthorizerRequest& req,
                         CompletionCallback* completion);
  virtual void Cancel(CompletionCallback* completion);

 private:
  /////////////////////////////////////////
  // RPC interface
  virtual void SetUserPassword(
      rpc::CallContext< MediaOpResult >* call,
      const string& user, const string& passwd);
  virtual void DeleteUser(
      rpc::CallContext< MediaOpResult >* call,
      const string& user);
  virtual void GetUsers(
      rpc::CallContext< map<string, string> >* call);

  bool LoadState();

 private:
  // if non zero the user was authorized for this long after which is dumped
  // automatically.
  const int32 time_limit_ms_;

  net::SimpleUserAuthenticator authenticator_;
  io::StateKeepUser* const state_keeper_;
  const string rpc_path_;
  rpc::HttpServer* const rpc_server_;
  bool is_registered_;

  DISALLOW_EVIL_CONSTRUCTORS(SimpleAuthorizer);
};
}

#endif  // __MEDIA_STANDARD_LIBRARY_SIMPLE_AUTHORIZER_H__
