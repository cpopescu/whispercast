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

#include <whisperlib/common/base/timer.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperstreamlib/elements/standard_library/switching/switching_element.h>
#include <whisperstreamlib/rtmp/objects/rtmp_objects.h>
#include <whisperstreamlib/rtmp/objects/amf/amf_util.h>

#define ILOG(level)  LOG(level) << name() << " " << this << ": "
#ifdef _DEBUG
#define ILOG_DEBUG   ILOG(INFO)
#else
#define ILOG_DEBUG   if (false) ILOG(INFO)
#endif
#define ILOG_INFO    ILOG(INFO)
#define ILOG_WARNING ILOG(WARNING)
#define ILOG_ERROR   ILOG(ERROR)
#define ILOG_FATAL   ILOG(FATAL)

DEFINE_int32(switching_default_write_ahead_ms,
             1000,
             "We keep the stream in the switching element "
             "this much ahead of the real time by default");

DEFINE_int32(switching_max_write_ahead_ms,
             5000,
             "We keep the stream in the switching element "
             "at most this much ahead of the real time");

namespace streaming {

const char SwitchingElement::kElementClassName[] = "switching";

////////////////////////////////////////////////////////////////////////////////
//
// SwitchingElement implementation
//
SwitchingElement::SwitchingElement(const char* _name,
                                   const char* id,
                                   ElementMapper* mapper,
                                   net::Selector* selector,
                                   const char* rpc_path,
                                   rpc::HttpServer* rpc_server,
                                   const streaming::Capabilities& caps,
                                   int default_flavour_id,
                                   int32 tag_timeout_ms,
                                   int32 write_ahead_ms,
                                   bool media_only_when_used,
                                   bool save_media_bootstrap)
    : PolicyDrivenElement(kElementClassName, _name, id, mapper),
      ServiceInvokerSwitchingElementService(
          ServiceInvokerSwitchingElementService::GetClassName()),
      caps_(caps),
      tag_timeout_ms_(tag_timeout_ms),
      write_ahead_ms_(write_ahead_ms),
      req_(NULL),
      req_info_(NULL),
      selector_(selector),
      rpc_path_(rpc_path),
      rpc_server_(rpc_server),
      media_only_when_used_(media_only_when_used),
      normalizer_(selector, FLAGS_switching_max_write_ahead_ms),
      process_tag_callback_(
          NewPermanentCallback(this, &SwitchingElement::ProcessTag)),
      tag_timeout_alarm_(*selector),
      last_tag_timeout_registration_time_(0),
      register_alarm_(*selector),
      stream_ended_alarm_(*selector),
      close_completed_(NULL) {
  for ( uint32 i = 0; i < NUMBEROF(distributors_); i++ ) {
    if ( caps_.flavour_mask_ & (1 << i) ) {
      distributors_[i] = new TagDistributor(1 << i, name());
    } else {
      distributors_[i] = NULL;
    }
  }
  // prepare the timeout alarm. But don't start it yet.
  // If tag_timeout_ms_ == 0, never timeout! this alarm is disabled.
  if ( tag_timeout_ms_ > 0 ) {
    tag_timeout_alarm_.Set(
        NewPermanentCallback(this, &SwitchingElement::TagReceiveTimeout),
        true, tag_timeout_ms_, false, false);
  }
  // prepare StreamEnded alarm. Don't start it.
  stream_ended_alarm_.Set(
      NewPermanentCallback(this, &SwitchingElement::StreamEnded),
      true, 0, false, false);
}

// DOES: nothing important, just free resources.
SwitchingElement::~SwitchingElement() {
  CHECK(!is_registered()) << "name: " << name();
  if ( rpc_server_ != NULL ) {
    rpc_server_->UnregisterService(rpc_path_, this);
  }

  delete process_tag_callback_;
  process_tag_callback_ = NULL;

  for ( int i = 0; i < NUMBEROF(distributors_); ++i ) {
    // A previous Close() should have cleaned everything
    if ( distributors_[i] != NULL ) {
      CHECK_EQ(distributors_[i]->count(), 0);
      delete distributors_[i];
      distributors_[i] = NULL;
    }
  }
}

uint32 SwitchingElement::CountClients() const {
  uint32 count = 0;
  for ( uint32 i = 0; i < NUMBEROF(distributors_); i++ ) {
    if ( distributors_[i] != NULL ) {
      count += distributors_[i]->count();
    }
  }
  return count;
}

// DOES: AddRequest upstream
void SwitchingElement::Register(string media_name) {
  if ( close_completed_ != NULL ) {
    LOG_WARNING << "Refusing Register() to: " << media_name
                << " because closing is in progress.";
    return;
  }
  CHECK(!is_registered()) << " Double register";

  ILOG_DEBUG << "Registering to: [" << media_name << "]";
  URL crt_url(string("http://x/") + media_name);
  req_ = new streaming::Request(crt_url);
  if ( req_info_ != NULL ) {
    *req_->mutable_info() = *req_info_;
  }
  req_->mutable_info()->internal_id_ = id();
  *req_->mutable_caps() = caps_;

  if (req_->info().write_ahead_ms_ <= 0 ) {
    req_->mutable_info()->write_ahead_ms_ =
        FLAGS_switching_default_write_ahead_ms;
  }

  normalizer_.Reset(req_);

  if ( !mapper_->AddRequest(req_->info().path_.c_str(),
                            req_, process_tag_callback_) ) {
    ILOG_ERROR << "Failed to register to: [" << media_name << "]";
    return;
  }
  ILOG_DEBUG << "Registered to media: [" << media_name << "], req: "
             << req_->ToString();

  current_media_ = media_name;
  MaybeReregisterTagTimeout(true);
  return;
}

// DOES: RemoveRequest upstream, maybe send SourceEnded downstream
void SwitchingElement::Unregister(bool send_source_ended, bool send_flush) {
  tag_timeout_alarm_.Stop();
  register_alarm_.Stop();
  if ( !is_registered() ) {
    return;
  }
  ILOG_INFO << "Unregistering from [" << current_media_ << "]";

  streaming::Request* req = req_;
  req_ = NULL;
  normalizer_.Reset(NULL);

  delete req_info_;
  req_info_ = NULL;

  current_media_ = "";
  mapper_->RemoveRequest(req, process_tag_callback_);

  // send the end stream tags to the clients
  // which were compatible with the upstream flavour
  uint32 flavour_mask = req->caps().flavour_mask_;
  while ( flavour_mask ) {
    const int id = RightmostFlavourId(flavour_mask);

    if ( send_flush ) {
      scoped_ref<Tag> flush_tag(new FlushTag(0, 1 << id));
      distributors_[id]->DistributeTag(
        flush_tag.get(), distributors_[id]->last_tag_ts());
    }

    if (send_source_ended) {
      distributors_[id]->Reset();
    } else {
      distributors_[id]->Switch();
    }
  }
}

// DOES: send SourceEnded downstream, RemoveRequest upstream
bool SwitchingElement::SwitchCurrentMedia(const string& target_media_name,
                                          const RequestInfo* req_info,
                                          bool force_switch) {
  if ( current_media_ == target_media_name && !force_switch ) {
    ILOG_WARNING << "Cannot switch to same element: ["
                 << target_media_name << "]";
    return true;
  }
  ILOG_INFO << "Switching from [" << current_media() << "] to ["
            << target_media_name << "]";

  Unregister(false, true);
  if ( target_media_name.empty() ) {
    // Basically it says - wait man ...
    return true;
  }
  if ( req_info != NULL ) {
    delete req_info_;
    req_info_ = new RequestInfo(*req_info);
  }

  if ( media_only_when_used_ && CountClients() == 0 ) {
    ILOG_INFO << "No clients right now, reserving registration to: ["
              << target_media_name << "] for later";
    media_name_to_register_ = target_media_name;
    return true;
  }
  CHECK_GE(selector_->now(), register_alarm_.last_fire_ts());
  int64 delay_register = max((int64)0LL, kRegisterMinIntervalMs -
      (selector_->now() - register_alarm_.last_fire_ts()));
  register_alarm_.Set(NewPermanentCallback(this,
      &SwitchingElement::Register, target_media_name),
      true, delay_register, false, true);
  return true;
}

// DOES: send SourceEnded & EOS downstream
void SwitchingElement::CloseAllClients(bool forced) {
  // if there's nothing to close, complete close now
  if ( CountClients() == 0 && close_completed_ != NULL ) {
    selector_->RunInSelectLoop(close_completed_);
    close_completed_ = NULL;
    return;
  }

  // close registered clients
  uint32 flavour_mask = caps_.flavour_mask_;
  while ( flavour_mask ) {
    const int id = RightmostFlavourId(flavour_mask);
    // send SOURCE_ENDED and EOS
    distributors_[id]->CloseAllCallbacks(forced);
  }
  // NEXT: as a consequence of the sent EOS, some RemoveRequest() calls are
  //       expected. There we check if we are completely closed, and call
  //       close_completed_;
}

void SwitchingElement::TagReceiveTimeout() {
  ILOG_ERROR << "Switching element timed out on no tags"
                " received - considering a closed element.. ";
  StreamEnded();
}

//////////////////////////////////////////////////////////////////

// DOES: send tag downstream, notify policy
void SwitchingElement::ProcessTag(const Tag* tag, int64 timestamp_ms) {
  normalizer_.ProcessTag(tag, timestamp_ms);

  if ( tag->type() == streaming::Tag::TYPE_EOS ) {
    ILOG_INFO << "EOS received";
    stream_ended_alarm_.Start();
    return;
  }

  scoped_ref<SourceChangedTag> clone;
  if ( tag->type() == streaming::Tag::TYPE_SOURCE_STARTED ||
       tag->type() == streaming::Tag::TYPE_SOURCE_ENDED ) {
    clone = static_cast<SourceChangedTag*>(tag->Clone());
    clone->set_is_final();

    tag = clone.get();
  }

  if ( policy_ != NULL && !policy_->NotifyTag(tag, timestamp_ms) ) {
    return;
  }

  MaybeReregisterTagTimeout(false);

  uint32 flavour_mask = tag->flavour_mask();
  while ( flavour_mask ) {
    const int id = RightmostFlavourId(flavour_mask);
    if ( distributors_[id] != NULL ) {
      distributors_[id]->DistributeTag(tag, timestamp_ms);
    }
  }
}

// DOES: Send SourceEnded on behalf of upstream elements,
//       RemoveRequest + AddRequest upstream
void SwitchingElement::StreamEnded() {
  if ( !is_registered() ) {
    return;
  }
  ILOG_INFO << "StreamEnded on media: [" << current_media_ << "]";
  Unregister(false, false);

  // If it returns:
  //  true => we maintain this element (and the implicit added callbacks).
  //  false => we unregister the element.
  if ( !policy_->NotifyEos() ) {
    ILOG_INFO << "Media stream ended for current element "
              << current_media() << " -> Closing All Clients !";
    CloseAllClients(false);
    return;
  }
  ILOG_INFO << "Media stream ended for current element "
            << current_media() << " -> Continue !";
}

bool SwitchingElement::Initialize() {
  if ( rpc_server_ != NULL &&
       !rpc_server_->RegisterService(rpc_path_, this) ) {
    return false;
  }
  return true;
}

// DOES: AddRequest upstream
bool SwitchingElement::AddRequest(const char* media_name,
                                  streaming::Request* req,
                                  streaming::ProcessingCallback* callback) {
  if ( name() != media_name ) {
    ILOG_ERROR << "Cannot AddRequest media_name: [" << media_name
               << "] is not the same with our name: [" << name() << "]";
    return false;
  }
  if ( !req->caps().IsCompatible(caps_) ) {
    ILOG_ERROR << "Incompatible caps, req->caps: " << req->caps().ToString()
               << ", ours: " << caps_.ToString();
    return false;
  }
  if ( req->rev_callbacks().find(this) != req->rev_callbacks().end() ||
       req == req_ ) {
    ILOG_ERROR << "Cannot create a loop in switching sources ...";
    return false;
  }
  req->mutable_caps()->IntersectCaps(caps_);

  if ( !media_name_to_register_.empty() ) {
    ILOG_INFO << "Delayed registration for switching element: "
              << " performed now for: " << media_name_to_register_;
    Register(media_name_to_register_);
    media_name_to_register_ = "";
  }

  // add the callback on all the requested flavours
  uint32 flavour_mask = req->caps().flavour_mask_;
  while ( flavour_mask ) {
    const int id = RightmostFlavourId(flavour_mask);
    // add the callback
    distributors_[id]->add_callback(req, callback);
  }
  return true;
}

// DOES: RemoveRequest upstream, notify policy
void SwitchingElement::RemoveRequest(streaming::Request* req) {
  // add the callback on all the requested flavours
  uint32 flavour_mask = req->caps().flavour_mask_;
  while ( flavour_mask ) {
    const int id = RightmostFlavourId(flavour_mask);
    // add the callback
    distributors_[id]->remove_callback(req);
  }

  if ( media_only_when_used_ && CountClients() == 0 ) {
    ILOG_INFO << "Unregistering from: " << current_media_
              << " per empty request set.";
    media_name_to_register_ = current_media_;
    Unregister(true, false);
    if ( policy_ != NULL && !selector_->IsExiting() ) {
      policy_->Reset();
    }
  }
  // maybe signal Close() completion
  if ( CountClients() == 0 && close_completed_ != NULL ) {
    selector_->RunInSelectLoop(close_completed_);
    close_completed_ = NULL;
  }
}

bool SwitchingElement::HasMedia(const char* media, Capabilities* out) {
  if ( name() != media ) {
    return false;
  }
  *out = caps_;
  return true;
}

void SwitchingElement::ListMedia(const char* media_dir,
                                 ElementDescriptions* media) {
  media->push_back(make_pair(name_, caps_));
}
bool SwitchingElement::DescribeMedia(const string& media,
                                     MediaInfoCallback* callback) {
  return mapper_->DescribeMedia(current_media_, callback);
}

// DOES: nothing. Delegate to InternalClose.
void SwitchingElement::Close(Closure* call_on_close) {
  CHECK_NULL(close_completed_);
  CHECK_NOT_NULL(call_on_close);
  close_completed_ = call_on_close;
  selector_->RunInSelectLoop(NewCallback(this,
      &SwitchingElement::InternalClose));
}

// DOES: send SourceEnded + EOS downstream, RemoveRequest upstream
void SwitchingElement::InternalClose() {
  Unregister(true, false);
  CloseAllClients(true);
}

void SwitchingElement::MaybeReregisterTagTimeout(bool force) {
  if ( tag_timeout_alarm_.IsSet() &&
       (force || tag_timeout_alarm_.ms_from_last_start() >
                 kTagTimeoutRegistrationGracePeriodMs) ) {
    tag_timeout_alarm_.Start();
  }
}

}
