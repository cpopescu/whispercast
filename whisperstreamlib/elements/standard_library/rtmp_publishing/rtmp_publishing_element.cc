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

#include <whisperstreamlib/raw/raw_tag_splitter.h>
#include <whisperstreamlib/base/saver.h>
#include <whisperstreamlib/base/tag_distributor.h>
#include <whisperstreamlib/elements/standard_library/rtmp_publishing/rtmp_publishing_element.h>
#include <whisperstreamlib/elements/standard_library/http_server/import_element_inl.h>

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

class RtmpPublishingData : public RtmpImporter {
  static const int kTagTimeoutMs = 20000;
  static const int kTagTimeoutRegistrationGracePeriodMs = 1000;
 public:
  RtmpPublishingData(net::Selector* selector,
                     const string& name,
                     Tag::Type tag_type,
                     const string& save_dir,
                     SplitterCreator* splitter_creator,
                     const string& authorizer_name,
                     ElementMapper* mapper,
                     bool append_only,
                     bool disable_preemption,
                     int32 prefill_buffer_ms,
                     int32 advance_media_ms,
                     int32 buildup_interval_sec,
                     int32 buildup_delay_sec)
      : selector_(selector),
        mapper_(mapper),
        name_(name),
        caps_(tag_type, kDefaultFlavourMask),
        authorizer_name_(authorizer_name),
        save_dir_(save_dir),
        append_only_(append_only),
        disable_preemption_(disable_preemption),
        prefill_buffer_ms_(prefill_buffer_ms),
        advance_media_ms_(advance_media_ms),
        buildup_interval_sec_(buildup_interval_sec),
        buildup_delay_sec_(buildup_delay_sec),
        close_publisher_(NULL),
        splitter_creator_(splitter_creator),
        splitter_(NULL),
        distributor_(kDefaultFlavourMask, name),
        saver_(NULL),
        processing_callback_(NewPermanentCallback(this,
            &RtmpPublishingData::ProcessTag)),
        tag_timeout_alarm_(*selector),
        is_closing_(false),
        info_(strutil::StringPrintf("[RtmpPublishingData %s]: ",
            name_.c_str())) {
    tag_timeout_alarm_.Set(NewPermanentCallback(this,
        &RtmpPublishingData::Close), true, kTagTimeoutMs, false, false);
  }
  virtual ~RtmpPublishingData() {
    // mapper_->RemoveImporter() should have already been called
    CHECK_NULL(mapper_->GetImporter(Importer::TYPE_RTMP, name_));

    CHECK_NULL(close_publisher_);
    delete processing_callback_;
    processing_callback_ = NULL;
  }

  bool Initialize() {
    if ( !mapper_->AddImporter(this) ) {
      ILOG_ERROR << "Cannot add importer name: " << name_;
      return false;
    }
    return true;
  }

  bool AddCallback(streaming::Request* request,
                   streaming::ProcessingCallback* callback) {
    distributor_.add_callback(request, callback);
    return true;
  }
  void RemoveCallback(streaming::Request* request) {
    distributor_.remove_callback(request);
  }

