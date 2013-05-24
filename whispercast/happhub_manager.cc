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

#include "happhub_manager.h"
#include <whisperlib/common/io/ioutil.h>
#include <whisperstreamlib/elements/standard_library/auto/standard_library_types.h>
#include <whisperstreamlib/elements/standard_library/aio_file/aio_file_element.h>
#include <whisperstreamlib/elements/standard_library/http_server/http_server_element.h>
#include <whisperstreamlib/elements/standard_library/switching/switching_element.h>
#include <whisperstreamlib/elements/standard_library/keyframe/keyframe_element.h>
#include <whisperstreamlib/elements/standard_library/policies/policy.h>
#include <private/extra_library/auto/extra_library_types.h>
#include <private/extra_library/http_authorizer/http_authorizer.h>
#include <private/extra_library/time_range/time_range_element.h>

DEFINE_string(hh_ping_url,
              "",
              "The http url where to send the ping. "
              "If empty, then the pings are disabled.");
DEFINE_int32(hh_ping_interval_sec,
             60,
             "Interval between consecutive pings.");

const string HapphubManager::kMediaTypeCamera("camera");
const string HapphubManager::kMediaTypeTimeRange("timerange");
const string HapphubManager::kMediaTypeFile("file");

const string HapphubManager::kFilesSubdir("files");
const string HapphubManager::kEventsSubdir("events");

HapphubManager::HapphubManager(MediaMapper* media_mapper)
  : ServiceInvokerHappHub(ServiceInvokerHappHub::GetClassName()),
    media_mapper_(media_mapper),
    ping_server_alarm_(*media_mapper_->selector()) {
}

HapphubManager::~HapphubManager() {
}

bool HapphubManager::Start() {
  if ( FLAGS_hh_ping_url != "" ) {
    ping_server_alarm_.Set(
        NewPermanentCallback(this, &HapphubManager::PingServer),
        true, FLAGS_hh_ping_interval_sec*1000LL, true, true);
  }
  return true;
}
void HapphubManager::Stop() {
  ping_server_alarm_.Stop();
}

string HapphubManager::MakeAbsPath(const string& path) const {
  return strutil::JoinMedia(media_mapper_->base_media_dir(), path);
}

