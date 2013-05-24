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

// TODO(cpopescu): config re-loading

#include <dlfcn.h>

#include <whisperlib/common/base/gflags.h>

#include "media_mapper.h"
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_encoder.h>
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_decoder.h>
#include <whisperstreamlib/elements/util/media_date.h>
#include <whisperlib/common/io/file/file.h>
#include <whisperlib/common/io/file/file_output_stream.h>
#include <whisperlib/common/io/file/file_input_stream.h>
#include <whisperlib/common/io/ioutil.h>
#include <whisperlib/common/io/buffer/io_memory_stream.h>
#include <whisperlib/common/base/ref_counted.h>

//////////////////////////////////////////////////////////////////////

DEFINE_int32(media_state_checkpoint_interval_sec,
             1200,
             "We checkpoint the state every so often..");
DEFINE_int32(media_state_expiration_interval_sec,
             30,
             "We expire old state keys every so often..");

//////////////////////////////////////////////////////////////////////

DEFINE_int32(media_aio_block_size,
             4096,
             "Alignment of aio memory blocks - should me multiple "
             "of disk block size");
DEFINE_int32(media_aio_buffer_size,
             64,
             "We read media from disk in chunks of this size -- "
             "IN BLOCKS !!");
DEFINE_int32(media_aio_buffer_pool_size,
             1000,
             "Basically this is the number of blocks that we can "
             "have allocated at one time");
DEFINE_string(disk_devices,
              "",
              "Comma separated list of disk devices "
              "(we create one aio manager)"
              "per disk instance :)");

//////////////////////////////////////////////////////////////////////

DECLARE_string(element_libraries_dir);

DEFINE_string(aux_elements_config_file,
              "",
              "Where auxiliary element mapper has config - if any");

//////////////////////////////////////////////////////////////////////

const string MediaMapper::kConfigFile = "whispercast.config";
const string MediaMapper::kHostAliasesFile = "hosts_aliases.config";

