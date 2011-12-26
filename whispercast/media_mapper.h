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

#ifndef __APPS_STREAMING_BROADCASTING_SERVER_MEDIA_MAPPER_H__
#define __APPS_STREAMING_BROADCASTING_SERVER_MEDIA_MAPPER_H__

#include <map>
#include <whisperstreamlib/elements/factory.h>
#include <whisperstreamlib/elements/factory_based_mapper.h>
#include <whisperstreamlib/base/saver.h>
#include <whisperlib/net/base/selector.h>
#include <whisperstreamlib/elements/auto/factory_types.h>
#include <whisperstreamlib/elements/auto/factory_invokers.h>

class MediaMapper :
  public ServiceInvokerMediaElementService {
public:
  MediaMapper(net::Selector* selector,
              http::Server* http_server,
              http::Server* rpc_http_server,
              rpc::HttpServer* rpc_server,
              net::UserAuthenticator* admin_authenticator,
              const string& config_dir,
              const string& base_media_dir,
              const string& media_state_directory,
              const string& media_state_name,
              const string& local_media_state_name);

  virtual ~MediaMapper();

  // This loads the config file and creates the associated structures
  enum LoadError {
    LOAD_OK,
    LOAD_FILE_ERROR,
    LOAD_DATA_ERROR,
  };
  static const char* LoadErrorName(LoadError err);

  LoadError Load();
  // Saves the current configuration in the config file (safely :)
  bool Save() const;

  // Asynchronous close.
  void Close(Closure* close_completed);

  //////////////////////////////////////////////////////////////////////

  void CheckpointState();
  void ExpireStateKeys();

  streaming::ElementMapper* mapper() { return &media_mapper_; }

 private:
  // Returns the admin root page
  void ConfigRootPage(http::ServerRequest* req);
  // Returns the status of buffers
  void BufferStatusPage(http::ServerRequest* req);
  // Does a state checkpoint.
  void CheckpointPage(http::ServerRequest* req);

  // Loading helpers
  MediaMapper::LoadError LoadElements();
  MediaMapper::LoadError LoadHostAliases();

  // Save helpers
  void InitializeSavers();
  bool SaveHostAliases() const;

  bool ExportElement(const ElementExportSpec& spec, string* error);
  void GetElementExports(vector< ElementExportSpec >* exports) const;

  void StartSaverAlarm(const string& name, int64 last_start_time);
  // This is run on the start saver alarm
  void OnStartSaver(string* name,
                    int duration_in_seconds,
                    int64 last_start_time);

  // Called from within the saver when it was stoped
  void OnSaveStopped(streaming::Saver* saver);

  // The function that actually starts the saver.
  //   if duration_in_seconds > 0 -> we stop after the give duration
  //              (else it means is an "on-command" save
  //   if last_start_time > 0 -> we 'relinquish' as save (continue saving
  //              in the same directory - happens after a crash.
  bool StartSaverInternal(const string& name,
                          const string& description,
                          int duration_in_seconds,
                          int64 last_start_time,
                          string* error);
  // Gets the current active saves
  void GetCurrentSaves(vector<MediaSaverState>* saves) const;
  // Gets the current saves configuration
  void GetSavesConfig(vector< MediaSaverSpec >* saves) const;
  // Composes the extra headers for the fiven path
  string GetExtraHeaders(const string& path) const;
  // Helper to get the current host aliases
  void GetHostAliases(vector <MediaHostAliasSpec>* aliases) const;

 protected:
  //////////////////////////////////////////////////////////////////////
  //
  // RPC Interface from RPCServiceInvokerMediaElementService
  //
  void AddPolicySpec(
      rpc::CallContext<MediaOperationErrorData>* call,
      const PolicySpecs& spec);
  void DeletePolicySpec(
      rpc::CallContext<MediaOperationErrorData>* call,
      const string& name);

  void AddAuthorizerSpec(
      rpc::CallContext<MediaOperationErrorData>* call,
      const AuthorizerSpecs& spec);
  void DeleteAuthorizerSpec(
      rpc::CallContext<MediaOperationErrorData>* call,
      const string& name);

  void AddElementSpec(
      rpc::CallContext<MediaOperationErrorData>* call,
      const MediaElementSpecs& spec);
  void DeleteElementSpec(
      rpc::CallContext<MediaOperationErrorData>* call,
      const string& name);

  void StartExportElement(
      rpc::CallContext<MediaOperationErrorData>* call,
      const ElementExportSpec& spec);
  void StopExportElement(
      rpc::CallContext<MediaOperationErrorData>* call,
      const string& protocol,
      const string& path);

  void AddElementSaver(
      rpc::CallContext<MediaOperationErrorData>* call,
      const MediaSaverSpec& spec);
  void DeleteElementSaver(
      rpc::CallContext<MediaOperationErrorData>* call,
      const string& name);

  void StartSaver(
      rpc::CallContext<MediaOperationErrorData>* call,
      const string& name, const string& description);
  void StopSaver(
      rpc::CallContext<MediaOperationErrorData>* call,
      const string& name);

  void ListMedia(
      rpc::CallContext<vector<string> >* call,
      const string& elementname);

  void GetHttpExportRoot(
      rpc::CallContext<string>* call);
  void GetAllElementConfig(
      rpc::CallContext<ElementConfigurationSpecs>* call);
  void GetElementConfig(
      rpc::CallContext<ElementConfigurationSpecs>* call,
      const string& element);
  void GetElementExports(
      rpc::CallContext< vector<ElementExportSpec> >* call);

  void GetSavesConfig(
      rpc::CallContext< vector<MediaSaverSpec> >* call);
  void GetCurrentSaves(
      rpc::CallContext<vector<MediaSaverState> >* call);

  void SaveConfig(
      rpc::CallContext<bool>* call);

  void AddHostAlias(
      rpc::CallContext<MediaOperationErrorData>* call,
      const string& alias_name,
      const string& alias_ip);
  void DeleteHostAlias(
      rpc::CallContext<MediaOperationErrorData>* call,
      const string& alias_name);
  void GetHostAliases(
    rpc::CallContext< vector <MediaHostAliasSpec> >* call);

  void GetAllMediaAliases(
      rpc::CallContext< vector<MediaAliasSpec> >* call);
  void SetMediaAlias(
      rpc::CallContext<MediaOperationErrorData>* call,
      const string& alias_name,
      const string& media_name);
  void GetMediaAlias(
      rpc::CallContext<string>* call,
      const string& alias_name);

 private:
  net::Selector* selector_;
  http::Server* http_server_;
  http::Server* rpc_http_server_;
  rpc::HttpServer* rpc_server_;
  net::UserAuthenticator* admin_authenticator_;

  typedef map<string, io::AioManager*> AioManagersMap;
  AioManagersMap aio_managers_;
  util::MemAlignedFreeArrayList freelist_;
  io::BufferManager buffer_manager_;
  io::StateKeeper state_keeper_;
  io::StateKeeper local_state_keeper_;

  const string config_dir_;                  // we keep here all our configs:
  const string config_file_;                 // config file for elements&savers
  const string hosts_aliases_config_file_;   // put names to IPs config
  const string saver_config_file_;           // media savers config file
  streaming::ElementFactory factory_;        // maintaines config
  streaming::FactoryBasedElementMapper media_mapper_;   // maintains sources
  streaming::ElementMapper* aux_mapper_;     // auxiliary mapper for
                                             // functions update
  bool loaded_;                              // is the config loaded ??

  typedef map<string, string>  Host2IpMap;
  Host2IpMap host_aliases_;

  // Maps from saver to saver config
  typedef map<string, MediaSaverSpec*> SaverSpecMap;
  SaverSpecMap saver_specs_;

  typedef map<string, streaming::Saver*> SaverMap;
  SaverMap savers_;
  typedef map<string, Closure*> SaverStopAlarmsMap;
  SaverStopAlarmsMap savers_stoppers_;

  // We checkpoint from time to time w/ this alarm
  util::Alarm state_checkpointing_alarm_;

  // We expire old keys in state from time to time w/ this alarm
  util::Alarm state_expiration_alarm_;

  // If this is set we accept only requests from user "admin" with this passwd
  // NOTE: this is by no means very secure for tons of reasons
  const string admin_password_;

  // Where the rpc for libraries is served ?
  vector< pair<string, string> > rpc_library_paths_;

  // We set this while deleting this object, so we know that what's ending
  // now is not because the things end naturally, but because we crash or what
  // else
  bool is_deleting_;

  DISALLOW_EVIL_CONSTRUCTORS(MediaMapper);
};

#endif  // __APPS_STREAMING_BROADCASTING_SERVER_MEDIA_MAPPER_H__
