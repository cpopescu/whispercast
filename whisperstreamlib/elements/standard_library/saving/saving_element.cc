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

#include <whisperlib/common/base/date.h>
#include <whisperstreamlib/elements/standard_library/saving/saving_element.h>

namespace streaming {
const char SavingElement::kElementClassName[] = "saving";
const uint64 SavingElement::kReconnectDelay = 5000;

SavingElement::SavingElement(const string& name,
                             ElementMapper* mapper,
                             net::Selector* selector,
                             const string& base_media_dir,
                             const string& media,
                             const string& save_dir)
  : Element(kElementClassName, name, mapper),
    selector_(selector),
    base_media_dir_(base_media_dir),
    media_(media),
    save_dir_(save_dir),
    internal_req_(NULL),
    process_tag_(NewPermanentCallback(this, &SavingElement::ProcessTag)),
    saver_(NULL),
    open_media_alarm_(*selector) {
  open_media_alarm_.Set(NewPermanentCallback(this, &SavingElement::OpenMedia),
      true, kReconnectDelay, false, false);
}
SavingElement::~SavingElement() {
  CloseMedia();
  CHECK_NULL(saver_);
  CHECK_NULL(internal_req_);
  delete process_tag_;
  process_tag_ = NULL;
}

bool SavingElement::Initialize() {
  OpenMedia();
  return true;
}
bool SavingElement::AddRequest(const string& media, Request* req,
                               ProcessingCallback* callback) {
  LOG_ERROR << "SavingElement cannot serve requests";
  return false;
}
void SavingElement::RemoveRequest(streaming::Request* req) {
  LOG_ERROR << "SavingElement cannot serve requests";
}
bool SavingElement::HasMedia(const string& media) {
  return false;
}
void SavingElement::ListMedia(const string& media, vector<string>* out) {
}
bool SavingElement::DescribeMedia(const string& media,
                                  MediaInfoCallback* callback) {
  return false;
}
void SavingElement::Close(Closure* call_on_close) {
  CloseMedia();
  if ( call_on_close != NULL ) {
    selector_->RunInSelectLoop(call_on_close);
  }
}

void SavingElement::ProcessTag(const Tag* tag, int64 timestamp_ms) {
  if ( tag->type() == streaming::Tag::TYPE_EOS ) {
    CloseMedia();
    return;
  }
  CHECK_NOT_NULL(saver_);
  saver_->ProcessTag(tag, timestamp_ms);
}

void SavingElement::OpenMedia() {
  CHECK_NULL(internal_req_);
  internal_req_ = new streaming::Request();

  if ( !mapper_->AddRequest(media_.c_str(), internal_req_, process_tag_) ) {
    LOG_ERROR << name() << " failed to register to media: [" << media_ << "]";
    delete internal_req_;
    internal_req_ = NULL;
    open_media_alarm_.Start();
    return;
  }

  CHECK_NULL(saver_);
  saver_ = new Saver(name(), // the same name as the element, just for logging
                     NULL, // NO mapper -> we send the tags manually into saver
                     MFORMAT_FLV,
                     media_,
                     strutil::JoinPaths(base_media_dir_, save_dir_),
                     NULL);
  if ( !saver_->StartSaving(0) ) {
    LOG_ERROR << name() << " Cannot start saver on dir: ["
              << saver_->media_dir() << "]";
    CloseMedia();
    return;
  }
}
void SavingElement::CloseMedia() {
  if ( internal_req_ != NULL ) {
    mapper_->RemoveRequest(internal_req_, process_tag_);
    internal_req_ = NULL;
  }
  delete saver_;
  saver_ = NULL;
}

}
