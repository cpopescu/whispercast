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
// Authors: Catalin Popescu

// This defines a simple interface for mapping from a stream id to a
// element.

#ifndef __MEDIA_ELEMENTS_FACTORY_BASED_MAPPER_H__
#define __MEDIA_ELEMENTS_FACTORY_BASED_MAPPER_H__

#include <string>
#include <map>

#include <whisperlib/common/base/types.h>
#include WHISPER_HASH_MAP_HEADER
#include WHISPER_HASH_SET_HEADER

#include <whisperstreamlib/base/consts.h>
#include <whisperstreamlib/base/request.h>
#include <whisperstreamlib/elements/factory.h>
#include <whisperlib/common/io/checkpoint/state_keeper.h>
#include <whisperlib/common/io/ioutil.h>

namespace streaming {

//////////////////////////////////////////////////////////////////////

class FactoryBasedElementMapper : public ElementMapper {
 public:
  FactoryBasedElementMapper(net::Selector* selector,
                            ElementFactory* factory,
                            io::StateKeepUser* alias_state_keeper);
  virtual ~FactoryBasedElementMapper();

  //////////////////////////////////////////////////////////////////////

  // ElementMapper interface

  virtual bool AddRequest(const char* media,
                          streaming::Request* req,
                          ProcessingCallback* callback);

  virtual void RemoveRequest(streaming::Request* req,
                             ProcessingCallback* callback);

  virtual void GetMediaDetails(const string& protocol,
                               const string& path,
                               streaming::Request* req,
                               Callback1<bool>* completion_callback);

  virtual bool GetElementByName(const string& name,
                                streaming::Element** element,
                                vector<streaming::Policy*>** policies);

  virtual bool IsKnownElementName(const string& name);

  virtual Authorizer* GetAuthorizer(const string& name) {
    const AuthorizerMap::const_iterator it = authorizer_map_.find(name);
    if ( it  == authorizer_map_.end() ) {
      return NULL;
    }
    return it->second;
  }

  virtual string TranslateMedia(const char* media_name) const;

  virtual bool DescribeMedia(const string& media, MediaInfoCallback* callback);

  //////////////////////////////////////////////////////////////////////

  // Our own interface
  bool Initialize();
  void Close(Closure* close_completed);

  bool AddElement(const string& name, bool is_global);
  void RemoveElement(const string& name);

  bool AddAuthorizer(const string& name);
  void RemoveAuthorizer(const string& name);

  bool AddServingInfo(const string& protocol,
                      const string& path,
                      RequestServingInfo* serving_info);  // we take control
  bool RemoveServingInfo(const string& protocol,
                         const string& path);

  bool SetMediaAlias(const string& alias_name, const string& media_name,
                     string* error);
  bool GetMediaAlias(const string& alias_name, string* media_name) const {
    if ( alias_state_keeper_ == NULL ) return false;
    return alias_state_keeper_->GetValue(alias_name, media_name);
  }
  void GetAllMediaAliases(vector< pair<string, string> >* aliases) const;

  typedef map<string, RequestServingInfo*> ServingInfoMap;
  const ServingInfoMap& serving_paths() const {
    return serving_paths_;
  }

  RequestServingInfo* GetMediaServingInfo(const string& protocol,
                                          const string& path,
                                          string* key) {
    *key = (protocol + ":" + strutil::NormalizeUrlPath("/" + path));
    return io::FindPathBased<RequestServingInfo*>(&serving_paths_,
                                                  *key);
  }
  virtual bool HasMedia(const char* media_name, Capabilities* out);
  virtual void ListMedia(const char* media_dir,
                         ElementDescriptions* medias);

  void set_extra_element_spec_map(ElementSpecMap* extra_element_spec_map) {
    extra_element_spec_map_ = extra_element_spec_map;
  }
  void set_extra_policy_spec_map(PolicySpecMap* extra_policy_spec_map) {
    extra_policy_spec_map_ = extra_policy_spec_map;
  }

  virtual int32 AddExportClient(const string& protocol, const string& path);
  virtual void RemoveExportClient(const string& protocol, const string& path);

 private:
  string GetElementName(const char* media);
  void RemoveTempElement(const string& name);
  // each element signals Close completion by calling this
  void ElementClosed(Element* element, vector<streaming::Policy*>* policies);

  typedef hash_map<string, streaming::Authorizer*> AuthorizerMap;

  ElementFactory* factory_;    // can be used by other guys
  io::StateKeepUser* alias_state_keeper_;

  ElementSpecMap* extra_element_spec_map_;
  PolicySpecMap* extra_policy_spec_map_;

  ElementMap global_element_map_;
  PolicyMap  global_policy_map_;
  ElementMap non_global_element_map_;
  PolicyMap  non_global_policy_map_;

  AuthorizerMap authorizer_map_;

  typedef hash_set<streaming::Request*> RequestSet;

  typedef hash_map<streaming::Request*, TempElementStruct*> TempMap;

  RequestSet requests_set_;
  ServingInfoMap serving_paths_;

  // map: export key -> client count
  // where export key is: "<protocol>:<export_path>"
  typedef map<string, uint32> ExportClientCountMap;
  ExportClientCountMap export_client_count_;

  // the number of elements that where asked to Close(), but their
  // close did not complete, yet.
  uint32 close_pending_element_count_;
  // when we are completely closed, call this callback.
  Closure* close_completed_;

  DISALLOW_EVIL_CONSTRUCTORS(FactoryBasedElementMapper);
};

// Helper to run initialize for all elements and policies in the given maps
bool InitializeElementsAndPolicies(const ElementMap& elements,
                                   const PolicyMap& policies);

}

#endif  // __MEDIA_BASE_ELEMENT_MAPPER_H__