void HapphubManager::GetEventMedia(const string& user_id,
    const string& event_id, HHEventMedia* out) {
  out->inputs_.ref();
  out->ranges_.ref();
  out->time_spans_.ref();
  streaming::TimeRangeElement* tr_element =
      media_mapper_->FindElementWithType<streaming::TimeRangeElement>(
          TimeRangeElementName(user_id, event_id));
  if ( tr_element == NULL ) {
    LOG_ERROR << "Element not found: "
              << TimeRangeElementName(user_id, event_id);
    return;
  }
  streaming::HttpServerElement* hs_element =
      media_mapper_->FindElementWithType<streaming::HttpServerElement>(
          HttpServerElementName(user_id, event_id));
  if ( hs_element == NULL ) {
    LOG_ERROR << "Element not found: "
              << HttpServerElementName(user_id, event_id);
    return;
  }

  // gather inputs
  hs_element->GetAllImports(&out->inputs_.ref());
  // gather named ranges
  tr_element->GetAllRangeNames(&out->ranges_.ref());
  // gather available time spans
  tr_element->GetAllTimeSpans(&out->time_spans_.ref());
}
void HapphubManager::GetUsers(vector<string>* out) {
  media_mapper_->FindElementMatches("", "_aio_file", out);
}
void HapphubManager::GetEvents(const string& user_id, vector<string>* out) {
  media_mapper_->FindElementMatches(TimeRangeElementPrefix(user_id),
      TimeRangeElementSufix(), out);
}
bool HapphubManager::IsValidUser(const string& user_id) {
  return media_mapper_->HasElement(AioFileElementName(user_id));
}
bool HapphubManager::IsValidEvent(const string& user_id, const string& event_id) {
  return media_mapper_->HasElement(HttpServerElementName(user_id, event_id));
}
void HapphubManager::DelUser(const string& user_id) {
  vector<string> events;
  GetEvents(user_id, &events);
  for ( uint32 i = 0; i < events.size(); i++ ) {
    DelEvent(user_id, events[i], true);
  }
  media_mapper_->DeleteElement(AioFileElementName(user_id));
  media_mapper_->DeleteExport("rtmp", FilesExportPath(user_id));
  media_mapper_->DeleteExport("http", FilesExportPath(user_id));
  io::Rm(MakeAbsPath(AioFileElementDir(user_id)));
}
void HapphubManager::DelEvent(const string& user_id, const string& event_id,
                              bool delete_timeranges) {
  media_mapper_->DeleteExport("rtmp", EventLiveExportPath(user_id, event_id));
  media_mapper_->DeleteExport("http", EventLiveExportPath(user_id, event_id));
  media_mapper_->DeleteExport("rtmp", EventPreviewExportPath(user_id, event_id));
  media_mapper_->DeleteExport("http", EventPreviewExportPath(user_id, event_id));
  media_mapper_->DeleteExport("rtmp", EventCameraExportPath(user_id, event_id));
  media_mapper_->DeleteExport("http", EventCameraExportPath(user_id, event_id));
  media_mapper_->DeleteExport("rtmp", EventCameraExportSlowPath(user_id, event_id));
  media_mapper_->DeleteExport("http", EventCameraExportSlowPath(user_id, event_id));
  media_mapper_->DeleteExport("rtmp", EventRangesExportPath(user_id, event_id));
  media_mapper_->DeleteExport("http", EventRangesExportPath(user_id, event_id));
  media_mapper_->DeleteMediaSaver(SwitchSaverName(user_id, event_id));
  media_mapper_->DeleteElement(SwitchElementName(user_id, event_id));
  media_mapper_->DeletePolicy(SwitchPolicyName(user_id, event_id));
  media_mapper_->DeleteElement(HttpServerElementName(user_id, event_id));
  if ( delete_timeranges ) {
    media_mapper_->DeleteElement(TimeRangeElementName(user_id, event_id));
  }
}
void HapphubManager::SaveConfig() {
  media_mapper_->Save();
}

