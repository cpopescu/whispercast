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

#include <whisperstreamlib/elements/standard_library/http_server/import_element.h>

namespace streaming {

ImportElement::ImportElement(const string& class_name,
              const string& name,
              ElementMapper* mapper,
              net::Selector* selector,
              const string& rpc_path,
              rpc::HttpServer* rpc_server,
              io::StateKeepUser* state_keeper,
              const string& authorizer_name)
   : Element(class_name, name, mapper),
     selector_(selector),
     rpc_path_(rpc_path),
     rpc_server_(rpc_server),
     state_keeper_(state_keeper),
     authorizer_name_(authorizer_name),
     close_completed_(NULL) {
}

ImportElement::~ImportElement() {
  CHECK(imports_.empty()) << "Close() did not complete!";
}

bool ImportElement::Initialize() {
  return LoadState();
}

bool ImportElement::AddRequest(
    const string& media, Request* req, ProcessingCallback* callback) {
  CHECK(requests_.find(req) == requests_.end());
  ImportMap::const_iterator it = imports_.find(media);
  if ( it == imports_.end() ) {
    LOG_ERROR << "Cannot find media: [" << media << "] under: " << name();
    LOG_ERROR << "Looking through imports: " << strutil::ToStringKeys(imports_);
    return false;
  }
  ImportElementData* data = it->second;
  if ( !data->AddCallback(req, callback) ) {
    LOG_ERROR << "The subelement refused callback on media name: ["
                << media << "] under: " << name();
    return false;
  }
  requests_[req] = data;
  req->mutable_caps()->flavour_mask_ = kDefaultFlavourMask;
  return true;
}


void ImportElement::RemoveRequest(
    streaming::Request* req) {
  RequestMap::iterator it = requests_.find(req);
  if ( it == requests_.end() ) {
    return;
  }
  it->second->RemoveCallback(req);
  requests_.erase(it);

  // maybe complete close
  if ( close_completed_ != NULL && requests_.empty() ) {
    selector_->RunInSelectLoop(close_completed_);
    close_completed_ = NULL;
  }
}


bool ImportElement::HasMedia(
    const string& media) {
  return imports_.find(media) != imports_.end();
}


void ImportElement::ListMedia(
    const string& media_dir, vector<string>* out) {
  for ( ImportMap::const_iterator it = imports_.begin();
        it != imports_.end(); ++it ) {
    if ( media_dir == "" || it->first == media_dir ) {
      out->push_back(it->first);
    }
  }
}


bool ImportElement::DescribeMedia(
    const string& submedia, MediaInfoCallback* callback) {
  const string media = strutil::JoinMedia(name(), submedia);
  ImportMap::const_iterator it = imports_.find(media);
  if ( it == imports_.end() ) {
    LOG_ERROR << "DescribeMedia: Cannot find media: [" << media << "], looking"
                 " through imports: " << strutil::ToStringKeys(imports_);
    return false;
  }
  callback->Run(it->second->media_info());
  return true;
}


void ImportElement::Close(Closure* call_on_close) {
  CHECK_NULL(close_completed_);

  // Close & delete all the imports.
  CloseAllImports();

  if ( requests_.empty() ) {
    // Because there are no requests => there won't be any RemoveRequest()
    // so it's safe to finalize close now
    selector_->RunInSelectLoop(call_on_close);
    return;
  }

  // async closing. Because of CloseAllImports(), each import will send
  // EOS downstream, causing a series of RemoveRequest() on us.
  // On the last RemoveRequest() the close is completed.
  close_completed_ = call_on_close;
}

bool ImportElement::AddImport(const string& import_name, bool save_state,
                              string* out_error) {
  if ( !strutil::IsValidIdentifier(import_name) ) {
    *out_error = "AddImport: invalid identifier: [" + import_name + "]";
    LOG_ERROR << (*out_error);
    return false;
  }
  ImportMap::const_iterator it = imports_.find(import_name);
  if ( it != imports_.end() ) {
    *out_error = "AddImport: [" + import_name + "] already exists";
    LOG_ERROR << (*out_error);
    return false;
  }
  ImportElementData* const src = CreateNewImport(import_name);
  if ( !src->Initialize() ) {
    delete src;
    *out_error = "AddImport: [" + import_name + "] cannot initialize";
    LOG_ERROR << (*out_error);
    return false;
  }
  imports_[import_name] = src;
  if ( save_state ) {
    SaveState();
  }
  return true;
}

bool ImportElement::DeleteImport(const string& import_name) {
  ImportMap::iterator it = imports_.find(import_name);
  if ( it == imports_.end() ) {
    LOG_ERROR << "Cannot find import: " << import_name
              << ", looking through: " << strutil::ToStringKeys(imports_);
    return false;
  }
  it->second->Close();
  delete it->second;
  imports_.erase(it);
  SaveState();
  return true;
}
void ImportElement::GetAllImports(vector<string>* out) const {
  for ( ImportMap::const_iterator it = imports_.begin();
        it != imports_.end(); ++it ) {
    out->push_back(it->first);
  }
}


void ImportElement::SaveState() {
  vector<string> imports;
  GetAllImports(&imports);
  string str_imports = rpc::JsonEncoder::EncodeObject(imports);
  state_keeper_->SetValue("imports", str_imports);
}
bool ImportElement::LoadState() {
  string str_imports;
  vector<string> imports;
  if ( !state_keeper_->GetValue("imports", &str_imports) ||
       !rpc::JsonDecoder::DecodeObject(str_imports, &imports) ) {
    LOG_ERROR << "Failed to load state. Found state: " << str_imports;
    return false;
  }
  for ( uint32 i = 0; i < imports.size(); i++ ) {
    string err;
    AddImport(imports[i], false, &err);
  }
  return true;
}

void ImportElement::CloseAllImports() {
  while ( !imports_.empty() ) {
    ImportMap::iterator it = imports_.begin();
    ImportElementData* data = it->second;
    data->Close();
    delete data;
    imports_.erase(it);
  }
}

}
