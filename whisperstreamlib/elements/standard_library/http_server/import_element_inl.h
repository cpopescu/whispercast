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


namespace streaming {

template<class Service, class Import, class ImportDataSpec>
inline ImportElement<Service, Import, ImportDataSpec>::ImportElement(
    const char* class_name,
    const char* name,
    const char* id,
    streaming::ElementMapper* mapper,
    net::Selector* selector,
    const char* media_dir,
    const char* rpc_path,
    rpc::HttpServer* rpc_server,
    io::StateKeepUser* state_keeper,
    streaming::SplitterCreator* splitter_creator,
    const char* authorizer_name)
    : Element(class_name, name, id, mapper),
      Service(Service::GetClassName()),
      selector_(selector),
      media_dir_(media_dir),
      rpc_path_(rpc_path),
      rpc_server_(rpc_server),
      rpc_registered_(false),
      state_keeper_(state_keeper),
      splitter_creator_(splitter_creator),
      authorizer_name_(authorizer_name),
      close_completed_(NULL) {
}


template<class Service, class Import, class ImportDataSpec>
inline ImportElement<Service, Import, ImportDataSpec>::~ImportElement() {
  if ( rpc_registered_ ) {
    rpc_server_->UnregisterService(rpc_path_, this);
  }
  for ( typename ImportMap::const_iterator it = imports_.begin();
        it != imports_.end(); ++it ) {
    delete it->second;
  }
  imports_.clear();
  delete splitter_creator_;
}


template<class Service, class Import, class ImportDataSpec>
inline bool ImportElement<Service, Import, ImportDataSpec>::Initialize() {
  if ( rpc_server_ != NULL ) {
    rpc_registered_ = rpc_server_->RegisterService(rpc_path_, this);
  }

  // Load the state:
  if ( state_keeper_ ) {
    map<string, string>::const_iterator begin, end;
    state_keeper_->GetBounds("import/", &begin, &end);
    for ( map<string, string>::const_iterator it = begin;
          it != end; ++it ) {
      ImportDataSpec data;
      if ( !rpc::JsonDecoder::DecodeObject(it->second, &data) ) {
        LOG_ERROR << name() << " Eror decoding state keeper saved import: "
                  << it->second;
      } else {
        MediaOperationErrorData ret;
        AddImport(&ret, data, false);  // do not save
      }
    }
  }
  return (rpc_server_ == NULL || rpc_registered_);
}

template<class Service, class Import, class ImportDataSpec>
inline bool ImportElement<Service, Import, ImportDataSpec>::AddRequest(
    const char* media_name,
    streaming::Request* req,
    streaming::ProcessingCallback* callback) {
  typename ImportMap::const_iterator it = imports_.find(media_name);
  if ( it == imports_.end() ) {
    LOG_ERROR << "Cannot find media: " << media_name << " under: " << name();
    LOG_ERROR << "Looking through imports: " << strutil::ToString(imports_);
    return false;
  }
  if ( !it->second->caps().IsCompatible(req->caps()) ) {
    LOG_WARNING << " Caps do not match: "
                << req->caps().ToString()
                << " v.s. " << it->second->caps().ToString();
    return false;
  }
  if ( it->second->AddCallback(req, callback) ) {
    requests_.insert(make_pair(req, it->second));
    req->mutable_caps()->IntersectCaps(it->second->caps());
    return true;
  }
  LOG_ERROR << "The subelement refused callback on media name="
              << media_name << " under: " << name();
  return false;
}

template<class Service, class Import, class ImportDataSpec>
inline void ImportElement<Service, Import, ImportDataSpec>::RemoveRequest(
    streaming::Request* req) {
  typename RequestMap::iterator it = requests_.find(req);
  if ( it == requests_.end() ) {
    return;
  }
  it->second->RemoveCallback(req);
  requests_.erase(it);

  // maybe complete close
  if ( close_completed_ != NULL && requests_.empty() ) {
    Closure* call_on_close = close_completed_;
    close_completed_ = NULL;

    selector_->RunInSelectLoop(call_on_close);
  }
}

template<class Service, class Import, class ImportDataSpec>
inline bool ImportElement<Service, Import, ImportDataSpec>::HasMedia(
    const char* media, Capabilities* out) {
  typename ImportMap::const_iterator it = imports_.find(string(media));
  if ( it == imports_.end() ) {
    return false;
  }
  *out = it->second->caps();
  return true;
}

template<class Service, class Import, class ImportDataSpec>
inline void ImportElement<Service, Import, ImportDataSpec>::ListMedia(
    const char* media_dir,
    streaming::ElementDescriptions* medias) {
  for ( typename ImportMap::const_iterator it = imports_.begin();
        it != imports_.end(); ++it ) {
    if ( *media_dir == '\0' || it->first == media_dir ) {
      medias->push_back(make_pair(it->first, it->second->caps()));
    }
  }
}

template<class Service, class Import, class ImportDataSpec>
inline bool ImportElement<Service, Import, ImportDataSpec>::DescribeMedia(
    const string& submedia, MediaInfoCallback* callback) {
  const string media = strutil::JoinMedia(name(), submedia);
  typename ImportMap::const_iterator it = imports_.find(media);
  if ( it == imports_.end() ) {
    LOG_ERROR << "DescribeMedia: Cannot find media: [" << media << "], looking"
                 " through imports: " << strutil::ToStringKeys(imports_);
    return false;
  }
  if ( it->second->splitter() == NULL ) {
    LOG_ERROR << "DescribeMedia: [" << media << "] , splitter is NULL";
    return false;
  }
  LOG_INFO << "DescribeMedia: [" << media << "] found media info: "
           << it->second->splitter()->media_info().ToString();
  callback->Run(&(it->second->splitter()->media_info()));
  return true;
}

template<class Service, class Import, class ImportDataSpec>
inline void ImportElement<Service, Import, ImportDataSpec>::Close(
    Closure* call_on_close) {
  CHECK_NULL(close_completed_);
  if ( requests_.empty() ) {
    selector_->RunInSelectLoop(call_on_close);
    return;
  }
  close_completed_ = call_on_close;

  selector_->RunInSelectLoop(
      NewCallback(this,
                  &ImportElement<Service, Import,
                  ImportDataSpec>::CloseAllImports));
}

template<class Service, class Import, class ImportDataSpec>
inline void ImportElement<Service, Import, ImportDataSpec>::CloseAllImports() {
  for ( typename ImportMap::const_iterator it = imports_.begin();
        it != imports_.end(); ++it ) {
    it->second->Close();   // causes the close of registered requests and
                           // eventually deletion
  }
}

// RPC interface

template<class Service, class Import, class ImportDataSpec>
inline void ImportElement<Service, Import, ImportDataSpec>::AddImport(
    rpc::CallContext<MediaOperationErrorData>* call,
    const ImportDataSpec& spec) {
  MediaOperationErrorData ret;
  AddImport(&ret, spec, true);  // save to state ..
  call->Complete(ret);
}


template<class Service, class Import, class ImportDataSpec>
inline void ImportElement<Service, Import, ImportDataSpec>::DeleteImport(
    rpc::CallContext<MediaOperationErrorData>* call,
    const string& name) {
  MediaOperationErrorData ret;
  if ( DeleteImport(name.c_str()) ) {
    state_keeper_->DeleteValue(strutil::StringPrintf("import/%s",
                                                     name.c_str()));
    ret.error_.ref() = 0;
  } else {
    ret.error_.ref() = 1;
    ret.description_.ref() = "Invalid Import - cannot delete";
  }
  call->Complete(ret);
}


template<class Service, class Import, class ImportDataSpec>
inline void ImportElement<Service, Import, ImportDataSpec>::GetImports(
    rpc::CallContext< vector<string> >* call) {
  vector<string> ret;
  for ( typename ImportMap::const_iterator it = imports_.begin();
        it != imports_.end(); ++it ) {
    ret.push_back(it->first);
  }
  call->Complete(ret);
}

template<class Service, class Import, class ImportDataSpec>
inline void ImportElement<Service, Import, ImportDataSpec>::GetImportDetail(
      rpc::CallContext<ImportDetail>* call,
      const string& name) {
  ImportDetail detail;
  typename ImportMap::const_iterator it = imports_.find(name);
  if ( it != imports_.end() ) {
    detail.media_type_.set(GetSmallTypeFromStreamType(
                               it->second->caps().tag_type_));
    if ( !it->second->save_dir().empty() ) {
      detail.save_dir_.set(
          it->second->save_dir().substr(media_dir_.size() + 1));
    }
  } else {
    detail.media_type_.set("");
  }
  call->Complete(detail);
}


template<class Service, class Import, class ImportDataSpec>
inline void ImportElement<Service, Import, ImportDataSpec>::AddImport(
    MediaOperationErrorData* ret,
    const ImportDataSpec& spec,
    bool save_state) {
  LOG_INFO << name() << " Adding import: "
           << spec.name_.get()
           << " of type: " << spec.media_type_.get();

  Tag::Type tag_type;
  if ( !GetStreamType(spec.media_type_.c_str(), &tag_type) ) {
    LOG_ERROR << " Unknown media type: " << spec.media_type_.get();
    ret->error_.ref() = 1;
    ret->description_.ref() = "Invalid Media: " +
                              spec.media_type_.get();
  } else {
    const string save_dir =
        spec.save_dir_.is_set() &&
        !spec.save_dir_.empty()
        ? string(media_dir_ + "/" + spec.save_dir_.get())
        : string("");
    const bool append_only =
        spec.append_only_.is_set()
        ? spec.append_only_.get() : false;
    const bool disable_preemption =
        spec.disable_preemption_.is_set()
        ? spec.disable_preemption_.get() : false;
    const int32 prefill_buffer_ms =
        spec.prefill_buffer_ms_.is_set()
        ? spec.prefill_buffer_ms_.get() : 3000;
    const int32 advance_media_ms =
        spec.advance_media_ms_.is_set()
        ? spec.advance_media_ms_.get() : 2000;
    const int32 buildup_interval_sec =
        spec.buildup_interval_sec_.is_set()
        ? spec.buildup_interval_sec_.get() : 1200;
    const int32 buildup_delay_sec =
        spec.buildup_delay_sec_.is_set()
        ? spec.buildup_delay_sec_.get() : 600;
    if ( !AddImport(spec.name_.c_str(),
                    tag_type,
                    save_dir.empty() ? NULL : save_dir.c_str(),
                    append_only,
                    disable_preemption,
                    prefill_buffer_ms,
                    advance_media_ms,
                    buildup_interval_sec,
                    buildup_delay_sec) ) {
      LOG_ERROR << "Cannot AddImport " << spec.ToString()
                << "  for RtmpServerElement " << name();
      ret->error_.ref() = 1;
      ret->description_.ref() =
          "Internal error adding import (probably one exists) "
          "with the same name";
    } else {
      ret->error_.ref() = 0;
      if ( save_state && state_keeper_ ) {
        string s;
        rpc::JsonEncoder::EncodeToString(spec, &s);
        state_keeper_->SetValue(
            strutil::StringPrintf("import/%s",
                                  spec.name_.c_str()), s);
      }
    }
  }
}


template<class Service, class Import, class ImportDataSpec>
inline bool ImportElement<Service, Import, ImportDataSpec>::AddImport(
    const char* import_name,
    Tag::Type tag_type,
    const char* save_dir,
    bool append_only,
    bool disable_preemption,
    int32 prefill_buffer_ms,
    int32 advance_media_ms,
    int32 buildup_interval_sec,
    int32 buildup_delay_sec) {
  if ( !strutil::IsValidIdentifier(import_name) ) {
    LOG_ERROR << "AddImport: invalid identifier: [" << import_name << "]";
    return false;
  }
  const string full_name = name_ + "/" + import_name;
  typename ImportMap::const_iterator it = imports_.find(full_name);
  if ( it != imports_.end() ) {
    LOG_ERROR << "AddImport: [" << full_name << "] already exists";
    return false;
  }
  Import* const src = CreateNewImport(import_name,
                                      full_name.c_str(),
                                      tag_type,
                                      save_dir,
                                      append_only,
                                      disable_preemption,
                                      prefill_buffer_ms,
                                      advance_media_ms,
                                      buildup_interval_sec,
                                      buildup_delay_sec);
  imports_.insert(make_pair(full_name, src));
  return true;
}


template<class Service, class Import, class ImportDataSpec>
inline bool ImportElement<Service, Import, ImportDataSpec>::AddImportRemoteUser(
    const char* import_name,
    const string& user,
    const string& passwd) {
  const string full_name = name_ + "/" + import_name;
  typename ImportMap::const_iterator it = imports_.find(full_name);
  if ( it == imports_.end() ) {
    return false;
  }
  it->second->AddRemoteUser(user, passwd);
  return true;
}


template<class Service, class Import, class ImportDataSpec>
inline bool ImportElement<Service, Import, ImportDataSpec>::DeleteImport(
    const char* import_name) {
  const string full_name = name_ + "/" + import_name;
  typename ImportMap::iterator it = imports_.find(full_name);
  if ( it == imports_.end() ) {
    return false;
  }
  it->second->Close();
  imports_.erase(it);
  return true;
}

}
