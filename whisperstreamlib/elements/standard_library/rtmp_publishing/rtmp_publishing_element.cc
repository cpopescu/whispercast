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
#include <whisperlib/common/base/timer.h>
#include <whisperlib/common/io/ioutil.h>
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_encoder.h>
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_decoder.h>

#include <whisperstreamlib/base/tag_splitter.h>
#include <whisperstreamlib/base/tag_distributor.h>
#include <whisperstreamlib/elements/standard_library/rtmp_publishing/rtmp_publishing_element.h>

#define ILOG(level)  LOG(level) << info_ << ": "
#ifdef _DEBUG
#define ILOG_DEBUG   ILOG(INFO)
#else
#define ILOG_DEBUG   if (false) ILOG(INFO)
#endif
#define ILOG_INFO    ILOG(INFO)
#define ILOG_WARNING ILOG(WARNING)
#define ILOG_ERROR   ILOG(ERROR)
#define ILOG_FATAL   ILOG(FATAL)

namespace streaming {

class RtmpPublishingData : public RtmpImporter, public ImportElementData {
  static const int kTagTimeoutMs = 20000;
  static const int kTagTimeoutRegistrationGracePeriodMs = 1000;
 public:
  RtmpPublishingData(net::Selector* selector,
                     const string& parent_element_name,
                     const string& name,
                     const string& authorizer_name,
                     ElementMapper* mapper)
      : selector_(selector),
        mapper_(mapper),
        parent_element_name_(parent_element_name),
        name_(name),
        authorizer_name_(authorizer_name),
        auth_(*selector),
        publisher_(NULL),
        distributor_(kDefaultFlavourMask,
                       strutil::JoinMedia(parent_element_name, name)),
        tag_timeout_alarm_(*selector),
        is_dropping_interframes_(true),
        is_closing_(false),
        info_(strutil::StringPrintf("[RtmpPublishingData %s]: ",
            name_.c_str())) {
    tag_timeout_alarm_.Set(NewPermanentCallback(this,
        &RtmpPublishingData::TagTimeout), true, kTagTimeoutMs, false, false);
  }
  virtual ~RtmpPublishingData() {
    // mapper_->RemoveImporter() should have already been called
    CHECK_NULL(mapper_->GetImporter(Importer::TYPE_RTMP, name_));
    CHECK_NULL(publisher_);
  }

  ////////////////////////////////////////////////////////////////////////
  // ImportElementData interface
  virtual const MediaInfo* media_info() const {
    return publisher_ == NULL ? NULL : &publisher_->GetMediaInfo(); }
  virtual bool Initialize() {
    if ( !mapper_->AddImporter(this) ) {
      ILOG_FATAL << "Cannot add importer name: " << name_;
      return false;
    }
    return true;
  }

  virtual bool AddCallback(Request* request, ProcessingCallback* callback) {
    distributor_.add_callback(request, callback);
    return true;
  }
  virtual void RemoveCallback(streaming::Request* request) {
    distributor_.remove_callback(request);
  }

  virtual void Close() {
    mapper_->RemoveImporter(this);
    is_closing_ = true;
    if ( publisher_ != NULL ) {
      publisher_->Stop();
      publisher_ = NULL;
    }
    CloseRequests(true);
  }
  ////////////////////////////////////////////////////////////////////
  // RtmpImporter interface
  virtual string ImportPath() const {
    return strutil::JoinMedia(parent_element_name_, name_);
  }
  virtual void Start(string subpath,
                     AuthorizerRequest auth_req,
                     string command,
                     Publisher* publisher) {
    CHECK(selector_->IsInSelectThread());

    // if no authorizer, simply complete with success
    if ( authorizer_name_.empty() ) {
      AuthorizationCompleted(command, publisher, true);
      return;
    }
    Authorizer* const auth = mapper_->GetAuthorizer(authorizer_name_);
    if ( auth == NULL ) {
      ILOG_ERROR << "Cannot find authorizer: " << authorizer_name_;
      AuthorizationCompleted(command, publisher, false);
      return;
    }

    ILOG_INFO << "Authorization START, auth_req: " << auth_req.ToString();

    // authorize
    auth_.Start(auth, auth_req,
        NewCallback(this, &RtmpPublishingData::AuthorizationCompleted,
            command, publisher),
        NewCallback(this, &RtmpPublishingData::ReauthorizationFailed));
  }
  void AuthorizationCompleted(string command,
                              Publisher* publisher,
                              bool allowed) {
    if ( !allowed ) {
      // detach publisher and reply with fail
      ILOG_ERROR << "Authorization failed";
      publisher->StartCompleted(false);
      return;
    }
    ILOG_INFO << "Authorization OK";

    if ( publisher_ != NULL ) {
      // close old publisher
      publisher_->Stop();
      publisher_ = NULL;
    }

    //we're attached to the calling publisher
    publisher_ = publisher;

    // reply with success
    publisher_->StartCompleted(true);
  }
  void ReauthorizationFailed() {
    if ( publisher_ != NULL ) {
      publisher_->Stop();
      publisher_ = NULL;
    }
    CloseRequests(false);
  }
  virtual void ProcessTag(const Tag* tag, int64 timestamp_ms) {
    CHECK(selector_->IsInSelectThread());
    MaybeReregisterTagTimeout(true);

    if ( tag->type() == streaming::Tag::TYPE_EOS ) {
      // publisher signaled EOS, so it's closed already.
      LOG_INFO << "EOS received: " << tag->ToString();
      publisher_ = NULL;
      CloseRequests(false);
      return;
    }

    distributor_.DistributeTag(tag, timestamp_ms);
  }


