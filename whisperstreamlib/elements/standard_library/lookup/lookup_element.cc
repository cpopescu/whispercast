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

#include "elements/standard_library/lookup/lookup_element.h"

namespace {
http::BaseClientConnection* CreateConnection(net::Selector* selector,
                                             net::NetFactory* net_factory,
                                             net::PROTOCOL net_protocol) {
  return new http::SimpleClientConnection(selector, *net_factory, net_protocol);
}
}

namespace streaming {
const char LookupElement::kElementClassName[] = "lookup";

LookupElement::LookupElement(
    const char* name,
    const char* id,
    ElementMapper* mapper,
    net::Selector* selector,
    HttpClientElement* http_client_element,
    const vector<net::HostPort>& lookup_servers,
    const string& lookup_query_path_format,
    const vector< pair<string, string> >& lookup_http_headers,
    int lookup_num_retries,
    int lookup_max_concurrent_requests,
    int lookup_req_timeout_ms,
    const string& lookup_force_host_header,
    bool local_lookup_first)
    : Element(kElementClassName, name, id, mapper),
      selector_(selector),
      net_factory_(selector_),
      failsafe_client_(NULL),
      lookup_query_path_format_(lookup_query_path_format),
      lookup_http_headers_(lookup_http_headers),
      local_lookup_first_(local_lookup_first),
      http_client_element_(NULL),
      closing_(false),
      http_closed_(false),
      call_on_close_(NULL),
      next_client_id_(0),
      next_lookup_id_(0) {
  client_params_.default_request_timeout_ms_ = lookup_req_timeout_ms;
  client_params_.max_concurrent_requests_ = lookup_max_concurrent_requests;
  connection_factory_ = NewPermanentCallback(&CreateConnection,
                                             selector, &net_factory_,
                                             net::PROTOCOL_TCP);

  failsafe_client_ = new http::FailSafeClient(
      selector,
      &client_params_,
      lookup_servers,
      connection_factory_,
      lookup_num_retries,
      lookup_req_timeout_ms,
      kReopenHttpConnectionIntervalMs,
      lookup_force_host_header);
  CHECK(http_client_element != NULL);
  http_client_element_ = http_client_element;
}

LookupElement::~LookupElement() {
  CHECK(lookup_ops_.empty()) << " Perform a Close first (and wait for it)";
  delete failsafe_client_;
  delete connection_factory_;
  delete http_client_element_;
}

bool LookupElement::Initialize() {
  return http_client_element_->Initialize();
}

bool LookupElement::AddRequest(const char* media,
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
      LOG_DEBUG << " Request for [" << media << "] served locally !";
      return true;
    }
    // Need lookup
  }
  req->mutable_caps()->flavour_mask_ = streaming::kDefaultFlavourMask;
  LookupReqStruct* const ls = PrepareLookupStruct(sub_media,
                                                  req, callback);
  lookup_ops_.insert(make_pair(req, ls));
  failsafe_client_->StartRequest(ls->http_request_,
                                 NewCallback(this,
                                             &LookupElement::LookupCompleted,
                                             ls));
  return true;   // for now :)
}

void LookupElement::RemoveRequest(streaming::Request* req) {
  LookupMap::const_iterator it_lookup = lookup_ops_.find(req);
  if ( it_lookup != lookup_ops_.end() ) {
    it_lookup->second->cancelled_ = true;
  } else {
    http_client_element_->RemoveRequest(req);
  }
  if ( call_on_close_!= NULL &&
       http_closed_ && lookup_ops_.empty() ) {
    LOG_INFO << "Lookup Element: " << name() << " closed..";
    call_on_close_->Run();
  }
}

bool LookupElement::HasMedia(const char* media, Capabilities* out) {
  pair<string, string> split(strutil::SplitFirst(media, '/'));
  if ( split.first != name() ) {
    return false;
  }
  *out = Capabilities(Tag::kAnyType, kDefaultFlavourMask);
  return true;
}

void LookupElement::ListMedia(const char* media_dir,
                              streaming::ElementDescriptions* medias) {
  pair<string, string> split(strutil::SplitFirst(media_dir, '/'));
  if ( split.first != name() ) {
    return;
  }
  mapper_->ListMedia(split.second.c_str(), medias);
}
bool LookupElement::DescribeMedia(const string& media,
                                  MediaInfoCallback* callback) {
  return mapper_->DescribeMedia(media, callback);
}

void LookupElement::HttpClientClosed() {
  if ( call_on_close_ != NULL &&
       http_closed_ && lookup_ops_.empty() ) {
    LOG_INFO << "Lookup Element: " << name() << " closed..";
    call_on_close_->Run();
  }
}

void LookupElement::Close(Closure* call_on_close) {
  if ( closing_ ) {
    return;
  }
  closing_ = true;
  scoped_ref<Tag> eos_tag(
      new EosTag(0, streaming::kDefaultFlavourMask, true));
  for ( LookupMap::const_iterator it = lookup_ops_.begin();
        it != lookup_ops_.end(); ++it ) {
    if ( !it->second->cancelled_ ) {
      it->second->callback_->Run(eos_tag.get(), 0);
      it->second->cancelled_ = true;
    }
  }
  http_client_element_->Close(NewCallback(this,
                                          &LookupElement::HttpClientClosed));
  call_on_close_ = call_on_close;
  if ( http_closed_ && lookup_ops_.empty() ) {
    selector_->RunInSelectLoop(call_on_close_);
  }
}

