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

#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/base/strutil.h>
#include "base/request.h"
#include "base/element.h"

DEFINE_string(req_seek_pos_key, streaming::kMediaUrlParam_SeekPos,
             "The URL query key for seek_pos...");
DEFINE_string(req_media_origin_key, streaming::kMediaUrlParam_MediaOrigin,
             "The URL query key for media_origin...");
DEFINE_string(req_limit_key, streaming::kMediaUrlParam_Limit,
             "The URL query key for limit...");
DEFINE_string(req_session_key, streaming::kMediaUrlParam_Session,
             "The URL query key for session_id...");
DEFINE_string(req_affiliate_key, streaming::kMediaUrlParam_Affiliate,
             "The URL query key for affiliate_id...");
DEFINE_string(req_client_key, streaming::kMediaUrlParam_Client,
             "The URL query key for client_id...");
DEFINE_string(req_flavour_key, streaming::kMediaUrlParam_FlavourMask,
             "The URL query key for required flavour...");

namespace streaming {

void Request::SetFromUrl(const URL& url) {
  CHECK(url.is_valid()) << "URL: [" << url.spec() << "]";
  info_.path_ = url.path();
  if ( info_.path_[0] == '/' ) {
    info_.path_ = info_.path_.substr(1);
  }
  vector< pair<string, string> > comp;
  url.GetQueryParameters(&comp, true);
  for ( int i = 0; i < comp.size(); ++i ) {
    errno = 0;
    if ( comp[i].first == FLAGS_req_seek_pos_key ) {
      const int64 n = strtoll(comp[i].second.c_str(), NULL, 10);
      if ( !errno ) info_.seek_pos_ms_ = max((int64)(0), n);
    } else if ( comp[i].first == FLAGS_req_media_origin_key ) {
      const int64 n = strtoll(comp[i].second.c_str(), NULL, 10);
      if ( !errno ) info_.media_origin_pos_ms_ = max((int64)(0), n);
    } else if ( comp[i].first == FLAGS_req_limit_key ) {
      const int64 n = strtoll(comp[i].second.c_str(), NULL, 10);
      if ( !errno ) info_.limit_ms_ = max((int64)(-1), n);
    } else if ( comp[i].first == FLAGS_req_session_key ) {
      info_.session_id_ = comp[i].second;
    } else if ( comp[i].first == FLAGS_req_affiliate_key ) {
      info_.affiliate_id_ = comp[i].second;
    } else if ( comp[i].first == FLAGS_req_client_key ) {
      info_.client_id_ = comp[i].second;
    } else if ( comp[i].first == FLAGS_req_flavour_key ) {
      const uint64 n = strtoul(comp[i].second.c_str(), NULL, 10);
      if ( !errno ) caps_.flavour_mask_ = n;
    }
  }
  info_.auth_req_.ReadQueryComponents(comp);
}

void Request::Close(Closure* close_completed) {
  temp_struct_.Close(close_completed);
}

string RequestInfo::GetUrlQuery(bool append_auth) const {
  vector<string> ret;
  if ( media_origin_pos_ms_ > 0 ) {
    ret.push_back(strutil::StringPrintf(
                      "%s=%"PRId64"",
                      FLAGS_req_media_origin_key.c_str(),
                      (media_origin_pos_ms_)));
  }
  if ( seek_pos_ms_ > 0 ) {
    ret.push_back(strutil::StringPrintf(
                      "%s=%"PRId64"",
                      FLAGS_req_seek_pos_key.c_str(),
                      (seek_pos_ms_)));
  }
  if ( limit_ms_ > 0 && limit_ms_ != kMaxInt64 ) {
    ret.push_back(strutil::StringPrintf(
                      "%s=%"PRId64"",
                      FLAGS_req_limit_key.c_str(),
                      (limit_ms_)));
  }
  if ( !session_id_.empty() ) {
    ret.push_back(FLAGS_req_session_key + "=" +
                  URL::UrlEscape(session_id_));
  }
  if ( !affiliate_id_.empty() ) {
    ret.push_back(FLAGS_req_affiliate_key + "=" +
                  URL::UrlEscape(affiliate_id_));
  }
  if ( !client_id_.empty() ) {
    ret.push_back(FLAGS_req_client_key + "=" +
                  URL::UrlEscape(client_id_));
  }
  if ( append_auth ) {
    const string auth_q = auth_req_.GetUrlQuery();
    if ( !auth_q.empty() ) {
      ret.push_back(auth_q);
    }
  }

  return strutil::JoinStrings(ret, "&");
}

// Generates the path and query part for a url, from curent members
string RequestInfo::GetUrlPath(const char* media_spec,
                               bool append_auth) const {
  const string query = GetUrlQuery(append_auth);
  string path;
  if ( !*media_spec ) {
    path = "/";
  } else if ( *media_spec == '/' ) {
    path = URL::UrlEscape(media_spec);
  } else {
    path = "/";
    path.append(URL::UrlEscape(media_spec));
  }
  if ( query.empty() ) return path;
  return path + "?" + query;
}

string RequestInfo::GetPathId() const {
  if ( session_id_.empty() && affiliate_id_.empty() && client_id_.empty() ) {
    return strutil::StringPrintf("ID-%p", this);
  }
  string path;
  if ( !session_id_.empty() ) {
    path += URL::UrlEscape(session_id_) + "/";
  } else {
    path += "sid/";
  }
  if ( !affiliate_id_.empty() ) {
    path += URL::UrlEscape(affiliate_id_) + "/";
  } else {
    path += "aid/";
  }
  if ( !client_id_.empty() ) {
    path += URL::UrlEscape(client_id_);
  } else {
    path += "cid";
  }
  return path;
}

string RequestInfo::GetId() const {
  vector<string> ret;
  const string query = GetUrlQuery(false);
  if ( !query.empty() ) {
    ret.push_back(query);
  }
  if ( !remote_address_.ip_object().IsInvalid() ) {
    ret.push_back(string(kMediaParam_RemoteIp) + "=" +
                  remote_address_.ip_object().ToString());
  }
  if ( local_address_.port() != 0 ) {
    ret.push_back(strutil::StringPrintf(
                      "%s=%d",
                      kMediaParam_LocalPort,
                      static_cast<int>(local_address_.port())));
  }
  return strutil::JoinStrings(ret, "&");
}

string RequestInfo::ToString() const {
  return strutil::StringPrintf("RequestInfo{"
      "seek_pos_ms_: %"PRId64""
      ", media_origin_pos_ms_: %"PRId64""
      ", limit_ms_: %"PRId64""
      ", session_is_: %s"
      ", affiliate_id_: %s"
      ", client_id_: %s"
      ", user_agent: %s"
      ", remote_address_: %s"
      ", local_address: %s"
      ", ip_class_: %d"
      ", path_: [%s]"
      ", auth_req_: %s"
      ", write_ahead_ms_: %"PRId64""
      ", is_internal_: %s"
      ", is_temporary_requestor_: %s}",
      seek_pos_ms_,
      media_origin_pos_ms_,
      limit_ms_,
      session_id_.c_str(),
      affiliate_id_.c_str(),
      client_id_.c_str(),
      user_agent_.c_str(),
      remote_address_.ToString().c_str(),
      local_address_.ToString().c_str(),
      ip_class_,
      path_.c_str(),
      auth_req_.ToString().c_str(),
      write_ahead_ms_,
      strutil::BoolToString(is_internal_).c_str(),
      strutil::BoolToString(is_temporary_requestor_).c_str());
}

string RequestServingInfo::ToString() const {
  return strutil::StringPrintf("(export_: %s, media_: %s, media_format_: %s"
                               ", content_type_: %s"
                               ", authorizer_name_: %s"
                               ", offset_: %"PRId64""
                               ", size_: %"PRId64""
                               ", flavour: %x"
                               ", max_clients_: %d)",
                               export_path_.c_str(),
                               media_name_.c_str(),
                               MediaFormatName(media_format_),
                               content_type_.c_str(),
                               authorizer_name_.c_str(),
                               (offset_),
                               (size_),
                               flavour_mask_,
                               max_clients_);
}

string Request::ToString() const {
  ostringstream oss;
  oss << "Request{" << endl
      << "    - caps_: " << caps_.ToString() << endl
      << "    - info_: " << info_.ToString() << endl
      << "    - serving_info_: " << serving_info_.ToString() << endl
      << "    - callbacks_: " << callbacks_.size() << "{";
  for ( CallbacksMap::const_iterator it = callbacks_.begin();
        it != callbacks_.end(); ++it ) {
    if ( it != callbacks_.begin() ) {
      oss << ", ";
    }
    oss << it->first << ": " << it->second->name();
  }
  oss << "}" << endl
      << "    - rev_callbacks_: #" << rev_callbacks_.size() << endl
      << "    - temp_struct_: " << temp_struct_.ToString() << endl
      << "    - controller_: " << controller_ << endl
      << "    - deleted_: " << strutil::BoolToString(deleted_) << "}";
  return oss.str();
}

void RequestInfo::ToSpec(MediaRequestInfoSpec* spec,
                         bool save_auth) const {
  spec->seek_pos_ms_ = seek_pos_ms_;
  spec->media_origin_pos_ms_ = media_origin_pos_ms_;
  spec->limit_ms_ = limit_ms_;

  spec->session_id_ = session_id_;
  spec->affiliate_id_ = affiliate_id_;
  spec->client_id_ = client_id_;

  spec->remote_address_ = remote_address_.ToString();
  spec->local_address_ = local_address_.ToString();

  spec->ip_class_ = ip_class_;
  spec->path_ = path_;

  spec->is_internal_ = is_internal_;

  if ( save_auth ) {
    auth_req_.ToSpec(&spec->auth_req_.ref());
  }
}
void RequestInfo::FromSpec(const MediaRequestInfoSpec& spec) {
  seek_pos_ms_ = spec.seek_pos_ms_.get();
  media_origin_pos_ms_ = spec.media_origin_pos_ms_.get();
  limit_ms_ = spec.limit_ms_.get();

  session_id_ = spec.session_id_.get();
  affiliate_id_ = spec.affiliate_id_.get();
  client_id_ = spec.client_id_.get();

  remote_address_ = net::HostPort(spec.remote_address_.get());
  local_address_ = net::HostPort(spec.local_address_.get());

  ip_class_ = spec.ip_class_.get();
  path_ = spec.path_.get();

  is_internal_ = spec.is_internal_.get();

  if ( spec.auth_req_.is_set() ) {
    auth_req_.FromSpec(spec.auth_req_.get());
  }
}

TempElementStruct::~TempElementStruct() {
  for ( PolicyMap::const_iterator it = policies_.begin();
        it != policies_.end(); ++it ) {
    for ( int i = 0; i < it->second->size(); ++i ) {
      delete (*it->second)[i];
    }
    delete it->second;
  }
  CHECK(elements_.empty());
}

void TempElementStruct::Close(Closure* close_completed) {
  if ( elements_.empty() ) {
    close_completed->Run();
    return;
  }
  CHECK_NULL(close_completed_) << " Duplicate close?";
  close_completed_ = close_completed;
  ElementMap tmp(elements_);
  for ( ElementMap::const_iterator it = tmp.begin(); it != tmp.end(); ++it ) {
    Element* element = it->second;
    LOG_INFO << "Closing temp source: " << element->name();
    element->Close(NewCallback(this, &TempElementStruct::ElementCloseCompleted,
        element));
  }
}
void TempElementStruct::ElementCloseCompleted(Element* element) {
  CHECK_NOT_NULL(close_completed_) << " Where is close_completed?";
  LOG_INFO << "Deleting temp source: " << element->name();
  elements_.erase(element->name());
  delete element;
  // if all elements were closed, run the external close_completed callback
  if ( elements_.empty() ) {
    close_completed_->Run();
    close_completed_ = NULL;
  }
}
string TempElementStruct::ToString() const {
  ostringstream oss;
  oss << "TempElementStruct{element_name_: " << element_name_
      << ", elements_: " << strutil::ToStringKeys(elements_)
      << ", policies_: #" << policies_.size() << "}";
  return oss.str();
}

void RequestServingInfo::FromElementExportSpec(
    const ElementExportSpec& spec) {
  export_path_ = spec.path_.get();
  media_name_ = spec.media_name_.get();
  if ( spec.extra_headers_.is_set() ) {
    for ( int i = 0; i < spec.extra_headers_.get().size(); ++i ) {
      extra_headers_.push_back(
          make_pair(spec.extra_headers_.get()[i].name_.get(),
                    spec.extra_headers_.get()[i].value_.get()));
    }
  }
  if ( spec.content_type_.is_set() ) {
    content_type_ = spec.content_type_.get();
    if ( !MediaFormatFromSmallType(content_type_, &media_format_) &&
         !MediaFormatFromContentType(content_type_, &media_format_) ) {
      LOG_ERROR << "Failed to read media format from"
                   " content_type: [" << content_type_ << "]";
    }
   }
  if ( spec.authorizer_name_.is_set() ) {
    authorizer_name_ = spec.authorizer_name_.get();
  }
  if ( spec.flow_control_total_ms_.is_set() ) {
    flow_control_total_ms_ =
        spec.flow_control_total_ms_.get();
  }
  if ( spec.flow_control_video_ms_.is_set() ) {
    flow_control_video_ms_ =
        spec.flow_control_video_ms_.get();
  }
  if ( spec.flavour_mask_.is_set() ) {
    flavour_mask_is_set_ = true;
    flavour_mask_ = static_cast<uint32>(spec.flavour_mask_.get());
  }
  if ( spec.max_clients_.is_set() ) {
    max_clients_ = spec.max_clients_.get();
  } else {
    max_clients_ = -1;
  }
}
void RequestServingInfo::ToElementExportSpec(
    const string& media_key, ElementExportSpec* spec) const {
  pair<string, string> p = strutil::SplitFirst(media_key, ':');
  CHECK(p.second != "") << "Invalid media_key: " << media_key;

  spec->media_name_ = media_name_;
  spec->protocol_ = p.first;
  spec->path_ = p.second;
  spec->authorizer_name_ = authorizer_name_;

  if ( !content_type_.empty() ) {
    spec->content_type_ = content_type_;
  }
  for ( int i = 0; i < extra_headers_.size(); ++i ) {
    spec->extra_headers_.ref().push_back(
        ExtraHeaders(extra_headers_[i].first,
                     extra_headers_[i].second));
  }
  if ( flow_control_video_ms_ > 0 ) {
    spec->flow_control_video_ms_ = flow_control_video_ms_;
  }
  if ( flow_control_total_ms_ > 0 ) {
    spec->flow_control_total_ms_ = flow_control_total_ms_;
  }
  if ( flavour_mask_is_set_ ) {
    spec->flavour_mask_ = flavour_mask_;
  }
  if ( max_clients_ >= 0 ) {
    spec->max_clients_ = max_clients_;
  }
}

}
