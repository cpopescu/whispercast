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
//

#include <whisperlib/common/base/types.h>
#include WHISPER_HASH_SET_HEADER
#include <whisperlib/common/io/buffer/io_memory_stream.h>
#include <whisperlib/net/http/http_header.h>
#include <whisperlib/net/http/http_request.h>
#include <whisperlib/common/io/ioutil.h>
#include <whisperlib/common/base/gflags.h>

#include <whisperstreamlib/base/tag_splitter.h>
#include <whisperstreamlib/base/saver.h>
#include <whisperstreamlib/base/tag_distributor.h>
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_encoder.h>
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_decoder.h>
#include <whisperstreamlib/elements/standard_library/http_server/http_server_element.h>

#define ILOG(level)  LOG(level) << info() << ": "
#ifdef _DEBUG
#define ILOG_DEBUG   ILOG(INFO)
#else
#define ILOG_DEBUG   if (false) ILOG(INFO)
#endif
#define ILOG_INFO    ILOG(INFO)
#define ILOG_WARNING ILOG(WARNING)
#define ILOG_ERROR   ILOG(ERROR)
#define ILOG_FATAL   ILOG(FATAL)

namespace streaming {

////////////////////////////////////////////////////////////////////////////////
//
// Helper for HttpServerElement - Holds the server request where the client
// uploads data.
// This class operates in 2 threads:
//  - the networking thread which belongs to the http_req_->net_selector()
//    Data is pulled from http upload, splitted and tags are scheduled
//    in media thread.
//  - the media thread (selector_). Does the authentication and sends tags
//    to the downstream internal clients.
//
class HttpServerImportData : public ImportElementData {
  static const int kTagTimeoutMs = 20000;
  static const int kTagTimeoutRegistrationGracePeriodMs = 1000;

public:
  HttpServerImportData(streaming::ElementMapper* mapper,
                       net::Selector* selector,
                       http::Server* http_server,
                       const string& name,
                       const string& listen_path,
                       const string& authorizer_name)
      : mapper_(mapper),
        selector_(selector),
        http_server_(http_server),
        name_(name),
        listen_path_(listen_path),
        authorizer_name_(authorizer_name),
        info_(name_ + "[" + http_server_->name() + " " + listen_path_ + "]: "),
        auth_(*selector),
        splitter_(NULL),
        distributor_(kDefaultFlavourMask, name),
        tag_timeout_alarm_(*selector),
        http_req_(NULL) {
    http_server_->RegisterProcessor(listen_path_,
        NewPermanentCallback(this, &HttpServerImportData::ProcessServerRequest),
        true, true);
    http_server_->RegisterClientStreaming(listen_path_, true);
    tag_timeout_alarm_.Set(NewPermanentCallback(this,
        &HttpServerImportData::TagTimeout), true, kTagTimeoutMs, false, false);
  }

  virtual ~HttpServerImportData() {
    CHECK_NULL(splitter_);
  }

  const string& info() const { return info_; }

  ///////////////////////////////////////////////////////////////////////////
  // ImportElementData interface
  virtual const MediaInfo* media_info() const {
    return splitter_ == NULL ? NULL : &splitter_->media_info();
  }

  virtual bool Initialize() {
    return true;
  }
  virtual bool AddCallback(Request* request, ProcessingCallback* callback) {
    distributor_.add_callback(request, callback);
    return true;
  }
  virtual void RemoveCallback(Request* request) {
    distributor_.remove_callback(request);
  }
  // Causes the complete close of this import in preparation for deletion
  virtual void Close() {
    CHECK(selector_->IsInSelectThread());

    http_server_->UnregisterProcessor(listen_path_);
    tag_timeout_alarm_.Stop();

    distributor_.CloseAllCallbacks(true);
    CloseHttpRequest(http::OK);
  }