void HapphubManager::GlobalSetup(
    rpc::CallContext< MediaOpResult >* call,
    const string& auth_server,
    bool use_https,
    const string& auth_query_path_format) {
  string error;
  do {
    HttpAuthorizerSpec spec;
    spec.servers_.ref().push_back(auth_server);
    spec.query_path_format_ = auth_query_path_format;
    spec.use_https_ = use_https;
    spec.http_headers_.ref();
    spec.body_lines_.ref();
    spec.include_auth_headers_ = true;
    spec.parse_json_authorizer_reply_ = true;
    if ( !media_mapper_->AddAuthorizer(
            streaming::HttpAuthorizer::kAuthorizerClassName,
            HttpAuthorizerName(),
            rpc::JsonEncoder::EncodeObject(spec), &error) ) {
      break;
    }
    // add the KeyframeExtractor to provide a slow stream
    KeyFrameExtractorElementSpec ke_spec(1, true);
    if ( !media_mapper_->AddElement(
            streaming::KeyFrameExtractorElement::kElementClassName,
            KeyframeExtractorElementName(),
            true, rpc::JsonEncoder::EncodeObject(ke_spec), &error) ) {
      break;
    }
    SaveConfig();
    call->Complete(MediaOpResult(true, ""));
  } while ( false );
  call->Complete(MediaOpResult(false, error));
}
void HapphubManager::GlobalCleanup(
    rpc::CallContext< rpc::Void >* call) {
  vector<string> users;
  GetUsers(&users);
  for ( uint32 i = 0; i < users.size(); i++ ) {
    DelUser(users[i]);
  }
  media_mapper_->DeleteAuthorizer(HttpAuthorizerName());
  media_mapper_->DeleteElement(KeyframeExtractorElementName());
  SaveConfig();
  call->Complete();
}
void HapphubManager::AddUser(
    rpc::CallContext< MediaOpResult >* call,
    const string& user_id) {
  // create user directories
  string home_dir = AioFileElementDir(user_id);
  if ( !io::Mkdir(MakeAbsPath(home_dir), false) ) {
    call->Complete(MediaOpResult(false, "Failed to create directory: [" +
        home_dir + "], error: " + string(GetLastSystemErrorDescription())));
    return;
  }
  string files_dir = strutil::JoinPaths(home_dir, kFilesSubdir);
  if ( !io::Mkdir(MakeAbsPath(files_dir), false) ) {
    call->Complete(MediaOpResult(false, "Failed to create directory: [" +
        files_dir + "], error: " + string(GetLastSystemErrorDescription())));
    return;
  }
  string events_dir = strutil::JoinPaths(home_dir, kEventsSubdir);
  if ( !io::Mkdir(MakeAbsPath(events_dir), false) ) {
    call->Complete(MediaOpResult(false, "Failed to create directory: [" +
        events_dir + "], error: " + string(GetLastSystemErrorDescription())));
    return;
  }

  // create user's AioFileElement
  string error;
  AioFileElementSpec spec(home_dir, "", "", "", false, false, false);
  if ( !media_mapper_->AddElement(
      streaming::AioFileElement::kElementClassName,
      AioFileElementName(user_id),
      true, rpc::JsonEncoder::EncodeObject(spec), &error) ) {
    call->Complete(MediaOpResult(false, error));
    return;
  }
  // export the file element
  if ( !media_mapper_->AddExport(AioFileMediaPath(user_id),
           "rtmp", FilesExportPath(user_id), HttpAuthorizerName(), &error) ) {
    call->Complete(MediaOpResult(false, error));
    return;
  }
  if ( !media_mapper_->AddExport(AioFileMediaPath(user_id),
           "http", FilesExportPath(user_id), HttpAuthorizerName(), &error) ) {
    call->Complete(MediaOpResult(false, error));
    return;
  }
  SaveConfig();
  call->Complete(MediaOpResult(true, ""));
}
void HapphubManager::DelUser(
    rpc::CallContext< rpc::Void >* call,
    const string& user_id) {
  DelUser(user_id);
  SaveConfig();
  call->Complete();
}
void HapphubManager::AddEvent(
    rpc::CallContext< MediaOpResult >* call,
    const string& user_id, const string& event_id, bool is_public) {
  if ( !IsValidUser(user_id) ) {
    call->Complete(MediaOpResult(false, "No such user: " + user_id));
    return;
  }
  string error;
  do {
    // create event directory (will contain time range files)
    string event_dir = SwitchSaverDir(user_id, event_id);
    if ( !io::Mkdir(MakeAbsPath(event_dir), false) ) {
      error = "Failed to create directory: [" + event_dir + "], error: "
              + GetLastSystemErrorDescription();
      break;
    }
    // add HttpServerElement to receive the input streams
    HttpServerElementSpec hs_spec;
    hs_spec.base_listen_path_ = strutil::JoinPaths(user_id, event_id);
    hs_spec.authorizer_name_ = HttpAuthorizerName();
    hs_spec.imports_.ref();
    if ( !media_mapper_->AddElement(
            streaming::HttpServerElement::kElementClassName,
            HttpServerElementName(user_id, event_id),
            true, rpc::JsonEncoder::EncodeObject(hs_spec), &error) ) {
      break;
    }
    // add the SwitchingElement to switch between input streams
    OnCommandPolicySpec policy_spec;
    policy_spec.default_element_ = "";
    policy_spec.max_history_size_ = 1;
    if ( !media_mapper_->AddPolicy(
            streaming::OnCommandPolicy::kPolicyClassName,
            SwitchPolicyName(user_id, event_id),
            rpc::JsonEncoder::EncodeObject(policy_spec), &error) ) {
      break;
    }
    SwitchingElementSpec sw_spec;
    sw_spec.media_only_when_used_ = false;
    sw_spec.policy_name_ = SwitchPolicyName(user_id, event_id);
    if ( !media_mapper_->AddElement(
            streaming::SwitchingElement::kElementClassName,
            SwitchElementName(user_id, event_id),
            true, rpc::JsonEncoder::EncodeObject(sw_spec), &error) ) {
      break;
    }
    // add the output saver
    vector<TimeSpec> sw_saver_spec;
    if ( !media_mapper_->AddMediaSaver(
            SwitchSaverName(user_id, event_id),
            SwitchElementName(user_id, event_id),
            sw_saver_spec,
            SwitchSaverDir(user_id, event_id),
            streaming::MediaFormatToSmallType(streaming::MFORMAT_FLV),
            &error) ||
         !media_mapper_->StartMediaSaver(
            SwitchSaverName(user_id, event_id), 0, &error) ) {
      break;
    }
    // add the TimeRangeElement
    TimeRangeElementSpec tr_spec;
    tr_spec.internal_media_path_ = TimeRangeMediaPath(user_id, event_id);
    tr_spec.update_media_interval_sec_ = 5;
    if ( !media_mapper_->AddElement(
        streaming::TimeRangeElement::kElementClassName,
        TimeRangeElementName(user_id, event_id),
        true, rpc::JsonEncoder::EncodeObject(tr_spec), &error) ) {
      break;
    }

    // export the live stream
    if ( !media_mapper_->AddExport(SwitchElementName(user_id, event_id),
             "rtmp", EventLiveExportPath(user_id, event_id),
             is_public ? string("") : HttpAuthorizerName(), &error) ) {
      break;
    }
    if ( !media_mapper_->AddExport(SwitchElementName(user_id, event_id),
             "http", EventLiveExportPath(user_id, event_id),
             is_public ? string("") : HttpAuthorizerName(), &error) ) {
      break;
    }
    // export the preview stream
    if ( !media_mapper_->AddExport(SwitchElementName(user_id, event_id),
             "rtmp", EventPreviewExportPath(user_id, event_id),
             HttpAuthorizerName(), &error) ) {
     break;
    }
    if ( !media_mapper_->AddExport(SwitchElementName(user_id, event_id),
             "http", EventPreviewExportPath(user_id, event_id),
             HttpAuthorizerName(), &error) ) {
     break;
    }
    // export camera preview
    if ( !media_mapper_->AddExport(HttpServerElementName(user_id, event_id),
             "rtmp", EventCameraExportPath(user_id, event_id),
             HttpAuthorizerName(), &error) ) {
     break;
    }
    if ( !media_mapper_->AddExport(HttpServerElementName(user_id, event_id),
             "http", EventCameraExportPath(user_id, event_id),
             HttpAuthorizerName(), &error) ) {
     break;
    }
    // export camera slow preview
    if ( !media_mapper_->AddExport(
             strutil::JoinMedia(KeyframeExtractorElementName(),
                                HttpServerElementName(user_id, event_id)),
             "rtmp", EventCameraExportSlowPath(user_id, event_id),
             HttpAuthorizerName(), &error) ) {
     break;
    }
    if ( !media_mapper_->AddExport(
             strutil::JoinMedia(KeyframeExtractorElementName(),
                                HttpServerElementName(user_id, event_id)),
             "http", EventCameraExportSlowPath(user_id, event_id),
             HttpAuthorizerName(), &error) ) {
     break;
    }
    // export event ranges
    if ( !media_mapper_->AddExport(TimeRangeElementName(user_id, event_id),
             "rtmp", EventRangesExportPath(user_id, event_id),
             is_public ? string("") : HttpAuthorizerName(), &error) ) {
      break;
    }
    if ( !media_mapper_->AddExport(TimeRangeElementName(user_id, event_id),
             "http", EventRangesExportPath(user_id, event_id),
             is_public ? string("") : HttpAuthorizerName(), &error) ) {
      break;
    }
    SaveConfig();
    call->Complete(MediaOpResult(true, ""));
    return;
  } while (false);
  DelEvent(user_id, event_id, true);
  call->Complete(MediaOpResult(false, error));
}
void HapphubManager::AddEventInput(
    rpc::CallContext< MediaOpResult >* call,
    const string& user_id, const string& event_id, const string& input_id) {
  if ( !IsValidEvent(user_id, event_id) ) {
    call->Complete(MediaOpResult(false,
        "No such event: user_id: " + user_id + ", event_id: " + event_id));
    return;
  }

  streaming::HttpServerElement* hs_element =
      media_mapper_->FindElementWithType<streaming::HttpServerElement>(
          HttpServerElementName(user_id, event_id));
  if ( hs_element == NULL ) {
    call->Complete(MediaOpResult(false,
        "Element not found: " + HttpServerElementName(user_id, event_id)));
    return;
  }
  string err;
  if ( !hs_element->AddImport(input_id, true, &err) ) {
    call->Complete(MediaOpResult(false, err));
    return;
  }
  SaveConfig();
  call->Complete(MediaOpResult(true, ""));
}
void HapphubManager::DelEventInput(
    rpc::CallContext< rpc::Void >* call,
    const string& user_id, const string& event_id, const string& input_id) {
  if ( !IsValidEvent(user_id, event_id) ) {
    call->Complete();
    return;
  }
  streaming::HttpServerElement* hs_element =
      media_mapper_->FindElementWithType<streaming::HttpServerElement>(
          HttpServerElementName(user_id, event_id));
  if ( hs_element == NULL ) {
    LOG_ERROR << "Element not found: "
              << HttpServerElementName(user_id, event_id);
    call->Complete();
    return;
  }
  hs_element->DeleteImport(input_id);
  SaveConfig();
  call->Complete();
}
void HapphubManager::DelEvent(
    rpc::CallContext< rpc::Void >* call,
    const string& user_id, const string& event_id) {
  DelEvent(user_id, event_id, true);
  SaveConfig();
  call->Complete();
}
void HapphubManager::SetTimeRange(
    rpc::CallContext< MediaOpResult >* call,
    const string& user_id, const string& event_id,
    const string& timerange_id,
    const string& timerange_dates) {
  if ( !IsValidEvent(user_id, event_id) ) {
    call->Complete(MediaOpResult(false,
        "No such event: user_id: " + user_id + ", event_id: " + event_id));
    return;
  }
  streaming::TimeRangeElement* tr_element =
      media_mapper_->FindElementWithType<streaming::TimeRangeElement>(
          TimeRangeElementName(user_id, event_id));
  if ( tr_element == NULL ) {
    call->Complete(MediaOpResult(false,
        "Element not found: " + TimeRangeElementName(user_id, event_id)));
    return;
  }
  string err;
  bool success = tr_element->SetRange(timerange_id, timerange_dates, &err);
  call->Complete(MediaOpResult(success, err));
}
void HapphubManager::GetTimeRange(rpc::CallContext< string >* call,
    const string& user_id, const string& event_id,
    const string& timerange_id) {
  if ( !IsValidEvent(user_id, event_id) ) {
    LOG_ERROR << "GetTimeRange: No such event: user_id: " << user_id
              << ", event_id: " << event_id;
    call->Complete("");
    return;
  }
  streaming::TimeRangeElement* tr_element =
      media_mapper_->FindElementWithType<streaming::TimeRangeElement>(
          TimeRangeElementName(user_id, event_id));
  if ( tr_element == NULL ) {
    LOG_ERROR << "GetTimeRange: Element not found: "
              << TimeRangeElementName(user_id, event_id);
    call->Complete("");
    return;
  }
  call->Complete(tr_element->GetRange(timerange_id));
}
void HapphubManager::GetUserMedia(
    rpc::CallContext< HHUserMedia >* call,
    const string& user_id) {
  HHUserMedia um;
  // gather files
  media_mapper_->ListMedia(AioFileMediaPath(user_id), &um.files_.ref());
  // gather events
  GetEvents(user_id, &um.events_.ref());
  call->Complete(um);
}

