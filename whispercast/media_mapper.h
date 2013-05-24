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

class MediaMapper : public ServiceInvokerMediaElementService {
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

  net::Selector* selector() { return selector_; }

  // This loads the config file and creates the associated structures
  bool Load();
  // Saves the current configuration in the config file (safely :)
  bool Save() const;

  // Asynchronous close.
  void Close(Closure* close_completed);

  //////////////////////////////////////////////////////////////////////

  void CheckpointState();
  void ExpireStateKeys();

  streaming::ElementMapper* mapper() { return &media_mapper_; }
  const string& base_media_dir() const { return factory_.base_media_dir(); }

  streaming::Element* FindElement(const string& element_name);
  // Finds elements matching: <prefix><middle><suffix>.
  // Returns the list of <middle>s.
  void FindElementMatches(const string& prefix, const string& suffix,
                          vector<string>* middle_out);
  bool HasElement(const string& element_name);
  template<typename T>
  T* FindElementWithType(const string& element_name) {
    streaming::Element* element = FindElement(element_name);
    if ( element == NULL ) { return NULL; }
    if ( element->type() != T::kElementClassName ) {
      LOG_ERROR << "Element: " << element_name << " found with type: "
                << element->type() << " is different than expected type: "
                << T::kElementClassName;
      return NULL;
    }
    return static_cast<T*>(element);
  }

 private:
  // Returns the admin root page
  void ConfigRootPage(http::ServerRequest* req);
  // Returns the status of buffers
  void BufferStatusPage(http::ServerRequest* req);
  // Does a state checkpoint.
  void CheckpointPage(http::ServerRequest* req);

  // Loading helpers
  bool LoadElements();

  // Save helpers
  void InitializeSavers();

  void GetElementExports(vector< ElementExportSpec >* exports) const;

  static string SaverStateKey(const string& saver_name) {
    return "saver/" + saver_name;
  }

  class MSaver;
  void ScheduleSaver(MSaver* saver);
  // This is run on the start saver alarm
  void OnStartSaver(string name, uint32 duration_in_seconds);
  // Called from within the saver when it has stopped
  void OnSaverStopped(streaming::Saver* saver);

  // Gets the current active saves
  void GetCurrentSaves(vector<MediaSaverState>* saves) const;
  // Gets the current saves configuration
  void GetSavesConfig(vector<MediaSaverSpec>* saves) const;

 public:
  bool AddElement(const string& element_class_name,
                  const string& element_name,
                  bool is_global,
                  const string& params,
                  string* out_error);
  void DeleteElement(const string& name);

  bool AddPolicy(const string& class_name,
                 const string& name,
                 const string& params,
                 string* out_error);
  void DeletePolicy(const string& name);

  bool AddAuthorizer(const string& class_name,
                     const string& name,
                     const string& params,
                     string* out_error);
  void DeleteAuthorizer(const string& name);

  bool AddExport(const string& media_name,
                 const string& export_protocol,
                 const string& export_path,
                 const string& authorizer_name,
                 string* out_error);
  void DeleteExport(const string& protocol, const string& path);

  bool AddMediaSaver(const string& saver_name,
                     const string& media_name,
                     const vector<TimeSpec>& timespecs,
                     const string& save_dir,
                     const string& save_format,
                     string* out_error);
  void DeleteMediaSaver(const string& saver_name);

  bool StartMediaSaver(const string& name, uint32 duration_sec,
                       string* out_error);
  void StopMediaSaver(const string& name);

  void ListMedia(const string& media, vector<string>* out);

 protected:
  //////////////////////////////////////////////////////////////////////
  //
  // RPC Interface from RPCServiceInvokerMediaElementService
  //
  void AddPolicySpec(rpc::CallContext<MediaOpResult>* call,
                     const PolicySpecs& spec);
  void DeletePolicySpec(rpc::CallContext<rpc::Void>* call,
                        const string& name);

  void AddAuthorizerSpec(rpc::CallContext<MediaOpResult>* call,
                         const AuthorizerSpecs& spec);
  void DeleteAuthorizerSpec(rpc::CallContext<rpc::Void>* call,
                            const string& name);

  void AddElementSpec(rpc::CallContext<MediaOpResult>* call,
                      const MediaElementSpecs& spec);
  void DeleteElementSpec(rpc::CallContext<rpc::Void>* call,
                         const string& name);

  void StartExportElement(rpc::CallContext<MediaOpResult>* call,
                          const ElementExportSpec& spec);
  void StopExportElement(rpc::CallContext<rpc::Void>* call,
                         const string& protocol,
                         const string& path);

  void AddMediaSaver(rpc::CallContext<MediaOpResult>* call,
                     const MediaSaverSpec& spec, bool start_now);
  void DeleteMediaSaver(rpc::CallContext<rpc::Void>* call,
                        const string& name);

  void StartMediaSaver(rpc::CallContext<MediaOpResult>* call,
                       const string& name, int32 duration_sec);
  void StopMediaSaver(rpc::CallContext<rpc::Void>* call,
                      const string& name);

  void ListMedia(rpc::CallContext<vector<string> >* call,
                 const string& elementname);

  void GetHttpExportRoot(rpc::CallContext<string>* call);
  void GetAllElementConfig(rpc::CallContext<ElementConfigurationSpecs>* call);
  void GetElementConfig(rpc::CallContext<ElementConfigurationSpecs>* call,
                        const string& element);
  void GetElementExports(rpc::CallContext< vector<ElementExportSpec> >* call);

  void GetSavesConfig(rpc::CallContext< vector<MediaSaverSpec> >* call);
  void GetCurrentSaves(rpc::CallContext<vector<MediaSaverState> >* call);

  void SaveConfig(rpc::CallContext<bool>* call);

  void GetAllMediaAliases(rpc::CallContext< vector<MediaAliasSpec> >* call);
  void SetMediaAlias(rpc::CallContext<MediaOpResult>* call,
                     const string& alias_name,
                     const string& media_name);
  void GetMediaAlias(rpc::CallContext<string>* call,
                     const string& alias_name);

 private:
  // relative to config_dir_
  static const string kConfigFile;      // config file for elements&savers
  static const string kHostAliasesFile; // names to IPs config

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
  streaming::ElementFactory factory_;        // maintaines config
  streaming::FactoryBasedElementMapper media_mapper_;   // maintains sources
  streaming::ElementMapper* aux_mapper_;     // auxiliary mapper for
                                             // functions update

  struct MSaver {
    // saver's config
    MediaSaverSpec conf_;
    // the actual saver
    streaming::Saver saver_;
    // schedules the next start for this saver
    ::util::Alarm start_alarm_;
    // just for status reporting
    int64 start_ts_;
    MSaver(net::Selector* selector,
           const MediaSaverSpec& conf,
           streaming::MediaFormat save_format,
           streaming::ElementMapper* mapper,
           streaming::Saver::StopCallback* stop_callback)
      : conf_(conf),
        saver_(conf.name_.get(),
               mapper,
               save_format,
               conf.media_name_.get(),
               conf.save_dir_.get(),
               stop_callback),
        start_alarm_(*selector),
        start_ts_(0) {}
  };
  // Maps from saver_name to saver config
  typedef map<string, MSaver*> SaverMap;
  SaverMap savers_;

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