 private:
  void ProcessServerRequest(http::ServerRequest* req) {
    CHECK_NOT_NULL(req);
    CHECK(req->net_selector()->IsInSelectThread());
    URL* const url = req->request()->url();
    if ( url == NULL ) {
      ILOG_ERROR << "Invalid request: " << req->ToString();
      return;
    }

    if ( http_req_ == req ) {
      // continue streaming
      ProcessMoreData();
      return;
    }

    if ( http_req_ != NULL ) {
      ILOG_INFO << "Closing old request: " << http_req_->ToString()
                << " in preparation for new request: " << req->ToString();
      CloseHttpRequest(http::CONFLICT);
    }

    ///////////////////////////////////////////////////////////////////////
    // start new request
    CHECK_NULL(http_req_);
    if ( authorizer_name_ == "" ) {
      // without authorization
      AuthorizationCompleted(req, true);
      return;
    }
    // with authorization
    StartAuthorization(req);
  }

  // Called when authorization from main thread is completed
  void AuthorizationCompleted(http::ServerRequest* req, bool allowed) {
    if ( !req->net_selector()->IsInSelectThread() ) {
      req->net_selector()->RunInSelectLoop(NewCallback(this,
          &HttpServerImportData::AuthorizationCompleted, req, allowed));
      return;
    }

    if ( req->is_orphaned() ) {
      req->clear_closed_callback();
      req->ReplyWithStatus(http::OK);
      return;
    }
    if ( !allowed ) {
      ILOG_ERROR << "Unauthorized request received from: "
                 << req->remote_address().ToString();
      req->clear_closed_callback();
      req->request()->server_data()->Write("Unauthorized");
      req->ReplyWithStatus(http::UNAUTHORIZED);
      return;
    }
    if ( http_req_ != NULL ) {
      ILOG_ERROR << "Closing previous request with CONFLICT";
      CloseHttpRequest(http::CONFLICT);
    }
    SetNewRequest(req);
    if ( http_req_ == NULL ) {
      // Sth went wrong and SetNewRequest() actually closed the req.
      return;
    }
    ILOG_INFO << "New stream uploading client from: "
              << req->remote_address().ToString();
    ProcessMoreData();
  }
  // read more data from http_req_ -> extract tags -> send tags downstream
  void ProcessMoreData() {
    CHECK_NOT_NULL(http_req_);
    CHECK(http_req_->net_selector()->IsInSelectThread());

    if ( http_req_->is_orphaned() ) {
      ILOG_ERROR << "Closing on orphaned request.";
      CloseHttpRequest(http::OK);
      return;
    }

    while ( true ) {
      scoped_ref<Tag> tag;
      int64 ts = 0;
      TagReadStatus result = splitter_->GetNextTag(
          http_req_->request()->client_data(), &tag, &ts,
          http_req_->is_parsing_finished());
      if ( result == streaming::READ_SKIP ) {
        continue;
      }
      if ( result == READ_NO_DATA ) {
        break;
      }
      if ( result == READ_EOF ) {
        ILOG_INFO << "Upload completed. Closing http request..";
        CloseHttpRequest(http::OK);
        return;
      }
      if ( result != READ_OK ) {
        LOG_ERROR << "splitter_->GetNextTag failed: "
                  << TagReadStatusName(result);
        CloseHttpRequest(http::INTERNAL_SERVER_ERROR);
        return;
      }
      // send tag to media selector
      HandleTag(tag, ts);
    }
  }
  // Sends the tag to downstream clients. If tag == NULL, just reset
  // the distributor (send SourceEnded and move callbacks to bootstrap).
  void HandleTag(scoped_ref<Tag> tag, int64 ts) {
    if ( !selector_->IsInSelectThread() ) {
      selector_->RunInSelectLoop(
          NewCallback(this, &HttpServerImportData::HandleTag, tag, ts));
      return;
    }
    if ( tag.get() == NULL ) {
      distributor_.Reset();
      return;
    }
    distributor_.DistributeTag(tag.get(), ts);
    MaybeReregisterTagTimeout(false);
  }
  // Called when a request gets closed on http protocol level
  void NotifyRequestClosed() {
    ILOG_INFO << "Client request closed.";
    CloseHttpRequest(http::OK);
  }

