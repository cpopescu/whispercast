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

#ifndef __MEDIA_STANDARD_LIBRARY_REMOTE_RESOLVER_H__
#define __MEDIA_STANDARD_LIBRARY_REMOTE_RESOLVER_H__

// An element that first looks locally, then resolves remotely
// the media to play, via ResolveMedia interface of standard library

// A element of media that gets its data from talking to a server
#include <string>
#include <map>
#include <list>

#include <whisperlib/common/base/types.h>
#include WHISPER_HASH_MAP_HEADER

#include <whisperlib/net/base/address.h>
#include <whisperstreamlib/base/element.h>
#include <whisperlib/net/rpc/lib/client/rpc_failsafe_client_connection_http.h>

#include <whisperstreamlib/elements/standard_library/auto/standard_library_wrappers.h>

namespace streaming {
class RemoteResolverElement : public Element {
 public:
  typedef map<string, string>  Host2IpMap;

  RemoteResolverElement(
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
      const string& auth_user,
      const string& auth_pass,
      const Capabilities& default_caps);

  ~RemoteResolverElement();

  // streaming::Element interface methods
  virtual bool Initialize();
  virtual bool AddRequest(const char* media, streaming::Request* req,
                          streaming::ProcessingCallback* callback);
  virtual void RemoveRequest(streaming::Request* req);
  virtual bool HasMedia(const char* media, Capabilities* out);
  virtual void ListMedia(const char* media_dir, ElementDescriptions* medias);
  virtual bool DescribeMedia(const string& media, MediaInfoCallback* callback);
  virtual void Close(Closure* call_on_close);

  static const char kElementClassName[];

 private:
  struct LookupResult {
    const string media_name_;
    int64 expiration_time_;
    int ref_count_;
    vector< pair<string, string> > to_play_;
    bool loop_;
    LookupResult(const string& key)
        : media_name_(key),
          expiration_time_(0),
          ref_count_(0),
          loop_(false) {
    }
    void IncRef() { ++ref_count_; }
    void DecRef() { --ref_count_; if ( ref_count_ <= 0 ) delete this; }
  };
  struct RequestStruct {
    const string media_name_;
    streaming::Request* req_;
    streaming::ProcessingCallback* callback_;     // w/ request

    rpc::CALL_ID rpc_id_;
    bool lookup_cancelled_;
    int crt_play_index_;
    LookupResult* lres_;
    streaming::ProcessingCallback* internal_processing_callback_;  // ours
    bool is_orphaned_;
    bool is_processing_;
    bool source_start_sent_;
    string crt_media_;
    RequestStruct(const char* media,
                  streaming::Request* req,
                  streaming::ProcessingCallback* callback)
        : media_name_(media),
          req_(req),
          callback_(callback),
          rpc_id_(0),
          lookup_cancelled_(false),
          crt_play_index_(-1),
          lres_(NULL),
          internal_processing_callback_(NULL),
          is_orphaned_(false),
          is_processing_(false),
          source_start_sent_(false) {
    }
    ~RequestStruct() {
      if ( lres_ )  {
        lres_->DecRef();
      }
      delete internal_processing_callback_;
    }
  };


  void ProcessTag(RequestStruct* req, const Tag* tag, int64 timestamp_ms);
  void LookupCompleted(
      RequestStruct* req,
      const rpc::CallResult< ResolveSpec > & response);
  bool StartPlaySequence(RequestStruct* req,
                         LookupResult* const lres,
                         bool no_eos_sent);
  bool ContinuePlaySequence(RequestStruct* req,
                            bool no_eos_sent);
  void AddToCache(LookupResult* res);
  void ExpireSomeCache();
  void PeriodicCacheExpirationCallback();

  //////////////////////////////////////////////////////////////////////

  net::Selector* selector_;
  net::NetFactory net_factory_;

  ServiceWrapperStandardLibraryService* service_;
  rpc::FailsafeClientConnectionHTTP* rpc_connection_;

  const int64 cache_expiration_time_ms_;
  const bool local_lookup_first_;
  const Capabilities default_caps_;

  http::ClientParams client_params_;
  ResultClosure<http::BaseClientConnection*>* connection_factory_;

  static const int32 kReopenHttpConnectionIntervalMs = 2000;

  bool closing_;
  Closure* call_on_close_;

  typedef hash_map<streaming::Request*, RequestStruct*> LookupMap;
  LookupMap lookup_ops_;
  typedef hash_map<streaming::Request*, RequestStruct*> RequestMap;
  RequestMap active_reqs_;

  // We keep a cache of remotely resolved media. The expiration time is
  // fixed, so we simply keep the records oredered by expiration time
  // in cache_exp_list_, and periodically we expire from the front of the
  // list. For duplicate keys, we replace the LookupResult in cache_,
  // but keep the record in cache_exp_list_ for later expiration.
  typedef map<string, LookupResult*> LookupCache;
  LookupCache cache_;
  typedef list<LookupResult*> LookupExpList;
  LookupExpList cache_exp_list_;
  static const int32 kPeriodicExpirationCallbackFrequencyMs = 5000;
  Closure* const periodic_expiration_callback_;

  DISALLOW_EVIL_CONSTRUCTORS(RemoteResolverElement);
};
}

#endif  // __MEDIA_STANDARD_LIBRARY_REMOTE_RESOLVER_H__
