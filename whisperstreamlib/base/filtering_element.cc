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
// Author: Catalin Popescu & Cosmin Tudorache

#include "base/filtering_element.h"

namespace streaming {

//////////////////////////////////////////////////////////////////////
//
// FilteringCallbackData implementation
//
FilteringCallbackData::FilteringCallbackData()
    : master_(NULL),
      mapper_(NULL),
      req_(NULL),
      here_process_tag_callback_(NULL),
      client_process_tag_callback_(NULL),
      flavour_mask_(0),
      private_flags_(0),
      ref_count_(0),
      eos_received_(false),
      last_tag_ts_(0) {
}

FilteringCallbackData::~FilteringCallbackData() {
  CHECK(ref_count_ == 0);
  if ( req_ ) {
    Unregister(req_);
  }
  CHECK_NULL(here_process_tag_callback_);
}

void FilteringCallbackData::SendTag(const Tag* tag, int64 timestamp_ms) {
  DCHECK(flavour_mask_ != 0 ||
         tag->type() == Tag::TYPE_EOS);

  if ( tag->type() == Tag::TYPE_EOS ) {
    if ( eos_received_ ) {
      LOG_WARNING << media_name() << ": Ignoring multiple EOS";
      return;
    }
    eos_received_ = true;
  }

  last_tag_ts_ = timestamp_ms;
  client_process_tag_callback_->Run(tag, timestamp_ms);

}

bool FilteringCallbackData::Register(streaming::Request* req) {
  CHECK_NULL(here_process_tag_callback_);
  CHECK_NULL(req_);
  here_process_tag_callback_ =
      NewPermanentCallback(this, &FilteringCallbackData::ProcessTag);

  if ( !mapper_->AddRequest(media_name_,
                            req,
                            here_process_tag_callback_) ) {
    delete here_process_tag_callback_;
    here_process_tag_callback_ = NULL;

    return false;
  }
  flavour_mask_ = req->caps().flavour_mask_;
  req_ = req;
  uint32 f = flavour_mask_;
  while ( f )  {
    const int id = RightmostFlavourId(f);  // id in range [0, 31]
    RegisterFlavour(id);
  }
  return true;
}

bool FilteringCallbackData::Unregister(streaming::Request* req) {
  if ( req_ != req ) {
    return false;
  }
  if ( req_ == NULL ) {
    return true;   // Already unregistered
  }
  flavour_mask_ = 0;
  req_ = NULL;  // we need to do that in order to avoin multiple inner calls
  mapper_->RemoveRequest(req, here_process_tag_callback_);
  master_->selector()->DeleteInSelectLoop(here_process_tag_callback_);
  here_process_tag_callback_ = NULL;
  return true;
}

void FilteringCallbackData::ProcessTag(const Tag* tag, int64 timestamp_ms) {
  IncRef();
  TagList tags;
  FilterTag(tag, timestamp_ms, &tags);

  while ( !tags.empty() ) {
    FilteredTag t = tags.front();
    tags.pop_front();
    DCHECK(tags.empty() || t.tag_->type() != Tag::TYPE_EOS);

    if ( (private_flags_ & Flag_DontAppendNameToPath) == 0 ) {
      if ( t.tag_->type() == Tag::TYPE_SOURCE_STARTED ||
           t.tag_->type() == Tag::TYPE_SOURCE_ENDED ) {
        const SourceChangedTag* source_changed =
            static_cast<const SourceChangedTag*>(t.tag_.get());
        if ( !source_changed->is_final() ) {
          scoped_ref<SourceChangedTag> td(static_cast<SourceChangedTag*>(
              source_changed->Clone()));
          td->set_path(strutil::JoinMedia(filtering_element_name_,
              source_changed->path()));
          t.tag_ = td.get();
        }
      }
    }

    if ( client_process_tag_callback_ != NULL ) {
      client_process_tag_callback_->Run(t.tag_.get(), t.timestamp_ms_);
    }
  }
  CHECK(tags.empty());
  DecRef();
}

void FilteringCallbackData::Close() {
  Unregister(req_);
  SendTag(scoped_ref<Tag>(
      new EosTag(0, kDefaultFlavourMask, true)).get(), 0);
}


//////////////////////////////////////////////////////////////////////
//
// FilteringElement implementation
//
FilteringElement::FilteringElement(const string& type,
                                   const string& name,
                                   ElementMapper* mapper,
                                   net::Selector* selector)
    : Element(type, name, mapper),
      selector_(selector),
      callbacks_(),
      close_completed_(NULL) {
}

FilteringElement::~FilteringElement() {
  // Close() should be called before deleting this element
  CHECK(callbacks_.empty()) << "#" << callbacks_.size()
                            << " callbacks pending";
}


void FilteringElement::CloseAllClients(Closure* call_on_close) {
  if ( callbacks_.empty() ) {
    LOG_INFO << name() << ": No callbacks, completing close";
    call_on_close->Run();
    return;
  }
  // Keep close_completed_ and RemoveRequest will call it when callbacks_ empty
  close_completed_ = call_on_close;
  for ( FilteringCallbackMap::const_iterator it = callbacks_.begin();
        it != callbacks_.end(); ++it ) {
    it->second->Close();
  }
}

bool FilteringElement::AddRequest(const string& media,
                                  streaming::Request* req,
                                  streaming::ProcessingCallback* callback) {
  if ( callbacks_.find(req) != callbacks_.end() ) {
    LOG_FATAL << "Duplicate AddRequest on path: [" << media << "]";
    return false;
  }
  FilteringCallbackData* const data = CreateCallbackData(media, req);
  if ( data == NULL ) {
    LOG_ERROR << "NULL CallbackData for subpath: [" << media << "]";
    return false;
  }
  data->IncRef();
  data->master_ = this;
  data->mapper_ = mapper_;
  data->media_name_ = media;
  data->client_process_tag_callback_ = callback;
  data->filtering_element_name_ = name();

  if ( !data->Register(req) ) {
    LOG_ERROR << "FilteringElement cannot add callback for subpath: ["
              << media << "]";
    DeleteCallbackData(data);
    return false;
  }
  callbacks_[req] = data;
  return true;
}

void FilteringElement::RemoveRequest(streaming::Request* req) {
  FilteringCallbackMap::iterator it = callbacks_.find(req);
  if ( it == callbacks_.end() ) {
    return;
  }
  FilteringCallbackData* data = it->second;
  callbacks_.erase(it);

  data->Unregister(req);

  DeleteCallbackData(data);
  data = NULL;

  if ( callbacks_.empty() && close_completed_ != NULL ) {
    LOG_INFO << name() << ": No more callbacks, completing close";
    close_completed_->Run();
  }
}

bool FilteringElement::HasMedia(const string& media) {
  return mapper_->HasMedia(media);
}

void FilteringElement::ListMedia(const string& media_dir,
                                 vector<string>* out) {
  mapper_->ListMedia(media_dir, out);
}
bool FilteringElement::DescribeMedia(const string& media,
                                     MediaInfoCallback* callback) {
  return mapper_->DescribeMedia(media, callback);
}

void FilteringElement::Close(Closure* call_on_close) {
  selector_->RunInSelectLoop(NewCallback(this,
      &FilteringElement::CloseAllClients, call_on_close));
}
}
