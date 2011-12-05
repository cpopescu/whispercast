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

DEFINE_string(saver_dir_prefix,
              "saved",
              "We write saved media under this directory - under the "
              "one pointed by -base_media_dir");

DEFINE_string(saver_description_file,
              "description.txt",
              "Where to put the description of our saves");


DEFINE_int64(max_default_save_duration_sec,
             10800,  // 3h
             "When starting a save on command, we stop automatically "
             "after these many seconds (even when no explicit stop command "
             "is given (to prevent `forgotten` save operations)");

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

// Auxiliary mapper stuff

//////////////////////////////////////////////////////////////////////

MediaMapper::MediaMapper(net::Selector* selector,
                         http::Server* http_server,
                         http::Server* rpc_http_server,
                         rpc::HttpServer* rpc_server,
                         rtmp::StreamManager* rtmp_manager,
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
    config_file_(strutil::JoinPaths(config_dir, "whispercast.config")),
    hosts_aliases_config_file_(strutil::JoinPaths(config_dir,
        "hosts_aliases.config")),
    factory_(&media_mapper_,
             selector,
             http_server,
             rpc_server,
             &aio_managers_,
             rtmp_manager,
             &buffer_manager_,
             &host_aliases_,
             base_media_dir.c_str(),
             &state_keeper_,
             &local_state_keeper_),
    media_mapper_(selector, &factory_,
                  new io::StateKeepUser(&state_keeper_, "a/", -1)),
    aux_mapper_(NULL),
    loaded_(false),
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

  rpc_http_server_->RegisterProcessor("/__admin__",
      NewPermanentCallback(this, &MediaMapper::ConfigRootPage), false, true);
  rpc_http_server_->RegisterProcessor("/__bufstats__",
      NewPermanentCallback(this, &MediaMapper::BufferStatusPage), false, true);
  rpc_http_server_->RegisterProcessor("/__checkpoint__",
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
  for ( SaverStopAlarmsMap::const_iterator it_stop = savers_stoppers_.begin();
        it_stop != savers_stoppers_.end(); ++it_stop ) {
    selector_->UnregisterAlarm(it_stop->second);
    delete it_stop->second;
  }
  LOG_INFO << " Deleting saver specs";
  for ( SaverSpecMap::const_iterator it_saver = saver_specs_.begin();
        it_saver != saver_specs_.end(); ++it_saver ) {
    delete it_saver->second;
  }
  for ( AioManagersMap::const_iterator it_managers = aio_managers_.begin();
        it_managers != aio_managers_.end(); ++it_managers ) {
    delete it_managers->second;
  }
  delete aux_mapper_;
  is_deleting_ = false;
}

const char* MediaMapper::LoadErrorName(LoadError err) {
  switch ( err ) {
    CONSIDER(LOAD_OK);
    CONSIDER(LOAD_FILE_ERROR);
    CONSIDER(LOAD_DATA_ERROR);
  }
  LOG_FATAL << "Illegal LoadError value: " << err;
  return "Unknown";
}

//////////////////////////////////////////////////////////////////////

// Various loading parts :)

MediaMapper::LoadError MediaMapper::LoadElements() {
  // Loading and decoding
  string config_str;
  if ( !io::FileInputStream::TryReadFile(config_file_.c_str(),
                                         &config_str) ) {
    LOG_ERROR << " Cannot open config file: " << config_file_;
    return LOAD_FILE_ERROR;
  }
  ElementConfigurationSpecs specs;
  io::MemoryStream iomis;
  iomis.Write(config_str);
  rpc::JsonDecoder decoder(iomis);

  LoadError err = LOAD_OK;
  const rpc::DECODE_RESULT config_error = decoder.Decode(specs);
  if ( config_error != rpc::DECODE_RESULT_SUCCESS ) {
    LOG_ERROR << "Error decoding ElementConfigurationSpecs from file: "
              << config_file_;
    err = LOAD_DATA_ERROR;
  }
  vector<ElementExportSpec> exports;
  const rpc::DECODE_RESULT exports_error = decoder.Decode(exports);
  if ( exports_error != rpc::DECODE_RESULT_SUCCESS ) {
    LOG_ERROR << "Error decoding vector<ElementExportSpec> from file: "
              << config_file_;
    err = LOAD_DATA_ERROR;
  }
  vector<MediaSaverSpec> saves;
  const rpc::DECODE_RESULT saves_error = decoder.Decode(saves);
  if ( saves_error != rpc::DECODE_RESULT_SUCCESS ) {
    LOG_ERROR << "Error decoding vector<MediaSaverSpec> from file: "
              << config_file_;
    err = LOAD_DATA_ERROR;
  }

  // Creation
  vector<streaming::ElementFactory::ErrorData> errors;
  if ( !factory_.AddSpecs(specs, &errors) ) {
    LOG_ERROR << " Errors encountered while adding specs to media factory: ";
    for ( int i = 0; i < errors.size(); ++i ) {
      LOG_ERROR << "ERROR: " << errors[i].description_;
      err = LOAD_DATA_ERROR;
    }
  }
  for ( int i = 0; i < exports.size(); ++i ) {
    string error;
    if ( !ExportElement(exports[i], &error) ) {
      LOG_ERROR << "Error exporting element: " << exports[i] << " : "
                << error;
      err = LOAD_DATA_ERROR;
    }
  }
  for ( int i = 0; i < saves.size(); ++i ) {
    LOG_INFO << " Adding save: " << saves[i];
    saver_specs_.insert(make_pair(saves[i].name_,
                                  new MediaSaverSpec(saves[i])));
  }
  return err;
}

void MediaMapper::InitializeSavers() {
  for ( SaverSpecMap::const_iterator it = saver_specs_.begin();
        it != saver_specs_.end(); ++it ) {
    const string key_prefix(
        strutil::StringPrintf("savers/%s/", it->first.c_str()));
    string str_started_on_command;
    string str_last_start_time;
    if ( local_state_keeper_.GetValue(key_prefix + "started_on_command",
                                      &str_started_on_command) &&
         local_state_keeper_.GetValue(key_prefix + "last_start_time",
                                      &str_last_start_time) ) {
      int64 last_start_time = ::strtoll(str_last_start_time.c_str(), NULL, 10);
      if ( str_started_on_command == "1" ) {
        string error;
        if ( !StartSaverInternal(it->first, "", 0,
                                 last_start_time, &error) ) {
          LOG_ERROR << "Cannot restart a previously started saver: "
                    << it->first << ". Error: " << error;
        }
      } else {
        StartSaverAlarm(it->first, last_start_time);
      }
    } else {
      StartSaverAlarm(it->first, 0);
    }
  }
}

MediaMapper::LoadError MediaMapper::LoadHostAliases() {
  // Loading and decoding the current aliases..
  string aliases_str;
  if ( !io::FileInputStream::TryReadFile(hosts_aliases_config_file_.c_str(),
                                         &aliases_str) ) {
    LOG_ERROR << " Cannot open aliases config file: "
              << hosts_aliases_config_file_;
    return LOAD_FILE_ERROR;
  }
  vector<MediaHostAliasSpec> current_aliases;
  MediaMapper::LoadError err = LOAD_OK;

  if ( !rpc::JsonDecoder::DecodeObject(aliases_str, &current_aliases) ) {
    LOG_ERROR << "Error decoding vector<MediaHostAlias> from file: ["
              << hosts_aliases_config_file_
              << "]\nDecoded so far: " << current_aliases;
    err = LOAD_DATA_ERROR;
  }
  for ( int i = 0; i < current_aliases.size(); ++i ) {
    const string& name = current_aliases[i].alias_name_;
    const string& ip = current_aliases[i].alias_ip_;
    host_aliases_[name] = ip;
    LOG_INFO << "Adding alias: [" << name << "] => [" << ip << "]";
  }
  return err;
}


// This loads the config file and creates the associated structures
MediaMapper::LoadError MediaMapper::Load() {
  if ( !state_keeper_.Initialize() ) {
    LOG_ERROR << " ============= ERROR - cannot initialize the state keeper !";
  }
  if ( !local_state_keeper_.Initialize() ) {
    LOG_ERROR << " ============= ERROR - cannot initialize the state keeper !";
  }

  state_checkpointing_alarm_.Start();
  state_expiration_alarm_.Start();

  MediaMapper::LoadError err_aliases = LoadHostAliases();
  MediaMapper::LoadError err_elements = LoadElements();

  media_mapper_.Initialize();
  InitializeSavers();

  if ( err_elements == LOAD_OK ) {
    return err_aliases;
  }
  return err_elements;
}

//////////////////////////////////////////////////////////////////////

// Saves the current configuration in the config file (safely :)
bool MediaMapper::Save() const {
  const string tmp_file(config_file_ + "__temp__");
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
    io::MemoryStream ms;
    rpc::JsonEncoder encoder(ms);
    vector< ElementExportSpec > ret;
    GetElementExports(&ret);
    encoder.Encode(ret);
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
  if ( !io::Rename(tmp_file, config_file_, true) ) {
    LOG_ERROR << "Cannot move the temp config to the main config file";
    return false;
  }
  return true;
}

void MediaMapper::Close(Closure* close_completed) {
  media_mapper_.Close(close_completed);
}

bool MediaMapper::SaveHostAliases() const {
  const string tmp_file(hosts_aliases_config_file_ + "__temp__");
  io::File f;
  if ( !f.Open(tmp_file,
               io::File::GENERIC_READ_WRITE,
               io::File::CREATE_ALWAYS) ) {
    LOG_ERROR << " Cannot open temporarely config file for writing: ["
              << tmp_file << "]";
    return false;
  }
  {
    vector <MediaHostAliasSpec> aliases;
    GetHostAliases(&aliases);
    string s;
    rpc::JsonEncoder::EncodeToString(aliases, &s);
    if ( f.Write(s) != s.size() ) {
      LOG_ERROR << " Error writing config to: " << tmp_file;
      return false;
    }
  }
  if ( !io::Rename(tmp_file, hosts_aliases_config_file_, true) ) {
    LOG_ERROR << "Cannot move the temp hosts aliases config to "
              << "the main config aliasesfile";
    return false;
  }
  return true;
}

void MediaMapper::CheckpointState() {
  CHECK(state_keeper_.Checkpoint());
  CHECK(local_state_keeper_.Checkpoint());
}
void MediaMapper::ExpireStateKeys() {
  state_keeper_.ExpireTimeoutedKeys();
  local_state_keeper_.ExpireTimeoutedKeys();
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
  rpc::CallContext< MediaOperationErrorData >* call,
  const PolicySpecs& spec) {
  MediaOperationErrorData ret;
  streaming::ElementFactory::ErrorData err_data;
  PolicySpecs* new_spec = new PolicySpecs(spec);
  const bool success = factory_.AddPolicySpec(new_spec, &err_data);
  if ( !success ) {
    delete new_spec;
    ret.error_.ref() = 1;
    ret.description_.ref() = err_data.description_;
  } else {
    ret.error_.ref() = 0;
  }
  call->Complete(ret);
}

void MediaMapper::DeletePolicySpec(
  rpc::CallContext< MediaOperationErrorData >* call,
  const string& name) {
  MediaOperationErrorData ret;
  streaming::ElementFactory::ErrorData err_data;
  const bool success = factory_.DeletePolicySpec(name, &err_data);
  if ( !success ) {
    ret.error_.ref() = 1;
    ret.description_.ref() = err_data.description_;
  } else {
    // We delete just the specs .. we leave the created policies around ..
    ret.error_.ref() = 0;
  }
  call->Complete(ret);
}

void MediaMapper::AddAuthorizerSpec(
  rpc::CallContext< MediaOperationErrorData >* call,
  const AuthorizerSpecs& spec) {
  MediaOperationErrorData ret;
  streaming::ElementFactory::ErrorData err_data;
  AuthorizerSpecs* new_spec = new AuthorizerSpecs(spec);
  const bool success = factory_.AddAuthorizerSpec(new_spec, &err_data);
  if ( !success ) {
    delete new_spec;
    ret.error_.ref() = 1;
    ret.description_.ref() = err_data.description_;
  } else {
    if ( !media_mapper_.AddAuthorizer(spec.name_) ) {
      factory_.DeleteAuthorizerSpec(spec.name_, &err_data);
      call->Complete(MediaOperationErrorData(
                         1, "Failed to create authorizer; "
                         "check whispercast log."));
      return;
    }
    ret.error_.ref() = 0;
  }
  call->Complete(ret);
}

void MediaMapper::DeleteAuthorizerSpec(
  rpc::CallContext< MediaOperationErrorData >* call,
  const string& name) {
  MediaOperationErrorData ret;
  streaming::ElementFactory::ErrorData err_data;
  const bool success = factory_.DeleteAuthorizerSpec(name, &err_data);
  if ( !success ) {
    ret.error_.ref() = 1;
    ret.description_.ref() = err_data.description_;
  } else {
    media_mapper_.RemoveAuthorizer(name);
    ret.error_.ref() = 0;
  }
  call->Complete(ret);
}


void MediaMapper::AddElementSpec(
    rpc::CallContext< MediaOperationErrorData >* call,
    const MediaElementSpecs& spec) {
  streaming::ElementFactory::ErrorData err_data;
  MediaElementSpecs* new_spec = new MediaElementSpecs(spec);
  bool success = factory_.AddElementSpec(new_spec,
                                         &err_data);
  if ( !success ) {
    delete new_spec;
    call->Complete(MediaOperationErrorData(1, err_data.description_));
    return;
  }

  const string& name(spec.name_);
  if ( !media_mapper_.AddElement(name, spec.is_global_) ) {
    factory_.DeleteElementSpec(new_spec->name_, &err_data);
    call->Complete(MediaOperationErrorData(
                       1, "failed to create element; check whispercast log."));
    return;
  }
  call->Complete(MediaOperationErrorData(0, ""));
}

void MediaMapper::DeleteElementSpec(
  rpc::CallContext< MediaOperationErrorData >* call,
  const string& name) {
  MediaOperationErrorData ret;
  streaming::ElementFactory::ErrorData err_data;
  const bool success = factory_.DeleteElementSpec(name, &err_data);
  if ( !success ) {
    ret.error_.ref() = 1;
    ret.description_.ref() = err_data.description_;
  } else {
    media_mapper_.RemoveElement(name);
    ret.error_.ref() = 0;
  }
  call->Complete(ret);
}

void MediaMapper::AddElementSaver(
    rpc::CallContext< MediaOperationErrorData >* call,
    const MediaSaverSpec& spec) {
  const string& saver_name = spec.name_;
  const string& media_name = spec.media_name_;

  if ( saver_specs_.find(saver_name) != saver_specs_.end() ) {
    call->Complete(MediaOperationErrorData(1,
        "Saver already exists:" + saver_name));
    return;
  }
  streaming::Capabilities caps;
  if ( !media_mapper_.HasMedia(media_name.c_str(), &caps) ) {
    call->Complete(MediaOperationErrorData(1, "Unknown Media::" + media_name));
    return;
  }

  CHECK(saver_specs_.insert(make_pair(
      saver_name, new MediaSaverSpec(spec))).second);
  StartSaverAlarm(saver_name, 0);

  call->Complete(MediaOperationErrorData(0, ""));
}

void MediaMapper::DeleteElementSaver(
  rpc::CallContext< MediaOperationErrorData >* call,
  const string& name) {
  const string& saver_name(name);
  MediaOperationErrorData ret;
  SaverSpecMap::iterator it = saver_specs_.find(saver_name);
  if ( saver_specs_.find(saver_name) == saver_specs_.end() ) {
    ret.error_.ref() = 1;
    ret.description_.ref() = "Saver does not exist:" + saver_name;
  } else {
    if ( savers_.find(saver_name) != savers_.end() ) {
      ret.description_.ref() = "Warning: the saver is still active "
        "(deleting the spec though)";
    }
    delete it->second;
    saver_specs_.erase(it);
    ret.error_.ref() = 0;
  }
  call->Complete(ret);
}

//////////////////////////////////////////////////////////////////////

bool MediaMapper::ExportElement(const ElementExportSpec& spec,
                                string* error) {
  if ( spec.protocol_ != "http" &&
       spec.protocol_ != "rtp" &&
       spec.protocol_ != "rtmp" ) {
    *error = "Invalid protocol specified";
    return false;
  }

  LOG_INFO << "=======> Exporting spec: " << spec.ToString();
  streaming::RequestServingInfo* serving_info =
      new streaming::RequestServingInfo();
  serving_info->FromElementExportSpec(spec);
  if ( !media_mapper_.AddServingInfo(spec.protocol_,
                                     spec.path_,
                                     serving_info) ) {
    *error = "Protocol:Path already registered";
    delete serving_info;
    return false;
  }
  return true;
}

void MediaMapper::StartExportElement(
  rpc::CallContext< MediaOperationErrorData >* call,
  const ElementExportSpec& spec) {
  MediaOperationErrorData ret;
  string error;
  if ( !ExportElement(spec, &error) ) {
    ret.error_.ref() = 1;
    ret.description_.ref() = error;
  } else {
    ret.error_.ref() = 0;
  }
  call->Complete(ret);
}

void MediaMapper::StopExportElement(
  rpc::CallContext< MediaOperationErrorData > * call,
  const string& protocol, const string& path) {
  MediaOperationErrorData ret;
  if ( !media_mapper_.RemoveServingInfo(protocol, path) ) {
    ret.error_.ref() = 1;
    ret.description_.ref() = "Proto/path not exported: " + path;
  } else {
    ret.error_.ref() = 0;
  }
  call->Complete(ret);
}

//////////////////////////////////////////////////////////////////////

void MediaMapper::StartSaver(
  rpc::CallContext< MediaOperationErrorData >* call,
  const string& name,
  const string& description) {
  const string& saver_name(name);
  MediaOperationErrorData ret;

  if ( saver_specs_.find(saver_name) == saver_specs_.end() ) {
    ret.error_.ref() = 1;
    ret.description_.ref() = "Saver does not exist:" + saver_name;
  } else if ( savers_.find(saver_name) != savers_.end() ) {
    ret.error_.ref() = 1;
    ret.description_.ref() = "Saver already started";
  } else {
    string error;
    if ( !StartSaverInternal(saver_name, description, 0, 0, &error) ) {
      ret.error_.ref() = 1;
      ret.description_.ref() = "Error starting saver: " + error;
    } else {
      ret.error_.ref() = 0;
    }
  }
  call->Complete(ret);
}

void MediaMapper::StopSaver(
  rpc::CallContext< MediaOperationErrorData >* call,
  const string& name) {
  const string& saver_name(name);
  MediaOperationErrorData ret;
  SaverMap::iterator it_saver = savers_.find(saver_name);
  if ( it_saver == savers_.end() ) {
    ret.error_.ref() = 1;
    ret.description_.ref() = "No active saver: " + saver_name;
  } else {
    ret.error_.ref() = 0;
    // will cause deletion and erasing from map:
    LOG_INFO << " Stopping saver: " << saver_name;
    it_saver->second->StopSaving();
  }
  call->Complete(ret);
}

//////////////////////////////////////////////////////////////////////

void MediaMapper::OnStartSaver(string* name,
                               int duration_in_seconds,
                               int64 last_start_time) {
  string error;
  if ( !StartSaverInternal(*name, "",
                           duration_in_seconds, last_start_time,
                           &error) ) {
    LOG_WARNING << "OnStartSaver: " << error;
  }
  delete name;
}

void MediaMapper::StartSaverAlarm(const string& name,
                                  int64 last_start_time) {
  SaverSpecMap::const_iterator it = saver_specs_.find(name);
  if ( it == saver_specs_.end() ) {
    LOG_WARNING << "Cannot schedule a saver start for: " << name
                << " as we don't have the saver spec..";
    return;
  }
  const timer::Date now(false);
  int64 min_next_happening = kMaxInt64;
  int duration_in_seconds = 0;
  for ( int i = 0; i < it->second->timespecs_.size(); ++i ) {
    const int64 crt = streaming::NextHappening(
      it->second->timespecs_[i], now);
    LOG_INFO << " Crt Next Happening for: " << name << ": " << crt;
    if ( crt > min_next_happening ) {
      continue;
    }
    min_next_happening = crt;
    duration_in_seconds =
      it->second->timespecs_[i].duration_in_seconds_;
    if ( 0 > min_next_happening ) {
      if ( -min_next_happening <= duration_in_seconds * 1000 ) {
        duration_in_seconds += min_next_happening / 1000;
      }
    }
    LOG_INFO << " Min Next Happening for: " << name
             << ": " << min_next_happening
             << " with duration: " << duration_in_seconds;
  }
  if ( duration_in_seconds < 0 ||
       min_next_happening == kMaxInt64 ) {
    LOG_WARNING << "Cannot schedule a saver start for: " << name
                << " as it is manual or has no valid time spec.";
    return;
  }
  const int64 to_wait = max(min_next_happening, static_cast<int64>(0));
  CHECK_GE(to_wait, 0);
  selector_->RegisterAlarm(NewCallback(this, &MediaMapper::OnStartSaver,
                                       new string(name),
                                       duration_in_seconds,
                                       last_start_time),
                           to_wait);
}

bool MediaMapper::StartSaverInternal(const string& name,
                                     const string& description,
                                     int duration_sec,
                                     int64 last_start_time,
                                     string* error) {
  const SaverMap::const_iterator it_saver = savers_.find(name);
  timer::Date now(false);
  if ( it_saver != savers_.end() ) {
    *error = strutil::StringPrintf(
      "Cannot start start saver '%s', as it is already running for %"PRId64" ms",
      name.c_str(),
      (now.GetTime() -
                                 it_saver->second->start_time()));
    return false;
  }
  const SaverSpecMap::const_iterator it_spec = saver_specs_.find(name);
  if ( it_spec == saver_specs_.end() ) {
    *error = ("Cannot start saver '" + name
              + "' as we don't have an associated saver spec.");
    return false;
  }
  const string& media_name = it_spec->second->media_name_;
  string dirname;
  if ( last_start_time == 0 ) {
    dirname = now.ToShortString();
    LOG_INFO << " Started new save at: " << now.ToString();
  } else {
    timer::Date lt(last_start_time, false);
    LOG_INFO << " Using retreived last start time: "
             << lt.ToString();
    dirname = lt.ToShortString();
  }
  const string saver_dir(
    strutil::StringPrintf("%s/%s/%s/%s/",
                          factory_.base_media_dir().c_str(),
                          FLAGS_saver_dir_prefix.c_str(),
                          name.c_str(),
                          dirname.c_str()));
  streaming::Saver* const saver = new streaming::Saver(
      name,
      &media_mapper_,
      streaming::Tag::kAnyType,
      media_name,
      saver_dir,
      last_start_time == 0 ? now.GetTime() : last_start_time,
      duration_sec == 0,
      NewCallback(this, &MediaMapper::OnSaveStopped));
  if ( !saver->StartSaving() ) {
    *error = ("Cannot seem to be able to start saver '"+name +
        "'(most probably a directory error).");
    delete saver;
    return false;
  }
  if ( !description.empty() ) {
    if ( !io::FileOutputStream::TryWriteFile(
           (saver_dir + "/" + FLAGS_saver_description_file).c_str(),
           description) ) {
      LOG_ERROR << "Cannot write saver description file to: " << saver_dir;
    }
  }
  if ( duration_sec > 0 ) {
    Closure* stopper = NewPermanentCallback(saver,
                                            &streaming::Saver::StopSaving);
    selector_->RegisterAlarm(stopper,
                             duration_sec * 1000);
    CHECK(savers_stoppers_.insert(make_pair(name, stopper)).second);
  } else if ( FLAGS_max_default_save_duration_sec > 0 ) {
    Closure* stopper = NewPermanentCallback(saver,
                                            &streaming::Saver::StopSaving);
    selector_->RegisterAlarm(stopper,
                             FLAGS_max_default_save_duration_sec * 1000);
    CHECK(savers_stoppers_.insert(make_pair(name, stopper)).second);
  }
  CHECK(savers_.insert(make_pair(name, saver)).second);
  const string key_prefix(strutil::StringPrintf("savers/%s/", name.c_str()));
  const bool started_on_command = saver->started_on_command();
  const int64 saver_last_start_time = saver->start_time();
  local_state_keeper_.SetValue(key_prefix + "started_on_command",
      started_on_command ? "1" : "0");
  local_state_keeper_.SetValue(key_prefix + "last_start_time",
      strutil::StringPrintf("%"PRId64, saver_last_start_time));
  return true;
}

void MediaMapper::OnSaveStopped(streaming::Saver* saver) {
  const SaverMap::iterator it_saver = savers_.find(saver->name());
  if ( it_saver != savers_.end() ) {
    savers_.erase(it_saver);
  } else {
    LOG_ERROR << "Saver " << saver->name()
              << " stopped - but I cannot find it in "
              << "the active saver list.";
  }
  LOG_INFO << " Saver " << saver->name() << " Stopped..";
  const SaverStopAlarmsMap::iterator
    it_stop = savers_stoppers_.find(saver->name());
  if ( it_stop != savers_stoppers_.end() ) {
    selector_->UnregisterAlarm(it_stop->second);
    // Deleting this way as we may be in it ..
    selector_->DeleteInSelectLoop(it_stop->second);
    savers_stoppers_.erase(it_stop);
  } else {
    LOG_ERROR << "Bad - no save stop callback found for: " << saver->name();
  }
  if ( !is_deleting_ && !selector_->IsExiting() ) {
    saver->CreateSignalingFile(".save_done", "");
    const string key_prefix(strutil::StringPrintf("savers/%s/",
                                                  saver->name().c_str()));
    local_state_keeper_.DeletePrefix(key_prefix);

    // Schedule the next moment of saving
    StartSaverAlarm(saver->name(), 0);
  } else {
    LOG_INFO << " Saver " << saver->name()
             << " stopped on deleting - keeping its state.. ";
  }
  selector_->DeleteInSelectLoop(saver);
}

//////////////////////////////////////////////////////////////////////

void MediaMapper::GetSavesConfig(
  vector< MediaSaverSpec >* saves) const {
  for ( SaverSpecMap::const_iterator it = saver_specs_.begin();
        it != saver_specs_.end(); ++it ) {
    saves->push_back(*it->second);
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
    saves->push_back(MediaSaverState(it->second->name(),
                                    it->second->start_time(),
                                    it->second->started_on_command()));
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
  streaming::ElementDescriptions medias;
  media_mapper_.ListMedia(media_name.c_str(), &medias);
  vector<string> ret;
  for ( int i = 0; i < medias.size(); ++i ) {
    ret.push_back(string(medias[i].first));
  }
  call->Complete(ret);
}

//////////////////////////////////////////////////////////////////////

void MediaMapper::AddHostAlias(rpc::CallContext<MediaOperationErrorData>* call,
                               const string& alias_name,
                               const string& alias_ip) {
  MediaOperationErrorData ret;
  net::IpAddress ip(alias_ip.c_str());
  if ( ip.IsInvalid() ) {
    ret.error_.ref() = 1;
    ret.description_.ref() = "Invalid IPV4 provided.";
  } else {
    Host2IpMap::iterator it = host_aliases_.find(alias_name);
    if ( it != host_aliases_.end() ) {
      ret.description_.ref() = string("Replacing old alias: [") +
                               it->second + "]";
      it->second = alias_ip;
    } else {
      host_aliases_.insert(make_pair(alias_name, alias_ip));
    }
    if ( !SaveHostAliases() ) {
      ret.error_.ref() = 1;
      ret.description_.ref() = "Error savind host aliases file!";
    } else {
      ret.error_.ref() = 0;
    }
  }
  call->Complete(ret);
}

void MediaMapper::DeleteHostAlias(
    rpc::CallContext<MediaOperationErrorData>* call,
    const string& alias_name) {
  MediaOperationErrorData ret;
  Host2IpMap::iterator it = host_aliases_.find(alias_name);
  if ( it != host_aliases_.end() ) {
    host_aliases_.erase(it);
    if ( !SaveHostAliases() ) {
      ret.error_.ref() = 1;
      ret.description_.ref() = "Error savind host aliases file!";
    } else {
      ret.error_.ref() = 0;
    }
  } else {
    ret.error_.ref() = 1;
    ret.description_.ref() = "Cannot find the given alias!";
  }
  call->Complete(ret);
}

void MediaMapper::GetHostAliases(
    vector <MediaHostAliasSpec>* aliases) const {
  for ( Host2IpMap::const_iterator it = host_aliases_.begin();
        it != host_aliases_.end(); ++it ) {
    MediaHostAliasSpec alias;
    alias.alias_name_.ref() = it->first;
    alias.alias_ip_.ref() = it->second;
    aliases->push_back(alias);
  }
}

void MediaMapper::GetHostAliases(
  rpc::CallContext< vector <MediaHostAliasSpec> >* call) {
  vector <MediaHostAliasSpec> aliases;
  GetHostAliases(&aliases);
  call->Complete(aliases);
}

//////////////////////////////////////////////////////////////////////

void MediaMapper::GetAllMediaAliases(
    rpc::CallContext< vector<MediaAliasSpec> >* call) {
  vector< pair<string, string> > vec_aliases;
  media_mapper_.GetAllMediaAliases(&vec_aliases);
  vector <MediaAliasSpec> aliases;
  for ( int i = 0; i < vec_aliases.size(); ++i ) {
    aliases.push_back(MediaAliasSpec(vec_aliases[i].first,
                                    vec_aliases[i].second));
  }
  call->Complete(aliases);
}
void MediaMapper::SetMediaAlias(rpc::CallContext<MediaOperationErrorData>* call,
                                const string& alias_name,
                                const string& media_name) {
  MediaOperationErrorData ret;
  string error;
  if ( media_mapper_.SetMediaAlias(alias_name,
                                   media_name,
                                   &error) ) {
    ret.error_.ref() = 0;
  } else {
    ret.error_.ref() = 1;
    ret.description_.ref() = error;
  }
  call->Complete(ret);
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
