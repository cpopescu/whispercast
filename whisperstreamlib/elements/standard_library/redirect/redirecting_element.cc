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
//
#include "redirecting_element.h"

namespace streaming {

const char RedirectingElement::kElementClassName[] = "redirecting";
RedirectingElement::RedirectingElement(
    const string& name, ElementMapper* mapper,
    const map<string, string>& redirection)
  : Element(kElementClassName, name, mapper),
    redirections_(),
    req_map_() {
  for ( map<string, string>::const_iterator it = redirection.begin();
        it != redirection.end(); ++it ) {
    const string& str_reg = it->first;
    const string& str_replace = it->second;
    re::RE* re = new re::RE(str_reg);
    if ( re->HasError() ) {
      LOG_ERROR << name << ": Invalid reg exp: [" << str_reg << "]"
                    ", error: " << re->ErrorName();
      delete re;
      continue;
    }
    redirections_.push_back(new Redirection(re, str_replace));
  }
}
RedirectingElement::~RedirectingElement() {
  for ( uint32 i = 0; i < redirections_.size(); i++ ) {
    const Redirection* redir = redirections_[i];
    delete redir;
  }
  redirections_.clear();
}

void RedirectingElement::ProcessTag(ReqStruct* rs,
                                    const streaming::Tag* tag,
                                    int64 timestamp_ms) {
  CHECK_NOT_NULL(rs);
  if ( (tag->type() == streaming::Tag::TYPE_SOURCE_STARTED ||
        tag->type() == streaming::Tag::TYPE_SOURCE_ENDED) &&
       !rs->redirection_path_.empty() ) {
    const streaming::SourceChangedTag* source_change =
      static_cast<const streaming::SourceChangedTag*>(tag);
    const string media_old = source_change->path();
    if ( !strutil::StrStartsWith(media_old, rs->redirection_path_) ) {
      LOG_ERROR << name() << ": Strange " << tag->type_name() << " tag"
                   ", media: [" << media_old << "] does not start with "
                   "redirection path: [" << rs->redirection_path_ << "]";
      rs->downstream_callback_->Run(tag, timestamp_ms);
      return;
    }
    const string media_orig = media_old.substr(rs->redirection_path_.size());
    const string media_new = strutil::JoinMedia(name(), media_orig);
    LOG_INFO << name() << ": ProcessTag modifying " << tag->type_name()
             << " tag from: [" << media_old << "] to: [" << media_new << "]";
    scoped_ref<SourceChangedTag> td(static_cast<SourceChangedTag*>(
        source_change->Clone()));
    td->set_path(media_new);
    rs->downstream_callback_->Run(td.get(), timestamp_ms);
    return;
  }
  rs->downstream_callback_->Run(tag, timestamp_ms);
}

bool RedirectingElement::Initialize() {
  return true;
}
bool RedirectingElement::AddRequest(const string& media, Request* req,
    ProcessingCallback* callback) {
  // find a suitable redirection according to the sub-path
  const Redirection* redir = NULL;
  for ( uint32 i = 0; i < redirections_.size(); i++ ) {
    redir = redirections_[i];
    if ( redir->re_->Matches(media) ) {
      break;
    }
  }
  // NOTE: redir may be NULL , for request that do not match any redirection

  // build the NEW sub-path
  static const string kStrEmpty;
  const string& redir_path = (redir == NULL ? kStrEmpty : redir->value_);
  const string media_new_path = strutil::JoinMedia(redir_path, media);

  // make a ReqStruct to remember this redirection
  ReqStruct* rs = new ReqStruct(callback, NULL, media, redir_path);
  rs->our_callback_ = NewPermanentCallback(this,
      &RedirectingElement::ProcessTag, rs);

  pair<ReqMap::iterator, bool> result = req_map_.insert(make_pair(req, rs));
  if ( !result.second ) {
    LOG_FATAL << name() << ": AddRequest: duplicate Request, req: " << req
              << ", path: " << media;
    delete rs;
    return false;
  }

  // Maybe we need to modify to 'req' according to the new sub-path.
  // For now we don't modify anything.

  LOG_INFO << name() << " redirecting request from [" << media << "] to"
              " [" << media_new_path << "]";

  if ( !mapper_->AddRequest(media_new_path, req, rs->our_callback_) ) {
    req_map_.erase(req);
    delete rs;
    return false;
  }

  return true;
}
void RedirectingElement::RemoveRequest(streaming::Request* req) {
  ReqMap::iterator it = req_map_.find(req);
  if ( it == req_map_.end() ) {
    LOG_ERROR << name() << ": RemoveRequest failed, we don't have this"
                 " request: " << req->ToString();
    return;
  }
  ReqStruct* rs = it->second;
  req_map_.erase(it);

  mapper_->RemoveRequest(req, rs->our_callback_);
  delete rs;
}
bool RedirectingElement::HasMedia(const string& media) {
  for ( uint32 i = 0; i < redirections_.size(); i++ ) {
    const string crt_name = strutil::JoinMedia(redirections_[i]->value_, media);
    if ( mapper_->HasMedia(crt_name) ) {
      return true;
    }
  }
  return false;
}
void RedirectingElement::ListMedia(const string& media,
                                   vector<string>* out) {
  for ( int i = 0; i < redirections_.size(); ++i ) {
    vector<string> elements;
    const string crt_name = strutil::JoinMedia(redirections_[i]->value_, media);
    mapper_->ListMedia(crt_name, &elements);
    for ( int j = 0; j < elements.size(); ++j ) {
      out->push_back(strutil::JoinMedia(name(), elements[i]));
    }
  }
}
bool RedirectingElement::DescribeMedia(const string& media,
                                       MediaInfoCallback* callback) {
  if ( redirections_.empty() ) {
    return false;
  }
  const string new_media = strutil::JoinMedia(redirections_[0]->value_, media);
  return mapper_->DescribeMedia(new_media, callback);
}
void RedirectingElement::Close(Closure* call_on_close) {
  while ( !req_map_.empty() ) {
    mapper_->RemoveRequest(req_map_.begin()->first,
                           req_map_.begin()->second->our_callback_);
  }
  call_on_close->Run();
}

} // namespace streaming