  // Closes the current http request. Does not close the clients.
  void CloseHttpRequest(http::HttpReturnCode http_code) {
    if ( http_req_ == NULL ) {
      return;
    }
    if ( !http_req_->net_selector()->IsInSelectThread() ) {
      http_req_->net_selector()->RunInSelectLoop(NewCallback(this,
          &HttpServerImportData::CloseHttpRequest, http_code));
      return;
    }

    CHECK_NOT_NULL(http_req_);
    http_req_->clear_closed_callback();
    http_req_->ReplyWithStatus(http_code);
    http_req_ = NULL;

    delete splitter_;
    splitter_ = NULL;

    // reset distributor_
    HandleTag(NULL, 0);
  }
  void Pause(bool pause) {
    CHECK_NOT_NULL(http_req_);
    if ( !http_req_->net_selector()->IsInSelectThread() ) {
      http_req_->net_selector()->RunInSelectLoop(NewCallback(this,
          &HttpServerImportData::Pause, pause));
    }

    if ( pause ) {
      http_req_->PauseReading();
    } else {
      http_req_->ResumeReading();
    }
  }

  void StartAuthorization(http::ServerRequest* req) {
    if ( !selector_->IsInSelectThread() ) {
      selector_->RunInSelectLoop(NewCallback(this,
          &HttpServerImportData::StartAuthorization, req));
      return;
    }

    streaming::Authorizer* const auth = mapper_->GetAuthorizer(authorizer_name_);
    if ( auth == NULL ) {
      LOG_ERROR << "Cannot find authorizer: [" << authorizer_name_ << "]";
      req->ReplyWithStatus(http::INTERNAL_SERVER_ERROR);
      return;
    }

    URL* const url = req->request()->url();
    streaming::AuthorizerRequest areq("", "", "",
        req->remote_address().ToString(),
        url->spec(),
        streaming::kActionPublish,
        0);
    areq.ReadFromUrl(*url);
    if ( areq.user_ == "" ) {
      req->request()->client_header()->GetAuthorizationField(
          &areq.user_, &areq.passwd_);
    }

    auth_.Start(auth, areq,
        NewCallback(this, &HttpServerImportData::AuthorizationCallback, req),
        NewCallback(this, &HttpServerImportData::ReauthorizationFailed, req));
  }
  void AuthorizationCallback(http::ServerRequest* req, bool allowed) {
    CHECK(selector_->IsInSelectThread());
    AuthorizationCompleted(req, allowed);
  }
  void ReauthorizationFailed(http::ServerRequest* req) {
    CHECK(selector_->IsInSelectThread());
    CloseHttpRequest(http::UNAUTHORIZED);
  }
  void MaybeReregisterTagTimeout(bool force) {
    if ( !selector_->IsInSelectThread() ) {
      selector_->RunInSelectLoop(NewCallback(this,
          &HttpServerImportData::MaybeReregisterTagTimeout, force));
      return;
    }
    if ( force || selector_->now() > tag_timeout_alarm_.last_start_ts() +
                                     kTagTimeoutRegistrationGracePeriodMs ) {
      tag_timeout_alarm_.Start();
    }
  }
  void TagTimeout() {
    CHECK(selector_->IsInSelectThread());
    CloseHttpRequest(http::REQUEST_TIME_OUT);
  }