MediaMapper::MediaMapper(net::Selector* selector,
                         http::Server* http_server,
                         http::Server* rpc_http_server,
                         rpc::HttpServer* rpc_server,
                         net::UserAuthenticator* admin_authenticator,
                         const string& config_dir,
                         const string& base_media_dir,
                         const string& media_state_directory,
                         const string& media_state_name,
                         const string& local_media_state_name)
  : ServiceInvokerMediaElementService(
      ServiceInvokerMediaElementService::GetClassName()),
    selector_(selector),
    http_server_(http_server),
    rpc_http_server_(rpc_http_server),
    rpc_server_(rpc_server),
    admin_authenticator_(admin_authenticator),
    freelist_(FLAGS_media_aio_buffer_size,
              FLAGS_media_aio_block_size,
              FLAGS_media_aio_buffer_pool_size,
              // allocate all buffers initially - per fragmentation issue:
              true),
    buffer_manager_(&freelist_),
    state_keeper_(media_state_directory, media_state_name),
    local_state_keeper_(media_state_directory, local_media_state_name),
    config_dir_(config_dir),
    factory_(&media_mapper_,
             selector,
             http_server,
             rpc_server,
             &aio_managers_,
             &buffer_manager_,
             base_media_dir.c_str(),
             &state_keeper_,
             &local_state_keeper_),
    media_mapper_(selector, &factory_,
                  new io::StateKeepUser(&state_keeper_, "a/", -1)),
    aux_mapper_(NULL),
    state_checkpointing_alarm_(*selector),
    state_expiration_alarm_(*selector),
    admin_password_(),
    is_deleting_(false) {
  aio_managers_[""] = new io::AioManager("GLOBAL", selector);
  if ( !FLAGS_disk_devices.empty() ) {
    vector<string> devices;
    strutil::SplitString(FLAGS_disk_devices, ",", &devices);
      for ( int i = 0; i < devices.size(); ++i ) {
        const string name(strutil::NormalizePath(devices[i]));
        aio_managers_[name] = new io::AioManager(name.c_str(), selector);
      }
  }
  state_checkpointing_alarm_.Set(
      NewPermanentCallback(this, &MediaMapper::CheckpointState), true,
      FLAGS_media_state_checkpoint_interval_sec * 1000, true, false);
  state_expiration_alarm_.Set(
      NewPermanentCallback(this, &MediaMapper::ExpireStateKeys), true,
      FLAGS_media_state_expiration_interval_sec * 1000, true, false);
  CHECK(factory_.InitializeLibraries(FLAGS_element_libraries_dir,
                                     &rpc_library_paths_));

  rpc_http_server_->RegisterProcessor("__admin__",
      NewPermanentCallback(this, &MediaMapper::ConfigRootPage), false, true);
  rpc_http_server_->RegisterProcessor("__bufstats__",
      NewPermanentCallback(this, &MediaMapper::BufferStatusPage), false, true);
  rpc_http_server_->RegisterProcessor("__checkpoint__",
      NewPermanentCallback(this, &MediaMapper::CheckpointPage), false, true);

  if ( !FLAGS_element_libraries_dir.empty() &&
       !FLAGS_aux_elements_config_file.empty() ) {
    void* extra_lib_handle = dlopen((FLAGS_element_libraries_dir + "/" +
                                     "libextra_streaming_elements.so").c_str(),
                                    RTLD_LAZY);
    if ( extra_lib_handle != NULL ) {
      void* (*aux_creator)(net::Selector*,
                           streaming::ElementFactory*,
                           streaming::ElementMapper*,
                           rpc::HttpServer*,
                           const char*);
      aux_creator = reinterpret_cast<void* (*)(
          net::Selector*,
          streaming::ElementFactory*,
          streaming::ElementMapper*,
          rpc::HttpServer*,
          const char*)>(dlsym(extra_lib_handle,
                              "CreateAuxElementMapper"));

      const char* const error = dlerror();
      if ( error != NULL || aux_creator == NULL ) {
        LOG_ERROR << " Error linking GetElementLibrary function: "
                  << error;
      } else {
        aux_mapper_ =
            reinterpret_cast<streaming::ElementMapper*>
            ((*aux_creator)(
                selector_,
                &factory_,
                &media_mapper_,
                rpc_server,
                FLAGS_aux_elements_config_file.c_str()));
          media_mapper_.set_fallback_mapper(aux_mapper_);
      }
    }
  }
}

MediaMapper::~MediaMapper() {
  rpc_http_server_->UnregisterProcessor("/__admin__");
  rpc_http_server_->UnregisterProcessor("/__bufstats__");
  rpc_http_server_->UnregisterProcessor("/__checkpoint__");

  is_deleting_ = true;
  LOG_INFO << "Deleting media mapper........";

  LOG_INFO << "Deleting savers";
  for ( SaverMap::const_iterator it_saver = savers_.begin();
        it_saver != savers_.end(); ++it_saver ) {
    delete it_saver->second;
  }
  savers_.clear();

  for ( AioManagersMap::const_iterator it_managers = aio_managers_.begin();
        it_managers != aio_managers_.end(); ++it_managers ) {
    delete it_managers->second;
  }
  delete aux_mapper_;
  is_deleting_ = false;
}

//////////////////////////////////////////////////////////////////////

// Various loading parts :)

