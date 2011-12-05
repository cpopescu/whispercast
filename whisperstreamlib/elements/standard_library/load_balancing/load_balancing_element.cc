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

#include <string>

#include <whisperlib/common/base/types.h>
#include WHISPER_HASH_SET_HEADER
#include <whisperstreamlib/base/element.h>

#include "elements/standard_library/load_balancing/load_balancing_element.h"

namespace streaming {

const char LoadBalancingElement::kElementClassName[] = "load_balancing";

LoadBalancingElement::LoadBalancingElement(const char* name,
                                           const char* id,
                                           ElementMapper* mapper,
                                           net::Selector* selector,
                                           const vector<string>& sub_elements)
    : Element(kElementClassName, name, id, mapper),
      selector_(selector),
      close_completed_(NULL) {
  for ( int i = 0; i < sub_elements.size(); ++i ) {
    sub_elements_.push_back(sub_elements[i]);
  }
}
LoadBalancingElement::~LoadBalancingElement() {
  DCHECK(req_map_.empty());
  DCHECK(close_completed_ == NULL);
}
bool LoadBalancingElement::Initialize() {
  if ( sub_elements_.empty() ) {
    return false;
  }
  return true;
}
bool LoadBalancingElement::AddRequest(
    const char* media,
    streaming::Request* req,
    streaming::ProcessingCallback* callback) {
  pair<string, string> media_pair = strutil::SplitFirst(media, '/');
  if ( media_pair.first != name() ) {
    return false;
  }
  ++next_element_;
  if ( next_element_ >= sub_elements_.size() ) next_element_ = 0;

  ReqStruct* rs = new ReqStruct(callback);
  rs->added_callback_ = NewPermanentCallback(this,
                                             &LoadBalancingElement::ProcessTag,
                                             rs);
  for ( int i = 0; i < sub_elements_.size(); ++i ) {
    int ndx = next_element_ + i;
    if ( ndx >= sub_elements_.size() ) {
      ndx -= sub_elements_.size();
    }
    rs->new_element_name_ = sub_elements_[ndx];
    const string crt_name = sub_elements_[ndx] + "/" + media_pair.second;
    if ( mapper_->AddRequest(crt_name.c_str(), req, rs->added_callback_) ) {
      LOG_INFO << name() << " Successfully redirected to: [" << crt_name << "]";
      req_map_.insert(make_pair(req, rs));
      next_element_ = (ndx + 1) >= sub_elements_.size() ? 0 : ndx;
      return true;
    }
    LOG_ERROR << name() << " cannot add media: [" << crt_name
              << "], trying the next prefix...";
  }
  LOG_ERROR << name() << " cannot add media: [" << media_pair.second << "]";
  delete rs;
  return false;
}

void LoadBalancingElement::RemoveRequest(streaming::Request* req) {
  ReqMap::iterator it = req_map_.find(req);
  if ( it == req_map_.end() ) {
    LOG_ERROR << "Cannot find request: " << req->ToString();
    return;
  }
  mapper_->RemoveRequest(req, it->second->added_callback_);
  delete it->second;
  req_map_.erase(it);

  // maybe complete close
  if ( close_completed_ != NULL && req_map_.empty() ) {
    Closure* call_on_close = close_completed_;
    close_completed_ = NULL;

    selector_->RunInSelectLoop(call_on_close);
  }
}

bool LoadBalancingElement::HasMedia(const char* media, Capabilities* out) {
  pair<string, string> media_pair = strutil::SplitFirst(media, '/');
  if ( media_pair.first != name() ) {
    return false;
  }
  for ( int i = 0; i < sub_elements_.size(); ++i ) {
    const string crt_name = strutil::JoinMedia(sub_elements_[i],
        media_pair.second);
    if ( mapper_->HasMedia(crt_name.c_str(), out) ) {
      return true;
    }
  }
  return false;
}
void LoadBalancingElement::ListMedia(const char* media_dir,
                                     streaming::ElementDescriptions* medias) {
  pair<string, string> media_pair = strutil::SplitFirst(media_dir, '/');
  if ( media_pair.first != name() ) {
    return;
  }
  for ( int i = 0; i < sub_elements_.size(); ++i ) {
    streaming::ElementDescriptions crt;
    string crt_name = strutil::JoinMedia(sub_elements_[i], media_pair.second);
    mapper_->ListMedia(crt_name.c_str(), &crt);
    for ( int j = 0; j < crt.size(); ++j ) {
      pair<string, string> crt_pair = strutil::SplitFirst(
          crt[i].first.c_str(), '/');
      medias->push_back(make_pair(name() + "/" + crt_pair.second,
                                  crt[i].second));
    }
  }
}
bool LoadBalancingElement::DescribeMedia(const string& media,
                                         MediaInfoCallback* callback) {
  for ( uint32 i = 0; i < sub_elements_.size(); ++i ) {
    const string new_media = strutil::JoinMedia(sub_elements_[i], media);
    if ( mapper_->DescribeMedia(new_media, callback) ) {
      return true;
    }
  }
  return false;
}
void LoadBalancingElement::Close(Closure* call_on_close) {
  CHECK_NULL(close_completed_);
  if ( req_map_.empty() ) {
    selector_->RunInSelectLoop(call_on_close);
    return;
  }
  close_completed_ = call_on_close;

  // Send EOS to all clients.
  vector<std::pair<streaming::Request*,
    streaming::ProcessingCallback*> > callbacks;
  for (ReqMap::iterator it = req_map_.begin(); it != req_map_.end(); ++it) {
    if ( !it->second->eos_received_ ) {
      it->second->eos_received_ = true;
      callbacks.push_back(make_pair(it->first, it->second->callback_));
    }
  }
  for ( int i = 0; i < callbacks.size(); ++i ) {
    callbacks[i].second->Run(scoped_ref<Tag>(new streaming::EosTag(
        0, callbacks[i].first->caps().flavour_mask_, 0, true)).get());
  }

  // NEXT: each client is responsible to call RemoveRequest() from us.
  //       When the req_map_ gets empty, close_completed_ is called.
}


void LoadBalancingElement::ProcessTag(ReqStruct* rs,
                                      const streaming::Tag* tag) {
  if ( tag->type() == streaming::Tag::TYPE_EOS ) {
    if ( rs->eos_received_ ) {
      return;
    }
    rs->eos_received_ = true;
  }

  if ( tag->type() == streaming::Tag::TYPE_SOURCE_STARTED ||
       tag->type() == streaming::Tag::TYPE_SOURCE_ENDED ) {
    const streaming::SourceChangedTag* source_change =
      static_cast<const streaming::SourceChangedTag*>(tag);
    pair<string, string> media_path_pair = strutil::SplitFirst(
        source_change->path().c_str(), '/');
    pair<string, string> media_name_pair = strutil::SplitFirst(
        source_change->source_element_name().c_str(), '/');
    if ( media_name_pair.first == rs->new_element_name_ ) {
      scoped_ref<SourceChangedTag> td(static_cast<SourceChangedTag*>(
          source_change->Clone(-1)));
      td->set_source_element_name(
          strutil::JoinMedia(name(), media_name_pair.second));
      td->set_path(strutil::JoinMedia(name(), media_path_pair.second));
      rs->callback_->Run(td.get());
      return;
    }
  }
  rs->callback_->Run(tag);
}

}