void HapphubManager::GetEventMedia(
    rpc::CallContext< HHEventMedia >* call,
    const string& user_id, const string& event_id) {
  HHEventMedia em;
  GetEventMedia(user_id, event_id, &em);
  call->Complete(em);
}
void HapphubManager::Switch(
    rpc::CallContext< MediaOpResult >* call,
    const string& user_id, const string& event_id,
    const string& media_type, const string& media_id) {
  if ( !IsValidEvent(user_id, event_id) ) {
    call->Complete(MediaOpResult(false,
        "No such event: user_id: " + user_id + ", event_id: " + event_id));
    return;
  }
  streaming::SwitchingElement* sw_element =
      media_mapper_->FindElementWithType<streaming::SwitchingElement>(
          SwitchElementName(user_id, event_id));
  if ( sw_element == NULL ) {
    call->Complete(MediaOpResult(false,
        "Element not found: " + SwitchElementName(user_id, event_id)));
    return;
  }
  // maybe switch to a camera
  if ( media_type == kMediaTypeCamera ) {
    sw_element->SwitchCurrentMedia(strutil::JoinMedia(
        HttpServerElementName(user_id, event_id), media_id), NULL, true);
    call->Complete(MediaOpResult(true, ""));
    return;
  }
  // maybe switch to a time range
  if ( media_type == kMediaTypeTimeRange ) {
    sw_element->SwitchCurrentMedia(strutil::JoinMedia(
        TimeRangeElementName(user_id, event_id), media_id), NULL, true);
    call->Complete(MediaOpResult(true, ""));
    return;
  }
  // maybe switch to a file
  if ( media_type == kMediaTypeFile ) {
    sw_element->SwitchCurrentMedia(strutil::JoinMedia(
        AioFileMediaPath(user_id), media_id), NULL, true);
    call->Complete(MediaOpResult(true, ""));
    return;
  }
  call->Complete(MediaOpResult(false, "Invalid media_type: " + media_type +
      ", valid values are: " + kMediaTypeCamera + ", " + kMediaTypeFile +
      ", or " + kMediaTypeTimeRange));
}
void HapphubManager::GetEventLiveExport(
    rpc::CallContext< string >* call,
    const string& user_id, const string& event_id) {
  call->Complete(EventLiveExportPath(user_id, event_id));
}
void HapphubManager::GetEventRangesExport(
    rpc::CallContext< string >* call,
    const string& user_id, const string& event_id) {
  call->Complete(EventRangesExportPath(user_id, event_id));
}