bool MediaMapper::LoadElements() {
  // Loading and decoding
  string config_file = strutil::JoinPaths(config_dir_, kConfigFile);
  string config_str;
  if ( !io::FileInputStream::TryReadFile(config_file, &config_str) ) {
    LOG_ERROR << " Cannot open config file: " << config_file;
    return false;
  }
  ElementConfigurationSpecs specs;
  io::MemoryStream iomis;
  iomis.Write(config_str);
  rpc::JsonDecoder decoder;

  bool success = true;
  const rpc::DECODE_RESULT config_error = decoder.Decode(iomis, &specs);
  if ( config_error != rpc::DECODE_RESULT_SUCCESS ) {
    LOG_ERROR << "Error decoding ElementConfigurationSpecs from file: "
              << config_file;
    success = false;
  }
  vector<ElementExportSpec> exports;
  const rpc::DECODE_RESULT exports_error = decoder.Decode(iomis, &exports);
  if ( exports_error != rpc::DECODE_RESULT_SUCCESS ) {
    LOG_ERROR << "Error decoding vector<ElementExportSpec> from file: "
              << config_file;
    success = false;
  }
  vector<MediaSaverSpec> saves;
  const rpc::DECODE_RESULT saves_error = decoder.Decode(iomis, &saves);
  if ( saves_error != rpc::DECODE_RESULT_SUCCESS ) {
    LOG_ERROR << "Error decoding vector<MediaSaverSpec> from file: "
              << config_file;
    success = false;
  }

  // Creation
  vector<string> errors;
  if ( !factory_.AddSpecs(specs, &errors) ) {
    LOG_ERROR << " Errors encountered while adding specs to media factory: \n"
              << strutil::JoinStrings(errors, "\n");
    success = false;
  }
  for ( int i = 0; i < exports.size(); ++i ) {
    string error;
    if ( !AddExport(exports[i].media_name_, exports[i].protocol_,
                    exports[i].path_, exports[i].authorizer_name_, &error) ) {
      LOG_ERROR << "Error exporting element: " << exports[i] << " : "
                << error;
      success = false;
    }
  }
  // load savers (don't start them, as the media_mapper_ is not yet initialized)
  for ( int i = 0; i < saves.size(); ++i ) {
    const MediaSaverSpec& conf = saves[i];
    LOG_INFO << "Adding saver: " << conf;
    streaming::MediaFormat save_format = streaming::MFORMAT_FLV;
    if ( !streaming::MediaFormatFromSmallType(conf.save_format_.get(),
                                              &save_format) ) {
      LOG_ERROR << "Invalid save format in saver config: " << conf;
      continue;
    }
    MSaver* s = new MSaver(selector_, conf, save_format,
        &media_mapper_, NewCallback(this, &MediaMapper::OnSaverStopped));
    savers_[saves[i].name_] = s;
  }
  return success;
}

void MediaMapper::InitializeSavers() {
  const int64 now_sec = timer::Date::Now() / 1000;
  for ( SaverMap::iterator it = savers_.begin(); it != savers_.end(); ++it ) {
    MSaver* s = it->second;
    string str_end_ts;
    if ( local_state_keeper_.GetValue(SaverStateKey(s->conf_.name_.get()),
                                      &str_end_ts) ) {
      int64 end_ts = ::strtoll(str_end_ts.c_str(), NULL, 10);
      if ( end_ts > now_sec ) {
        s->saver_.StartSaving(end_ts - now_sec);
      }
    }
  }
}

// This loads the config file and creates the associated structures
bool MediaMapper::Load() {
  if ( !state_keeper_.Initialize() ) {
    LOG_ERROR << " ============= ERROR - cannot initialize the state keeper !";
  }
  if ( !local_state_keeper_.Initialize() ) {
    LOG_ERROR << " ============= ERROR - cannot initialize the state keeper !";
  }

  state_checkpointing_alarm_.Start();
  state_expiration_alarm_.Start();

  bool success_elements = LoadElements();

  media_mapper_.Initialize();
  InitializeSavers();

  return success_elements;
}

//////////////////////////////////////////////////////////////////////

// Saves the current configuration in the config file (safely :)
bool MediaMapper::Save() const {
  const string config_file = strutil::JoinPaths(config_dir_, kConfigFile);
  const string tmp_file(config_file + "__temp__");
  io::File f;
  if ( !f.Open(tmp_file,
               io::File::GENERIC_READ_WRITE,
               io::File::CREATE_ALWAYS) ) {
    LOG_ERROR << "Cannot open temporarily config file for writing: ["
              << tmp_file << "]";
    return false;
  }
  {
    ElementConfigurationSpecs* specs = factory_.GetSpec();
    string s;
    rpc::JsonEncoder::EncodeToString(*specs, &s);
    delete specs;
    if ( f.Write(s) != s.size() ) {
      LOG_ERROR << " Error writing config to: " << tmp_file;
      return false;
    }
  }
  {
    vector< ElementExportSpec > ret;
    GetElementExports(&ret);
    io::MemoryStream ms;
    rpc::JsonEncoder().Encode(ret, &ms);
    if ( ms.Size() != f.Write(ms) ) {
      LOG_ERROR << " Error writing config to: " << tmp_file;
      return false;
    }
  }
  {
    vector<MediaSaverSpec> ret;
    GetSavesConfig(&ret);
    string s;
    rpc::JsonEncoder::EncodeToString(ret, &s);
    if ( f.Write(s) != s.size() ) {
      LOG_ERROR << " Error writing config to: " << tmp_file;
      return false;
    }
  }
  if ( !io::Rename(tmp_file, config_file, true) ) {
    LOG_ERROR << "Cannot move the temp config to the main config file";
    return false;
  }
  return true;
}

