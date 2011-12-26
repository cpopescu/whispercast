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
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_encoder.h>
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_decoder.h>
#include <whisperlib/net/rpc/lib/rpc_util.h>
#include <whisperlib/common/io/buffer/io_memory_stream.h>
#include <whisperlib/common/io/ioutil.h>

#include "elements/standard_library/standard_library.h"

#include "elements/standard_library/aio_file/aio_file_element.h"
#include "elements/standard_library/debugger/debugger_element.h"
#include "elements/standard_library/dropping/dropping_element.h"
#include "elements/standard_library/keyframe/keyframe_element.h"
#include "elements/standard_library/stream_renamer/stream_renamer_element.h"
#include "elements/standard_library/timesaving/timesaving_element.h"
#include "elements/standard_library/normalizing/normalizing_element.h"
#include "elements/standard_library/load_balancing/load_balancing_element.h"
#include "elements/standard_library/saving/saving_element.h"
#include "elements/standard_library/splitting/splitting_element.h"
#include "elements/standard_library/switching/switching_element.h"
#include "elements/standard_library/http_client/http_client_element.h"
#include "elements/standard_library/http_poster/http_poster_element.h"
#include "elements/standard_library/http_server/http_server_element.h"
#include "elements/standard_library/rtmp_publishing/rtmp_publishing_element.h"
#include "elements/standard_library/policies/policy.h"
#include "elements/standard_library/policies/failover_policy.h"
#include "elements/standard_library/simple_authorizer/simple_authorizer.h"
#include "elements/standard_library/remote_resolver/remote_resolver_element.h"
#include "elements/standard_library/lookup/lookup_element.h"
#include "elements/standard_library/f4v_to_flv_converter/f4v_to_flv_converter_element.h"
#include "elements/standard_library/redirect/redirecting_element.h"

#include "elements/standard_library/auto/standard_library_invokers.h"

//////////////////////////////////////////////////////////////////////

// Library Stub

#ifdef __cplusplus
extern "C" {
#endif

  void* GetElementLibrary() {
    return new streaming::StandardLibrary();
  }

#ifdef __cplusplus
}
#endif

//////////////////////////////////////////////////////////////////////

