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
// TODO [cpopescu] - important:
//   -- support Pause / Unpause !!
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

#define ILOG(level)  LOG(level) << Info() << ": "
#ifdef _DEBUG
#define ILOG_DEBUG   ILOG(INFO)
#else
#define ILOG_DEBUG   if (false) ILOG(INFO)
#endif
#define ILOG_INFO    ILOG(INFO)
#define ILOG_WARNING ILOG(WARNING)
#define ILOG_ERROR   ILOG(ERROR)
#define ILOG_FATAL   ILOG(FATAL)

namespace {
void AuthorizationCompleted(streaming::Authorizer* auth,
                            streaming::AuthorizerReply* areply,
                            Callback1<bool>* completion_callback) {
  const bool allowed = areply->allowed_;
  delete areply;
  auth->DecRef();
  completion_callback->Run(allowed);
}
}
namespace streaming {

class RtmpPublishingData {
 public:
  RtmpPublishingData(net::Selector* selector,
                     rtmp::StreamManager* stream_manager,
                     const string& name,
                     Tag::Type tag_type,
                     const char* save_dir,
                     streaming::SplitterCreator* splitter_creator,
                     bool append_only,
                     bool disable_preemption,
                     int32 prefill_buffer_ms,
                     int32 advance_media_ms,
                     int32 buildup_interval_sec,
                     int32 buildup_delay_sec);


  ~RtmpPublishingData();

  string Info() const {
    return strutil::StringPrintf("[RtmpPublishingData %s]: ",
                                 name_.c_str());
  }

  bool AddCallback(streaming::Request* request,
                   streaming::ProcessingCallback* callback);
  void RemoveCallback(streaming::Request* request);

  bool IsStreaming() const {
    return stream_params_ != NULL;
  }
  const TagSplitter* splitter() const { return splitter_; }
  void Close() {
    is_closing_ = true;
    CloseRequest(true);
  }
  bool disable_preemption() const {
    return disable_preemption_;
  }
 private:
  void CloseRequest(bool send_eos);
  bool InitializeSplitter();

 public:
  // The media tag type we produce (mainly :)
  const streaming::Capabilities& caps() const { return caps_; }
  // If non empty we can save as we publish
  const string& save_dir() const { return save_dir_; }

  streaming::ProcessingCallback* StartPublisher(string name,
      const rtmp::StreamParams* params) {
    if ( IsStreaming() ) {
      ILOG_INFO << "Preempting a request out .. ";
      CloseRequest(false);
    }
    CHECK(stream_params_ == NULL);
    stream_params_ = params;
    return processing_callback_;
  }

 private:
  void ProcessTag(const Tag* tag);

  // Helper to register timeout on tags that we receive
  void MaybeReregisterTagTimeout(bool force);

  //////////////////////////////////////////////////////////////////////

  net::Selector* const selector_;
  rtmp::StreamManager* const stream_manager_;
  const string name_;
  const Capabilities caps_;
  const string save_dir_;
  const bool append_only_;
  const bool disable_preemption_;
  const int32 prefill_buffer_ms_;
  const int32 advance_media_ms_;
  const int32 buildup_interval_sec_;
  const int32 buildup_delay_sec_;
  io::MemoryStream data_;

  bool is_first_tag_;
  int64 first_tag_time_;
  int64 start_decode_time_;
  int64 start_stream_time_;

  const rtmp::StreamParams* stream_params_;  // what is streaming now / NULL

  streaming::SplitterCreator* const splitter_creator_;
  TagSplitter* splitter_;
  TagDistributor distributor_;

  Saver* saver_;

  streaming::ProcessingCallback* const processing_callback_;

  static const int kTagTimeoutMs = 20000;
  static const int kTagTimeoutRegistrationGracePeriodMs = 1000;

  int64 last_tag_timeout_registration_time_;
  Closure* const tag_timeout_callback_;

  bool is_closing_;

