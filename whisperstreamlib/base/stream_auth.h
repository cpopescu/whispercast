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

#ifndef __MEDIA_BASE_STREAM_AUTH_H__
#define __MEDIA_BASE_STREAM_AUTH_H__

#include <string>

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/callback.h>
#include <whisperlib/common/base/alarm.h>
#include <whisperlib/net/url/url.h>
//#include <whisperstreamlib/base/request.h>

class MediaAuthorizerRequestSpec;

namespace streaming {

struct AuthorizerRequest {
  // Identifies who needs to be authorized (any of these can be empty)
  string user_;
  string passwd_;
  string token_;
  string net_address_;   // normally the ip

  // Identifies on which resource it needs authorization
  string resource_;

  // Identifies what user wants to do on the resource
  string action_;

  // How long (ms) the action was performed so far (for reauthorizations)
  int64 action_performed_ms_;

  AuthorizerRequest() : action_performed_ms_(0) {}
  AuthorizerRequest(const string& user,
                    const string& passwd,
                    const string& token,
                    const string& net_address,
                    const string& resource,
                    const string& action,
                    int64 action_performed_ms)
      : user_(user),
        passwd_(passwd),
        token_(token),
        net_address_(net_address),
        resource_(resource),
        action_(action),
        action_performed_ms_(action_performed_ms) {
  }

  void ReadFromUrl(const URL& url);
  void ReadQueryComponents(const vector< pair<string, string> >& comp);
  string GetUrlQuery() const;

  void ToSpec(MediaAuthorizerRequestSpec* spec) const;
  void FromSpec(const MediaAuthorizerRequestSpec& spec);

  string ToString() const;
};

struct AuthorizerReply {
  // true = allowed, false = denied
  bool allowed_;
  // if non zero the *thing* was authorized for this long after which should
  // be dumped unless reauthorized.
  int32 time_limit_ms_;

  AuthorizerReply(bool allowed,
                  int32 time_limit_ms)
      : allowed_(allowed),
        time_limit_ms_(time_limit_ms) {
  }
  string ToString() const;
};

class Authorizer {
 public:
  Authorizer(const string& name)
      : name_(name),
        ref_count_(0) {
  }
  virtual ~Authorizer() {
    CHECK_EQ(ref_count_, 0);
  }

  typedef Callback1<const AuthorizerReply&> CompletionCallback;

  virtual bool Initialize() = 0;
  virtual void Authorize(const AuthorizerRequest& req,
                         CompletionCallback* completion) = 0;
  virtual void Cancel(CompletionCallback* completion) = 0;

  // Because of the nature of the work (async requests that can be completed
  // at different times then the point of deletion), we use a ref-count based
  // management for these
  void IncRef() { ++ref_count_; }
  void DecRef() {
    CHECK_GT(ref_count_, 0);
    --ref_count_;
    if ( ref_count_ <= 0 ) delete this;
  }

  const string& name() const { return name_; }

 private:
  const string name_;
  int32 ref_count_;
  DISALLOW_EVIL_CONSTRUCTORS(Authorizer);
};

class AsyncAuthorize {
 public:
  AsyncAuthorize(net::Selector& selector);
  ~AsyncAuthorize();

  // Params:
  // auth: the authorizer which does the actual authentication
  // req: contains the user name, password, token, user ip address, ...
  // completion: non permanent callback,
  //             called on first authentication completed.
  // reauthorization_failed: non permanent callback,
  //                         called when reauthorization fails (if ever)
  //                         If NULL reauthorization is disabled.
  // start: If true, start authorization now.
  //        If false, it doesn't start now; you can start it later by Start().
  void Start(Authorizer* auth,
             const AuthorizerRequest& req,
             Callback1<bool>* completion,
             Closure* reauthorization_failed);

  // Stop/Cancel current authorization process (if in progress).
  // Also stops reauthorization.
  void Stop();

 private:
  // Restart authorization
  void Reauthorize();

  // Every authorization (first + reauthorizations) ends up here.
  void AuthorizationCompleted(const AuthorizerReply& reply);

 private:
  Authorizer* auth_;
  AuthorizerRequest req_;

  // permanent callback to AuthorizationCompleted
  // Used for first authorization + subsequent reauthorizations
  Authorizer::CompletionCallback* authorization_completed_;

  // called on first authorization completion
  Callback1<bool>* first_auth_completion_;
  // called on reauthorization failed (if ever)
  Closure* reauthorization_failed_;

  // just calls Reauthorize() after a while
  ::util::Alarm reauthorization_alarm_;

  // timestamp of the first authorization. Used to calculate
  // action_performed_ms_ for reauthorization.
  int64 first_auth_ts_;
};

}

#endif  // __MEDIA_BASE_STREAM_AUTH_H__
