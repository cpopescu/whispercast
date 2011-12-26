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
// IMPORTANT NOTE:
//  When adding a new element make sure to add your code (if necessary)
//  to some of the HERE_SPECIFIC sections
//

#include <dlfcn.h>
#include <whisperlib/net/rpc/lib/server/execution/rpc_execution_simple.h>
#include <whisperlib/common/io/ioutil.h>
#include <whisperlib/common/base/re.h>

//////////////////////////////////////////////////////////////////////

#include "elements/factory.h"
#include "aac/aac_tag_splitter.h"
#include "flv/flv_tag_splitter.h"
#include "mp3/mp3_tag_splitter.h"
#include "raw/raw_tag_splitter.h"
#include "f4v/f4v_tag_splitter.h"
#include "internal/internal_tag_splitter.h"
#include "elements/factory_based_mapper.h"

DEFINE_int32(flv_splitter_max_tag_composition_time_ms,
             0,
             "We compose flv tags in chunks of this size, after they are "
             "extracted by a splitter");
DEFINE_int32(f4v_splitter_max_tag_composition_time_ms,
             0,
             "We compose f4v tags in chunks of this size, after they are "
             "extracted by a splitter");
DEFINE_int32(raw_splitter_bps,
             1 << 20, // 1 mbps
             "Data processing speed for RawTagSplitter.");