LookupElement::LookupReqStruct* LookupElement::PrepareLookupStruct(
    const string& media,
    streaming::Request* req,
    streaming::ProcessingCallback* callback) {
  LookupReqStruct* lr = new LookupReqStruct(media, req, callback);

  map<string, string> params;
  params["RESOURCE"] = URL::UrlEscape(media);
  params["REQ_QUERY"] = req->info().GetUrlQuery(false);
  params["AUTH_QUERY"] = req->info().auth_req_.GetUrlQuery();

  const string query_path = strutil::StrMapFormat(
      lookup_query_path_format_.c_str(), params);

  lr->http_request_ = new http::ClientRequest(http::METHOD_GET, query_path);
  if ( !lookup_http_headers_.empty() ) {
    for ( vector< pair<string, string> >::const_iterator
              it = lookup_http_headers_.begin();
          it != lookup_http_headers_.end(); ++it ) {
      lr->http_request_->request()->client_header()->AddField(
          it->first,
          strutil::StrMapFormat(it->second.c_str(), params),
          true, true);
    }
  }
  return lr;
}

void LookupElement::LookupCompleted(LookupReqStruct* lr) {
  if ( lr->cancelled_ ) {
    CHECK(lookup_ops_.erase(lr->req_));
    delete lr;
  } else {
    if ( !closing_ && call_on_close_ == NULL ) {
      bool fetch_started = false;
      if ( lr->http_request_->error() != http::CONN_OK ) {
        LOG_ERROR << " Connection error looking up media: " << lr->media_name_;
      } else if (
          lr->http_request_->request()->server_header()->status_code() !=
          http::OK ) {
        LOG_ERROR << " HTTP error looking up media: " << lr->media_name_
                  << " - Code: " << lr->http_request_->request()->server_header()->status_code();
      } else {
        const string body(
            lr->http_request_->request()->server_data()->ToString());
        vector<string> url_names;
        strutil::SplitString(body, "\n", &url_names);
        if ( url_names.empty() ) {
          LOG_WARNING << name() << " No urls received in lookup";
        } else {
          const int start_id = next_client_id_ % url_names.size();
          ++next_client_id_;
                               // round robin :)
          for ( int i = 0; i < url_names.size() && !fetch_started; ++i ) {
            const string crt_str(
                strutil::StrTrim(url_names[(i + start_id) % url_names.size()]));
            if ( strutil::StrPrefix(crt_str.c_str(), "http://") ) {
              URL* const url = new URL(crt_str);
              if ( url->is_valid() && !url->is_empty() &&
                   url->scheme() == "http" ) {
                net::HostPort hp;
                if ( url->port().empty() ) {
                  hp = net::HostPort(url->host().c_str(), 80);
                } else {
                  hp = net::HostPort((url->host() + ":" + url->port()).c_str());
                }
                if ( hp.IsInvalid() ) {
                  LOG_WARNING << name() << " - Invalid url received in lookup: "
                              << url_names[i];
                } else {
                  // GOOD PATH --> Start fetch !!
                  fetch_started = StartFetch(lr, hp, url->PathForRequest());
                }
              } else {
                LOG_WARNING << name() << " - Invalid url received in lookup: "
                            << url_names[i];
              }
              delete url;
            } else {
              // Resolve to internal path..
              fetch_started = StartLocalRedirect(lr, crt_str);
            }
          }
        }
      }
      CHECK(lookup_ops_.erase(lr->req_));
      streaming::ProcessingCallback* callback = lr->callback_;
      delete lr;
      if ( !fetch_started ) {
        callback->Run(scoped_ref<Tag>(new EosTag(
            0, streaming::kDefaultFlavourMask, 0)).get(), 0);
      }
    } else {
      CHECK(lookup_ops_.erase(lr->req_));
      delete lr;
    }
  }
  if ( call_on_close_ != NULL &&
       http_closed_ && lookup_ops_.empty() ) {
    LOG_INFO << "Lookup Element: " << name() << " closed..";
    call_on_close_->Run();
  }
}

bool LookupElement::StartLocalRedirect(LookupReqStruct* lr,
                                       const string& media_path) {
  if ( !mapper_->AddRequest(media_path.c_str(),
                            lr->req_,
                            lr->callback_) ) {
    LOG_WARNING << " Error in redirect: " << lr->media_name_ << " -> "
                << media_path;
    return false;
  }
  LOG_DEBUG << " Redirect performed: " << lr->media_name_ << " -> "
            << media_path;
  return true;
}
bool LookupElement::StartFetch(LookupReqStruct* lr,
                               const net::HostPort& hp,
                               const string& query_path) {
  const string fname(strutil::StringPrintf(
                         "fetch_%"PRId64"",
                         (next_lookup_id_)));
  ++next_lookup_id_;
  LOG_INFO << name() << " Lookup completed, we start fetching from: "
           << hp.ToString() << " [" << query_path << "]";

  if ( !http_client_element_->AddElement(
           fname.c_str(),
           hp.ip_object().ToString().c_str(),
           hp.port(),
           query_path.c_str(),
           false,
           true,
           lr->req_->caps().tag_type_) ) {
    return false;
  }
  if ( !http_client_element_->AddRequest(
           (http_client_element_->name() + "/" + fname).c_str(),
           lr->req_, lr->callback_) ) {
    http_client_element_->DeleteElement(fname.c_str());
    return false;
  }
  return true;
}
}