void MediaMapper::Close(Closure* close_completed) {
  media_mapper_.Close(close_completed);
}

void MediaMapper::CheckpointState() {
  CHECK(state_keeper_.Checkpoint());
  CHECK(local_state_keeper_.Checkpoint());
}
void MediaMapper::ExpireStateKeys() {
  state_keeper_.ExpireTimeoutedKeys();
  local_state_keeper_.ExpireTimeoutedKeys();
}
streaming::Element* MediaMapper::FindElement(const string& element_name) {
  streaming::Element* element = NULL;
  vector<streaming::Policy*>* policies = NULL;
  if ( !media_mapper_.GetElementByName(element_name, &element, &policies) ) {
    return NULL;
  }
  return element;
}
bool MediaMapper::HasElement(const string& element_name) {
  return FindElement(element_name) != NULL;
}
void MediaMapper::FindElementMatches(const string& prefix, const string& suffix,
                                     vector<string>* middle_out) {
  vector<string> elems;
  media_mapper_.GetAllElements(&elems);
  for ( uint32 i = 0; i < elems.size(); i++ ) {
    const string& e = elems[i];
    if ( strutil::StrStartsWith(e, prefix) &&
         strutil::StrEndsWith(e, suffix) &&
         e.size() > prefix.size() + suffix.size() ) {
      middle_out->push_back(e.substr(prefix.size(),
          e.size() - prefix.size() - suffix.size()));
    }
  }
}
void MediaMapper::BufferStatusPage(http::ServerRequest* req) {
  if ( !req->AuthenticateRequest(admin_authenticator_) ) {
    // TODO: we may want to authenticate asynchronously
    req->AnswerUnauthorizedRequest(admin_authenticator_);
    return;
  }
  req->request()->server_data()->Write("<html><body><h2>Buffer Status:</h2>");
  req->request()->server_data()->Write(buffer_manager_.GetHtmlStats());
  req->request()->server_data()->Write("</body></html>");
  req->Reply();
}

void MediaMapper::CheckpointPage(http::ServerRequest* req) {
  if ( !req->AuthenticateRequest(admin_authenticator_) ) {
    // TODO: we may want to authenticate asynchronously
    req->AnswerUnauthorizedRequest(admin_authenticator_);
    return;
  }
  CheckpointState();
  req->request()->server_data()->Write("<html><body>"
      "<h2>Checkpoint Done</h2>"
      "</body></html>");
  req->Reply();
}

void MediaMapper::ConfigRootPage(http::ServerRequest* req) {
  if ( !req->AuthenticateRequest(admin_authenticator_) ) {
    // TODO: we may want to authenticate asynchronously
    req->AnswerUnauthorizedRequest(admin_authenticator_);
    return;
  }
  req->request()->server_data()->Write(
      "<html><body><h2>Select your command path:</h2>");
  req->request()->server_data()->Write(
      strutil::StringPrintf(
          "<a href=\"%s/MediaElementService/__forms\">Global Config</a><br/>",
          rpc_server_->path().c_str()));
  req->request()->server_data()->Write(
      strutil::StringPrintf(
          "<a href=\"%s/HappHub/__forms\">HappHub</a><br/>",
          rpc_server_->path().c_str()));
  for ( int i = 0; i < rpc_library_paths_.size(); ++i ) {
    req->request()->server_data()->Write(
        strutil::StringPrintf(
            "<a href=\"%s/__forms\">"
            "Library [%s] Config</a><br/>",
            rpc_library_paths_[i].second.c_str(),
            rpc_library_paths_[i].first.c_str()));
  }
  req->request()->server_data()->Write(
      "<a href=\"/__bufstats__\">Buffer Status</a><br/>");
  req->request()->server_data()->Write(
      "<a href=\"/__checkpoint__\">Checkpoint state</a><br/>");
  req->request()->server_data()->Write("</body></html>");
  req->Reply();
}

  //////////////////////////////////////////////////////////////////////


