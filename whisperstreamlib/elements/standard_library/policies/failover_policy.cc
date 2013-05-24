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

#include "elements/standard_library/policies/failover_policy.h"
namespace streaming {

const char FailoverPolicy::kPolicyClassName[] = "failover_policy";

FailoverPolicy::FailoverPolicy(const string& name,
                               PolicyDrivenElement* element,
                               ElementMapper* mapper,
                               net::Selector* selector,
                               const string& main_media,
                               const string& failover_media,
                               int32 main_media_tags_received_switch_limit,
                               int32 failover_timeout_sec,
                               bool change_to_main_only_on_switch)
    : Policy(kPolicyClassName, name, element),
      mapper_(mapper),
      selector_(selector),
      main_media_(main_media),
      failover_media_(failover_media),
      main_media_tags_received_switch_limit_(
          main_media_tags_received_switch_limit),
      failover_timeout_ms_(failover_timeout_sec * 1000),
      change_to_main_only_on_switch_(change_to_main_only_on_switch),
      main_media_tags_received_(false),
      internal_req_(NULL),
      process_tag_callback_(NULL),
      open_media_callback_(NewPermanentCallback(
                               this, &FailoverPolicy::OpenMedia)),
      tag_timeout_callback_(
          NewPermanentCallback(this, &FailoverPolicy::TagReceiveTimeout)),
      last_tag_timeout_registration_time_(0) {
}

FailoverPolicy::~FailoverPolicy() {
  CloseMedia();

  delete open_media_callback_;
  selector_->UnregisterAlarm(tag_timeout_callback_);
  delete tag_timeout_callback_;
  tag_timeout_callback_ = NULL;
}

bool FailoverPolicy::Initialize() {
  OpenMedia();
  return true;
}

bool FailoverPolicy::MaybeReregisterTagTimeout(bool force,
                                               int32 tag_timeout) {
  if ( tag_timeout > 0
       && (force
           || ((last_tag_timeout_registration_time_ +
                kTagTimeoutRegistrationGracePeriodMs) < selector_->now())) ) {
    last_tag_timeout_registration_time_ = selector_->now();
    selector_->RegisterAlarm(tag_timeout_callback_, tag_timeout);
    return true;
  }
  return false;
}

void FailoverPolicy::OpenMedia() {
  CHECK(process_tag_callback_ == NULL);
  CHECK(internal_req_ == NULL);
  process_tag_callback_ =
      NewPermanentCallback(this, &FailoverPolicy::ProcessMainTag);
  internal_req_ = new streaming::Request();
  if ( !mapper_->AddRequest(main_media_.c_str(),
                            internal_req_,
                            process_tag_callback_) ) {
    LOG_ERROR << name() << " faild to add itself to: " << main_media_;
    delete process_tag_callback_;
    process_tag_callback_ = NULL;

    delete internal_req_;
    internal_req_ = NULL;
    selector_->RegisterAlarm(open_media_callback_, kRetryOpenMediaTimeMs);
  } else {
    MaybeReregisterTagTimeout(true, failover_timeout_ms_);
    if ( current_media_ != main_media_ ) {
      current_media_ = main_media_;
      element_->SwitchCurrentMedia(main_media_, NULL, false);
    }
  }
}


void FailoverPolicy::CloseMedia() {
 if ( process_tag_callback_ != NULL ) {
    CHECK_NOT_NULL(internal_req_);

    mapper_->RemoveRequest(internal_req_, process_tag_callback_);
    internal_req_ = NULL;

    delete process_tag_callback_;
    process_tag_callback_ = NULL;
  }
}

void FailoverPolicy::TagReceiveTimeout() {
  main_media_tags_received_ = 0;
  current_media_ = failover_media_;
  element_->SwitchCurrentMedia(failover_media_, NULL, false);
}

bool FailoverPolicy::NotifyEos() {
  if ( current_media_ != main_media_ &&
       main_media_tags_received_ > main_media_tags_received_switch_limit_ ) {
    current_media_ = main_media_;
    element_->SwitchCurrentMedia(main_media_, NULL, false);
  } else {
    element_->SwitchCurrentMedia(current_media_, NULL, false);
  }
  return true;
}

bool FailoverPolicy::NotifyTag(const Tag* tag, int64 timestamp_ms) {
  if ( current_media_ != main_media_ &&
       tag->type() == streaming::Tag::TYPE_SOURCE_STARTED &&
       main_media_tags_received_ > main_media_tags_received_switch_limit_ ) {
    current_media_ = main_media_;
    element_->SwitchCurrentMedia(main_media_, NULL, false);
  }
  return true;
}
void FailoverPolicy::Reset() {
}
string FailoverPolicy::GetPolicyConfig() {
  return "";
}

void FailoverPolicy::ProcessMainTag(const Tag* tag, int64 timestamp_ms) {
  if ( tag->type() == streaming::Tag::TYPE_EOS ) {
    CloseMedia();
    selector_->RegisterAlarm(open_media_callback_, 100);
    main_media_tags_received_ = false;
  } else {
    MaybeReregisterTagTimeout(false, failover_timeout_ms_);
    ++main_media_tags_received_;
    if ( current_media_ != main_media_ &&
         main_media_tags_received_ > main_media_tags_received_switch_limit_ &&
         !change_to_main_only_on_switch_ ) {
      current_media_ = main_media_;
      element_->SwitchCurrentMedia(main_media_, NULL, false);
    }
  }
}
}