namespace {
class StandardSplitterCreator : public streaming::SplitterCreator {
 public:
  StandardSplitterCreator() : streaming::SplitterCreator() {
  }
  virtual streaming::TagSplitter* CreateSplitter(
      const string& name,
      streaming::Tag::Type tag_type) {
    switch ( tag_type ) {
      case streaming::Tag::TYPE_FLV: {
        streaming::TagSplitter* s = new streaming::FlvTagSplitter(name);
        s->set_max_composition_tag_time_ms(
            FLAGS_flv_splitter_max_tag_composition_time_ms);
        return s;
      }
      case streaming::Tag::TYPE_MP3:
        return new streaming::Mp3TagSplitter(name);
      case streaming::Tag::TYPE_AAC:
        return new streaming::AacTagSplitter(name);
      case streaming::Tag::TYPE_INTERNAL:
        return new streaming::InternalTagSplitter(name);
      case streaming::Tag::TYPE_F4V: {
        streaming::TagSplitter* s =
            new streaming::F4vTagSplitter(name);
        s->set_max_composition_tag_time_ms(
            FLAGS_f4v_splitter_max_tag_composition_time_ms);
        return s;
      }
      default:
        DLOG_DEBUG << "Unknown tag_type: " << tag_type
                   << ", creating a RawTagSplitter w/ name: [" << name << "]";
        return new streaming::RawTagSplitter(name, FLAGS_raw_splitter_bps);
    }
    return NULL;
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(StandardSplitterCreator);
};
}

//////////////////////////////////////////////////////////////////////

namespace streaming  {

ElementFactory::ElementFactory(
    streaming::FactoryBasedElementMapper* mapper,
    net::Selector* selector,
    http::Server* http_server,
    rpc::HttpServer* rpc_server,
    map<string, io::AioManager*>* aio_managers,
    io::BufferManager* buffer_manager,
    const Host2IpMap* host_aliases,
    const char* base_media_dir,
    io::StateKeeper* state_keeper,
    io::StateKeeper* local_state_keeper)
    : ServiceInvokerElementConfigService(
        ServiceInvokerElementConfigService::GetClassName()),
      mapper_(mapper),
      selector_(selector),
      http_server_(http_server),
      rpc_server_(rpc_server),
      aio_managers_(aio_managers),
      buffer_manager_(buffer_manager),
      host_aliases_(host_aliases),
      base_media_dir_(base_media_dir),
      state_keeper_(state_keeper),
      local_state_keeper_(local_state_keeper),
      elem_creation_callback_(
          NewPermanentCallback(this, &ElementFactory::AddLibraryElementSpec)),
      policy_creation_callback_(
          NewPermanentCallback(this, &ElementFactory::AddLibraryPolicySpec)),
      authorizer_creation_callback_(
          NewPermanentCallback(this, &ElementFactory::AddLibraryAuthorizerSpec)) {
  if ( rpc_server ) {
    CHECK(rpc_server->RegisterService(this));
  }
}

ElementFactory::~ElementFactory() {
  if ( rpc_server_ ) {
    rpc_server_->UnregisterService(this);
  }
  for ( ElementSpecMap::const_iterator it = elements_map_.begin();
        it != elements_map_.end(); ++it ) {
    delete it->second;
  }
  elements_map_.clear();
  for ( PolicySpecMap::const_iterator it = policies_map_.begin();
        it != policies_map_.end(); ++it ) {
    delete it->second;
  }
  policies_map_.clear();
  for ( AuthorizerSpecMap::const_iterator it = authorizers_map_.begin();
        it != authorizers_map_.end(); ++it ) {
    delete it->second;
  }
  authorizers_map_.clear();
  for ( int i = 0; i < libraries_.size(); ++i ) {
    libraries_[i].second->UnregisterLibraryRpc(rpc_server_);
    delete libraries_[i].second;
    dlclose(libraries_[i].first);
  }
  libraries_.clear();
  delete authorizer_creation_callback_;
  delete policy_creation_callback_;
  delete elem_creation_callback_;
}

//////////////////////////////////////////////////////////////////////

bool ElementFactory::InitializeLibraries(
    const string& libraries_dir,
    vector< pair<string, string> >* rpc_http_paths) {
  LOG_INFO << "Looking for libraries in directory: [" << libraries_dir << "]";
  vector<string> files;
  re::RE reg(".*_streaming_elements\\.so");
  if ( !io::DirList(libraries_dir, &files, false, &reg) ) {
    return false;
  }
  libraries_dir_ = libraries_dir;
  LOG_INFO << "Found libraries: " << strutil::ToString(files);
  for ( int i = 0; i < files.size(); ++i ) {
    void* lib_handle = dlopen((libraries_dir + "/" + files[i]).c_str(),
                              RTLD_LAZY);
    if ( lib_handle == NULL ) {
      LOG_ERROR << "Cannot load element library: "
                << files[i]  << ": " << dlerror();
      continue;
    }
    void* (*fun)(void);
    fun = reinterpret_cast<void* (*)()>(dlsym(lib_handle,
                                                 "GetElementLibrary"));
    const char* const error = dlerror();
    if ( error != NULL || fun == NULL ) {
      LOG_ERROR << " Error linking GetElementLibrary function: "
                << error;
      continue;
    }
    LOG_INFO << " Loaded element library: "
             << files[i] << " - now creating the element library";
    ElementLibrary* lib = reinterpret_cast<ElementLibrary*>((*fun)());
    if ( lib == NULL ) {
      dlclose(lib_handle);
      LOG_ERROR << " No element library provided by the loaded lib: "
                << files[i];
      continue;
    }
    libraries_.push_back(make_pair(lib_handle, lib));
    lib->SetParameters(mapper_,
                       elem_creation_callback_,
                       policy_creation_callback_,
                       authorizer_creation_callback_);
    if ( rpc_http_paths != NULL ) {
      string rpc_path;
      lib->RegisterLibraryRpc(rpc_server_, &rpc_path);
      rpc_http_paths->push_back(make_pair(lib->name(), rpc_path));
    }
    vector<string> element_types;
    lib->GetExportedElementTypes(&element_types);
    if ( element_types.empty() ) {
      LOG_WARNING << " No elements provided by: " << files[i];
    } else {
      for ( int j = 0; j < element_types.size(); ++j ) {
        LOG_INFO << " Library: " << files[i] << " provides element: "
                 << element_types[j];
        if ( known_element_types_.find(element_types[j]) !=
             known_element_types_.end() ) {
          LOG_ERROR << " Duplicate provider for element type: "
                    << element_types[j] << ", skipping";
        } else {
          known_element_types_.insert(make_pair(element_types[j], lib));
        }
      }
    }
    vector<string> policy_types;
    lib->GetExportedPolicyTypes(&policy_types);
    if ( policy_types.empty() ) {
      LOG_WARNING << " No policies provided by: " << files[i];
    } else {
      for ( int j = 0; j < policy_types.size(); ++j ) {
        LOG_INFO << " Library: " << files[i] << " provides policy: "
                 << policy_types[j];
        if ( known_policy_types_.find(policy_types[j]) !=
             known_policy_types_.end() ) {
          LOG_ERROR << " Duplicate provider for policy type: "
                    << policy_types[j] << ", skipping";
        } else {
          known_policy_types_.insert(make_pair(policy_types[j], lib));
        }
      }
    }
    vector<string> authorizer_types;
    lib->GetExportedAuthorizerTypes(&authorizer_types);
    if ( authorizer_types.empty() ) {
      LOG_WARNING << " No authorizers provided by: " << files[i];
    } else {
      for ( int j = 0; j < authorizer_types.size(); ++j ) {
        LOG_INFO << " Library: " << files[i] << " provides authorizer: "
                 << authorizer_types[j];
        if ( known_authorizer_types_.find(authorizer_types[j]) !=
             known_authorizer_types_.end() ) {
          LOG_ERROR << " Duplicate provider for authorizer type: "
                    << authorizer_types[j] << ", skipping";
        } else {
          known_authorizer_types_.insert(make_pair(authorizer_types[j], lib));
        }
      }
    }

  }
  return true;
}

//////////////////////////////////////////////////////////////////////


bool ElementFactory::SetCreationParams(
    const string& type,
    int64 needs,
    ElementLibrary::CreationObjectParams* params,
    const string& global_state_prefix,
    const string& local_state_prefix) const {
  if ( (needs & streaming::ElementLibrary::NEED_SELECTOR) != 0 ) {
    if ( selector_ == NULL ) {
      LOG_ERROR << "Need for type: " << type << " unsatisfied, per selector";
      return false;
    }
    params->selector_ = selector_;
  }
  if ( (needs & streaming::ElementLibrary::NEED_HTTP_SERVER) != 0 ) {
    if ( http_server_ == NULL ) {
      LOG_ERROR << "Need for type: " << type << " unsatisfied, per http_server";
      return false;
    }
    params->http_server_ = http_server_;
  }
  if ( (needs & streaming::ElementLibrary::NEED_RPC_SERVER) != 0 ) {
    if ( rpc_server_ == NULL ) {
      LOG_ERROR << "Need for type: " << type
                << " unsatisfied, per http_rpc_server";
      return false;
    }
    params->rpc_server_ = rpc_server_;
  }
  if ( (needs & streaming::ElementLibrary::NEED_AIO_FILES) != 0 ) {
    if ( aio_managers_ == NULL ) {
      LOG_ERROR << "Need for type: " << type
                << " unsatisfied, per aio_managers";
      return false;
    }
    params->aio_managers_ = aio_managers_;
    CHECK(buffer_manager_ != NULL);
    params->buffer_manager_ = buffer_manager_;
  }
  if ( (needs & streaming::ElementLibrary::NEED_HOST2IP_MAP) != 0 ) {
    params->host_aliases_ = host_aliases_;
  }
  if ( (needs & streaming::ElementLibrary::NEED_MEDIA_DIR) != 0 ) {
    params->media_dir_ = base_media_dir_;
  }
  if ( (needs & streaming::ElementLibrary::NEED_STATE_KEEPER) != 0 ) {
    if ( state_keeper_ != NULL ) {
      params->state_keeper_ = new io::StateKeepUser(state_keeper_,
                                                    global_state_prefix, -1);
    }
    if ( local_state_keeper_ != NULL ) {
      if ( !local_state_prefix.empty() ) {
        params->local_state_keeper_ = new io::StateKeepUser(
            local_state_keeper_, local_state_prefix, -1);
      } else {
        params->local_state_keeper_ = new io::StateKeepUser(
            local_state_keeper_, global_state_prefix, -1);
      }
    }
  }
  if ( (needs & streaming::ElementLibrary::NEED_SPLITTER_CREATOR) != 0 ) {
    params->splitter_creator_ = new StandardSplitterCreator();
  }
  return true;
}


streaming::Element* ElementFactory::CreateElement(
    const string& name,
    const ElementSpecMap* extra_elements_map,
    const PolicySpecMap* extra_policies_map,
    const streaming::Request* request,
    PolicyMap* new_policies) const {
  // First, find the element name (if any..);
  ElementSpecMap::const_iterator it_name = elements_map_.find(name);
  if ( it_name == elements_map_.end() ) {
    if ( extra_elements_map != NULL ) {
      it_name = extra_elements_map->find(name);
      if ( it_name == extra_elements_map->end() ) {
        LOG_ERROR << " Unknown element name: " << name;
        return NULL;
      }
    } else {
      LOG_ERROR << " Unknown element name: " << name;
      return NULL;
    }
  }
  // Then, locate the library that creates this element
  const MediaElementSpecs* spec = it_name->second;
  CHECK_EQ(spec->name_.get(), name);
  const string& type = spec->type_.get();
  const LibMap::const_iterator it_type = known_element_types_.find(type);
  if ( it_type == known_element_types_.end() ) {
    LOG_ERROR << " No library found to create type: " << type;
    return NULL;
  }
  ElementLibrary* const lib = it_type->second;
  // Prepare the parameters that the element needs
  int64 needs = lib->GetElementNeeds(type);
  // we disable the RPC if required
  if ( spec->disable_rpc_.is_set() && spec->disable_rpc_.get() ) {
    needs &= ~ElementLibrary::NEED_RPC_SERVER;
  }
  string local_state_prefix, global_state_prefix;
  global_state_prefix = string("src/") + name + "/";
  if ( request != NULL ) {
    local_state_prefix = string("id/") + request->info().GetId() + "/" +
                         global_state_prefix;
  }
  ElementLibrary::CreationObjectParams params;
  if ( !SetCreationParams(type, needs, &params,
                          global_state_prefix,
                          local_state_prefix) ) {
    return NULL;
  }
  // Create the element w/ the library
  string error_description;
  vector<string> needed_policies;
  streaming::Element* const element = lib->CreateElement(
      type, name, spec->params_.get(), request, params,
      &needed_policies, &error_description);
  if ( element == NULL ) {
    LOG_ERROR << " Library error creating element: " << error_description;
    return NULL;
  }
  // Maybe create any needed policies
  if ( !needed_policies.empty() ) {
    vector<streaming::Policy*>*  policies = new vector<streaming::Policy*>;
    for ( int i = 0; i < needed_policies.size(); ++i ) {
      streaming::Policy* policy = CreatePolicy(
          needed_policies[i],
          extra_policies_map,
          reinterpret_cast<streaming::PolicyDrivenElement*>(element),
          request);
      if ( policy == NULL ) {
        for ( int j = 0; j < policies->size(); ++j ) {
          delete (*policies)[j];
        }
        delete policies;
        delete element;
        return NULL;
      }
      policies->push_back(policy);
    }
    new_policies->insert(make_pair(element, policies));
  }
  // Done !
  return element;
}

streaming::Policy* ElementFactory::CreatePolicy(
    const string& name,
    const PolicySpecMap* extra_policies_map,
    streaming::PolicyDrivenElement* element,
    const streaming::Request* req) const {
  // First, find the policy name (if any..);
  PolicySpecMap::const_iterator it_name = policies_map_.find(name);
  if ( it_name == policies_map_.end() ) {
    if ( extra_policies_map != NULL ) {
      it_name = extra_policies_map->find(name);
      if ( it_name == extra_policies_map->end() ) {
        LOG_ERROR << " Unknown policy name: " << name;
        return NULL;
      }
    } else {
      LOG_ERROR << " Unknown policy name: " << name;
      return NULL;
    }
  }
  // Then, locate the library that creates this policy
  const PolicySpecs* spec = it_name->second;
  DCHECK_EQ(spec->name_.get(), name);
  const string& type = spec->type_.get();
  const LibMap::const_iterator it_type = known_policy_types_.find(type);
  if ( it_type == known_policy_types_.end() ) {
    LOG_ERROR << " No library found to create type: " << type;
    return NULL;
  }
  ElementLibrary* const lib = it_type->second;
  // Prepare the parameters that the element needs
  const int64 needs = lib->GetPolicyNeeds(type);
  string local_state_prefix, global_state_prefix;
  global_state_prefix = string("src/") + element->name() + ":policy/" +
                        name + "/";
  if ( req != NULL ) {
    local_state_prefix = string("id/") + req->info().GetId() + "/" +
                         global_state_prefix;
  }
  ElementLibrary::CreationObjectParams params;
  if ( !SetCreationParams(type, needs, &params,
                          global_state_prefix,
                          local_state_prefix) ) {
    return NULL;
  }
  // Create the element w/ the library
  string error_description;
  streaming::Policy* const policy = lib->CreatePolicy(
      type, name, spec->params_.get(),
      reinterpret_cast<streaming::PolicyDrivenElement*>(element), req, params,
      &error_description);
  if ( policy == NULL ) {
    LOG_ERROR << " Library error creating policy: " << error_description;
    return NULL;
  }
  LOG_INFO << " Created policy, type: " << type << " , name: " << name;
  return policy;
}

bool ElementFactory::CreateAllAuthorizers(AuthorizerMap* authorizers_map) const {
  bool success = true;
  for ( AuthorizerSpecMap::const_iterator it = authorizers_map_.begin();
        it != authorizers_map_.end(); ++it ) {
    streaming::Authorizer* const auth = CreateAuthorizer(it->first);
    if ( auth != NULL ) {
      authorizers_map->insert(make_pair(it->first, auth));
    } else {
      success = false;
    }
  }
  return success;
}

streaming::Authorizer* ElementFactory::CreateAuthorizer(
    const string& name) const {
  // First, find the authorizer name (if any..);
  const AuthorizerSpecMap::const_iterator it_name = authorizers_map_.find(name);
  if ( it_name == authorizers_map_.end() ) {
    LOG_ERROR << " Unknown authorizer name: " << name;
    return NULL;
  }
  // Then, locate the library that creates this authorizer
  const AuthorizerSpecs* spec = it_name->second;
  DCHECK_EQ(spec->name_.get(), name);
  const string& type = spec->type_.get();
  const LibMap::const_iterator it_type = known_authorizer_types_.find(type);
  if ( it_type == known_authorizer_types_.end() ) {
    LOG_ERROR << " No library found to create type: " << type;
    return NULL;
  }
  ElementLibrary* const lib = it_type->second;
  // Prepare the parameters that the element needs
  const int64 needs = lib->GetAuthorizerNeeds(type);
  string state_prefix(string("auth/") + name + "/");

  ElementLibrary::CreationObjectParams params;
  if ( !SetCreationParams(type, needs, &params,
                          state_prefix, "") ) {
    return NULL;
  }
  // Create the element w/ the library
  string error_description;
  streaming::Authorizer* const auth = lib->CreateAuthorizer(
      type, name, spec->params_.get(), params,
      &error_description);
  if ( auth == NULL ) {
    LOG_ERROR << " Library error creating policy: " << error_description;
    return NULL;
  }
  LOG_INFO << " Created authorizer, type: " << type << " , name: " << name;
  return auth;
}


//////////////////////////////////////////////////////////////////////

bool ElementFactory::CreateAllElements(ElementMap* global_elements,
                                       PolicyMap* global_policies,
                                       ElementMap* non_global_elements,
                                       PolicyMap* non_global_policies) const {
  bool success = true;
  for ( ElementSpecMap::const_iterator it = elements_map_.begin();
        it != elements_map_.end(); ++it ) {
    const MediaElementSpecs* const spec = it->second;
    const bool is_global = spec->is_global_.get();
    Element* const element = CreateElement(spec->name_.get(),
                                           NULL,
                                           NULL,
                                           NULL,
                                           is_global
                                           ? global_policies
                                           : non_global_policies);
    if ( element == NULL ) {
      success = false;
    } else if ( is_global ) {
      global_elements->insert(
          make_pair(spec->name_.get(), element));
    } else {
      non_global_elements->insert(
          make_pair(spec->name_.get(), element));
    }
  }
  return success;
}

//////////////////////////////////////////////////////////////////////

void ElementFactory::GetAllNames(vector<string>* names) const {
  for ( ElementSpecMap::const_iterator it = elements_map_.begin();
        it != elements_map_.end(); ++it ) {
    names->push_back(it->first);
  }
}

ElementConfigurationSpecs* ElementFactory::GetSpec() const {
  ElementConfigurationSpecs* specs = new ElementConfigurationSpecs();
  for ( ElementSpecMap::const_iterator it = elements_map_.begin();
        it != elements_map_.end(); ++it ) {
    specs->elements_.ref().push_back(*it->second);
  }
  for ( PolicySpecMap::const_iterator it = policies_map_.begin();
        it != policies_map_.end(); ++it ) {
    specs->policies_.ref().push_back(*it->second);
  }
  for ( AuthorizerSpecMap::const_iterator it = authorizers_map_.begin();
        it != authorizers_map_.end(); ++it ) {
    specs->authorizers_.ref().push_back(*it->second);
  }

  return specs;
}

//////////////////////////////////////////////////////////////////////

bool ElementFactory::ElementExists(const string& name) {
  return elements_map_.find(name) != elements_map_.end();
}

bool ElementFactory::IsValidPolicySpecToAdd(const PolicySpecs& spec,
                                            ErrorData* error) {
  if ( !strutil::IsValidIdentifier(spec.name_.get().c_str()) ) {
    error->description_ = (string("Invalid spec name: ") +
                           spec.name_.get());
    return false;
  }
  if ( policies_map_.find(spec.name_.get()) != policies_map_.end() ) {
    error->description_ = ("Policy Spec [" + spec.name_.get() +
                           "] already exists");
    return false;
  }
  if ( known_policy_types_.find(spec.type_.get()) ==
       known_policy_types_.end() ) {
    error->description_ =
        ("Unknown policy type [" + spec.type_.get() + "]");
    return false;
  }
  // TODO: check params..
  return true;
}

bool ElementFactory::IsValidAuthorizerSpecToAdd(const AuthorizerSpecs& spec,
                                                ErrorData* error) {
  if ( !strutil::IsValidIdentifier(spec.name_.get().c_str()) ) {
    error->description_ = (string("Invalid spec name: ") +
                           spec.name_.get());
    return false;
  }
  if ( authorizers_map_.find(spec.name_.get()) != authorizers_map_.end() ) {
    error->description_ = ("Authorizer Spec [" + spec.name_.get() +
                           "] already exists");
    return false;
  }
  if ( known_authorizer_types_.find(spec.type_.get()) ==
       known_authorizer_types_.end() ) {
    error->description_ =
        ("Unknown authorizer type [" + spec.type_.get() + "]");
    return false;
  }
  // TODO: check params..
  return true;
}

bool ElementFactory::IsValidElementSpecToAdd(const MediaElementSpecs& spec,
                                             ErrorData* error) {
  if ( !strutil::IsValidIdentifier(spec.name_.get().c_str()) ) {
    error->description_ = (string("Invalid spec name") +
                           spec.name_.get());
    return false;
  }
  if ( elements_map_.find(spec.name_.get()) != elements_map_.end() ) {
    error->description_ = ("Element Spec [" + spec.name_.get() +
                           "] already exists");
    return false;
  }
  if ( known_element_types_.find(spec.type_.get()) ==
       known_element_types_.end() ) {
    error->description_ =
        ("Unknown stream element type [" + spec.type_.get() + "]");
    return false;
  }
  // TODO: check params..
  return true;
}

//////////////////////////////////////////////////////////////////////

bool ElementFactory::AddLibraryElementSpec(
    ElementLibrary::ElementSpecCreateParams* spec) {
  ErrorData error;
  MediaElementSpecs* const elem_spec = new MediaElementSpecs(
      spec->type_, spec->name_, spec->is_global_, spec->disable_rpc_,
      spec->element_params_);
  if ( AddElementSpec(elem_spec, &error) ) {
    const string name(elem_spec->name_.get());
    if ( !mapper_->AddElement(name, elem_spec->is_global_.get()) ) {
      DeleteElementSpec(name, &error);
      return false;
    }
    return true;
  }
  delete elem_spec;
  spec->error_ = error.description_;
  return false;
}

bool ElementFactory::AddElementSpec(MediaElementSpecs* spec,
                                    ErrorData* err) {
  if ( !IsValidElementSpecToAdd(*spec, err) ) {
    LOG_INFO << " Error adding element spec: " << *spec
             << " err: " << err->ToString();
    return false;
  }
  elements_map_.insert(make_pair(spec->name_.get(), spec));
  LOG_INFO << " Added element spec: " << *spec;
  return true;
}

bool ElementFactory::AddLibraryPolicySpec(
    ElementLibrary::PolicySpecCreateParams* spec) {
  ErrorData error;
  PolicySpecs* const policy_spec = new PolicySpecs(
      spec->type_, spec->name_, spec->policy_params_);
  if ( AddPolicySpec(policy_spec, &error) ) {
    return true;
  }
  delete policy_spec;
  spec->error_ = error.description_;
  return false;
}

bool ElementFactory::AddPolicySpec(PolicySpecs* spec, ErrorData* error) {
  if ( !IsValidPolicySpecToAdd(*spec,  error) ) {
    LOG_INFO << " Error adding policy spec: " << *spec
             << " err: " << error->ToString();
    return false;
  }
  policies_map_.insert(make_pair(spec->name_.get(), spec));
  LOG_INFO << " Added policy spec: " << *spec;
  return true;
}

bool ElementFactory::AddLibraryAuthorizerSpec(
    ElementLibrary::AuthorizerSpecCreateParams* spec) {
  ErrorData error;
  AuthorizerSpecs* const authorizer_spec = new AuthorizerSpecs(
      spec->type_, spec->name_, spec->authorizer_params_);
  if ( AddAuthorizerSpec(authorizer_spec, &error) ) {
    const string name(spec->name_);
    if ( !mapper_->AddAuthorizer(name) ) {
      DeleteAuthorizerSpec(name, &error);
      return false;
    }
    return true;
  }
  delete authorizer_spec;
  spec->error_ = error.description_;
  return false;
}

bool ElementFactory::AddAuthorizerSpec(AuthorizerSpecs* spec, ErrorData* error) {
  if ( !IsValidAuthorizerSpecToAdd(*spec,  error) ) {
    LOG_INFO << " Error adding authorizer spec: " << *spec
             << " err: " << error->ToString();
    return false;
  }
  // TODO: ---> Create ?
  authorizers_map_.insert(make_pair(spec->name_.get(), spec));
  LOG_INFO << " Added authorizer spec: " << *spec;
  return true;
}

bool  ElementFactory::AddSpecs(const ElementConfigurationSpecs& spec,
                               vector<ErrorData>* errors) {
  int size_begin = errors->size();
  ErrorData error;

  // Add authorizers..
  for ( int i = 0; i < spec.authorizers_.get().size(); ++i ) {
    AuthorizerSpecs* const new_spec =
        new AuthorizerSpecs(spec.authorizers_.get()[i]);
    if ( !AddAuthorizerSpec(new_spec, &error) ) {
      errors->push_back(error);
    }
  }

  // Add policies..
  for ( int i = 0; i < spec.policies_.get().size(); ++i ) {
    PolicySpecs* const new_spec =
        new PolicySpecs(spec.policies_.get()[i]);
    if ( !AddPolicySpec(new_spec, &error) ) {
      errors->push_back(error);
    }
  }

  // Verify the validity of the element names and dependencies:
  for ( int i = 0; i < spec.elements_.get().size(); ++i ) {
    MediaElementSpecs* const new_spec =
        new MediaElementSpecs(spec.elements_.get()[i]);
    if ( !AddElementSpec(new_spec, &error) ) {
      errors->push_back(error);
    }
  }
  return errors->size() <= size_begin;
}


// This is not very fast is fine ..
bool ElementFactory::DeleteElementSpec(const string& name, ErrorData* error) {
  ElementSpecMap::iterator it_src = elements_map_.find(name);
  if ( it_src == elements_map_.end() ) {
    error->description_ = "Unknown element: [" + name + "]";
    return false;
  }
  delete it_src->second;
  elements_map_.erase(it_src);

  // Clear the state of keys that were taken..
  if ( state_keeper_ != NULL ) {
    state_keeper_->DeletePrefix(string("src/") + name + "/");
    state_keeper_->DeletePrefix(string("src/") + name +
                                ":policy/" + name + "/");
  }
  if ( local_state_keeper_ != NULL ) {
    local_state_keeper_->DeletePrefix(string("src/") + name + "/");
    local_state_keeper_->DeletePrefix(string("src/") + name +
                                      ":policy/" + name + "/");
  }
  return true;
}

bool ElementFactory::DeletePolicySpec(const string& policy_name,
                                      ErrorData* error) {
  PolicySpecMap::iterator it_policy = policies_map_.find(policy_name);
  if ( it_policy == policies_map_.end() ) {
    error->description_ = "Cannot find policy name [" + policy_name + "]";
    return false;
  }
  delete it_policy->second;
  policies_map_.erase(it_policy);
  return true;
}

bool ElementFactory::DeleteAuthorizerSpec(const string& name,
                                          ErrorData* error) {
  AuthorizerSpecMap::iterator it_authorizer = authorizers_map_.find(name);
  if ( it_authorizer == authorizers_map_.end() ) {
    error->description_ = "Cannot find policy name [" + name + "]";
    return false;
  }
  delete it_authorizer->second;
  authorizers_map_.erase(it_authorizer);
  return true;
  if ( state_keeper_ != NULL ) {
    state_keeper_->DeletePrefix(string("auth/") + name + "/");
  }
  if ( local_state_keeper_ != NULL ) {
    local_state_keeper_->DeletePrefix(string("auth/") + name + "/");
  }
}


void ElementFactory::GetElementConfig(
    rpc::CallContext<MediaRequestConfigSpec>* call,
    const MediaRequestSpec& spec) {
  const string transmedia = mapper_->TranslateMedia(
      spec.media_.get().c_str());
  vector<string> components;
  strutil::SplitString(transmedia, "/", &components);

  MediaRequestConfigSpec ret_spec;
  streaming::Element* element;
  vector<streaming::Policy*>* policies;
  for ( int i = 0; i < components.size(); ++i ) {
    policies = NULL;
    element = NULL;
    if ( mapper_->GetElementByName(components[i],
                                   &element, &policies) ) {
      const MediaElementSpecs*
          elem_spec = GetElementSpec(element->name());
      if ( elem_spec != NULL ) {
        ret_spec.persistent_element_specs_.ref().push_back(*elem_spec);
        const string param = element->GetElementConfig();
        if ( !param.empty() ) {
          const size_t ndx = ret_spec.persistent_element_specs_.get().size() - 1;
          ret_spec.persistent_element_specs_.ref()[ndx].params_.ref() =
              param;
        }
      }
      if ( policies ) {
        for ( int j = 0; j < policies->size(); ++j ) {
          const PolicySpecs*
              policy_spec = GetPolicySpec((*policies)[j]->name());
          if ( policy_spec != NULL ) {
            ret_spec.persistent_policy_specs_.ref().push_back(*policy_spec);
            const string param = (*policies)[j]->GetPolicyConfig();
            if ( !param.empty() ) {
              const size_t ndx =
                  ret_spec.persistent_policy_specs_.get().size() - 1;
              ret_spec.persistent_policy_specs_.ref()[ndx].params_.ref() =
                  param;
            }
          }
        }
      }
    }
  }
  ret_spec.error_.set(0);
  call->Complete(ret_spec);
}

void ElementFactory::GetMediaDetails(
    rpc::CallContext<ElementExportSpec>* call,
    const string& protocol,
    const string& path) {
  string key;
  RequestServingInfo* info = mapper_->GetMediaServingInfo(
      protocol, path, &key);
  ElementExportSpec spec;
  if ( info == NULL ) {
    spec.media_name_.ref() = "";
    spec.protocol_.set(protocol);
    spec.path_.set(path);
  } else {
    info->ToElementExportSpec(key, &spec);
  }
  call->Complete(spec);
}


}

////////////////////////////////////////////////////////////////////////////////