  void Close() {
    mapper_->RemoveImporter(this);
    is_closing_ = true;
    if ( close_publisher_ == NULL ) {
      // no publisher attached
      CloseRequests(true);
      return;
    }
    // close the publisher. It will send us an EosTag which will trigger
    // CloseRequests().
    close_publisher_->Run();
    close_publisher_ = NULL;
  }
  ////////////////////////////////////////////////////////////////////////////
  // ImportDataSpec methods
  const streaming::Capabilities& caps() const { return caps_; }
  const streaming::TagSplitter* splitter() const { return splitter_; }
  const string& save_dir() const { return save_dir_; }
 protected:
  ////////////////////////////////////////////////////////////////////
  // RtmpImporter interface
  virtual const string& ImportPath() const {
    return name_;
  }
  virtual void Start(string subpath,
                     AuthorizerRequest auth_req,
                     string command,
                     Closure* close_publisher,
                     Callback1<ProcessingCallback*>* completed) {
    CHECK(selector_->IsInSelectThread());
    if ( close_publisher_ != NULL ) {
      if ( disable_preemption_ ) {
        ILOG_ERROR << "Preemption disabled, cannot re-publish";
        completed->Run(NULL);
        return;
      }
      // Currently: current publisher is refused, and old publisher is stopped
      //            (asynchronous). Next Start() will succeed.
      // Alternative: remember this Start() call in a Closure*
      // pending_start_ = NewCallback(this, &RtmpPublishingElement::Start,
      //    subpath, auth_req, command, close_publisher, completed);
      // and run pending_start_ after EOS.

      // close old publisher, we will receive EOS asynchronous
      close_publisher_->Run();
      close_publisher_ = NULL;
      return;
    }
    //we're attached to the calling publisher
    close_publisher_ = close_publisher;

    // if no authorizer, simply complete with success
    if ( authorizer_name_.empty() ) {
      AuthorizationCompleted(NULL, new AuthorizerReply(true),
                             command, completed);
      return;
    }
    Authorizer* const auth = mapper_->GetAuthorizer(authorizer_name_);
    if ( auth == NULL ) {
      ILOG_ERROR << "Cannot find authorizer: " << authorizer_name_;
      AuthorizationCompleted(NULL, new AuthorizerReply(true),
                             command, completed);
      return;
    }

    // authorize
    AuthorizerReply* areply = new AuthorizerReply(false);
    auth->IncRef();
    auth->Authorize(auth_req, areply,
                    NewCallback(this,
                                &RtmpPublishingData::AuthorizationCompleted,
                                auth, areply, command, completed));
  }
  void AuthorizationCompleted(Authorizer* auth,
                              AuthorizerReply* areply,
                              string command,
                              Callback1<ProcessingCallback*>* completed) {
    const bool allowed = areply->allowed_;
    delete areply;

    if ( auth != NULL ) {
      auth->DecRef();
    }

    if ( !allowed ) {
      // detach publisher
      ILOG_ERROR << "Authorization failed";
      close_publisher_ = NULL;
      completed->Run(NULL);
      return;
    }

    // possibly create the saver
    if ( save_dir_ != "" && saver_ == NULL &&
         (command == "record" || command == "append") ) {
      timer::Date now(false);
      string dirname(string("publish_") + now.ToShortString());
      if ( append_only_ ) {
        dirname = "";
      } else if ( command == "append" ) {
        re::RE refile("^publish_.*");
        vector<string> dirs;
        io::DirList(save_dir_, &dirs, true, &refile);
        if ( !dirs.empty() ) {
          dirname = dirs.back();
        }
      }
      saver_ = new Saver(
          name_, NULL, caps_.tag_type_, name_,
          save_dir_ + "/" + dirname,
          now.GetTime(), false, NULL);
    }

    // possibly start the saver
    if ( saver_ != NULL && !saver_->IsSaving() ) {
      if ( !saver_->StartSaving() ) {
        ILOG_ERROR << "Cannot start saver on publishing: "
                   << saver_->name() << " in dir: " << saver_->media_dir();
      }
      saver_->CreateSignalingFile(".save_done",
                                  strutil::StringPrintf("buildup\n%d\n%d",
                                                        buildup_interval_sec_,
                                                        buildup_delay_sec_));
    }

    completed->Run(processing_callback_);
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

    delete splitter_;
    splitter_ = NULL;

    tag_timeout_alarm_.Stop();

    delete saver_;
    saver_ = NULL;
    data_.Clear();
  }