void MediaMapper::AddPolicySpec(
    rpc::CallContext< MediaOpResult >* call,
    const PolicySpecs& spec) {
  string err;
  call->Complete(MediaOpResult(AddPolicy(
      spec.type_, spec.name_, spec.params_, &err), err));
}

void MediaMapper::DeletePolicySpec(
    rpc::CallContext< rpc::Void >* call,
    const string& name) {
  DeletePolicy(name);
  call->Complete();
}

void MediaMapper::AddAuthorizerSpec(
    rpc::CallContext< MediaOpResult >* call,
    const AuthorizerSpecs& spec) {
  string err;
  bool success = AddAuthorizer(spec.type_, spec.name_, spec.params_, &err);
  call->Complete(MediaOpResult(success, err));
}

void MediaMapper::DeleteAuthorizerSpec(
    rpc::CallContext< rpc::Void >* call,
    const string& name) {
  DeleteAuthorizer(name);
  call->Complete();
}


void MediaMapper::AddElementSpec(
    rpc::CallContext< MediaOpResult >* call,
    const MediaElementSpecs& spec) {
  string err;
  bool success = AddElement(spec.type_, spec.name_, spec.is_global_,
      spec.params_, &err);
  call->Complete(MediaOpResult(success, err));
}

void MediaMapper::DeleteElementSpec(
    rpc::CallContext< rpc::Void >* call,
    const string& name) {
  DeleteElement(name);
  call->Complete();
}

void MediaMapper::AddMediaSaver(
    rpc::CallContext< MediaOpResult >* call,
    const MediaSaverSpec& spec,
    bool start_now) {
  string err;
  if ( !AddMediaSaver(spec.name_.get(), spec.media_name_.get(),
                      spec.schedule_.get(), spec.save_dir_.get(),
                      spec.save_format_.get(), &err) ) {
    call->Complete(MediaOpResult(false, err));
    return;
  }
  if ( start_now ) {
    if ( !StartMediaSaver(spec.name_.get(), 0, &err) ) {
      call->Complete(MediaOpResult(false, err));
      return;
    }
  }
  call->Complete(MediaOpResult(true, ""));
}

void MediaMapper::DeleteMediaSaver(
    rpc::CallContext< rpc::Void >* call,
    const string& name) {
  DeleteMediaSaver(name);
  call->Complete();
}

void MediaMapper::StartMediaSaver(
    rpc::CallContext< MediaOpResult >* call,
    const string& name,
    int32 duration_sec) {
  string err;
  bool success = StartMediaSaver(name, duration_sec, &err);
  call->Complete(MediaOpResult(success, err));
}

void MediaMapper::StopMediaSaver(
    rpc::CallContext< rpc::Void >* call,
    const string& name) {
  StopMediaSaver(name);
  call->Complete();
}


//////////////////////////////////////////////////////////////////////

void MediaMapper::StartExportElement(
    rpc::CallContext< MediaOpResult >* call,
    const ElementExportSpec& spec) {
  string error;
  call->Complete(MediaOpResult(AddExport(spec.media_name_, spec.protocol_,
      spec.path_, spec.authorizer_name_, &error), error));
}

void MediaMapper::StopExportElement(
    rpc::CallContext< rpc::Void > * call,
    const string& protocol, const string& path) {
  DeleteExport(protocol, path);
  call->Complete();
}

//////////////////////////////////////////////////////////////////////

