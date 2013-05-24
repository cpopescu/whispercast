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

#ifndef __MEDIA_BASE_MEDIA_SOURCE_FACTORY_H__
#define __MEDIA_BASE_MEDIA_SOURCE_FACTORY_H__

#include <string>
#include <vector>
#include <map>
#include <list>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/types.h>

#include WHISPER_HASH_MAP_HEADER
#include WHISPER_HASH_SET_HEADER

#include <whisperlib/net/rpc/lib/server/irpc_async_query_executor.h>
#include <whisperlib/net/rpc/lib/server/rpc_services_manager.h>
#include <whisperlib/net/rpc/lib/server/rpc_http_processor.h>
#include <whisperlib/net/rpc/lib/server/rpc_http_server.h>
#include <whisperlib/common/io/checkpoint/state_keeper.h>

#include <whisperstreamlib/elements/auto/factory_types.h>
#include <whisperstreamlib/elements/auto/factory_invokers.h>
#include <whisperstreamlib/base/element.h>
#include <whisperstreamlib/base/stream_auth.h>

#include <whisperstreamlib/elements/element_library.h>

namespace streaming  {
class FactoryBasedElementMapper;

typedef map<string, MediaElementSpecs*> ElementSpecMap;
typedef map<string, PolicySpecs*> PolicySpecMap;
typedef map<string, AuthorizerSpecs*> AuthorizerSpecMap;

class ElementFactory :
      public ServiceInvokerElementConfigService {
 public:
  // We take control on the specification..
  ElementFactory(streaming::FactoryBasedElementMapper* mapper,
                 net::Selector* selector,
                 http::Server* http_server,
                 rpc::HttpServer* rpc_server,
                 map<string, io::AioManager*>* aio_managers,
                 io::BufferManager* buffer_manager,
                 const char* base_media_dir,
                 io::StateKeeper* state_keeper,
                 io::StateKeeper* local_state_keeper);

  virtual ~ElementFactory();

  // Initializes the element libraries, returns the success status and
  // paths for the library specific rpc-s that got registered
  bool InitializeLibraries(const string& libraries_dir,
                           vector< pair<string, string> >* rpc_http_paths);

  // Returns all the known element and policy types
  void GetKnownObjectTypes(vector<string>* element_types,
                           vector<string>* policy_types,
                           vector<string>* authorizer_types);

  // Returns of element specs / policy specs for a media
  const MediaElementSpecs* GetElementSpec(const string& name) const {
    ElementSpecMap::const_iterator it_name = elements_map_.find(name);
    if ( it_name != elements_map_.end() ) {
      return it_name->second;
    }
    return NULL;
  }
  const PolicySpecs* GetPolicySpec(const string& name) const {
    PolicySpecMap::const_iterator it_name = policies_map_.find(name);
    if ( it_name != policies_map_.end() ) {
      return it_name->second;
    }
    return NULL;
  }

  const string& base_media_dir() const {
    return base_media_dir_;
  }

  virtual void GetElementConfig(
      rpc::CallContext<MediaRequestConfigSpec>* call,
      const MediaRequestSpec& req);
  virtual void GetMediaDetails(
      rpc::CallContext<ElementExportSpec>* call,
      const string& protocol,
      const string& path);

  //////////////////////////////////////////////////////////////////////

  // Whatever elements we create we store in these structures:
  // Points from element name to element pointer
  typedef hash_map<string, streaming::Element*>  ElementMap;
  // Points from **element** name to associated policy
  typedef hash_map<streaming::Element*,
                   vector<streaming::Policy*>* >  PolicyMap;
  typedef hash_map<string, streaming::Authorizer*>  AuthorizerMap;

  // IMPORTANT: for these creation function we first clear the provided maps !

  // Creates all elements and puts them in the given maps, depending if they
  // are global or not.
  // Returns the number of elements and policies which were not
  // created correctly.
  // Returns the success status.
  bool CreateAllElements(ElementMap* global_elements,
                         PolicyMap* global_policies,
                         ElementMap* non_global_elements,
                         PolicyMap* non_global_policies) const;

  // Creates a temporarely element (per request) and (maybe)
  // the associated policies.
  // Returns the success status.
  streaming::Element* CreateElement(const string& name,
                                    const ElementSpecMap* extra_elements_map,
                                    const PolicySpecMap* extra_policies_map,
                                    const streaming::Request* request,
                                    bool is_temporary_template,
                                    PolicyMap* new_policies) const;

  // Creates a policy for the given element, and associated elements
  streaming::Policy* CreatePolicy(const string& name,
                                  const PolicySpecMap* extra_policies_map,
                                  streaming::PolicyDrivenElement* element,
                                  const streaming::Request* request) const;

  // Creates an authorizer conform w/ specifications of the authorizers
  // w/ the given anem
  streaming::Authorizer* CreateAuthorizer(const string& name) const;

  // Creates all known authorizers and puts the results in the provided map
  bool CreateAllAuthorizers(AuthorizerMap* authorizer_map) const;

  //////////////////////////////////////////////////////////////////////

  // Verification functions - in order to see if we could add a new policy
  // or a new element
  bool ElementExists(const string& name);

  bool IsValidAuthorizerSpecToAdd(const AuthorizerSpecs& spec,
                                  string* out_error);

  bool IsValidPolicySpecToAdd(const PolicySpecs& spec,
                              string* out_error);

  // If registered_names is set, it checks
  // for dependencies here, insetead of elements_map_
  bool IsValidElementSpecToAdd(const MediaElementSpecs& spec,
                               string* out_error);

  //////////////////////////////////////////////////////////////////////

  // The following functions are for modifying the element / policy
  // specifications.
  // Return value is the success status (true - done OK, false - errors)

  // The actual configuration modifier functions  -
  bool AddElementSpec(const MediaElementSpecs& spec, string* out_error);
  bool AddPolicySpec(const PolicySpecs& spec, string* out_error);
  bool AddAuthorizerSpec(const AuthorizerSpecs& spec, string* out_error);

  // Delete a policy / element from our specifications
  void DeleteElementSpec(const string& name);
  void DeletePolicySpec(const string& name);
  void DeleteAuthorizerSpec(const string& name);

  //////////////////////////////////////////////////////////////////////

  // Main initialization function - "loads" a configuration in the
  // creation functions and structs that we have around
  bool AddSpecs(const ElementConfigurationSpecs& spec,
                vector<string>* errors);

  //////////////////////////////////////////////////////////////////////

  // Builds the specs in a new object (builds them from components :)
  // NOTE: the return object is newly allocated - you MUST delete it !x
  ElementConfigurationSpecs* GetSpec() const;

  // Builds a list with all known element names
  void GetAllNames(vector<string>* names) const;

 private:
  // Another version for adding specs, but provided for the libraries..
  bool AddLibraryElementSpec(ElementLibrary::ElementSpecCreateParams* spec);
  bool AddLibraryPolicySpec(ElementLibrary::PolicySpecCreateParams* spec);
  bool AddLibraryAuthorizerSpec(ElementLibrary::AuthorizerSpecCreateParams* spec);

  // Helper for preparing the creation parameters for a source
  bool SetCreationParams(const string& type,
                         int64 needs,
                         ElementLibrary::CreationObjectParams* params,
                         const string& global_state_prefix,
                         const string& local_state_prefix) const;

  //////////////////////////////////////////////////////////////////////

  // Helper objects used in creation

  streaming::FactoryBasedElementMapper* mapper_;
  net::Selector* const selector_;
  http::Server* const http_server_;
  rpc::HttpServer* const   rpc_server_;
  map<string, io::AioManager*>* const aio_managers_;
  io::BufferManager* const buffer_manager_;
  const string base_media_dir_;
  io::StateKeeper* const state_keeper_;
  io::StateKeeper* const local_state_keeper_;


  // Our specifications
  ElementSpecMap elements_map_;
  PolicySpecMap policies_map_;
  AuthorizerSpecMap authorizers_map_;

  // Related to libraries..
  string libraries_dir_;
  vector< pair<void*, ElementLibrary*> > libraries_;
  typedef map<string, ElementLibrary*> LibMap;
  LibMap known_element_types_;
  LibMap known_policy_types_;
  LibMap known_authorizer_types_;

  ElementLibrary::ElementSpecCreationCallback* elem_creation_callback_;
  ElementLibrary::PolicySpecCreationCallback* policy_creation_callback_;
  ElementLibrary::AuthorizerSpecCreationCallback* authorizer_creation_callback_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ElementFactory);
};

}

#endif  // __MEDIA_BASE_MEDIA_SOURCE_FACTORY_H__