namespace streaming {
const char DebuggerElement::kElementClassName[] = "debugger";

StandardLibrary::StandardLibrary()
    : ElementLibrary("standard_library"),
      rpc_impl_(NULL) {
}

StandardLibrary::~StandardLibrary() {
  CHECK(rpc_impl_ == NULL)
      << " Deleting the library before unregistering the rpc";
}

void StandardLibrary::GetExportedElementTypes(vector<string>* element_types) {
  element_types->push_back(AioFileElement::kElementClassName);
  element_types->push_back(DroppingElement::kElementClassName);
  element_types->push_back(DebuggerElement::kElementClassName);
  element_types->push_back(HttpClientElement::kElementClassName);
  element_types->push_back(HttpPosterElement::kElementClassName);
  element_types->push_back(RemoteResolverElement::kElementClassName);
  element_types->push_back(HttpServerElement::kElementClassName);
  element_types->push_back(RtmpPublishingElement::kElementClassName);
  element_types->push_back(KeyFrameExtractorElement::kElementClassName);
  element_types->push_back(StreamRenamerElement::kElementClassName);
  element_types->push_back(NormalizingElement::kElementClassName);
  element_types->push_back(LoadBalancingElement::kElementClassName);
  element_types->push_back(SavingElement::kElementClassName);
  element_types->push_back(SplittingElement::kElementClassName);
  element_types->push_back(SwitchingElement::kElementClassName);
  element_types->push_back(TimeSavingElement::kElementClassName);
  element_types->push_back(LookupElement::kElementClassName);
  element_types->push_back(F4vToFlvConverterElement::kElementClassName);
  element_types->push_back(RedirectingElement::kElementClassName);
}

void StandardLibrary::GetExportedPolicyTypes(vector<string>* policy_types) {
  policy_types->push_back(RandomPolicy::kPolicyClassName);
  policy_types->push_back(PlaylistPolicy::kPolicyClassName);
  policy_types->push_back(TimedPlaylistPolicy::kPolicyClassName);
  policy_types->push_back(FailoverPolicy::kPolicyClassName);
  policy_types->push_back(OnCommandPolicy::kPolicyClassName);
}

void StandardLibrary::GetExportedAuthorizerTypes(vector<string>* auth_types) {
  auth_types->push_back(SimpleAuthorizer::kAuthorizerClassName);
}

int64 StandardLibrary::GetElementNeeds(const string& element_type) {
  if ( element_type == AioFileElement::kElementClassName ) {
    return (NEED_SELECTOR |
            NEED_AIO_FILES |
            NEED_MEDIA_DIR |
            NEED_SPLITTER_CREATOR);
  } else if ( element_type == DebuggerElement::kElementClassName ) {
    return (NEED_SELECTOR);
  } else if ( element_type == DroppingElement::kElementClassName ) {
    return (NEED_SELECTOR);
  } else if ( element_type == HttpClientElement::kElementClassName ) {
    return (NEED_SELECTOR |
            NEED_SPLITTER_CREATOR |
            NEED_HOST2IP_MAP);
  } else if ( element_type == HttpPosterElement::kElementClassName ) {
    return (NEED_SELECTOR |
            NEED_HOST2IP_MAP);
  } else if ( element_type == RemoteResolverElement::kElementClassName ) {
    return (NEED_SELECTOR);
  } else if ( element_type == HttpServerElement::kElementClassName ) {
    return (NEED_SELECTOR |
            NEED_MEDIA_DIR |
            NEED_RPC_SERVER |
            NEED_SPLITTER_CREATOR |
            NEED_HTTP_SERVER |
            NEED_STATE_KEEPER);
  } else if ( element_type == RtmpPublishingElement::kElementClassName ) {
    return (NEED_SELECTOR |
            NEED_MEDIA_DIR |
            NEED_RPC_SERVER |
            NEED_SPLITTER_CREATOR |
            NEED_STATE_KEEPER);
  } else if ( element_type == KeyFrameExtractorElement::kElementClassName ) {
    return (NEED_SELECTOR);
  } else if ( element_type == StreamRenamerElement::kElementClassName ) {
    return (NEED_SELECTOR);
  } else if ( element_type == NormalizingElement::kElementClassName ) {
    return (NEED_SELECTOR);
  } else if ( element_type == LookupElement::kElementClassName ) {
    return (NEED_SELECTOR |
            NEED_SPLITTER_CREATOR |
            NEED_HOST2IP_MAP);
  } else if ( element_type == LoadBalancingElement::kElementClassName ) {
    return (NEED_SELECTOR);
  } else if ( element_type == SavingElement::kElementClassName ) {
    return (NEED_SELECTOR |
            NEED_MEDIA_DIR);
  } else if ( element_type == SplittingElement::kElementClassName ) {
    return (NEED_SPLITTER_CREATOR |
            NEED_SELECTOR);
  } else if ( element_type == SwitchingElement::kElementClassName ) {
    return (NEED_RPC_SERVER |
            NEED_SELECTOR);
  } else if ( element_type == TimeSavingElement::kElementClassName ) {
    return (NEED_SELECTOR |
            NEED_STATE_KEEPER);
  } else if ( element_type == F4vToFlvConverterElement::kElementClassName ) {
    return (NEED_SELECTOR);
  } else if ( element_type == RedirectingElement::kElementClassName ) {
    return 0;
  }
  LOG_ERROR << "GetElementNeeds: Unknown element type: ["
            << element_type << "]";
  return 0;
}

int64 StandardLibrary::GetPolicyNeeds(const string& policy_type) {
  if ( policy_type == RandomPolicy::kPolicyClassName ) {
    return (NEED_SELECTOR |
            NEED_STATE_KEEPER);
  } else if ( policy_type == PlaylistPolicy::kPolicyClassName ) {
    return (NEED_RPC_SERVER |
            NEED_STATE_KEEPER |
            NEED_SELECTOR);
  } else if ( policy_type == TimedPlaylistPolicy::kPolicyClassName ) {
    return (NEED_SELECTOR |
            NEED_STATE_KEEPER);
  } else if ( policy_type == FailoverPolicy::kPolicyClassName ) {
    return (NEED_SELECTOR);
  } else if ( policy_type == OnCommandPolicy::kPolicyClassName ) {
    return (NEED_RPC_SERVER |
            NEED_SELECTOR |
            NEED_STATE_KEEPER);
  }
  LOG_ERROR << "GetPolicyNeeds: Unknown policy type: ["
            << policy_type << "]";
  return 0;
}

int64 StandardLibrary::GetAuthorizerNeeds(const string& auth_type) {
  if ( auth_type == SimpleAuthorizer::kAuthorizerClassName ) {
    return (NEED_RPC_SERVER |
            NEED_STATE_KEEPER);
  }

  return 0;
}

streaming::Element* StandardLibrary::CreateElement(
    const string& element_type,
    const string& name,
    const string& element_params,
    const streaming::Request* req,
    const CreationObjectParams& params,
    // Output:
    vector<string>* needed_policies,
    string* error_description) {
  streaming::Element* ret = NULL;
  if ( element_type == AioFileElement::kElementClassName ) {
    CREATE_ELEMENT_HELPER(AioFile);
    return ret;
  } else if ( element_type == DebuggerElement::kElementClassName ) {
    CREATE_ELEMENT_HELPER(Debugger);
    return ret;
  } else if ( element_type == DroppingElement::kElementClassName ) {
    CREATE_ELEMENT_HELPER(Dropping);
    return ret;
  } else if ( element_type == HttpClientElement::kElementClassName ) {
    CREATE_ELEMENT_HELPER(HttpClient);
    return ret;
  } else if ( element_type == HttpPosterElement::kElementClassName ) {
    CREATE_ELEMENT_HELPER(HttpPoster);
    return ret;
  } else if ( element_type == RemoteResolverElement::kElementClassName ) {
    CREATE_ELEMENT_HELPER(RemoteResolver);
    return ret;
  } else if ( element_type == HttpServerElement::kElementClassName ) {
    CREATE_ELEMENT_HELPER(HttpServer);
    return ret;
  } else if ( element_type == RtmpPublishingElement::kElementClassName ) {
    CREATE_ELEMENT_HELPER(RtmpPublishing);
    return ret;
  } else if ( element_type == KeyFrameExtractorElement::kElementClassName ) {
    CREATE_ELEMENT_HELPER(KeyFrameExtractor);
    return ret;
  } else if ( element_type == StreamRenamerElement::kElementClassName ) {
    CREATE_ELEMENT_HELPER(StreamRenamer);
    return ret;
  } else if ( element_type == NormalizingElement::kElementClassName ) {
    CREATE_ELEMENT_HELPER(Normalizing);
    return ret;
  } else if ( element_type == LoadBalancingElement::kElementClassName ) {
    CREATE_ELEMENT_HELPER(LoadBalancing);
    return ret;
  } else if ( element_type == SavingElement::kElementClassName ) {
    CREATE_ELEMENT_HELPER(Saving);
    return ret;
  } else if ( element_type == SplittingElement::kElementClassName ) {
    CREATE_ELEMENT_HELPER(Splitting);
    return ret;
  } else if ( element_type == SwitchingElement::kElementClassName ) {
    CREATE_ELEMENT_HELPER(Switching);
    return ret;
  } else if ( element_type == LookupElement::kElementClassName ) {
    CREATE_ELEMENT_HELPER(Lookup);
    return ret;
  } else if ( element_type == TimeSavingElement::kElementClassName ) {
    CREATE_ELEMENT_HELPER(TimeSaving);
    return ret;
  } else if ( element_type == F4vToFlvConverterElement::kElementClassName ) {
    CREATE_ELEMENT_HELPER(F4vToFlvConverter);
    return ret;
  } else if ( element_type == RedirectingElement::kElementClassName ) {
    CREATE_ELEMENT_HELPER(Redirecting);
    return ret;
  }
  LOG_ERROR << "Dunno to create element of type: '" << element_type << "'";
  return NULL;
}

streaming::Policy* StandardLibrary::CreatePolicy(
      const string& policy_type,
      const string& name,
      const string& policy_params,
      streaming::PolicyDrivenElement* element,
      const streaming::Request* req,
      const CreationObjectParams& params,
      // Output:
      string* error_description) {
  streaming::Policy* ret = NULL;
  if ( policy_type == RandomPolicy::kPolicyClassName ) {
    CREATE_POLICY_HELPER(Random);
    return ret;
  } else if ( policy_type == PlaylistPolicy::kPolicyClassName ) {
    CREATE_POLICY_HELPER(Playlist);
    return ret;
  } else if ( policy_type == TimedPlaylistPolicy::kPolicyClassName ) {
    CREATE_POLICY_HELPER(TimedPlaylist);
    return ret;
  } else if ( policy_type == FailoverPolicy::kPolicyClassName ) {
    CREATE_POLICY_HELPER(Failover);
    return ret;
  } else if ( policy_type == OnCommandPolicy::kPolicyClassName ) {
    CREATE_POLICY_HELPER(OnCommand);
    return ret;
  }
  return ret;
}

streaming::Authorizer* StandardLibrary::CreateAuthorizer(
    const string& authorizer_type,
    const string& name,
    const string& authorizer_params,
    const CreationObjectParams& params,
    // Output:
    string* error_description) {
  streaming::Authorizer* ret = NULL;
  if ( authorizer_type == SimpleAuthorizer::kAuthorizerClassName ) {
    CREATE_AUTHORIZER_HELPER(Simple);
    return ret;
  }
  return ret;
}


//////////////////////////////////////////////////////////////////////
//
// Element Creation functions:
//

///////  AioFile

streaming::Element* StandardLibrary::CreateAioFileElement(
    const string& element_name,
    const AioFileElementSpec& spec,
    const streaming::Request* req,
    const CreationObjectParams& params,
    vector<string>* needed_policies,
    string* error) {
  CHECK(mapper_ != NULL);
  const string home_dir(spec.home_dir_.c_str());
  const string full_home_dir(params.media_dir_ + "/" + home_dir);
  if ( !io::IsDir(full_home_dir.c_str()) ) {
    LOG_ERROR << "Cannot create media element under directory : ["
              << home_dir << "] (looking under: ["
              << full_home_dir << "]";
    delete params.splitter_creator_;
    return NULL;
  }
  Tag::Type tag_type;
  if ( !GetStreamType(spec.media_type_, &tag_type) ) {
    *error = "Invalid media type specified for AioFileElement";
    delete params.splitter_creator_;
    return NULL;
  }
  const char* data_key_prefix =
      spec.data_key_prefix_.is_set()
      ? spec.data_key_prefix_.c_str() : "";
  const bool disable_pause = (spec.disable_pause_.is_set() &&
                             spec.disable_pause_.get());
  const bool disable_seek = (spec.disable_seek_.is_set() &&
                             spec.disable_seek_.get());
  const bool disable_duration = (spec.disable_duration_.is_set() &&
                                 spec.disable_duration_.get());
  const char* default_index_file =
      spec.default_index_file_.is_set()
      ? spec.default_index_file_.c_str() : "";
  const string id(req != NULL
                  ? element_name + "-" + req->GetUrlId() : element_name);
  return new AioFileElement(element_name.c_str(),
                            id.c_str(),
                            mapper_,
                            params.selector_,
                            params.splitter_creator_,
                            params.aio_managers_,
                            params.buffer_manager_,
                            tag_type,
                            full_home_dir.c_str(),
                            spec.file_pattern_.c_str(),
                            default_index_file,
                            data_key_prefix,
                            disable_pause,
                            disable_seek,
                            disable_duration);
}

/////// Debugger

streaming::Element* StandardLibrary::CreateDebuggerElement(
    const string& element_name,
    const DebuggerElementSpec& spec,
    const streaming::Request* req,
    const CreationObjectParams& params,
    vector<string>* needed_policies,
    string* error) {
  const string id(req != NULL
                  ? element_name + "-" + req->GetUrlId() : element_name);
  return new streaming::DebuggerElement(
      element_name.c_str(),
      id.c_str(),
      mapper_,
      params.selector_);
}

/////// Dropping

streaming::Element* StandardLibrary::CreateDroppingElement(
    const string& element_name,
    const DroppingElementSpec& spec,
    const streaming::Request* req,
    const CreationObjectParams& params,
    vector<string>* needed_policies,
    string* error) {
  const string id(req != NULL
                  ? element_name + "-" + req->GetUrlId() : element_name);
  return new streaming::DroppingElement(
      element_name.c_str(),
      id.c_str(),
      mapper_,
      params.selector_,
      spec.media_filtered_.c_str(),
      spec.audio_accept_period_ms_,
      spec.audio_drop_period_ms_,
      spec.video_accept_period_ms_,
      spec.video_drop_period_ms_,
      spec.video_grace_period_key_frames_);
}

/////// HttpClient

streaming::Element* StandardLibrary::CreateHttpClientElement(
    const string& element_name,
    const HttpClientElementSpec& spec,
    const streaming::Request* req,
    const CreationObjectParams& params,
    vector<string>* needed_policies,
    string* error) {
  const int media_http_maximum_tag_size =
      spec.media_http_maximum_tag_size_.is_set()
      ? spec.media_http_maximum_tag_size_ : (1 << 18);
  const int32 prefill_buffer_ms =
      spec.prefill_buffer_ms_.is_set()
      ? spec.prefill_buffer_ms_ : 3000;
  const int32 advance_media_ms =
      spec.advance_media_ms_.is_set()
      ? spec.advance_media_ms_ : 2000;

  const string id(req != NULL
                  ? element_name + "-" + req->GetUrlId(): element_name);
  HttpClientElement* const src =
      new streaming::HttpClientElement(
          element_name.c_str(),
          id.c_str(),
          mapper_,
          params.selector_,
          params.host_aliases_,
          params.splitter_creator_,
          prefill_buffer_ms,
          advance_media_ms,
          media_http_maximum_tag_size);
  for ( int i = 0; spec.http_data_.size() > i; ++i )  {
    const HttpClientElementDataSpec&
        crt_spec = spec.http_data_[i];
    Tag::Type tag_type;
    if ( !GetStreamType(crt_spec.media_type_.c_str(), &tag_type) ) {
      LOG_ERROR << " Unknown media type: " << crt_spec.media_type_.c_str();
      continue;
    }
    bool fetch_only_on_request = false;
    if ( crt_spec.fetch_only_on_request_.is_set() ) {
      fetch_only_on_request = crt_spec.fetch_only_on_request_;
    }
    if ( !src->AddElement(crt_spec.name_.c_str(),
                          crt_spec.host_ip_.c_str(),
                          crt_spec.port_,
                          crt_spec.path_escaped_.c_str(),
                          crt_spec.should_reopen_,
                          fetch_only_on_request,
                          tag_type) ) {
      // TODO(cpopescu): should we report an error on this one ?
      LOG_ERROR << "Cannot AddElement " << crt_spec.ToString()
                << "  for HttpClientElement " << element_name;
    } else if ( crt_spec.remote_user_.is_set() ||
                crt_spec.remote_password_.is_set()  ) {
      const string user(crt_spec.remote_user_.is_set()
                        ? crt_spec.remote_user_.c_str()
                        : "");
      const string pass(crt_spec.remote_password_.is_set()
                        ? crt_spec.remote_password_.c_str()
                        : "");
      src->SetElementRemoteUser(crt_spec.name_.c_str(),
                                user.c_str(), pass.c_str());
    }
  }
  return src;
}

/////// HttpPoster

streaming::Element* StandardLibrary::CreateHttpPosterElement(
    const string& element_name,
    const HttpPosterElementSpec& spec,
    const streaming::Request* req,
    const CreationObjectParams& params,
    vector<string>* needed_policies,
    string* error) {
  Tag::Type tag_type;
  if ( !GetStreamType(spec.media_type_.c_str(), &tag_type) ) {
    LOG_ERROR << " Unknown media type: " << spec.media_type_.c_str();
    return NULL;
  }

  const int32 max_buffer_size =
      spec.max_buffer_size_.is_set()
      ? spec.max_buffer_size_
      : 256000;
  const int32 desired_http_chunk_size =
      spec.desired_http_chunk_size_.is_set()
      ? spec.desired_http_chunk_size_
      : 8192;
  const int64 media_retry_timeout_ms =
      spec.media_retry_timeout_ms_.is_set()
      ? spec.media_retry_timeout_ms_
      : 2500;
  const int64 http_retry_timeout_ms =
      spec.http_retry_timeout_ms_.is_set()
      ? spec.http_retry_timeout_ms_
      : 2500;

  const string user(spec.remote_user_.is_set()
                    ? spec.remote_user_.c_str()
                    : "");
  const string pass(spec.remote_password_.is_set()
                    ? spec.remote_password_.c_str()
                    : "");

  const string id(req != NULL
                  ? element_name + "-" + req->GetUrlId(): element_name);
  return new streaming::HttpPosterElement(
      element_name.c_str(),
      id.c_str(),
      mapper_,
      params.selector_,
      params.host_aliases_,
      spec.media_name_.c_str(),
      spec.host_ip_.c_str(),
      spec.port_,
      spec.path_escaped_.c_str(),
      tag_type,
      user.c_str(), pass.c_str(),
      max_buffer_size,
      desired_http_chunk_size,
      media_retry_timeout_ms,
      http_retry_timeout_ms);
}


/////// Lookup

streaming::Element* StandardLibrary::CreateLookupElement(
    const string& element_name,
    const LookupElementSpec& spec,
    const streaming::Request* req,
    const CreationObjectParams& params,
    vector<string>* needed_policies,
    string* error) {

  streaming::Element* http_client_element =
      CreateHttpClientElement(element_name + "_http_client",
                              spec.http_spec_,
                              req, params, needed_policies, error);
  if ( http_client_element == NULL ) {
    return NULL;
  }
  const string force_host_header =
      spec.lookup_force_host_header_.is_set()
      ? spec.lookup_force_host_header_.c_str() : "";
  const int num_retries =
      spec.lookup_num_retries_.is_set()
      ? spec.lookup_num_retries_ : 3;
  if ( num_retries < 1 ) {
    *error = "num_retries should be at least 1";
    return NULL;
  }
  const int max_concurrent_requests =
      spec.lookup_max_concurrent_requests_.is_set()
      ? spec.lookup_max_concurrent_requests_ : 1;
  if ( max_concurrent_requests < 1 ) {
    *error = "max_concurrent_requests should be at lease 1";
    return NULL;
  }
  const int req_timeout_ms =
      spec.lookup_req_timeout_ms_.is_set()
      ? spec.lookup_req_timeout_ms_ : 10000;
  if ( req_timeout_ms < 100 ) {
    *error = "unrealistic req_timeout_ms";
    return NULL;
  }
  vector<net::HostPort> servers;
  for ( int i = 0; i < spec.lookup_servers_.size(); ++i ) {
    net::HostPort hp(spec.lookup_servers_[i].c_str());
    if ( !hp.IsInvalid() ) {
      servers.push_back(hp);
    }
  }
  if ( servers.empty() ) {
    *error = "No valid servers_ host-port found";
    return NULL;
  }
  vector< pair<string, string> > http_headers;
  for ( int i = 0; i < spec.lookup_http_headers_.size(); ++i ) {
    http_headers.push_back(
        make_pair(
            spec.lookup_http_headers_[i].name_,
            spec.lookup_http_headers_[i].value_));
  }
  bool local_lookup_first = spec.local_lookup_first_.is_set()
                            ? spec.local_lookup_first_
                            : true;
  const string id(req != NULL
                  ? element_name + "-" + req->GetUrlId(): element_name);
  return new LookupElement(element_name.c_str(),
                           id.c_str(),
                           mapper_,
                           params.selector_,
                           reinterpret_cast<HttpClientElement*>(
                               http_client_element),
                           servers,
                           spec.lookup_query_path_format_,
                           http_headers,
                           num_retries,
                           max_concurrent_requests,
                           req_timeout_ms,
                           force_host_header,
                           local_lookup_first);
}

/////// RemoteResolver

streaming::Element* StandardLibrary::CreateRemoteResolverElement(
    const string& element_name,
    const RemoteResolverElementSpec& spec,
    const streaming::Request* req,
    const CreationObjectParams& params,
    vector<string>* needed_policies,
    string* error) {
  Tag::Type tag_type;
  if ( !GetStreamType(spec.media_type_.c_str(), &tag_type) ) {
    *error = string(" Unknown media type: ") + spec.media_type_.get();
    LOG_ERROR << " Unknown media type: " << spec.media_type_.c_str();
    return NULL;
  }
  const uint32 flavour_mask =
      spec.flavour_mask_.is_set()
      ? spec.flavour_mask_ : streaming::kDefaultFlavourMask;

  const int num_retries =
      spec.lookup_num_retries_.is_set()
      ? spec.lookup_num_retries_ : 3;
  if ( num_retries < 1 ) {
    *error = "num_retries should be at least 1";
    return NULL;
  }
  const int max_concurrent_requests =
      spec.lookup_max_concurrent_requests_.is_set()
      ? spec.lookup_max_concurrent_requests_ : 1;
  if ( max_concurrent_requests < 1 ) {
    *error = "max_concurrent_requests should be at lease 1";
    return NULL;
  }
  const int req_timeout_ms =
      spec.lookup_req_timeout_ms_.is_set()
      ? spec.lookup_req_timeout_ms_ : 10000;
  if ( req_timeout_ms < 100 ) {
    *error = "unrealistic req_timeout_ms";
    return NULL;
  }
  vector<net::HostPort> servers;
  for ( int i = 0; i < spec.lookup_servers_.size(); ++i ) {
    net::HostPort hp(spec.lookup_servers_[i].c_str());
    if ( !hp.IsInvalid() ) {
      servers.push_back(hp);
    }
  }
  if ( servers.empty() ) {
    *error = "No valid servers_ host-port found";
    return NULL;
  }
  string auth_user, auth_pass;
  if ( spec.lookup_auth_user_.is_set() ) {
    auth_user = spec.lookup_auth_user_;
  }
  if ( spec.lookup_auth_pass_.is_set() ) {
    auth_pass = spec.lookup_auth_pass_;
  }
  string lookup_rpc_path("/rpc-config/standard_library");
  if ( spec.lookup_rpc_path_.is_set() ) {
    lookup_rpc_path = spec.lookup_rpc_path_;
  }
  bool local_lookup_first = spec.local_lookup_first_.is_set()
                            ? spec.local_lookup_first_
                            : true;
  const string id(req != NULL
                  ? element_name + "-" + req->GetUrlId(): element_name);
  return new RemoteResolverElement(
      element_name.c_str(),
      id.c_str(),
      mapper_,
      params.selector_,
      spec.cache_expiration_time_ms_,
      servers,
      lookup_rpc_path,
      num_retries,
      max_concurrent_requests,
      req_timeout_ms,
      local_lookup_first,
      auth_user, auth_pass,
      streaming::Capabilities(tag_type, flavour_mask));
}

/////// HttpServer

streaming::Element* StandardLibrary::CreateHttpServerElement(
    const string& element_name,
    const HttpServerElementSpec& spec,
    const streaming::Request* req,
    const CreationObjectParams& params,
    vector<string>* needed_policies,
    string* error) {
  delete params.local_state_keeper_;
  if ( req != NULL ) {
    delete params.state_keeper_;
    delete params.splitter_creator_;
    LOG_ERROR << " Cannot create temporary HttpServerElement";
    return NULL;
  }
  string rpc_path(name() + "/" + element_name + "/");
  string authorizer_name;
  if ( spec.authorizer_name_.is_set() ) {
    authorizer_name = spec.authorizer_name_;
  }
  const string id(req != NULL
                  ? element_name + "-" + req->GetUrlId(): element_name);
  HttpServerElement* const src =
      new streaming::HttpServerElement(element_name,
                                       id,
                                       mapper_,
                                       spec.path_escaped_,
                                       params.selector_,
                                       params.media_dir_,
                                       params.http_server_,
                                       rpc_path.c_str(),
                                       params.rpc_server_,
                                       params.state_keeper_,
                                       params.splitter_creator_,
                                       authorizer_name);
  for ( int i = 0; i < spec.http_data_.size(); ++i ) {
    const HttpServerElementDataSpec& crt_spec = spec.http_data_[i];
    MediaOperationErrorData ret;
    src->AddImport(&ret, crt_spec, false);  // do not save to state ..
  }
  return src;
}

/////// RtmpPublishing

streaming::Element* StandardLibrary::CreateRtmpPublishingElement(
    const string& element_name,
    const RtmpPublishingElementSpec& spec,
    const streaming::Request* req,
    const CreationObjectParams& params,
    vector<string>* needed_policies,
    string* error) {
  delete params.local_state_keeper_;
  string rpc_path(name() + "/" + element_name + "/");
  if ( req != NULL ) {
    delete params.state_keeper_;
    delete params.splitter_creator_;
    LOG_ERROR << " Cannot create temporary RtmpPublishingElement";
    return NULL;   // Only global publishing elements pls
  }
  string authorizer_name;
  if ( spec.authorizer_name_.is_set() ) {
    authorizer_name = spec.authorizer_name_;
  }
  const string id(req != NULL
                  ? element_name + "-" + req->GetUrlId(): element_name);
  RtmpPublishingElement* const src =
      new streaming::RtmpPublishingElement(element_name,
                                           id,
                                           mapper_,
                                           params.selector_,
                                           params.media_dir_,
                                           rpc_path,
                                           params.rpc_server_,
                                           params.state_keeper_,
                                           params.splitter_creator_,
                                           authorizer_name);
  for ( int i = 0; i < spec.rtmp_data_.size(); ++i ) {
    const RtmpPublishingElementDataSpec& crt_spec = spec.rtmp_data_[i];
    MediaOperationErrorData ret;
    src->AddImport(&ret, crt_spec, false);  // do not save to state ..
  }
  return src;
}

/////// KeyFrameExtractor

streaming::Element* StandardLibrary::CreateKeyFrameExtractorElement(
    const string& element_name,
    const KeyFrameExtractorElementSpec& spec,
    const streaming::Request* req,
    const CreationObjectParams& params,
    vector<string>* needed_policies,
    string* error) {
  const string id(req != NULL
                  ? element_name + "-" + req->GetUrlId(): element_name);
  return new KeyFrameExtractorElement(
      element_name.c_str(),
      id.c_str(),
      mapper_,
      params.selector_,
      spec.media_filtered_.c_str(),
      spec.ms_between_video_frames_,
      spec.drop_audio_);
}

/////// StreamRenamer

streaming::Element* StandardLibrary::CreateStreamRenamerElement(
    const string& element_name,
    const StreamRenamerElementSpec& spec,
    const streaming::Request* req,
    const CreationObjectParams& params,
    vector<string>* needed_policies,
    string* error) {
  const string id(req != NULL
                  ? element_name + "-" + req->GetUrlId(): element_name);
  return new StreamRenamerElement(
      element_name.c_str(),
      id.c_str(),
      mapper_,
      params.selector_,
      spec.pattern_,
      spec.replace_);
}

/////// Normalizing

streaming::Element* StandardLibrary::CreateNormalizingElement(
    const string& element_name,
    const NormalizingElementSpec& spec,
    const streaming::Request* req,
    const CreationObjectParams& params,
    vector<string>* needed_policies,
    string* error) {
  const int64 flow_control_write_ahead_ms =
      spec.flow_control_write_ahead_ms_.is_set()
      ? spec.flow_control_write_ahead_ms_
      : 1500;
  const int64 flow_control_extra_write_ahead_ms =
      spec.flow_control_extra_write_ahead_ms_.is_set()
      ? spec.flow_control_extra_write_ahead_ms_
      : 500;
  const string id(req != NULL
                  ? element_name + "-" + req->GetUrlId(): element_name);
  return new streaming::NormalizingElement(element_name.c_str(),
                                           id.c_str(),
                                           mapper_,
                                           params.selector_,
                                           flow_control_write_ahead_ms,
                                           flow_control_extra_write_ahead_ms);
}

/////// Normalizing

streaming::Element* StandardLibrary::CreateLoadBalancingElement(
    const string& element_name,
    const LoadBalancingElementSpec& spec,
    const streaming::Request* req,
    const CreationObjectParams& params,
    vector<string>* needed_policies,
    string* error) {
  if ( spec.sub_elements_.size() == 0 ) {
    *error = "Please specify some sub elements";
    return NULL;
  }
  vector<string> sub_elements;
  for ( int i = 0; i < spec.sub_elements_.size(); ++i ) {
    sub_elements.push_back(spec.sub_elements_[i]);
  }
  const string id(req != NULL
                  ? element_name + "-" + req->GetUrlId(): element_name);
  return new streaming::LoadBalancingElement(element_name.c_str(),
                                             id.c_str(),
                                             mapper_,
                                             params.selector_,
                                             sub_elements);
}

/////// Saving

streaming::Element* StandardLibrary::CreateSavingElement(
    const string& element_name,
    const SavingElementSpec& spec,
    const streaming::Request* req,
    const CreationObjectParams& params,
    vector<string>* needed_policies,
    string* error) {
  const string id(req != NULL
                  ? element_name + "-" + req->GetUrlId(): element_name);
  return new streaming::SavingElement(element_name.c_str(),
                                      id.c_str(),
                                      mapper_,
                                      params.selector_,
                                      params.media_dir_,
                                      spec.media_.get(),
                                      spec.save_dir_.get());
}

/////// Splitting

streaming::Element* StandardLibrary::CreateSplittingElement(
    const string& element_name,
    const SplittingElementSpec& spec,
    const streaming::Request* req,
    const CreationObjectParams& params,
    vector<string>* needed_policies,
    string* error) {
  Tag::Type tag_type;
  if ( !GetStreamType(spec.splitter_media_type_.c_str(), &tag_type) ) {
    *error = string(" Unknown media type: ") +
             spec.splitter_media_type_.get();
    LOG_ERROR << " Unknown media type: "
              << spec.splitter_media_type_.ToString();
    delete params.splitter_creator_;
    return NULL;
  }
  const int max_tag_size =
      spec.max_tag_size_.is_set()
      ? spec.max_tag_size_
      : (1 << 18);
  const string id(req != NULL
                  ? element_name + "-" + req->GetUrlId(): element_name);
  return new streaming::SplittingElement(element_name.c_str(),
                                         id.c_str(),
                                         mapper_,
                                         params.selector_,
                                         params.splitter_creator_,
                                         tag_type,
                                         max_tag_size);
}

/////// Switching

streaming::Element* StandardLibrary::CreateSwitchingElement(
    const string& element_name,
    const SwitchingElementSpec& spec,
    const streaming::Request* req,
    const CreationObjectParams& params,
    vector<string>* needed_policies,
    string* error) {
  Tag::Type tag_type;
  if ( !GetStreamType(spec.media_type_.c_str(), &tag_type) ) {
    *error = string(" Unknown media type: ") + spec.media_type_.get();
    LOG_ERROR << " Unknown media type: " << spec.media_type_.c_str();
    return NULL;
  }
  const uint32 flavour_mask =
      spec.flavour_mask_.is_set()
      ? 1 * spec.flavour_mask_ : streaming::kDefaultFlavourMask;
  const int default_flavour_id =
      spec.default_flavour_id_.is_set()
      ? 1 * spec.default_flavour_id_ : streaming::kDefaultFlavourId;

  string rpc_path(name() + "/" + element_name + "/");
  if ( req != NULL ) {
    rpc_path += req->info().GetPathId() + "/";
  }
  const int tag_timeout_ms =
      spec.tag_timeout_sec_.is_set()
      ? spec.tag_timeout_sec_ * 1000 : 20000;
  const bool media_only_when_used =
      spec.media_only_when_used_.is_set()
      ? spec.media_only_when_used_ : false;
  const int write_ahead_ms =
      spec.write_ahead_ms_.is_set()
      ? spec.write_ahead_ms_ : 1000;
  needed_policies->push_back(spec.policy_name_);
  const string id(req != NULL
                  ? element_name + "-" + req->GetUrlId(): element_name);
  return new streaming::SwitchingElement(
      element_name.c_str(),
      id.c_str(),
      mapper_,
      params.selector_,
      rpc_path.c_str(),
      params.rpc_server_,
      streaming::Capabilities(tag_type, flavour_mask),
      default_flavour_id,
      tag_timeout_ms,
      write_ahead_ms,
      media_only_when_used,
      // Save media bootstrap only for global
      req == NULL);
}

/////// TimeSaving

streaming::Element* StandardLibrary::CreateTimeSavingElement(
    const string& element_name,
    const TimeSavingElementSpec& spec,
    const streaming::Request* req,
    const CreationObjectParams& params,
    vector<string>* needed_policies,
    string* error) {
  delete params.state_keeper_;
  const string id(req != NULL
                  ? element_name + "-" + req->GetUrlId(): element_name);
  return new streaming::TimeSavingElement(element_name.c_str(),
                                          id.c_str(),
                                          mapper_,
                                          params.selector_,
                                          params.local_state_keeper_);
}

/////// F4vToFlvConverter

streaming::Element* StandardLibrary::CreateF4vToFlvConverterElement(
    const string& element_name,
    const F4vToFlvConverterElementSpec& spec,
    const streaming::Request* req,
    const CreationObjectParams& params,
    vector<string>* needed_policies,
    string* error) {
  const string id(req != NULL
                  ? element_name + "-" + req->GetUrlId(): element_name);
  return new streaming::F4vToFlvConverterElement(element_name.c_str(),
                                                 id.c_str(),
                                                 mapper_,
                                                 params.selector_);
}

/////// Redirecting

streaming::Element* StandardLibrary::CreateRedirectingElement(
    const string& element_name,
    const RedirectingElementSpec& spec,
    const streaming::Request* req,
    const CreationObjectParams& params,
    vector<string>* needed_policies,
    string* error) {
  const string id(req != NULL
                  ? element_name + "-" + req->GetUrlId(): element_name);
  return new streaming::RedirectingElement(element_name.c_str(),
                                           id.c_str(),
                                           mapper_,
                                           spec.redirections_);
}


//////////////////////////////////////////////////////////////////////
//
// Policy Creation Functions
//

/////// Random

streaming::Policy* StandardLibrary::CreateRandomPolicy(
    const string& name,
    const RandomPolicySpec& spec,
    streaming::PolicyDrivenElement* element,
    const streaming::Request* req,
    const CreationObjectParams& params,
    string* error) {
  const int64 to_ms = spec.state_timeout_sec_.is_set()
                      ? 1000LL * spec.state_timeout_sec_
                      : static_cast<int64>(-1);
  if ( params.local_state_keeper_ != NULL ) {
    params.local_state_keeper_->set_timeout_ms(to_ms);
  }
  return new RandomPolicy(
      name.c_str(),
      element,
      req != NULL,
      params.state_keeper_,
      params.local_state_keeper_,
      spec.max_history_size_);
}

/////// Playlist

streaming::Policy* StandardLibrary::CreatePlaylistPolicy(
    const string& policy_name,
    const PlaylistPolicySpec& spec,
    streaming::PolicyDrivenElement* element,
    const streaming::Request* req,
    const CreationObjectParams& params,
    string* error) {
  string rpc_path(name() + "/");
  rpc_path += element->name() + "/" + policy_name + "/" ;
  string local_rpc_path(rpc_path);
  if ( req != NULL ) {
    local_rpc_path += req->info().GetPathId() + "/";
  }
  const bool loop_playlist = spec.loop_playlist_;
  vector<string> playlist;
  for ( int i = 0; i < spec.playlist_.size(); ++i ) {
    playlist.push_back(spec.playlist_[i]);
  }
  const int64 to_ms = spec.state_timeout_sec_.is_set()
                      ? 1000LL * spec.state_timeout_sec_
                      : static_cast<int64>(-1);
  if ( params.local_state_keeper_ != NULL ) {
    params.local_state_keeper_->set_timeout_ms(to_ms);
  }
  rpc::HttpServer* rpc_server = params.rpc_server_;
  if ( spec.disable_rpc_.is_set() && spec.disable_rpc_ ) {
    rpc_server = NULL;
  }
  return new PlaylistPolicy(
      policy_name.c_str(),
      element,
      req != NULL,
      params.state_keeper_,
      params.local_state_keeper_,
      playlist, loop_playlist,
      rpc_path.c_str(),
      local_rpc_path.c_str(),
      rpc_server);
}

/////// TimedPlaylist

streaming::Policy* StandardLibrary::CreateTimedPlaylistPolicy(
    const string& policy_name,
    const TimedPlaylistPolicySpec& spec,
    streaming::PolicyDrivenElement* element,
    const streaming::Request* req,
    const CreationObjectParams& params,
    string* error) {
  const int empty_policy = spec.empty_policy_;
  const bool loop_playlist = spec.loop_playlist_;
  if ( empty_policy >= TimedPlaylistPolicy::NUM_EMPTY_POLICY ||
       empty_policy < TimedPlaylistPolicy::FIRST_EMPTY_POLICY ) {
    *error = string(" Invalid empty policy for ") + spec.ToString();
    LOG_ERROR << " Invalid empty policy for " << spec.ToString();
    return NULL;
  }
  vector< pair<int64, string> > playlist;
  for ( int i = 0; i < spec.playlist_.size(); ++i ) {
    const TimePolicySpec& crt = spec.playlist_[i];
    playlist.push_back(make_pair(crt.time_in_ms_,
                                 crt.media_name_));
  }
  const int64 to_ms = spec.state_timeout_sec_.is_set()
                      ? 1000LL * spec.state_timeout_sec_
                      : static_cast<int64>(-1);
  if ( params.local_state_keeper_ != NULL ) {
    params.local_state_keeper_->set_timeout_ms(to_ms);
  }
  return new TimedPlaylistPolicy(
      policy_name.c_str(),
      element,
      params.selector_,
      req != NULL,
      params.state_keeper_,
      params.local_state_keeper_,
      playlist,
      static_cast<TimedPlaylistPolicy::EmptyPolicy>(empty_policy),
      loop_playlist);
}


/////// Failover

streaming::Policy* StandardLibrary::CreateFailoverPolicy(
    const string& policy_name,
    const FailoverPolicySpec& spec,
    streaming::PolicyDrivenElement* element,
    const streaming::Request* req,
    const CreationObjectParams& params,
    string* error) {
  int32 main_media_tags_received_switch_limit = 2;  // tags
  if ( spec.main_media_tags_received_switch_limit_.is_set() ) {
    main_media_tags_received_switch_limit =
        spec.main_media_tags_received_switch_limit_;
  }
  int32 failover_timeout_sec = 2;  // sec
  if ( spec.failover_timeout_sec_.is_set() ) {
    failover_timeout_sec = spec.failover_timeout_sec_;
  }
  return new FailoverPolicy(
      policy_name.c_str(),
      element,
      mapper_,
      params.selector_,
      spec.main_media_,
      spec.failover_media_,
      main_media_tags_received_switch_limit,
      failover_timeout_sec,
      spec.change_to_main_only_on_switch_);
}


/////// OnCommand

streaming::Policy* StandardLibrary::CreateOnCommandPolicy(
    const string& policy_name,
    const OnCommandPolicySpec& spec,
    streaming::PolicyDrivenElement* element,
    const streaming::Request* req,
    const CreationObjectParams& params,
    string* error) {
  string rpc_path(name() + "/");
  rpc_path += element->name() + "/" + policy_name + "/" ;
  string local_rpc_path(rpc_path);
  if ( req != NULL ) {
    local_rpc_path += req->info().GetPathId() + "/";
  }
  const int64 to_ms = spec.state_timeout_sec_.is_set()
                      ? 1000LL * spec.state_timeout_sec_
                      : static_cast<int64>(-1);
  if ( params.local_state_keeper_ != NULL ) {
    params.local_state_keeper_->set_timeout_ms(to_ms);
  }
  rpc::HttpServer* rpc_server = params.rpc_server_;
  if ( spec.disable_rpc_.is_set() && spec.disable_rpc_ ) {
    rpc_server = NULL;
  }
  return new OnCommandPolicy(
      policy_name.c_str(),
      element,
      req != NULL,
      params.state_keeper_,
      params.local_state_keeper_,
      spec.default_element_,
      rpc_path.c_str(),
      local_rpc_path.c_str(),
      rpc_server);
}

streaming::Authorizer* StandardLibrary::CreateSimpleAuthorizer(
    const string& auth_name,
    const SimpleAuthorizerSpec& spec,
    const CreationObjectParams& params,
    string* error) {
  string rpc_path(name() + "/" + auth_name + "/");
  int32 reauthorize_interval_ms = 0;
  if ( spec.reauthorize_interval_ms_.is_set() ) {
    reauthorize_interval_ms = spec.reauthorize_interval_ms_;
  }
  int32 time_limit_ms = 0;
  if ( spec.time_limit_ms_.is_set() ) {
    time_limit_ms = spec.time_limit_ms_;
  }
  delete params.local_state_keeper_;
  return new SimpleAuthorizer(
      auth_name.c_str(),
      reauthorize_interval_ms,
      time_limit_ms,
      params.state_keeper_,
      rpc_path.c_str(),
      params.rpc_server_);
}

//////////////////////////////////////////////////////////////////////

class ServiceInvokerStandardLibraryServiceImpl
    : public ServiceInvokerStandardLibraryService {
 public:
  ServiceInvokerStandardLibraryServiceImpl(
      streaming::ElementMapper* mapper,
      streaming::ElementLibrary::ElementSpecCreationCallback*
      create_element_spec_callback,
      streaming::ElementLibrary::PolicySpecCreationCallback*
      create_policy_spec_callback,
      streaming::ElementLibrary::AuthorizerSpecCreationCallback*
      create_authorizer_spec_callback)
      : ServiceInvokerStandardLibraryService(
          ServiceInvokerStandardLibraryService::GetClassName()),
        mapper_(mapper),
        create_element_spec_callback_(create_element_spec_callback),
        create_policy_spec_callback_(create_policy_spec_callback),
        create_authorizer_spec_callback_(create_authorizer_spec_callback) {
  }

 private:
  virtual void AddAioFileElementSpec(
      rpc::CallContext< MediaOperationErrorData >* call,
      const string& name,
      bool is_global,
      bool disable_rpc,
      const AioFileElementSpec& spec) {
    STANDARD_RPC_ELEMENT_ADD(AioFile);
  }
  virtual void AddDebuggerElementSpec(
      rpc::CallContext< MediaOperationErrorData >* call,
      const string& name,
      bool is_global,
      bool disable_rpc,
      const DebuggerElementSpec& spec) {
    STANDARD_RPC_ELEMENT_ADD(Debugger);
  }
  virtual void AddDroppingElementSpec(
      rpc::CallContext< MediaOperationErrorData >* call,
      const string& name,
      bool is_global,
      bool disable_rpc,
      const DroppingElementSpec& spec) {
    STANDARD_RPC_ELEMENT_ADD(Dropping);
  }
  virtual void AddHttpClientElementSpec(
      rpc::CallContext< MediaOperationErrorData >* call,
      const string& name,
      bool is_global,
      bool disable_rpc,
      const HttpClientElementSpec& spec) {
    STANDARD_RPC_ELEMENT_ADD(HttpClient);
  }
  virtual void AddHttpPosterElementSpec(
      rpc::CallContext< MediaOperationErrorData >* call,
      const string& name,
      bool is_global,
      bool disable_rpc,
      const HttpPosterElementSpec& spec) {
    STANDARD_RPC_ELEMENT_ADD(HttpPoster);
  }
  virtual void AddRemoteResolverElementSpec(
      rpc::CallContext< MediaOperationErrorData >* call,
      const string& name,
      bool is_global,
      bool disable_rpc,
      const RemoteResolverElementSpec& spec) {
    STANDARD_RPC_ELEMENT_ADD(RemoteResolver);
  }
  virtual void AddHttpServerElementSpec(
      rpc::CallContext< MediaOperationErrorData >* call,
      const string& name,
      bool is_global,
      bool disable_rpc,
      const HttpServerElementSpec& spec) {
    STANDARD_RPC_ELEMENT_ADD(HttpServer);
  }
  virtual void AddRtmpPublishingElementSpec(
      rpc::CallContext< MediaOperationErrorData >* call,
      const string& name,
      bool is_global,
      bool disable_rpc,
      const RtmpPublishingElementSpec& spec) {
    STANDARD_RPC_ELEMENT_ADD(RtmpPublishing);
  }
  virtual void AddKeyFrameExtractorElementSpec(
      rpc::CallContext< MediaOperationErrorData >* call,
      const string& name,
      bool is_global,
      bool disable_rpc,
      const KeyFrameExtractorElementSpec& spec) {
    STANDARD_RPC_ELEMENT_ADD(KeyFrameExtractor);
  }
  virtual void AddStreamRenamerElementSpec(
      rpc::CallContext< MediaOperationErrorData >* call,
      const string& name,
      bool is_global,
      bool disable_rpc,
      const StreamRenamerElementSpec& spec) {
    STANDARD_RPC_ELEMENT_ADD(StreamRenamer);
  }
  virtual void AddNormalizingElementSpec(
      rpc::CallContext< MediaOperationErrorData >* call,
      const string& name,
      bool is_global,
      bool disable_rpc,
      const NormalizingElementSpec& spec) {
    STANDARD_RPC_ELEMENT_ADD(Normalizing);
  }
  virtual void AddLoadBalancingElementSpec(
      rpc::CallContext< MediaOperationErrorData >* call,
      const string& name,
      bool is_global,
      bool disable_rpc,
      const LoadBalancingElementSpec& spec) {
    STANDARD_RPC_ELEMENT_ADD(LoadBalancing);
  }
  virtual void AddSavingElementSpec(
      rpc::CallContext< MediaOperationErrorData >* call,
      const string& name,
      bool is_global,
      bool disable_rpc,
      const SavingElementSpec& spec) {
    STANDARD_RPC_ELEMENT_ADD(Saving);
  }
  virtual void AddSplittingElementSpec(
      rpc::CallContext< MediaOperationErrorData >* call,
      const string& name,
      bool is_global,
      bool disable_rpc,
      const SplittingElementSpec& spec) {
    STANDARD_RPC_ELEMENT_ADD(Splitting);
  }
  virtual void AddSwitchingElementSpec(
      rpc::CallContext< MediaOperationErrorData >* call,
      const string& name,
      bool is_global,
      bool disable_rpc,
      const SwitchingElementSpec& spec) {
    STANDARD_RPC_ELEMENT_ADD(Switching);
  }
  virtual void AddTimeSavingElementSpec(
      rpc::CallContext< MediaOperationErrorData >* call,
      const string& name,
      bool is_global,
      bool disable_rpc,
      const TimeSavingElementSpec& spec) {
    STANDARD_RPC_ELEMENT_ADD(TimeSaving);
  }
  virtual void AddLookupElementSpec(
      rpc::CallContext< MediaOperationErrorData >* call,
      const string& name,
      bool is_global,
      bool disable_rpc,
      const LookupElementSpec& spec) {
    STANDARD_RPC_ELEMENT_ADD(Lookup);
  }
  virtual void AddF4vToFlvConverterElementSpec(
      rpc::CallContext< MediaOperationErrorData >* call,
      const string& name,
      bool is_global,
      bool disable_rpc,
      const F4vToFlvConverterElementSpec& spec) {
    STANDARD_RPC_ELEMENT_ADD(F4vToFlvConverter);
  }
  virtual void AddRedirectingElementSpec(
      rpc::CallContext< MediaOperationErrorData >* call,
      const string& name,
      bool is_global,
      bool disable_rpc,
      const RedirectingElementSpec& spec) {
    STANDARD_RPC_ELEMENT_ADD(Redirecting);
  }

  /////

  virtual void AddRandomPolicySpec(
      rpc::CallContext< MediaOperationErrorData >* call,
      const string& name,
      const RandomPolicySpec& spec) {
    STANDARD_RPC_POLICY_ADD(Random);
  }
  virtual void AddPlaylistPolicySpec(
      rpc::CallContext< MediaOperationErrorData >* call,
      const string& name,
      const PlaylistPolicySpec& spec) {
    STANDARD_RPC_POLICY_ADD(Playlist);
  }
  virtual void AddTimedPlaylistPolicySpec(
      rpc::CallContext< MediaOperationErrorData >* call,
      const string& name,
      const TimedPlaylistPolicySpec& spec) {
    STANDARD_RPC_POLICY_ADD(TimedPlaylist);
  }
  virtual void AddFailoverPolicySpec(
      rpc::CallContext< MediaOperationErrorData >* call,
      const string& name,
      const FailoverPolicySpec& spec) {
    STANDARD_RPC_POLICY_ADD(Failover);
  }
  virtual void AddOnCommandPolicySpec(
      rpc::CallContext< MediaOperationErrorData >* call,
      const string& name,
      const OnCommandPolicySpec& spec) {
    STANDARD_RPC_POLICY_ADD(OnCommand);
  }
  virtual void AddSimpleAuthorizerSpec(
      rpc::CallContext< MediaOperationErrorData >* call,
      const string& name,
      const SimpleAuthorizerSpec& spec) {
    STANDARD_RPC_AUTHORIZER_ADD(Simple);
  }
  virtual void ResolveMedia(
      rpc::CallContext< ResolveSpec >* call,
      const string& media);
  virtual void GetSwitchCurrentMedia(
      rpc::CallContext< string >* call,
      const string& media);
  virtual void RecursiveGetSwitchCurrentMedia(
      rpc::CallContext< string >* call,
      const string& media,
      int32 max_recursion);

 private:
  string GetSwitchCurrentMedia(const string& media);

  streaming::ElementMapper* mapper_;
  streaming::ElementLibrary::ElementSpecCreationCallback*
  create_element_spec_callback_;
  streaming::ElementLibrary::PolicySpecCreationCallback*
  create_policy_spec_callback_;
  streaming::ElementLibrary::AuthorizerSpecCreationCallback*
  create_authorizer_spec_callback_;

  DISALLOW_EVIL_CONSTRUCTORS(ServiceInvokerStandardLibraryServiceImpl);
};

string ServiceInvokerStandardLibraryServiceImpl::GetSwitchCurrentMedia(
    const string& media) {
  streaming::Element* element;
  vector<streaming::Policy*>* policies;
  if ( mapper_->GetElementByName(media, &element, &policies) ) {
    if ( element != NULL && policies != NULL && policies->size() == 1 &&
         element->type() == SwitchingElement::kElementClassName ) {
      const SwitchingElement* const sw_element =
          reinterpret_cast<const SwitchingElement*>(element);
      return sw_element->current_media();
    }
    return media;
  }
  return "";
}

void ServiceInvokerStandardLibraryServiceImpl::GetSwitchCurrentMedia(
    rpc::CallContext< string >* call,
    const string& media) {
  call->Complete(GetSwitchCurrentMedia(media));
}

void ServiceInvokerStandardLibraryServiceImpl::RecursiveGetSwitchCurrentMedia(
    rpc::CallContext< string >* call,
    const string& media,
    int32 max_recursion) {
  int crt_max_recursion = min(max_recursion, 16);
  string ret_media = media;
  size_t slash_pos = 0;
  string query_media = ret_media;
  while ( crt_max_recursion-- > 0 && !query_media.empty() ) {
    const string crt = GetSwitchCurrentMedia(query_media);
    if ( crt.empty() || crt == query_media ) {
      break;
    }
    ret_media.resize(slash_pos);
    if ( !ret_media.empty() ) {
      ret_media.append("/");
    }
    ret_media.append(crt);
    slash_pos = ret_media.rfind('/');
    if ( slash_pos == string::npos ) {
      query_media = ret_media;
      slash_pos = 0;
    } else {
      query_media = crt.substr(slash_pos + 1);
    }
  }
  call->Complete(ret_media);
}



void ServiceInvokerStandardLibraryServiceImpl::ResolveMedia(
    rpc::CallContext< ResolveSpec >* call,
    const string& media) {
  ResolveSpec ret;
  ret.loop_ = false;
  ret.media_.ref();  // make sure this gets set
  streaming::Element* element;
  vector<streaming::Policy*>* policies;
  if ( mapper_->GetElementByName(media, &element, &policies) ) {
    if ( element != NULL && policies != NULL && policies->size() == 1 &&
         element->type() == SwitchingElement::kElementClassName &&
         (*policies)[0]->type() == PlaylistPolicy::kPolicyClassName ) {
      PlaylistPolicy* const policy =
          reinterpret_cast<PlaylistPolicy*>((*policies)[0]);
      ret.loop_.ref() = policy->loop_playlist();
      for ( int i = 0; i < policy->playlist().size(); ++i ) {
        const string& crt = policy->playlist()[i];
        ret.media_.ref().push_back(
            MediaAliasSpec(crt, mapper_->TranslateMedia(crt.c_str())));
      }
    } else {
      const string alias = mapper_->TranslateMedia(media.c_str());
      ret.media_.ref().push_back(MediaAliasSpec(media, alias));
    }
  } else {
    const string alias = mapper_->TranslateMedia(media.c_str());
    ret.media_.ref().push_back(MediaAliasSpec(media, alias));
  }
  call->Complete(ret);
}

//////////////////////////////////////////////////////////////////////

bool StandardLibrary::RegisterLibraryRpc(rpc::HttpServer* rpc_server,
                                         string* rpc_http_path) {
  CHECK(rpc_impl_ == NULL);
  rpc_impl_ = new ServiceInvokerStandardLibraryServiceImpl(
      mapper_,
      create_element_spec_callback_,
      create_policy_spec_callback_,
      create_authorizer_spec_callback_);
  if ( !rpc_server->RegisterService(name() + "/", rpc_impl_) ) {
    delete rpc_impl_;
    return false;
  }
  *rpc_http_path = strutil::NormalizePath(
      rpc_server->path() + "/" + name() + "/" + rpc_impl_->GetName());
  return true;
}

bool StandardLibrary::UnregisterLibraryRpc(rpc::HttpServer* rpc_server) {
  CHECK(rpc_impl_ != NULL);
  const bool success = rpc_server->UnregisterService(name() + "/", rpc_impl_);
  delete rpc_impl_;
  rpc_impl_ = NULL;
  return success;
}
}
