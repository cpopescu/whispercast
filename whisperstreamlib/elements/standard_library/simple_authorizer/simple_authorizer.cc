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

#include <unistd.h>
#include "elements/standard_library/simple_authorizer/simple_authorizer.h"

namespace streaming {

const char SimpleAuthorizer::kAuthorizerClassName[] = "simple_authorizer";

SimpleAuthorizer::SimpleAuthorizer(const string& name,
                                   int32 time_limit_ms,
                                   io::StateKeepUser* state_keeper,
                                   const string& rpc_path,
                                   rpc::HttpServer* rpc_server)
    : Authorizer(name),
      ServiceInvokerSimpleAuthorizerService(
          ServiceInvokerSimpleAuthorizerService::GetClassName()),
      time_limit_ms_(time_limit_ms),
      authenticator_(name),
      state_keeper_(state_keeper),
      rpc_path_(rpc_path),
      rpc_server_(rpc_server),
      is_registered_(false) {
}

SimpleAuthorizer::~SimpleAuthorizer() {
  if ( is_registered_ ) {
    rpc_server_->UnregisterService(rpc_path_, this);
  }
}

bool SimpleAuthorizer::Initialize() {
  const bool state_loaded = LoadState();
  if ( rpc_server_ != NULL ) {
    is_registered_ = rpc_server_->RegisterService(rpc_path_, this);
  }
  return state_loaded && (is_registered_ || rpc_server_ == NULL);
}

void SimpleAuthorizer::Authorize(const AuthorizerRequest& req,
                                 CompletionCallback* completion) {
  const string crypted_pass(crypt(req.passwd_.c_str(), "xX"));
  net::UserAuthenticator::Answer answer =
      authenticator_.Authenticate(req.user_, crypted_pass);
  if ( answer == net::UserAuthenticator::Authenticated ) {
    completion->Run(AuthorizerReply(true, time_limit_ms_));
  } else {
    completion->Run(AuthorizerReply(false, 0));
  }
}
void SimpleAuthorizer::Cancel(CompletionCallback* completion) {
  // nothing to do, this SimpleAuthorizer uses synchronous authorization
}

bool SimpleAuthorizer::LoadState() {
  if ( state_keeper_ == NULL ) {
    LOG_ERROR << " No state keeper to Load State !!";
    return true;
  }
  map<string, string> keys;
  state_keeper_->GetKeyValues("user:", &keys);
  for ( map<string, string>::const_iterator it = keys.begin();
        it != keys.end(); ++it ) {
    authenticator_.set_user_password(
        it->first.substr(sizeof("user:") - 1),
        it->second);
  }
  return true;
}

////////////////////
//
// RPC Interface
//
void SimpleAuthorizer::SetUserPassword(
    rpc::CallContext< MediaOpResult >* call,
    const string& user, const string& passwd) {
  if ( state_keeper_ == NULL ) {
    call->Complete(MediaOpResult(false,
        "Cannot set anything in the state of this authenticator"));
    return;
  }
  const string crypted_pass(crypt(passwd.c_str(), "xX"));
  if ( !state_keeper_->SetValue(
           strutil::StringPrintf("user:%s", user.c_str()),
           crypted_pass) ) {
    call->Complete(MediaOpResult(false, "Error saving the state"));
    return;
  }
  authenticator_.set_user_password(user, crypted_pass);
  call->Complete(MediaOpResult(true, ""));
}
void SimpleAuthorizer::DeleteUser(
    rpc::CallContext< MediaOpResult >* call,
    const string& user) {
  if ( state_keeper_ == NULL ) {
    call->Complete(MediaOpResult(false,
        "Cannot set anything in the state of this authenticator"));
    return;
  }
  if ( !state_keeper_->DeleteValue(
          strutil::StringPrintf("user:%s", user.c_str())) ) {
    call->Complete(MediaOpResult(false,
        "Error saving the state or non existent user"));
    return;
  }
  authenticator_.remove_user(user);
  call->Complete(MediaOpResult(true, ""));
}
void SimpleAuthorizer::GetUsers(
    rpc::CallContext< map<string, string> >* call) {
  const hash_map<string, string>& m = authenticator_.user_passwords();
  map<string, string> ret(m.begin(), m.end());
  call->Complete(ret);
}

}