 private:
  void ProcessTag(const Tag* tag, int64 timestamp_ms) {
    // possibly create the splitter
    if ( splitter_ == NULL ) {
      splitter_ = splitter_creator_->CreateSplitter(name_, caps_.tag_type_);
      if ( splitter_ == NULL ) {
        ILOG_ERROR << "Cannot create splitter for: name_: [" << name_
                   << "], caps_: " << caps_.ToString();
        CloseRequests(false);
        return;
      }
    }

    MaybeReregisterTagTimeout(true);

    // process the tag
    if ( tag->type() == streaming::Tag::TYPE_EOS ) {
      // publisher signaled EOS, so it's closed already. Forget about
      // close_publisher_
      LOG_INFO << "EOS received: " << tag->ToString();
      close_publisher_ = NULL;
      CloseRequests(false);
      return;
    }

    if ( saver_ != NULL ) {
      saver_->ProcessTag(tag, timestamp_ms);
    }
    distributor_.DistributeTag(tag, timestamp_ms);
  }

  // Helper to register timeout on tags that we receive
  void MaybeReregisterTagTimeout(bool force) {
    if ( force || tag_timeout_alarm_.last_start_ts() +
           kTagTimeoutRegistrationGracePeriodMs < selector_->now() ) {
      tag_timeout_alarm_.Start();
    }
  }

  //////////////////////////////////////////////////////////////////////

  net::Selector* const selector_;
  ElementMapper* mapper_;
  const string name_;
  const Capabilities caps_;
  const string authorizer_name_;
  const string save_dir_;
  const bool append_only_;
  const bool disable_preemption_;
  const int32 prefill_buffer_ms_;
  const int32 advance_media_ms_;
  const int32 buildup_interval_sec_;
  const int32 buildup_delay_sec_;
  io::MemoryStream data_;

  Closure* close_publisher_;  // The publisher which is streaming now / NULL

  streaming::SplitterCreator* const splitter_creator_;
  TagSplitter* splitter_;
  TagDistributor distributor_;

  Saver* saver_;

  streaming::ProcessingCallback* processing_callback_;

  util::Alarm tag_timeout_alarm_;

  bool is_closing_;

  // just for logging
  string info_;

  DISALLOW_EVIL_CONSTRUCTORS(RtmpPublishingData);
};

//////////////////////////////////////////////////////////////////////

const char RtmpPublishingElement::kElementClassName[] = "rtmp_publishing";

RtmpPublishingElement::RtmpPublishingElement(
    const string& name,
    const string& id,
    ElementMapper* mapper,
    net::Selector* selector,
    const string& media_dir,
    const string& rpc_path,
    rpc::HttpServer* rpc_server,
    io::StateKeepUser* state_keeper,
    streaming::SplitterCreator* splitter_creator,
    const string& authorizer_name)
    : BaseClass(kElementClassName, name, id, mapper,
                selector, media_dir,
                rpc_path, rpc_server, state_keeper, splitter_creator,
                authorizer_name),
      info_(string("[RtmpPublishingElement ") + name + "]: ") {
}

RtmpPublishingElement::~RtmpPublishingElement() {
}

bool RtmpPublishingElement::Initialize() {
  return BaseClass::Initialize();
}

void  RtmpPublishingElement::Close(Closure* call_on_close) {
  BaseClass::Close(call_on_close);
}

//////////////////////////////

RtmpPublishingData* RtmpPublishingElement::CreateNewImport(
    const char* import_name,
    const char* full_name,
    Tag::Type tag_type,
    const char* save_dir,
    bool append_only,
    bool disable_preemption,
    int32 prefill_buffer_ms,
    int32 advance_media_ms,
    int32 buildup_interval_sec,
    int32 buildup_delay_sec) {
  return new RtmpPublishingData(
      selector_, full_name,
      tag_type, save_dir == NULL ? "" : save_dir, splitter_creator_,
      authorizer_name_,
      mapper_,
      append_only,
      disable_preemption,
      prefill_buffer_ms,
      advance_media_ms,
      buildup_interval_sec,
      buildup_delay_sec);
}

}
