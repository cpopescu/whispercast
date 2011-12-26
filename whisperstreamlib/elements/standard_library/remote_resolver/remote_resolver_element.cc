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

#include "elements/standard_library/remote_resolver/remote_resolver_element.h"

namespace {
http::BaseClientConnection* CreateConnection(net::Selector* selector,
                                             net::NetFactory* net_factory,
                                             net::PROTOCOL net_protocol) {
  return new http::SimpleClientConnection(selector, *net_factory, net_protocol);
}
}

namespace streaming {
const char RemoteResolverElement::kElementClassName[] = "remote_resolver";

RemoteResolverElement::RemoteResolverElement(
    const char* name,
    const char* id,
    ElementMapper* mapper,
    net::Selector* selector,
    int64 cache_expiration_time_ms,
    const vector<net::HostPort>& lookup_servers,
    const string& lookup_rpc_path,
    int lookup_num_retries,
    int lookup_max_concurrent_requests,
    int lookup_req_timeout_ms,
    bool local_lookup_first,
    const string& lookup_auth_user,
    const string& lookup_auth_pass,
    const Capabilities& default_caps)
    : Element(kElementClassName, name, id, mapper),
      selector_(selector),
      net_factory_(selector_),
      service_(NULL),
      rpc_connection_(NULL),
      cache_expiration_time_ms_(cache_expiration_time_ms),
      local_lookup_first_(local_lookup_first),
      default_caps_(default_caps),
      closing_(false),
      call_on_close_(NULL),
      periodic_expiration_callback_(
          NewPermanentCallback(
              this,
              &RemoteResolverElement::PeriodicCacheExpirationCallback)) {
  client_params_.default_request_timeout_ms_ = lookup_req_timeout_ms;
  client_params_.max_concurrent_requests_ = lookup_max_concurrent_requests;
  connection_factory_ = NewPermanentCallback(&CreateConnection,
                                             selector, &net_factory_,
                                             net::PROTOCOL_TCP);

  http::FailSafeClient* failsafe_client = new http::FailSafeClient(
      selector,
      &client_params_,
      lookup_servers,
      connection_factory_,
      lookup_num_retries,
      lookup_req_timeout_ms,
      kReopenHttpConnectionIntervalMs,
      "");
  rpc_connection_ = new rpc::FailsafeClientConnectionHTTP(
      selector, rpc::CID_JSON,  // rpc::CID_BINARY,  //  rpc::CID_JSON
      failsafe_client,
      lookup_rpc_path,
      lookup_auth_user, lookup_auth_pass);
  service_ = new ServiceWrapperStandardLibraryService(
      *rpc_connection_,
      ServiceWrapperStandardLibraryService::ServiceClassName());
  selector_->RegisterAlarm(periodic_expiration_callback_,
                           kPeriodicExpirationCallbackFrequencyMs);
}

RemoteResolverElement::~RemoteResolverElement() {
  CHECK(lookup_ops_.empty()) << " Perform a Close first (and wait for it)";
  delete connection_factory_;
  delete service_;
  delete rpc_connection_;
  while ( !cache_exp_list_.empty() ) {
    LookupResult* const res = cache_exp_list_.front();
    cache_exp_list_.pop_front();
    res->DecRef();
  }
  cache_.clear();
  selector_->UnregisterAlarm(periodic_expiration_callback_);
  delete periodic_expiration_callback_;
}

bool RemoteResolverElement::Initialize() {
  return true;
}

bool RemoteResolverElement::AddRequest(
    const char* media,
    streaming::Request* req,
    streaming::ProcessingCallback* callback) {
  if ( closing_ ) {
    LOG_DEBUG << " Closing element cannot add request for: " << media;
    return false;
  }
  if ( !strutil::StrPrefix(media,
                           (name() + "/").c_str()) ) {
    LOG_WARNING << " Cannot add media: [" << media << "] to " << name();
    return false;
  }
  const string sub_media(media + name().length() + 1);
  if ( lookup_ops_.find(req) != lookup_ops_.end() ) {
    LOG_WARNING << "Cannot serve same request twice: " << req->ToString();
    return false;
  }
  if ( local_lookup_first_ ) {
    LOG_INFO << " Internal lookup for: " << sub_media;
    if ( mapper_->AddRequest(sub_media.c_str(), req, callback) ) {
      // We basically don't care about this any more..
      DLOG_DEBUG << " Request for [" << media << "] served locally !";
      return true;
    }
    // Need lookup
  }
  if ( !default_caps_.IsCompatible(req->caps()) ) {
    LOG_WARNING << " Caps don't match: "
                << req->caps().ToString() << " v.s. "
                << default_caps_.ToString();
    return false;
  }
  req->mutable_caps()->IntersectCaps(default_caps_);

  RequestStruct* ls = new RequestStruct(sub_media.c_str(), req, callback);
  // Lookup Cache
  const LookupCache::iterator it = cache_.find(ls->media_name_);
  if ( it != cache_.end() ) {
    DLOG_DEBUG << name() << " Internal cache found: " << sub_media << " -> "
               << it->second->media_name_;
    return StartPlaySequence(ls, it->second, true);
  }
  DLOG_DEBUG << name() << " Starting Remote lookup for: " << sub_media;
  lookup_ops_.insert(make_pair(req, ls));

  // VERY important - we need to do not get immediate return from this call..
  ls->rpc_id_ = service_->ResolveMedia(
      NewCallback(this, &RemoteResolverElement::LookupCompleted, ls),
      sub_media);
  return true;   // for now :)
}

void RemoteResolverElement::RemoveRequest(streaming::Request* req) {
  LookupMap::const_iterator it_lookup = lookup_ops_.find(req);
  if ( it_lookup != lookup_ops_.end() ) {
    it_lookup->second->lookup_cancelled_ = true;
  } else {
    // Is registered somewhere ...
    RequestMap::iterator it = active_reqs_.find(req);
    if ( it != active_reqs_.end() ) {
      if ( !it->second->crt_media_.empty() ) {
        mapper_->RemoveRequest(it->second->req_,
                               it->second->internal_processing_callback_);
      }
      it->second->crt_media_.clear();
      if ( it->second->is_processing_ ) {
        // Avoid possible iterator / function - in - function crap
        it->second->is_orphaned_ = true;
      } else {
        // Safe and should remove it ourselves
        delete it->second;
        active_reqs_.erase(it);
      }

    }
  }
  if ( call_on_close_!= NULL &&
       lookup_ops_.empty() &&
       active_reqs_.empty() ) {
    LOG_INFO << "Remote Resolver Element: " << name() << " closed..";
    call_on_close_->Run();
  }
}

bool RemoteResolverElement::HasMedia(const char* media, Capabilities* out) {
  pair<string, string> split(strutil::SplitFirst(media, '/'));
  if ( split.first != name() ) {
    return false;
  }
  *out = default_caps_;
  return true;
}

void RemoteResolverElement::ListMedia(
    const char* media_dir,
    streaming::ElementDescriptions* medias) {
  pair<string, string> split(strutil::SplitFirst(media_dir, '/'));
  if ( split.first != name() ) {
    return;
  }
  mapper_->ListMedia(split.second.c_str(), medias);
}
bool RemoteResolverElement::DescribeMedia(const string& media,
                                          MediaInfoCallback* callback) {
  return mapper_->DescribeMedia(media, callback);
}

void RemoteResolverElement::Close(Closure* call_on_close) {
  if ( closing_ ) {
    return;
  }
  closing_ = true;
  for ( LookupMap::const_iterator it = lookup_ops_.begin();
        it != lookup_ops_.end(); ++it ) {
    if ( !it->second->lookup_cancelled_ ) {
      it->second->is_processing_ = true;
      it->second->callback_->Run(scoped_ref<Tag>(new EosTag(
          0, it->second->req_->caps().flavour_mask_, true)).get(), 0);
      it->second->is_processing_ = false;
      it->second->lookup_cancelled_ = true;
    }
  }
  for ( RequestMap::const_iterator it = active_reqs_.begin();
        it != active_reqs_.end(); ++it ) {
    it->second->is_processing_ = true;
    it->second->callback_->Run(scoped_ref<Tag>(new EosTag(
        0, it->second->req_->caps().flavour_mask_, true)).get(), 0);
    delete it->second;
  }
  active_reqs_.clear();

  call_on_close_ = call_on_close;
  if ( lookup_ops_.empty() && active_reqs_.empty() ) {
    LOG_INFO << "Remote Resolver Element: " << name() << " closed..";
    call_on_close_->Run();
  }
}

void RemoteResolverElement::LookupCompleted(
    RemoteResolverElement::RequestStruct* req,
    const rpc::CallResult< ResolveSpec > & response) {
  if ( req->lookup_cancelled_ ) {
    CHECK(lookup_ops_.erase(req->req_));
    delete req;
  } else {
    if ( !closing_ && call_on_close_ == NULL ) {
      bool media_added = false;
      if ( response.success_ ) {
        LookupResult* const lres = new LookupResult(req->media_name_);
        lres->expiration_time_ = selector_->now() + cache_expiration_time_ms_;
        const int resp_size = response.result_.media_.get().size();
        for ( int i = 0; i < resp_size; ++i )  {
          const MediaAliasSpec& s = response.result_.media_.get()[i];
          lres->to_play_.push_back(make_pair(s.alias_name_.get(),
                                             s.media_name_.get()));
        }
        lres->loop_ = response.result_.loop_.get();
        if ( !lres->to_play_.empty() ) {
          // The good path - start playing from what we got
          AddToCache(lres);
          StartPlaySequence(req, lres, false);
          media_added = true;
        } else {
          delete lres;
        }
      }
      CHECK(lookup_ops_.erase(req->req_));
      if ( !media_added ) {
        streaming::ProcessingCallback* callback = req->callback_;
        streaming::Request* saved_req = req->req_;
        delete req;
        callback->Run(scoped_ref<Tag>(new EosTag(
            0, saved_req->caps().flavour_mask_, 0)).get(), 0);
      }
    } else {
      CHECK(lookup_ops_.erase(req->req_));
      delete req;
    }
  }
  if ( call_on_close_ != NULL &&
       active_reqs_.empty() && lookup_ops_.empty() ) {
    LOG_INFO << "Lookup Element: " << name() << " closed..";
    call_on_close_->Run();
  }
}

void RemoteResolverElement::ProcessTag(RequestStruct* req,
                                       const Tag* tag,
                                       int64 timestamp_ms) {
  if ( req->is_orphaned_ ) {
    active_reqs_.erase(req->req_);
    delete req;
    if ( call_on_close_!= NULL &&
         lookup_ops_.empty() && active_reqs_.empty() ) {
      call_on_close_->Run();
    }
    return;
  }

  if ( !req->source_start_sent_ ) {
    req->source_start_sent_ = true;

    req->is_processing_ = true;
    req->callback_->Run(scoped_ref<Tag>(
        new SourceStartedTag(0,
            req->req_->caps().flavour_mask_,
            name(), name(),
            false,
            timestamp_ms)).get(), timestamp_ms);
    req->is_processing_ = false;

  }

  //
  // MEGA BIG
  // TODO: -- what we do with controllers ?
  //
  if ( tag->type() == streaming::Tag::TYPE_EOS ) {
    // Just pass it forward .. good things will happen
    ContinuePlaySequence(req, false);
  } else {
    req->callback_->Run(tag, timestamp_ms);
  }
}

bool RemoteResolverElement::StartPlaySequence(
    RequestStruct* req,
    LookupResult* const lres,
    bool do_not_send_control_tags) {
  lres->IncRef();
  req->lres_ = lres;
  req->internal_processing_callback_ = NewPermanentCallback(
      this, &RemoteResolverElement::ProcessTag, req);
  active_reqs_.insert(make_pair(req->req_, req));
  return ContinuePlaySequence(req, do_not_send_control_tags);
}

bool RemoteResolverElement::ContinuePlaySequence(RequestStruct* req,
                                                 bool do_not_send_control_tags) {
  int num_failures = 0;
  do {
    if ( !req->crt_media_.empty() ) {
      mapper_->RemoveRequest(req->req_,
                             req->internal_processing_callback_);
    }
    req->crt_media_.clear();
    req->crt_play_index_++;
    if ( req->crt_play_index_ >= req->lres_->to_play_.size() &&
         req->lres_->loop_ ) {
      req->crt_play_index_ = 0;
    }
    if ( req->crt_play_index_ >= req->lres_->to_play_.size() ) {
      if ( !do_not_send_control_tags ) {
        goto SendEos;
      }
      // Things will flow from here by themselves..
      return false;
    } else {
      req->crt_media_ = req->lres_->to_play_[req->crt_play_index_].second;
      if ( !req->is_orphaned_ &&
           mapper_->AddRequest(req->crt_media_.c_str(),
                               req->req_,
                               req->internal_processing_callback_) ) {
        LOG_DEBUG << name() << " went to the next media: "
                  << req->crt_media_ << " - ndx: " << req->crt_play_index_
                  << " out: " << req->lres_->to_play_.size();
        return true;
      }
      num_failures++;
      req->crt_media_.clear();
    }
  } while ( !req->is_orphaned_ && num_failures < req->lres_->to_play_.size() );
  if ( req->is_orphaned_ ) {
    active_reqs_.erase(req->req_);
    delete req;
    if ( call_on_close_!= NULL &&
         lookup_ops_.empty() && active_reqs_.empty() ) {
      call_on_close_->Run();
    }
  } else if ( num_failures >= req->lres_->to_play_.size() &&
              !do_not_send_control_tags ) {
    goto SendEos;
  }
  return false;

SendEos:
  if ( req->source_start_sent_ ) {
    req->is_processing_ = true;
    req->callback_->Run(scoped_ref<Tag>(new SourceEndedTag(
        0, req->req_->caps().flavour_mask_,
        name(), name())).get(), 0);
    req->is_processing_ = false;
  }
  if ( !req->is_orphaned_ ) {
    req->callback_->Run(scoped_ref<Tag>(new EosTag(0,
        req->req_->caps().flavour_mask_, 0)).get(), 0);
  }
  return false;
}

void RemoteResolverElement::AddToCache(LookupResult* res) {
  LookupCache::iterator it = cache_.find(res->media_name_);
  res->IncRef();
  if ( it == cache_.end() ) {
    cache_.insert(make_pair(res->media_name_, res));
  } else {
    it->second = res;   // The one that was there will be deleted by expiration
  }
  cache_exp_list_.push_back(res);
}

void RemoteResolverElement::ExpireSomeCache() {
  while ( !cache_exp_list_.empty() ) {
    LookupResult* const res = cache_exp_list_.front();
    if ( res->expiration_time_ > selector_->now() ) {
      return;
    }
    cache_exp_list_.pop_front();
    const LookupCache::iterator it = cache_.find(res->media_name_);
    CHECK(it != cache_.end());
    if ( it->second == res ) {
      cache_.erase(it);
    }
    LOG_DEBUG << name() << " expired entry for: " << res->media_name_;
    res->DecRef();
  }
}
void RemoteResolverElement::PeriodicCacheExpirationCallback() {
  ExpireSomeCache();
  selector_->RegisterAlarm(periodic_expiration_callback_,
                             kPeriodicExpirationCallbackFrequencyMs);
}

}