 private:
  void CloseRequests(bool send_eos) {
    ILOG_INFO << "CloseRequests, send_eos: " << send_eos
              << ", is_closing_: " << is_closing_
              << ", callbacks: " << distributor_.count();

    if ( send_eos ) {
      distributor_.CloseAllCallbacks(is_closing_);
    } else {
      distributor_.Reset();
    }

    tag_timeout_alarm_.Stop();
  }

  // Helper to register timeout on tags that we receive
  void MaybeReregisterTagTimeout(bool force) {
    if ( force || tag_timeout_alarm_.last_start_ts() +
           kTagTimeoutRegistrationGracePeriodMs < selector_->now() ) {
      tag_timeout_alarm_.Start();
    }
  }
  void TagTimeout() {
    if ( publisher_ != NULL ) {
      publisher_->Stop();
      publisher_ = NULL;
    }
    CloseRequests(false);
  }

  //////////////////////////////////////////////////////////////////////

  net::Selector* const selector_;
  ElementMapper* mapper_;
  const string parent_element_name_;
  const string name_;
  const string authorizer_name_;

  AsyncAuthorize auth_;

  // The publisher which is streaming now / NULL
  Publisher* publisher_;

  TagDistributor distributor_;

  util::Alarm tag_timeout_alarm_;

  // drop interframes, synchronize on the first keyframe
  bool is_dropping_interframes_;

  bool is_closing_;

  // just for logging
  string info_;

  DISALLOW_EVIL_CONSTRUCTORS(RtmpPublishingData);
};

//////////////////////////////////////////////////////////////////////

const char RtmpPublishingElement::kElementClassName[] = "rtmp_publishing";

RtmpPublishingElement::RtmpPublishingElement(
    const string& name,
    ElementMapper* mapper,
    net::Selector* selector,
    const string& rpc_path,
    rpc::HttpServer* rpc_server,
    io::StateKeepUser* state_keeper,
    const string& authorizer_name)
    : ImportElement(kElementClassName, name, mapper, selector,
                    rpc_path, rpc_server, state_keeper, authorizer_name),
      ServiceInvokerRtmpPublishingElementService(
          ServiceInvokerRtmpPublishingElementService::GetClassName()),
      info_("[RtmpPublishingElement " + name + "]: ") {
}

RtmpPublishingElement::~RtmpPublishingElement() {
  if ( rpc_server_ != NULL ) {
    rpc_server_->UnregisterService(rpc_path_, this);
  }
}

bool RtmpPublishingElement::Initialize() {
  return ImportElement::Initialize() &&
         (rpc_server_ == NULL ||
          rpc_server_->RegisterService(rpc_path_, this));
}

ImportElementData* RtmpPublishingElement::CreateNewImport(
    const string& import_name) {
  return new RtmpPublishingData(
      selector_, name(), import_name,
      authorizer_name_,
      mapper_);
}

void RtmpPublishingElement::AddImport(rpc::CallContext<MediaOpResult>* call,
    const string& name) {
  string err;
  call->Complete(MediaOpResult(AddImport(name, true, &err), err));
}
void RtmpPublishingElement::DeleteImport(rpc::CallContext<rpc::Void>* call,
    const string& name) {
  DeleteImport(name);
  call->Complete();
}
void RtmpPublishingElement::GetImports(rpc::CallContext< vector<string> >* call) {
  vector<string> ret;
  GetAllImports(&ret);
  call->Complete(ret);
}

}