void HapphubManager::GetFilesExport(
    rpc::CallContext< string >* call,
    const string& user_id) {
  call->Complete(FilesExportPath(user_id));
}

void HapphubManager::PingServer() {
  URL url(FLAGS_hh_ping_url);
  net::HostPort hp(url.host());
  if ( url.has_port() ) {
    hp.set_port(::atol(url.port().c_str()));
  }

  http::ClientParams* http_params = new http::ClientParams();
  http::ClientRequest* http_req = new http::ClientRequest(
      http::METHOD_POST, &url);
  http_req->request()->client_data()->Write("blabla");

  http::SimpleClientConnection* http_conn = new http::SimpleClientConnection(
      media_mapper_->selector());
  http::ClientProtocol* http_proto = new http::ClientProtocol(
      http_params, http_conn, hp);

  LOG_WARNING << "####################################################";
  LOG_WARNING << "#### PingServer to:" << url.spec() << " with data: "
              << http_req->request()->client_data()->DebugString();
  http_proto->SendRequest(http_req, NewCallback(this,
      &HapphubManager::PingComplete,
      http_params, http_req, http_proto));
}
void HapphubManager::PingComplete(
    http::ClientParams* http_params,
    http::ClientRequest* http_req,
    http::ClientProtocol* http_proto) {
  LOG_WARNING << "####################################################";
  LOG_WARNING << "#### PingComplete reply: " << http_req->request()->server_data()->DebugString();
  http_proto->Clear();
  delete http_proto;
  delete http_req;
  delete http_params;
}