void MediaMapper::ScheduleSaver(MSaver* s) {
  const timer::Date now(false);
  int64 min_next_happening = kMaxInt64;
  int duration_in_seconds = 0;
  for ( int i = 0; i < s->conf_.schedule_.size(); ++i ) {
    const int64 crt = streaming::NextHappening(s->conf_.schedule_[i], now);
    if ( crt > min_next_happening ) {
      continue;
    }
    min_next_happening = crt;
    duration_in_seconds = s->conf_.schedule_[i].duration_in_seconds_;
    if ( min_next_happening < 0 ) {
      // event in the near past
      if ( -min_next_happening <= duration_in_seconds * 1000 ) {
        duration_in_seconds += min_next_happening / 1000; // reduce duration
      }
    }
  }
  if ( duration_in_seconds < 0 ||
       min_next_happening == kMaxInt64 ) {
    LOG_ERROR << "Cannot schedule a saver start for: " << s->conf_.name_.get()
              << " as it is manual or has no valid time spec.";
    return;
  }
  LOG_INFO << "Min Next Happening for: " << s->conf_.name_.get() << ": "
           << timer::Date(now.GetTime() + min_next_happening).ToString()
           << " with duration: " << duration_in_seconds;
  const int64 to_wait = max(min_next_happening, (int64)0);
  CHECK_GE(to_wait, 0);

  s->start_alarm_.Set(NewCallback(this, &MediaMapper::OnStartSaver,
                        s->conf_.name_.get(), (uint32)duration_in_seconds),
      true, to_wait, false, true);
}

void MediaMapper::OnStartSaver(string name,
                               uint32 duration_in_seconds) {
  string error;
  if ( !StartMediaSaver(name, duration_in_seconds, &error) ) {
    LOG_ERROR << "OnStartSaver: " << error;
  }
}

void MediaMapper::OnSaverStopped(streaming::Saver* saver) {
  SaverMap::iterator it = savers_.find(saver->name());
  CHECK(it != savers_.end()) << "missing saver: [" << saver->name() << "]";
  MSaver* s = it->second;
  s->start_ts_ = 0;
  local_state_keeper_.DeleteValue(SaverStateKey(saver->name()));
  // Schedule the next moment of saving
  ScheduleSaver(s);
}

//////////////////////////////////////////////////////////////////////

void MediaMapper::GetSavesConfig(vector< MediaSaverSpec >* saves) const {
  for ( SaverMap::const_iterator it = savers_.begin();
        it != savers_.end(); ++it ) {
    saves->push_back(it->second->conf_);
  }
}

void MediaMapper::GetSavesConfig(
    rpc::CallContext< vector< MediaSaverSpec > >* call) {
  vector<MediaSaverSpec> ret;
  GetSavesConfig(&ret);
  call->Complete(ret);
}

///////

void MediaMapper::GetCurrentSaves(
  vector< MediaSaverState >* saves) const {
  for ( SaverMap::const_iterator it = savers_.begin();
        it != savers_.end(); ++it ) {
    saves->push_back(MediaSaverState(it->first,
        timer::Date(it->second->start_ts_).ToString()));
  }
}

void MediaMapper::GetCurrentSaves(
  rpc::CallContext< vector< MediaSaverState > >* call) {
  vector<MediaSaverState> ret;
  GetCurrentSaves(&ret);
  call->Complete(ret);
}

///////

void MediaMapper::GetHttpExportRoot(
  rpc::CallContext< string >* call) {
  call->Complete(factory_.base_media_dir());
}

void MediaMapper::GetAllElementConfig(
  rpc::CallContext< ElementConfigurationSpecs >* call) {
  ElementConfigurationSpecs* specs = factory_.GetSpec();
  call->Complete(*specs);
  delete specs;
}