  DISALLOW_EVIL_CONSTRUCTORS(RtmpPublishingData);
};

RtmpPublishingData::RtmpPublishingData(
    net::Selector* selector,
    rtmp::StreamManager* stream_manager,
    const string& name,
    Tag::Type tag_type,
    const char* save_dir,
    streaming::SplitterCreator* splitter_creator,
    bool append_only,
    bool disable_preemption,
    int32 prefill_buffer_ms,
    int32 advance_media_ms,
    int32 buildup_interval_sec,
    int32 buildup_delay_sec)
    : selector_(selector),
      stream_manager_(stream_manager),
      name_(name),
      caps_(tag_type, kDefaultFlavourMask),
      save_dir_(save_dir != NULL ? save_dir : ""),
      append_only_(append_only),
      disable_preemption_(disable_preemption),
      prefill_buffer_ms_(prefill_buffer_ms),
      advance_media_ms_(advance_media_ms),
      buildup_interval_sec_(buildup_interval_sec),
      buildup_delay_sec_(buildup_delay_sec),
      is_first_tag_(true),
      first_tag_time_(0),
      start_decode_time_(0),
      start_stream_time_(0),
      stream_params_(NULL),
      splitter_creator_(splitter_creator),
      splitter_(NULL),
      distributor_(kDefaultFlavourMask, name),
      saver_(NULL),
      processing_callback_(NewPermanentCallback(this,
          &RtmpPublishingData::ProcessTag)),
      last_tag_timeout_registration_time_(0),
      tag_timeout_callback_(NewPermanentCallback(this,
          &RtmpPublishingData::CloseRequest, false)),
      is_closing_(false) {
}

RtmpPublishingData::~RtmpPublishingData() {
  delete processing_callback_;
  delete tag_timeout_callback_;
}

bool RtmpPublishingData::AddCallback(
    streaming::Request* request,
    streaming::ProcessingCallback* callback) {
  distributor_.add_callback(request, callback);
  return true;
}
void RtmpPublishingData::RemoveCallback(streaming::Request* request) {
  distributor_.remove_callback(request);
}

bool RtmpPublishingData::InitializeSplitter() {
  CHECK(splitter_ == NULL);
  splitter_ = splitter_creator_->CreateSplitter(name_, caps_.tag_type_);
  if ( !splitter_ ) {
    ILOG_ERROR << "Failed to create splitter";
    return false;
  }
  ILOG_INFO << "Splitter " << splitter_ << "("
            << splitter_->type_name() << ") created";
  MaybeReregisterTagTimeout(true);
  return true;
}

void RtmpPublishingData::CloseRequest(bool send_eos) {
  ILOG_INFO << "CloseRequest, send_eos: " << send_eos
            << ", is_closing_: " << is_closing_
            << ", callbacks: " << distributor_.count();

  if ( send_eos ) {
    distributor_.CloseAllCallbacks(is_closing_);
  } else {
    distributor_.Reset();
  }

  delete splitter_;
  splitter_ = NULL;

  selector_->UnregisterAlarm(tag_timeout_callback_);

  delete saver_;
  saver_ = NULL;
  data_.Clear();

  start_stream_time_ = selector_->now();
  is_first_tag_ = true;
  start_decode_time_ = 0;
  first_tag_time_ = 0;
  // Marking the stop of publishing
  stream_params_ = NULL;
}

void RtmpPublishingData::MaybeReregisterTagTimeout(bool force) {
  if ( force
       || ((last_tag_timeout_registration_time_ +
            kTagTimeoutRegistrationGracePeriodMs) < selector_->now()) ) {
    last_tag_timeout_registration_time_ = selector_->now();
    selector_->RegisterAlarm(tag_timeout_callback_,
                               kTagTimeoutMs + prefill_buffer_ms_);
  }
}

void RtmpPublishingData::ProcessTag(const Tag* tag) {
  // possibly create the splitter
  if ( splitter_ == NULL ) {
    if ( !InitializeSplitter() ) {
      CloseRequest(false);
      return;
    }
  }
  MaybeReregisterTagTimeout(true);

  // possibly create the saver
  if ( !save_dir_.empty() && saver_ == NULL &&
       stream_params_ != NULL &&
       (stream_params_->command_ == "record" ||
        stream_params_->command_ == "append") ) {
    timer::Date now(false);
    string dirname(string("publish_") + now.ToShortString());
    if ( append_only_ ) {
      dirname = "";
    } else if ( stream_params_->command_ == "append" ) {
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

  // process the tag
  if ( tag->type() == streaming::Tag::TYPE_EOS ) {
    CloseRequest(false);
    return;
  }
  if ( tag->type() == streaming::Tag::TYPE_RAW ) {
    const RawTag* raw_tag = static_cast<const RawTag*>(tag);
    data_.AppendStreamNonDestructive(raw_tag->data());

    if ( start_decode_time_ == 0 ) {
      if ( (selector_->now() - start_stream_time_) <
           prefill_buffer_ms_ ) {
        return;
      }
      start_decode_time_ = selector_->now();
    }

    while ( true ) {
      scoped_ref<Tag> tag;
      TagReadStatus status = splitter_->GetNextTag(&data_, &tag, false);
      if ( status == streaming::READ_SKIP ) {
        continue;
      }
      if ( status == streaming::READ_NO_DATA ) {
        // TODO: verify data_.Size() for max tag size..
        break;
      }
      if ( status != streaming::READ_OK ) {
        ILOG_ERROR << "Error reading next tag, status: "
                   << TagReadStatusName(status);
        CloseRequest(false);
        return;
      }

      distributor_.DistributeTag(tag.get());

      if ( saver_ != NULL ) {
        saver_->ProcessTag(tag.get());
      }

      if ( is_first_tag_ ) {
        is_first_tag_ = false;
        first_tag_time_ = tag->timestamp_ms();
        continue;
      }

      const int64 delta = (tag->timestamp_ms() - first_tag_time_) -
                          ( selector_->now() - start_decode_time_);
      if ( delta > advance_media_ms_ ) {
        break;
      }
    }
    return;
  }

  // other tag types, simply forward them
  distributor_.DistributeTag(tag);
}

//////////////////////////////////////////////////////////////////////

const char RtmpPublishingElement::kElementClassName[] = "rtmp_publishing";

RtmpPublishingElement::RtmpPublishingElement(
    const char* name,
    const char* id,
    ElementMapper* mapper,
    net::Selector* selector,
    const char* media_dir,
    rtmp::StreamManager* stream_manager,
    const char* rpc_path,
    rpc::HttpServer* rpc_server,
    io::StateKeepUser* state_keeper,
    streaming::SplitterCreator* splitter_creator,
    const char* authorizer_name)
    : BaseClass(kElementClassName, name, id, mapper,
                selector, media_dir,
                rpc_path, rpc_server, state_keeper, splitter_creator,
                authorizer_name),
      rtmp::StreamPublisher(name),
      stream_manager_(stream_manager),
      rtmp_registered_(false) {
}

RtmpPublishingElement::~RtmpPublishingElement() {
  if ( rtmp_registered_ ) {
    if ( stream_manager_->UnregisterPublishingStream(this) ) {
      ILOG_ERROR << "Error unregistering published stream";
    }
  }
}

bool RtmpPublishingElement::Initialize() {
  if ( !BaseClass::Initialize() ) {
    return false;
  }
  rtmp_registered_ = stream_manager_->RegisterPublishingStream(this);
  return rtmp_registered_;
}

void  RtmpPublishingElement::Close(Closure* call_on_close) {
  if ( rtmp_registered_ ) {
    if ( !stream_manager_->UnregisterPublishingStream(this) ) {
      ILOG_ERROR << "Error unregistering published stream";
    }
    rtmp_registered_ = false;
  }
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
      selector_, stream_manager_, full_name,
      tag_type, save_dir, splitter_creator_,
      append_only,
      disable_preemption,
      prefill_buffer_ms,
      advance_media_ms,
      buildup_interval_sec,
      buildup_delay_sec);
}

streaming::ProcessingCallback* RtmpPublishingElement::StartPublisher(
    string name, const rtmp::StreamParams* params) {
  ImportMap::iterator it = imports_.find(name);
  if ( it == imports_.end() ) {
    ILOG_ERROR << "Cannot find import stream: " << name;
    return NULL;
  }
  if ( it->second->IsStreaming() ) {
    if ( it->second->disable_preemption() ) {
      ILOG_ERROR << "Import stream already publishing: "
                 << name;
      return NULL;
    }
  }
  return it->second->StartPublisher(name, params);
}

void RtmpPublishingElement::Authorize(string name,
    const rtmp::StreamParams* params, Callback1<bool>* completion_callback) {
  if ( authorizer_name_.empty() ) {
    completion_callback->Run(true);
  } else {
    streaming::Authorizer* const auth =
        mapper_->GetAuthorizer(authorizer_name_);
    if ( auth == NULL ) {
      completion_callback->Run(true);
    } else {
      streaming::AuthorizerReply* areply = new AuthorizerReply(false);
      auth->IncRef();
      auth->Authorize(params->auth_req_, areply,
                      NewCallback(&AuthorizationCompleted,
                                  auth, areply, completion_callback));
    }
  }
}

void RtmpPublishingElement::CanStopPublishing(string name,
    const rtmp::StreamParams* params,
    Callback1<bool>* completion_callback) {
  ImportMap::iterator it = imports_.find(name);
  if ( it == imports_.end() ) {
    completion_callback->Run(true);
  } else {
    if ( !it->second->IsStreaming() ) {
      completion_callback->Run(true);
    } else {
      Authorize(name, params, completion_callback);
    }
  }
}

void RtmpPublishingElement::CanStartPublishing(string name,
    const rtmp::StreamParams* params,
    Callback1<bool>* completion_callback) {
  ImportMap::iterator it = imports_.find(name);
  if ( it == imports_.end() ) {
    completion_callback->Run(false);
  } else {
    if ( it->second->IsStreaming() &&
         it->second->disable_preemption() ) {
      completion_callback->Run(false);
    } else {
      Authorize(name, params, completion_callback);
    }
  }
}
string RtmpPublishingElement::Info() const {
  return strutil::StringPrintf("[RtmpPublishingElement %s]: ", name().c_str());
}

}
