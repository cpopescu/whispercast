// Copyright (c) 2012, Whispersoft s.r.l.
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
// Authors: Cosmin Tudorache

#ifndef __APP_WHISPERCAST_HAPPHUB_MANAGER_H__
#define __APP_WHISPERCAST_HAPPHUB_MANAGER_H__

#include "media_mapper.h"
#include <whisperlib/common/base/strutil.h>
#include <whisperlib/common/base/alarm.h>
#include <whisperlib/net/http/http_client_protocol.h>

class HapphubManager : public ServiceInvokerHappHub {
 public:
  static const string kMediaTypeCamera;
  static const string kMediaTypeTimeRange;
  static const string kMediaTypeFile;

  static const string kFilesSubdir;
  static const string kEventsSubdir;

  static string HttpAuthorizerName() {
    return "http_authorizer";
  }
  static string KeyframeExtractorElementName() {
    return "keyframe_extractor";
  }
  static string AioFileElementName(const string& user_id) {
    return user_id + "_aio_file";
  }
  static string AioFileElementDir(const string& user_id) {
    return user_id;
  }
  static string AioFileMediaPath(const string& user_id) {
    return strutil::JoinPaths(AioFileElementName(user_id), kFilesSubdir);
  }
  static string HttpServerElementPrefix(const string& user_id) {
    return user_id + "_";
  }
  static string HttpServerElementSufix() {
    return "_http_server";
  }
  static string HttpServerElementName(const string& user_id,
      const string& event_id) {
    return HttpServerElementPrefix(user_id) + event_id +
           HttpServerElementSufix();
  }
  static string SwitchPolicyName(const string& user_id,
      const string& event_id) {
    return user_id + "_" + event_id + "_switch_policy";
  }
  static string SwitchElementName(const string& user_id,
      const string& event_id) {
    return user_id + "_" + event_id + "_switch";
  }
  static string SwitchSaverName(const string& user_id,
      const string& event_id) {
    return user_id + "_" + event_id + "_saver";
  }
  static string SwitchSaverDir(const string& user_id, const string& event_id) {
    return strutil::JoinPaths(user_id, kEventsSubdir, event_id);
  }
  static string TimeRangeElementPrefix(const string& user_id) {
    return user_id + "_";
  }
  static string TimeRangeElementSufix() {
    return "_time_range";
  }
  static string TimeRangeElementName(const string& user_id,
      const string& event_id) {
    return TimeRangeElementPrefix(user_id) + event_id +
           TimeRangeElementSufix();
  }
  static string TimeRangeMediaPath(const string& user_id,
      const string& event_id) {
    return strutil::JoinMedia(AioFileElementName(user_id),
                              kEventsSubdir, event_id);
  }
  static string EventLiveExportPath(const string& user_id,
      const string& event_id) {
    return strutil::JoinMedia(user_id, event_id, "live");
  }
  static string EventPreviewExportPath(const string& user_id,
      const string& event_id) {
    return strutil::JoinMedia(user_id, event_id, "preview");
  }
  static string EventCameraExportPath(const string& user_id,
      const string& event_id) {
    return strutil::JoinMedia(user_id, event_id, "camera_live");
  }
  static string EventCameraExportSlowPath(const string& user_id,
      const string& event_id) {
    return strutil::JoinMedia(user_id, event_id, "camera_slow");
  }
  static string EventRangesExportPath(const string& user_id,
      const string& event_id) {
    return strutil::JoinMedia(user_id, event_id, "ranges");
  }
  static string FilesExportPath(const string& user_id) {
    return strutil::JoinMedia(user_id, "files");
  }

 public:
  HapphubManager(MediaMapper* media_mapper);
  virtual ~HapphubManager();

  bool Start();
  void Stop();

 private:
  // adds the "base_media_dir" prefix
  string MakeAbsPath(const string& path) const;
  void GetEventMedia(const string& user_id, const string& event_id,
                     HHEventMedia* out);
  void GetUsers(vector<string>* out);
  void GetEvents(const string& user, vector<string>* out);
  bool IsValidUser(const string& user);
  bool IsValidEvent(const string& user, const string& event);
  void DelUser(const string& user_id);
  void DelEvent(const string& user_id, const string& event_id,
                bool delete_timeranges);
  void SaveConfig();

  //////////////////////////////////////////////////////////////////////
  //
  // RPC Interface from RPCServiceInvokerHappHub
  //
  virtual void GlobalSetup(rpc::CallContext< MediaOpResult >* call,
                           const string& auth_server,
                           bool use_https,
                           const string& auth_query_path_format);
  virtual void GlobalCleanup(rpc::CallContext< rpc::Void >* call);
  virtual void AddUser(rpc::CallContext< MediaOpResult >* call,
                       const string& user_id);
  virtual void DelUser(rpc::CallContext< rpc::Void >* call,
                       const string& user_id);
  virtual void AddEvent(rpc::CallContext< MediaOpResult >* call,
                        const string& user_id, const string& event_id,
                        bool is_public);
  virtual void AddEventInput(rpc::CallContext< MediaOpResult >* call,
                             const string& user_id, const string& event_id,
                             const string& input_id);
  virtual void DelEventInput(rpc::CallContext< rpc::Void >* call,
                             const string& user_id, const string& event_id,
                             const string& input_id);
  virtual void DelEvent(rpc::CallContext< rpc::Void >* call,
                        const string& user_id, const string& event_id);
  virtual void SetTimeRange(rpc::CallContext< MediaOpResult >* call,
                            const string& user_id, const string& event_id,
                            const string& timerange_id,
                            const string& timerange_dates);
  virtual void GetTimeRange(rpc::CallContext< string >* call,
                            const string& user_id, const string& event_id,
                            const string& timerange_id);
  virtual void GetUserMedia(rpc::CallContext< HHUserMedia >* call,
                            const string& user_id);
  virtual void GetEventMedia(rpc::CallContext< HHEventMedia >* call,
                            const string& user_id, const string& event_id);
  virtual void Switch(rpc::CallContext< MediaOpResult >* call,
                      const string& user_id, const string& event_id,
                      const string& media_type, const string& media_id);
  virtual void GetEventLiveExport(rpc::CallContext< string >* call,
                                  const string& user_id,
                                  const string& event_id);
  virtual void GetEventRangesExport(rpc::CallContext< string >* call,
                                    const string& user_id,
                                    const string& event_id);
  virtual void GetFilesExport(rpc::CallContext< string >* call,
                              const string& user_id);
 private:
  // 'pings' a configurable server through HTTP Post
  void PingServer();
  void PingComplete(http::ClientParams* http_params,
                    http::ClientRequest* http_req,
                    http::ClientProtocol* http_proto);

 private:
  MediaMapper* media_mapper_;
  util::Alarm ping_server_alarm_;
};

#endif /* __APP_WHISPERCAST_HAPPHUB_MANAGER_H__ */