void MediaMapper::GetElementConfig(
    rpc::CallContext<ElementConfigurationSpecs>* call,
    const string& element_name) {
  ElementConfigurationSpecs ret;
  streaming::Element* element;
  vector<streaming::Policy*>* policies;
  if ( media_mapper_.GetElementByName(element_name, &element, &policies) ) {
    MediaElementSpecs spec;
    spec.type_.set(element->type());
    spec.name_.set(element->name());
    const MediaElementSpecs* int_spec =
        factory_.GetElementSpec(element->name());
    CHECK(int_spec != NULL);
    spec.is_global_.set(int_spec->is_global_);
    if ( int_spec->disable_rpc_.is_set() ) {
      spec.disable_rpc_.set(int_spec->disable_rpc_);
    }
    spec.params_.set(element->GetElementConfig());
    ret.elements_.ref().push_back(spec);
    if ( policies ) {
      for ( int i = 0; i < policies->size(); ++i ) {
        PolicySpecs pspec;
        streaming::Policy* policy = (*policies)[i];
        pspec.type_.set(policy->type());
        pspec.name_.set(policy->name());
        pspec.params_.set(policy->GetPolicyConfig());
        ret.policies_.ref().push_back(pspec);
      }
    }
  }
  call->Complete(ret);
}


///////

void MediaMapper::GetElementExports(
  vector< ElementExportSpec >* exports) const {
  const streaming::FactoryBasedElementMapper::ServingInfoMap&
      serving_paths = media_mapper_.serving_paths();
  for ( streaming::FactoryBasedElementMapper::ServingInfoMap::const_iterator
            it = serving_paths.begin(); it  != serving_paths.end(); ++it ) {
    ElementExportSpec src_spec;
    it->second->ToElementExportSpec(it->first, &src_spec);
    exports->push_back(src_spec);
  }
}

void MediaMapper::GetElementExports(
  rpc::CallContext< vector< ElementExportSpec > >* call) {
  vector< ElementExportSpec > ret;
  GetElementExports(&ret);
  call->Complete(ret);
}

///////

void MediaMapper::SaveConfig(
  rpc::CallContext< bool >* call) {
  call->Complete(Save());
}

void MediaMapper::ListMedia(rpc::CallContext< vector<string> >* call,
                            const string& media_name) {
  vector<string> medias;
  ListMedia(media_name, &medias);
  call->Complete(medias);
}

//////////////////////////////////////////////////////////////////////

void MediaMapper::GetAllMediaAliases(
    rpc::CallContext< vector<MediaAliasSpec> >* call) {
  map<string, string> aliases;
  media_mapper_.GetAllMediaAliases(&aliases);
  vector <MediaAliasSpec> specs;
  for ( map<string, string>::const_iterator it = aliases.begin();
        it != aliases.end(); ++it ) {
    specs.push_back(MediaAliasSpec(it->first, it->second));
  }
  call->Complete(specs);
}
void MediaMapper::SetMediaAlias(rpc::CallContext<MediaOpResult>* call,
                                const string& alias_name,
                                const string& media_name) {
  string error;
  call->Complete(MediaOpResult(media_mapper_.SetMediaAlias(
      alias_name, media_name, &error), error));
}
void MediaMapper::GetMediaAlias(rpc::CallContext<string>* call,
                                const string& alias_name) {
  string ret;
  if ( media_mapper_.GetMediaAlias(alias_name, &ret) ) {
    call->Complete(ret);
  } else {
    call->Complete(string());
  }
}

//////////////////////////////////////////////////////////////////////////////
bool MediaMapper::AddElement(const string& class_name,
                             const string& name,
                             bool is_global,
                             const string& params,
                             string* out_error) {
  if ( !factory_.AddElementSpec(MediaElementSpecs(class_name,
      name, is_global, false, params), out_error) ) {
    return false;
  }
  if ( !media_mapper_.AddElement(name, is_global) ) {
    factory_.DeleteElementSpec(name);
    *out_error = "Failed to AddElement: [" + name + "]";
    return false;
  }
  return true;
}
void MediaMapper::DeleteElement(const string& name) {
  factory_.DeleteElementSpec(name);
  media_mapper_.RemoveElement(name);
}

bool MediaMapper::AddPolicy(const string& class_name,
                            const string& name,
                            const string& params,
                            string* out_error) {
  return factory_.AddPolicySpec(PolicySpecs(class_name, name, params), out_error);
}
void MediaMapper::DeletePolicy(const string& name) {
  factory_.DeletePolicySpec(name);
}

