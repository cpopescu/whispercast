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
#include <whisperstreamlib/base/request.h>

namespace streaming {

class Authorizer {
 public:
  explicit Authorizer(const char* type,
                      const char* name)
      : type_(type),
        name_(name),
        ref_count_(0) {
    LOG_INFO << "Creating authorizer: " << name_ << " of type: " << type_;
  }
  virtual ~Authorizer() {
    CHECK_EQ(ref_count_, 0);
    LOG_INFO << "Deleting authorizer: " << name_ << " of type: " << type_;
  }
  virtual bool Initialize() = 0;
  virtual void Authorize(const AuthorizerRequest& req,
                         AuthorizerReply* reply,
                         Closure* completion) = 0;
  // Because of the nature of the work (async requests that can be completed
  // at different times then the point of deletion), we use a ref-count based
  // management for these
  void IncRef() { ++ref_count_; }
  void DecRef() { --ref_count_; if ( ref_count_ <= 0 ) delete this; }

  const string& type() const { return type_; }
  const string& name() const { return name_; }

 private:
  const string type_;
  const string name_;
  int32 ref_count_;
  DISALLOW_EVIL_CONSTRUCTORS(Authorizer);
};

class AuthorizeHelper {
 public:
  AuthorizeHelper(Authorizer* auth)
      : req_(),
        reply_(false),
        auth_(auth),
        canceled_(false),
        completion_callback_(NULL) {
    auth_->IncRef();
  }
  ~AuthorizeHelper() {
    CHECK(completion_callback_ == NULL);
    auth_->DecRef();
  }
  void Start(Closure* completion_callback) {
    CHECK(completion_callback_ == NULL);
    completion_callback_ = completion_callback;
    canceled_ = false;
    auth_->Authorize(req_, &reply_,
                     NewCallback(this, &AuthorizeHelper::Completion));

  }
  void Cancel() {
    canceled_ = true;
  }

  const AuthorizerRequest& req() const        { return req_;    }
  const AuthorizerReply& reply() const        { return reply_;  }
  AuthorizerRequest* mutable_req()            { return &req_;   }
  bool is_started() const  { return completion_callback_ != NULL; }

 private:
  void Completion() {
    if ( canceled_ ) {
      delete completion_callback_;
      completion_callback_ = NULL;
      delete this;
    } else {
      Closure* completion_callback = completion_callback_;
      completion_callback_ = NULL;
      completion_callback->Run();
    }
  }
  AuthorizerRequest req_;
  AuthorizerReply reply_;
  Authorizer* auth_;
  bool canceled_;
  Closure* completion_callback_;
};

}

#endif  // __MEDIA_BASE_STREAM_AUTH_H__