  void SetNewRequest(http::ServerRequest* req) {
    CHECK(req->net_selector()->IsInSelectThread());
    URL* const url = req->request()->url();
    CHECK_NULL(http_req_);
    http_req_ = req;
    http_req_->set_closed_callback(
      NewCallback(this, &HttpServerImportData::NotifyRequestClosed));

    string publish_command = "";
    vector< pair<string, string> > comp;
    if ( url->GetQueryParameters(&comp, true) ) {
      for ( int i = 0; i < comp.size(); ++i ) {
        if ( comp[i].first == streaming::kMediaUrlParam_PublishCommand ) {
          publish_command = comp[i].second;
        }
      }
    }
    const string content_type = req->request()->client_header()->FindField(
        http::kHeaderContentType);
    MediaFormat media_format;
    if ( !MediaFormatFromContentType(content_type, &media_format) ) {
      LOG_ERROR << "Invalid content type: " << content_type;
      CloseHttpRequest(http::NOT_ACCEPTABLE);
      return;
    }

    if ( !InitializeSplitter(media_format, publish_command) ) {
      LOG_ERROR << "InitializeSplitter() failed";
      CloseHttpRequest(http::INTERNAL_SERVER_ERROR);
      return;
    }
    MaybeReregisterTagTimeout(true);
  }
  bool InitializeSplitter(MediaFormat media_format, string publish_command) {
    CHECK(http_req_->net_selector()->IsInSelectThread());

    // initialize splitter
    CHECK_NULL(splitter_);
    splitter_ = CreateSplitter(name_, media_format);
    if ( splitter_ == NULL ) {
      ILOG_ERROR << "Failed to create splitter for media_format: "
                 << MediaFormatName(media_format);
      return false;
    }

    return true;
  }

 private:
  streaming::ElementMapper* const mapper_;
  net::Selector* const selector_;
  http::Server* const http_server_;
  const string name_;
  const string listen_path_;
  const string authorizer_name_;

  // just for logging
  const string info_;

  AsyncAuthorize auth_;

  TagSplitter* splitter_;

  TagDistributor distributor_;

  util::Alarm tag_timeout_alarm_;

  // current request being processed
  http::ServerRequest* http_req_;

  DISALLOW_EVIL_CONSTRUCTORS(HttpServerImportData);
};

///////////////////////////////////////////////////////////////////////////

const char HttpServerElement::kElementClassName[] = "http_server";

HttpServerElement::HttpServerElement(
    const string& name,
    ElementMapper* mapper,
    const string& listen_path,
    net::Selector* selector,
    http::Server* http_server,
    const string& rpc_path,
    rpc::HttpServer* rpc_server,
    io::StateKeepUser* state_keeper,
    const string& authorizer_name)
    : ImportElement(kElementClassName, name, mapper, selector, rpc_path,
                    rpc_server, state_keeper, authorizer_name),
      ServiceInvokerHttpServerElementService(
          ServiceInvokerHttpServerElementService::GetClassName()),
      listen_path_(listen_path),
      http_server_(http_server) {
}

HttpServerElement::~HttpServerElement() {
  if ( rpc_server_ != NULL ) {
    rpc_server_->UnregisterService(rpc_path_, this);
  }
}

bool HttpServerElement::Initialize() {
  return ImportElement::Initialize() &&
         (rpc_server_ == NULL ||
          rpc_server_->RegisterService(rpc_path_, this));
}

ImportElementData* HttpServerElement::CreateNewImport(
    const string& import_name) {
  return new HttpServerImportData(mapper_, selector_, http_server_,
                                  import_name,
                                  strutil::JoinPaths(listen_path_, import_name),
                                  authorizer_name_);
}

void HttpServerElement::AddImport(rpc::CallContext<MediaOpResult>* call,
    const string& name) {
  string err;
  call->Complete(MediaOpResult(AddImport(name, true, &err), err));
}
void HttpServerElement::DeleteImport(rpc::CallContext<rpc::Void>* call,
    const string& name) {
  DeleteImport(name);
  call->Complete();
}
void HttpServerElement::GetImports(rpc::CallContext< vector<string> >* call) {
  vector<string> ret;
  GetAllImports(&ret);
  call->Complete(ret);
}

}