bool MediaMapper::AddAuthorizer(const string& class_name,
                                const string& name,
                                const string& params,
                                string* out_error) {
  if ( !factory_.AddAuthorizerSpec(AuthorizerSpecs(class_name, name, params),
      out_error) ) {
    return false;
  }
  if ( !media_mapper_.AddAuthorizer(name) ) {
    factory_.DeleteAuthorizerSpec(name);
    *out_error = "Failed to AddAuthorizer: [" + name + "]";
    return false;
  }
  return true;
}
void MediaMapper::DeleteAuthorizer(const string& name) {
  factory_.DeleteAuthorizerSpec(name);
  media_mapper_.RemoveAuthorizer(name);
}
bool MediaMapper::AddExport(const string& media_name,
                            const string& export_protocol,
                            const string& export_path,
                            const string& authorizer_name,
                            string* out_error) {
  if ( export_protocol != "http" &&
       export_protocol != "rtmp" &&
       export_protocol != "rtp" ) {
    *out_error = "Invalid protocol: " + export_protocol;
    return false;
  }
  streaming::RequestServingInfo* serving_info =
      new streaming::RequestServingInfo();
  serving_info->media_name_ = media_name;
  serving_info->export_path_ = export_path;
  serving_info->authorizer_name_ = authorizer_name;

  if ( !media_mapper_.AddServingInfo(export_protocol,
                                     export_path,
                                     serving_info) ) {
    *out_error = "Already registered: " + export_protocol + ":" + export_path;
    delete serving_info;
    return false;
  }
  return true;
}
void MediaMapper::DeleteExport(const string& protocol, const string& path) {
  media_mapper_.RemoveServingInfo(protocol, path);
}
bool MediaMapper::AddMediaSaver(const string& saver_name,
                                const string& media_name,
                                const vector<TimeSpec>&  timespecs,
                                const string& save_dir,
                                const string& save_format,
                                string* out_error) {
  if ( savers_.find(saver_name) != savers_.end() ) {
    *out_error = "Saver already exists: " + saver_name;
    return false;
  }
  if ( !media_mapper_.HasMedia(media_name) ) {
    *out_error = "Unknown Media: " + media_name;
    return false;
  }
  streaming::MediaFormat mformat = streaming::MFORMAT_FLV;
  if ( !streaming::MediaFormatFromSmallType(save_format, &mformat) ) {
    *out_error = "Invalid save format: " + save_format;
    return false;
  }

  MSaver* s = new MSaver(selector_,
      MediaSaverSpec(saver_name, media_name, timespecs, save_dir, save_format),
      mformat, &media_mapper_,
      NewCallback(this, &MediaMapper::OnSaverStopped));
  savers_[saver_name] = s;
  ScheduleSaver(s);
  return true;
}
void MediaMapper::DeleteMediaSaver(const string& name) {
  SaverMap::iterator it = savers_.find(name);
  if ( it == savers_.end() ) {
    return;
  }
  delete it->second;
  savers_.erase(it);
}

bool MediaMapper::StartMediaSaver(const string& name,
                                  uint32 duration_sec,
                                  string* out_error) {
  const SaverMap::iterator it = savers_.find(name);
  if ( it == savers_.end() ) {
    *out_error = "Saver does not exist: [" + name + "]";
    return false;
  }
  if ( !it->second->saver_.StartSaving(duration_sec) ) {
    *out_error = "Failed to StartSaving(" + name + "), the saved media"
                 " probably does not exist";
    return false;
  }
  it->second->start_ts_ = timer::Date::Now();

  int64 end_time = duration_sec == 0 ? 0 :
                   (timer::Date::Now()/1000 + duration_sec);
  local_state_keeper_.SetValue(SaverStateKey(name),
                               strutil::StringPrintf("%"PRId64"", end_time));

  return true;
}
void MediaMapper::StopMediaSaver(const string& name) {
  SaverMap::iterator it_saver = savers_.find(name);
  if ( it_saver == savers_.end() ) {
    return;
  }
  // will cause deletion and erasing from map:
  LOG_INFO << "Stopping saver: " << name;
  it_saver->second->saver_.StopSaving();
}

void MediaMapper::ListMedia(const string& media, vector<string>* out) {
  media_mapper_.ListMedia(media, out);
}
