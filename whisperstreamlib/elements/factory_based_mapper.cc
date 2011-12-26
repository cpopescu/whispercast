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

#include "elements/factory_based_mapper.h"

namespace streaming {

bool InitializeElementsAndPolicies(const ElementMap& elements,
                                   const PolicyMap& policies) {
  bool success = true;
  for ( ElementMap::const_iterator it = elements.begin();
        it != elements.end(); ++it ) {
    if ( !it->second->Initialize() ) {
      LOG_ERROR << "=========> Could not initialize correctly the element: "
                <<  it->second->name();
      success = false;
    }
  }
  for ( PolicyMap::const_iterator it = policies.begin();
        it != policies.end(); ++it ) {
    for ( int i = 0; i < it->second->size(); ++i ) {
      Policy& policy = *(*it->second)[i];
      if ( !policy.Initialize() ) {
        LOG_ERROR << "Could not initialize correctly the policy: "
                  <<  policy.name();
        success = false;
      }
      LOG_DEBUG << "Successfully initialized policy, "
                  "type: " << policy.type() << " , "
                  "name: " << policy.name();
    }
  }
  return success;
}

//////////////////////////////////////////////////////////////////////

FactoryBasedElementMapper::FactoryBasedElementMapper(
    net::Selector* selector,
    ElementFactory* factory,
    io::StateKeepUser* alias_state_keeper)
    : ElementMapper(selector),
      factory_(factory),
      alias_state_keeper_(alias_state_keeper),
      extra_element_spec_map_(NULL),
      extra_policy_spec_map_(NULL),
      close_pending_element_count_(0),
      close_completed_(NULL) {
  CHECK_NOT_NULL(factory_);
}

FactoryBasedElementMapper::~FactoryBasedElementMapper() {
  CHECK(requests_set_.empty()) << requests_set_.size() << " requests lingering"
      ", Close() must be called before deleting the FactoryBasedElementMapper";
  CHECK_EQ(close_pending_element_count_, 0);
  CHECK(global_element_map_.empty());
  CHECK(global_policy_map_.empty());
  CHECK(non_global_element_map_.empty());
  CHECK(non_global_policy_map_.empty());

  for ( AuthorizerMap::const_iterator it = authorizer_map_.begin();
        it != authorizer_map_.end(); ++it ) {
    it->second->DecRef();
  }
  authorizer_map_.clear();
  delete alias_state_keeper_;
}

//////////////////////////////////////////////////////////////////////

bool FactoryBasedElementMapper::Initialize() {
  LOG_DEBUG << "Initializing FactoryBasedElementMapper";
  const bool creation_success =
      factory_->CreateAllElements(&global_element_map_,
                                  &global_policy_map_,
                                  &non_global_element_map_,
                                  &non_global_policy_map_);
  if ( !creation_success ) {
    LOG_ERROR << " Error creating initalized elements - some elements may not "
              << "be available";
    // TODO(cosmin): commented the return below.
    //               check correctness in case CreateAllElements failed.
    // return false;
  }
  const bool globals_initialized =
      InitializeElementsAndPolicies(global_element_map_,
                                    global_policy_map_);
  const bool non_globals_initialized =
      InitializeElementsAndPolicies(non_global_element_map_,
                                    non_global_policy_map_);

  const bool auth_creation_success =
      factory_->CreateAllAuthorizers(&authorizer_map_);
  for ( AuthorizerMap::const_iterator it = authorizer_map_.begin();
        it != authorizer_map_.end(); ++it ) {
    it->second->IncRef();
    if ( !it->second->Initialize() ) {
      LOG_ERROR << "Cannot initialize authorizer: " << it->first;
    }
  }
  return (creation_success &&
          auth_creation_success &&
          non_globals_initialized &&
          globals_initialized);
}

void FactoryBasedElementMapper::Close(Closure* close_completed) {
  CHECK_NULL(close_completed_);
  vector<string> names;
  factory_->GetAllNames(&names);
  if ( names.empty() ) {
    close_completed->Run();
    return;
  }
  close_completed_ = close_completed;
  for ( int i = 0; i < names.size(); ++i  ) {
    RemoveElement(names[i]);
  }
  // NEXT: each element signals close completion through ElementClosed()
  //       There we check if all elements were closed, and then call
  //       close_completed_.
}

//////////////////////////////////////////////////////////////////////

bool FactoryBasedElementMapper::AddAuthorizer(const string& name) {
  if ( authorizer_map_.find(name) != authorizer_map_.end() ) {
    return false;
  }
  Authorizer* const auth = factory_->CreateAuthorizer(name);
  if ( auth == NULL ) {
    return false;
  }
  auth->IncRef();
  if ( !auth->Initialize() ) {
    LOG_ERROR << "Cannot initialize authorizer: " << auth->name();
    auth->DecRef();
    return false;
  }
  LOG_DEBUG << "Created authorizer: [" << name << "]";
  authorizer_map_.insert(make_pair(name, auth));
  return true;
}

void FactoryBasedElementMapper::RemoveAuthorizer(const string& name) {
  const AuthorizerMap::iterator it = authorizer_map_.find(name);
  if ( it == authorizer_map_.end() ) {
    return;
  }
  it->second->DecRef();
  authorizer_map_.erase(it);
}

//////////////////////////////////////////////////////////////////////

bool FactoryBasedElementMapper::AddElement(const string& name,
                                           bool is_global) {
  if ( IsKnownElementName(name) ) {
    return false;
  }
  string test;
  if ( GetMediaAlias(name, &test) ) {
    return false;
  }
  // If the element is global we create it. If eveything is bug-free
  // this should be successful
  PolicyMap new_policies;
  Element* const element = factory_->CreateElement(name,
                                                   extra_element_spec_map_,
                                                   extra_policy_spec_map_,
                                                   NULL, &new_policies);
  if ( element == NULL ) {
    return false;
  }
  LOG_DEBUG << "Added and created element: " << name;
  ElementMap elements;
  elements[name] = element;
  InitializeElementsAndPolicies(elements, new_policies);
  if ( is_global ) {
    global_element_map_.insert(make_pair(name, element));
  } else {
    non_global_element_map_.insert(make_pair(name, element));
  }
  if ( !new_policies.empty() ) {
    DCHECK_EQ(new_policies.size(), 1);
    DCHECK(new_policies.find(element) != new_policies.end());
    if ( is_global ) {
      global_policy_map_[element] = new_policies[element];
    } else {
      non_global_policy_map_[element] = new_policies[element];
    }
  }
  return true;
}

void FactoryBasedElementMapper::RemoveTempElement(const string& name) {
  // This is quite slow, but, hopefully we will not delete elements that often
  for ( RequestSet::iterator it = requests_set_.begin();
        it != requests_set_.end();  ) {
    TempElementStruct* ts = (*it)->mutable_temp_struct();
    ElementMap::iterator it_element = ts->elements_.find(name);
    if ( it_element != ts->elements_.end() ) {
      // We have a source w/ this name in this temp entry..
      Element* const elem = it_element->second;
      // Here is how chain of calls should go, on the req path:
      //
      //  [x1] -> [x2] -> [elem] -> [y1] -> [y2]
      //
      //  elem->Close()
      //    |-------> EOS to y1
      //    |           |-------> RemoveRequest(y1->elem callback)
      //    |           |---> EOS to y2
      //    |                     |---> RequestRemove(y2->y1 callback)
      //    |
      //    |-------> RemoveRequest(elem->x2 callback)
      //                 |----> RemoveRequest(x2->x1 callback)
      ts->elements_.erase(it_element);
      PolicyMap::iterator it_policies = ts->policies_.find(elem);
      vector<streaming::Policy*>* policies = NULL;
      if ( it_policies != ts->policies_.end() ) {
        policies = it_policies->second;
        ts->policies_.erase(it_policies);
      }
      ++it;   // so it does not get invalidated..
      LOG_DEBUG << "Removing temporary element: [" << elem->name() << "]";
      close_pending_element_count_++;
      elem->Close(NewCallback(this, &FactoryBasedElementMapper::ElementClosed,
          elem, policies));
    } else {
      ++it;
    }
  }
}

void FactoryBasedElementMapper::RemoveElement(const string& name) {
  //
  // Check first the global structures..
  //
  ElementMap* element_map[] = { &global_element_map_,
                                &non_global_element_map_ };
  PolicyMap* policy_map[] = { &global_policy_map_,
                              &non_global_policy_map_ };
  for ( int i = 0; i < NUMBEROF(element_map); ++i ) {
    const ElementMap::iterator it_perm = element_map[i]->find(name);
    if ( it_perm != element_map[i]->end() ) {
      Element* const elem = it_perm->second;
      element_map[i]->erase(it_perm);
      PolicyMap::iterator it_policies = policy_map[i]->find(elem);
      vector<streaming::Policy*>* policies = NULL;
      if ( it_policies != policy_map[i]->end() ) {
        policies = it_policies->second;
        policy_map[i]->erase(it_policies);
      }
      LOG_DEBUG << "RemoveElement: [" << elem->name() << "]";
      close_pending_element_count_++;
      elem->Close(NewCallback(this, &FactoryBasedElementMapper::ElementClosed,
          elem, policies));
    }
  }
  RemoveTempElement(name);
}
void FactoryBasedElementMapper::ElementClosed(Element* element,
    vector<streaming::Policy*>* policies) {
  CHECK_GT(close_pending_element_count_, 0);
  close_pending_element_count_--;
  LOG_DEBUG << "Deleting element: " << element->name() << " and "
           << (policies == NULL ? 0 : policies->size())
           << " associated policies, close_pending: "
           << close_pending_element_count_;
  if ( policies != NULL ) {
    for ( int i = 0; i < policies->size(); ++i ) {
      delete (*policies)[i];
    }
    delete policies;
  }
  delete element;
  if ( close_pending_element_count_ == 0 && close_completed_ != NULL ) {
    if ( !requests_set_.empty() ) {
      LOG_ERROR << "Pending requests: " << strutil::ToStringP(requests_set_);
    }
    selector_->RunInSelectLoop(close_completed_);
    close_completed_ = NULL;
  }
}

bool FactoryBasedElementMapper::IsKnownElementName(const string& name) {
  if ( global_element_map_.find(name) != global_element_map_.end() ||
       non_global_element_map_.find(name) != non_global_element_map_.end() ) {
    return true;
  }
  string test;
  if ( GetMediaAlias(name, &test) ) {
    return false;
  }
  if ( fallback_mapper_ != NULL ) {
    return fallback_mapper_->IsKnownElementName(name);
  }
  return false;
}

bool FactoryBasedElementMapper::GetElementByName(
    const string& name,
    streaming::Element** element,
    vector<streaming::Policy*>** policies) {
  *element = NULL;
  *policies = NULL;
  const ElementMap::const_iterator
      it_perm = global_element_map_.find(name);
  if ( it_perm != global_element_map_.end() ) {
    *element = it_perm->second;
    const PolicyMap::const_iterator
        it_policy = global_policy_map_.find(it_perm->second);
    if ( it_policy != global_policy_map_.end() ) {
      *policies = it_policy->second;
    }
    return true;
  }
  const ElementMap::const_iterator
      it_non_perm = non_global_element_map_.find(name);
  if ( it_non_perm != non_global_element_map_.end() ) {
    *element = it_non_perm->second;
    const PolicyMap::const_iterator
        it_policy = non_global_policy_map_.find(it_non_perm->second);
    if ( it_policy != non_global_policy_map_.end() ) {
      *policies = it_policy->second;
    }
    return true;
  }
  if ( fallback_mapper_ != NULL ) {
    return fallback_mapper_->GetElementByName(name, element, policies);
  }
  return false;
}

//////////////////////////////////////////////////////////////////////

bool FactoryBasedElementMapper::AddServingInfo(
    const string& protocol,
    const string& path,
    RequestServingInfo* serving_info) {
  const string key = (protocol + ":" +
                      strutil::NormalizeUrlPath("/" + path));
  if ( serving_paths_.find(key) != serving_paths_.end() ) {
    return false;
  }
  serving_paths_.insert(make_pair(key, serving_info));
  return true;
}

bool FactoryBasedElementMapper::RemoveServingInfo(const string& protocol,
                                                  const string& path) {
  const string key = (protocol + ":" +
                      strutil::NormalizeUrlPath("/" + path));
  const ServingInfoMap::iterator it = serving_paths_.find(key);
  if ( it == serving_paths_.end() ) {
    return false;
  }
  delete it->second;
  serving_paths_.erase(it);
  return true;
}

bool FactoryBasedElementMapper::SetMediaAlias(const string& alias_name,
                                              const string& media_name,
                                              string* error) {
  if ( media_name.empty() ) {
    if ( alias_state_keeper_ != NULL ) {
      if ( !alias_state_keeper_->DeleteValue(alias_name) ) {
        *error = "Error deleting alias";
        return false;
      }
    }
    return true;
  }
  if ( global_element_map_.find(alias_name) !=
       global_element_map_.end() ||
       non_global_element_map_.find(alias_name) !=
       non_global_element_map_.end() ) {
    *error = "Invalid alias name - matches some element name";
    return false;
  }
  if ( !strutil::IsValidIdentifier(alias_name.c_str()) ) {
    *error = "Invalid alias identifier";
    return false;
  }
  vector<string> components;
  strutil::SplitString(media_name, "/", &components);
  string test;
  for ( int i = 0; i < components.size(); ++i ) {
    if ( GetMediaAlias(components[i], &test) ) {
      *error = "Invalid internal alias reference";
      return false;
    }
  }
  if ( alias_state_keeper_ != NULL ) {
    if ( !alias_state_keeper_->SetValue(alias_name, media_name) ) {
      *error = "Error saving alias value";
      return false;
    }
  }
  return true;
}

void FactoryBasedElementMapper::GetAllMediaAliases(
    vector< pair<string, string> >* aliases) const {
  if ( alias_state_keeper_ == NULL ) return;
  map<string, string>::const_iterator begin, end, it;
  alias_state_keeper_->GetBounds("", &begin, &end);
  for ( it = begin; it != end; ++it ) {
    aliases->push_back(make_pair(it->first.substr(
                                     alias_state_keeper_->prefix().size()),
                                 it->second));
  }
}

//////////////////////////////////////////////////////////////////////


void FactoryBasedElementMapper::GetMediaDetails(
    const string& protocol,
    const string& path,
    streaming::Request* req,
    Callback1<bool>* completion_callback) {
  if ( fallback_mapper_ != NULL ) {
    fallback_mapper_->GetMediaDetails(
        protocol, path, req, completion_callback);
    return;
  }
  const string original_key = (protocol + ":" +
                               strutil::NormalizeUrlPath("/" + path));
  DLOG_DEBUG << "Looking for key: " << original_key
            << " in req: " << req->ToString();
  string key = original_key;
  const RequestServingInfo* const
      info = io::FindPathBased<RequestServingInfo*>(&serving_paths_, key);
  if ( info == NULL ) {
    LOG_ERROR << "Cannot find key=" << key << " in serving_paths_: "
              << strutil::ToStringKeys(serving_paths_);
    completion_callback->Run(false);
    return;
  }
  *req->mutable_serving_info() = *info;
  req->mutable_info()->write_ahead_ms_ =
      (req->serving_info().flow_control_total_ms_ > 1) ?
          req->serving_info().flow_control_total_ms_/2 : 0;

  const string element_path = original_key.substr(key.size());
  if ( !element_path.empty() && element_path[0] != '/' ) {
    req->mutable_serving_info()->media_name_ =
        strutil::NormalizeUrlPath(info->media_name_ + "/" + element_path);
  } else {
    req->mutable_serving_info()->media_name_ =
        info->media_name_ + element_path;
  }
  DLOG_DEBUG << "Details found: " << req->ToString();
  completion_callback->Run(true);
}

string FactoryBasedElementMapper::GetElementName(const char* media) {
  if ( *media == '/' ) {
    media++;
  }
  const char* const slash_pos = strchr(media, '/');
  return string(slash_pos == NULL
                ? string(media)
                : string(media, slash_pos - media));

}

int32 FactoryBasedElementMapper::AddExportClient(
    const string& protocol, const string& path) {
  const string key = protocol + ":" + path;
  ExportClientCountMap::iterator it = export_client_count_.find(key);
  if ( it == export_client_count_.end() ) {
    it = export_client_count_.insert(make_pair(key, 0)).first;
  }
  it->second++;
  return it->second;
}
void FactoryBasedElementMapper::RemoveExportClient(
    const string& protocol, const string& path) {
  const string key = protocol + ":" + path;
  ExportClientCountMap::iterator it = export_client_count_.find(key);
  if ( it == export_client_count_.end() ) {
    LOG_ERROR << "RemoveExportClient failed, no export client found"
                 " for key: [" << key << "]";
    return;
  }
  CHECK(it->second > 0) << " Too many RemoveExportClient";
  it->second--;
}

namespace {
string ImporterKey(Importer::Type importer_type, const string& path) {
  return strutil::StringPrintf("%s:%s",
      Importer::TypeName(importer_type), path.c_str());
}
string ImporterKey(Importer* importer) {
  return ImporterKey(importer->type(), importer->ImportPath());
}
}
bool FactoryBasedElementMapper::AddImporter(Importer* importer) {
  CHECK(selector_->IsInSelectThread()); // media selector
  const string key = ImporterKey(importer);
  ImporterMap::iterator it = importer_map_.find(key);
  if ( it != importer_map_.end() ) {
    LOG_ERROR << "AddImporter failed, duplicate key: [" << key << "]";
    return false;
  }
  importer_map_[key] = importer;
  return true;
}
void FactoryBasedElementMapper::RemoveImporter(Importer* importer) {
  CHECK(selector_->IsInSelectThread()); // media selector
  importer_map_.erase(ImporterKey(importer));
}
Importer* FactoryBasedElementMapper::GetImporter(
    Importer::Type importer_type, const string& path) {
  CHECK(selector_->IsInSelectThread()); // media selector
  const string key = ImporterKey(importer_type, path);
  ImporterMap::iterator it = importer_map_.find(key);
  if ( it == importer_map_.end() ) {
    LOG_ERROR << "GetImporter failed, cannot find key: [" << key << "]";
    return NULL;
  }
  return it->second;
}

string FactoryBasedElementMapper::TranslateMedia(const char* media) const {
  if ( *media == '\0' ) {
    return "";
  }
  if ( *media == '/' ) {
    media++;
  }
  pair<string, string> media_pair = strutil::SplitFirst(media, '/');
  const ElementMap::const_iterator
      it_perm = global_element_map_.find(media_pair.first);
  if ( it_perm == global_element_map_.end() ) {
    const ElementMap::const_iterator
        it_non_perm = non_global_element_map_.find(media_pair.first);
    if ( it_non_perm == non_global_element_map_.end() ) {
      string alias;
      if ( !GetMediaAlias(media_pair.first, &alias) ) {
        return media;
      }
      media_pair.first = alias;
    }
  }
  if ( media_pair.second.empty() ) {
    if ( fallback_mapper_ != NULL ) {
      return fallback_mapper_->TranslateMedia(media);
    }
    return media_pair.first;
  } else if ( master_mapper_ ) {
    return media_pair.first + "/" +
        master_mapper_->TranslateMedia(media_pair.second.c_str());
  } else {
    return media_pair.first + "/" + TranslateMedia(media_pair.second.c_str());
  }
}

bool FactoryBasedElementMapper::DescribeMedia(const string& media,
    MediaInfoCallback* callback) {
  pair<string, string> media_pair = strutil::SplitFirst(media.c_str(), '/');
  const ElementMap::const_iterator it_perm =
      global_element_map_.find(media_pair.first);
  if ( it_perm != global_element_map_.end() ) {
    return it_perm->second->DescribeMedia(media_pair.second, callback);
  }
  const ElementMap::const_iterator it_non_perm =
      non_global_element_map_.find(media_pair.first);
  if ( it_non_perm != non_global_element_map_.end() ) {
    return it_non_perm->second->DescribeMedia(media_pair.second, callback);
  }
  string alias;
  if ( GetMediaAlias(media_pair.first, &alias) ) {
    string translated_media = strutil::JoinMedia(alias, media_pair.second);
    if ( master_mapper_ ) {
      return master_mapper_->DescribeMedia(translated_media, callback);
    }
    return DescribeMedia(translated_media, callback);
  }
  if ( fallback_mapper_ != NULL ) {
    return fallback_mapper_->DescribeMedia(media, callback);
  }
  // media not found
  LOG_ERROR << "DescribeMedia: not found. Element: [" << media_pair.first
            << "], media: [" << media << "]";
  return false;
}

bool FactoryBasedElementMapper::HasMedia(const char* media, Capabilities* out) {
  if ( *media == '\0' ) {
    return false;
  }
  if ( *media == '/' ) {
    media++;
  }
  pair<string, string> media_pair = strutil::SplitFirst(media, '/');
  DLOG_DEBUG
  << "Looking for element: [" << media_pair.first << "] from media: "
            << "[" << media << "]";
  const ElementMap::const_iterator
      it_perm = global_element_map_.find(media_pair.first);
  if ( it_perm != global_element_map_.end() ) {
    return it_perm->second->HasMedia(media, out);
  }
  const ElementMap::const_iterator
      it_non_perm = non_global_element_map_.find(media_pair.first);
  if ( it_non_perm != non_global_element_map_.end() ) {
    return it_non_perm->second->HasMedia(media, out);
  }
  string alias;
  if ( GetMediaAlias(media_pair.first, &alias) ) {
    if ( media_pair.second.empty() ) {
      if ( master_mapper_ ) {
        return master_mapper_->HasMedia(alias.c_str(), out);
      }
      return HasMedia(alias.c_str(), out);
    } else {
      if ( master_mapper_ ) {
        master_mapper_->HasMedia(strutil::JoinMedia(
            alias, media_pair.second).c_str(), out);
      }
      return HasMedia(strutil::JoinMedia(
          alias, media_pair.second).c_str(), out);
    }
  }
  if ( fallback_mapper_ != NULL ) {
    return fallback_mapper_->HasMedia(media, out);
  }
  return false;
}

void FactoryBasedElementMapper::ListMedia(const char* media,
                                          ElementDescriptions* medias) {
  if ( *media == '\0' ) {
    return;
  }
  if ( *media == '/' ) {
    media++;
  }
  pair<string, string> media_pair = strutil::SplitFirst(media, '/');
  const ElementMap::const_iterator
      it_perm = global_element_map_.find(media_pair.first);
  if ( it_perm != global_element_map_.end() ) {
    it_perm->second->ListMedia(media, medias);
    return;
  }
  const ElementMap::const_iterator
      it_non_perm = non_global_element_map_.find(media_pair.first);
  if ( it_non_perm != non_global_element_map_.end() ) {
    it_non_perm->second->ListMedia(media, medias);
    return;
  }
  string alias;
  if ( GetMediaAlias(media_pair.first, &alias) ) {
    if ( media_pair.second.empty() ) {
      if ( master_mapper_ ) {
        master_mapper_->ListMedia(alias.c_str(), medias);
        return;
      }
      ListMedia(alias.c_str(), medias);
    } else {
      if ( master_mapper_ ) {
        master_mapper_->ListMedia(
            (alias + "/" + media_pair.second).c_str(), medias);
        return;
      }
      ListMedia((alias + "/" + media_pair.second).c_str(), medias);
    }
  }
  if ( fallback_mapper_ != NULL ) {
    fallback_mapper_->ListMedia(media, medias);
    return;
  }
}

//////////////////////////////////////////////////////////////////////

bool FactoryBasedElementMapper::AddRequest(const char* media,
                                           streaming::Request* req,
                                           ProcessingCallback* callback) {
  if ( *media == '\0' ) {
    return false;
  }
  if ( *media == '/' ) {
    media++;
  }
  pair<string, string> media_pair = strutil::SplitFirst(media, '/');
  DLOG_DEBUG << "Add request: [" << media << "], req: " << req->ToString()
             << ", callback: " << callback
             << ", element: [" << media_pair.first << "]" ;

  const ElementMap::const_iterator
      it_perm = global_element_map_.find(media_pair.first);
  if ( it_perm != global_element_map_.end() ) {
    Element* element = it_perm->second;
    if ( req->rev_callbacks().find(element) != req->rev_callbacks().end() ) {
      LOG_ERROR << "Duplicate element found in media path @: " << media;
      return false;
    }
    if ( element->AddRequest(media, req, callback) ) {
      requests_set_.insert(req);
      // It is a certain possibility that the element calls AddRequest
      // with the same callback, but on another path
      if ( req->callbacks().find(callback) == req->callbacks().end() ) {
        req->mutable_callbacks()->insert(make_pair(callback, element));
      }
      req->mutable_rev_callbacks()->insert(make_pair(element, callback));
      return true;
    }
    LOG_ERROR << "AddRequest failed for media: [" << media << "]"
                 ", element: [" << element->name()
              << "], refused req: " << req->ToString();
    return false;
  }
  // Look for aliases
  string alias;
  if ( GetMediaAlias(media_pair.first, &alias) ) {
    if ( media_pair.second.empty() ) {
      DLOG_DEBUG << "Redirecting request to: " << media
                 << " via Alias to: " << alias;
      if ( master_mapper_ != NULL ) {
        return master_mapper_->AddRequest(alias.c_str(), req, callback);
      }
      return AddRequest(alias.c_str(), req, callback);
    } else {
      DLOG_DEBUG << "Redirecting request to: " << media
                 << " via Alias to: " << (alias + "/" + media_pair.second);
      if ( master_mapper_ != NULL ) {
        return master_mapper_->AddRequest(
            (alias + "/" + media_pair.second).c_str(),
            req, callback);
      }
      return AddRequest((alias + "/" + media_pair.second).c_str(),
                        req, callback);
    }
  }
  // Look for temp sources..
  if ( non_global_element_map_.find(media_pair.first) ==
       non_global_element_map_.end() ) {
    if ( fallback_mapper_ != NULL ) {
      return fallback_mapper_->AddRequest(media, req, callback);
    }
    LOG_ERROR << " Bad element name: "<< media_pair.first;
    return false;
  }

  if ( req->temp_struct().elements_.empty() ) {
    if ( !req->callbacks().empty() ) {
      if ( fallback_mapper_ != NULL ) {
        return fallback_mapper_->AddRequest(media, req, callback);
      }
      LOG_ERROR << " Cannot request for a temp element after registering "
                << "to global ones: "
                << media_pair.first;
      return false;
    }
    req->mutable_temp_struct()->element_name_ = media_pair.first;
  }
  ElementMap::const_iterator
      root_it = req->temp_struct().elements_.find(media_pair.first);
  if ( root_it == req->temp_struct().elements_.end() ) {
    Element* const element = factory_->CreateElement(
        media_pair.first,
        extra_element_spec_map_,
        extra_policy_spec_map_,
        req,
        &req->mutable_temp_struct()->policies_);
    if ( element == NULL ) {
      if ( fallback_mapper_ != NULL ) {
        return fallback_mapper_->AddRequest(media, req, callback);
      }
      LOG_ERROR << " Cannot create temp structures for element: "
                << media_pair.first;
      return false;
    }
    root_it = req->mutable_temp_struct()->elements_.insert(
        make_pair(media_pair.first, element)).first;

    PolicyMap::const_iterator policy_it =
        req->temp_struct().policies_.find(element);
    element->Initialize();
    if ( policy_it != req->temp_struct().policies_.end() ) {
      for ( int i = 0; i < policy_it->second->size(); ++i ) {
        Policy& policy = *(*policy_it->second)[i];
        policy.Initialize();
      }
    }

    DLOG_DEBUG << "Temp element created: " << media_pair.first
              << " under id: ["
              << req->info().GetId() << "]"
              << " for request: " << req->ToString()
              << " other: " << req->temp_struct().elements_.size()
              << " elements created.. "
              << " and: " << req->temp_struct().policies_.size()
              << " policies.";
  }
  if ( req->rev_callbacks().find(root_it->second) != req->rev_callbacks().end() ) {
    LOG_ERROR << " Duplicate element found in media path @: " << media;
    return false;
  }
  if ( !root_it->second->AddRequest(media, req, callback) ) {
    LOG_ERROR << "Error adding callback for media: [" << media << "]"
              << " to temp source: " << root_it->second->name();
    return false;
  }
  requests_set_.insert(req);

  // As above - it is a certain possibility that the element calls AddRequest
  // with the same callback, but on another path
  if ( req->callbacks().find(callback) == req->callbacks().end() ) {
    req->mutable_callbacks()->insert(make_pair(callback, root_it->second));
  }
  req->mutable_rev_callbacks()->insert(make_pair(root_it->second, callback));

  DLOG_DEBUG << " Request for " << media << " in req: " << req->ToString()
            << " - OK - via TEMP";
  return true;
}

void FactoryBasedElementMapper::RemoveRequest(streaming::Request* req,
                                              ProcessingCallback* callback) {
  if ( fallback_mapper_ != NULL ) {
    fallback_mapper_->RemoveRequest(req, callback);
  }
  DLOG_DEBUG << "Removing callback: " << callback
             << ", from request: " << req->ToString();
  Request::CallbacksMap::iterator it =
      req->mutable_callbacks()->find(callback);
  if ( it == req->mutable_callbacks()->end() ) {
    LOG_FATAL << "CANNOT find callback: " << callback
              << " in req: " << req->ToString();
  }
  if ( it != req->mutable_callbacks()->end() ) {
    Element* elem = it->second;
    req->mutable_callbacks()->erase(it);
    req->mutable_rev_callbacks()->erase(elem);

    if ( !req->callbacks().empty() ) {
      elem->RemoveRequest(req);  // we do this now - after the empty check
                                 // as this may call this function recursively
      return;
    }
    elem->RemoveRequest(req);
  }

  if ( req->callbacks().empty() &&
       master_mapper_ == NULL &&
       !req->deleted_) {
    requests_set_.erase(req);
    // OK - a request with no left callbacks - time to delete !!
    DLOG_DEBUG << "Deleting request with no callbacks left: "
              << req->ToString();
    req->deleted_ = true;
    req->Close(NewCallback(selector_, &net::Selector::DeleteInSelectLoop, req));
  }
}

}
