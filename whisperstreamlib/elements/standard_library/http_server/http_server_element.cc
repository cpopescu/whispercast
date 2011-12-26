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

#include <whisperstreamlib/elements/standard_library/http_server/http_server_element.h>
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_encoder.h>
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_decoder.h>
#include <whisperstreamlib/elements/standard_library/http_server/import_element_inl.h>
#include <whisperstreamlib/base/saver.h>
#include <whisperstreamlib/base/tag_distributor.h>

#define ILOG(level)  LOG(level) << Info() << ": "
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
//
// We opperate in two threads:
//  - the mapper thread (which belongs to the selector)
//  - the networking thread which belongs to the http_req_->net_selector()
//
// We synchronize for closing after a call to Close() which can be called
// from either threads.
//
// The networking thread functions and members are marked clearly, and we
// only use that to pull data from the client and bring it in the mapper
// thread.
//
//
class HttpServerImportDataController;

class HttpServerImportData : private RefCounted {
 public:
  HttpServerImportData(streaming::ElementMapper* mapper,
                       net::Selector* selector,
                       http::Server* http_server,
                       const string& name,
                       Tag::Type tag_type,
                       const char* save_dir,
                       const string& url_escaped_listen_path,
                       const string& authorizer_name,
                       streaming::SplitterCreator* splitter_creator,
                       bool append_only,
                       bool disable_preemption,
                       int32 prefill_buffer_ms_,
                       int32 advance_media_ms_,
                       int32 buildup_interval_sec,
                       int32 buildup_delay_sec);


  ~HttpServerImportData();

  bool Initialize() {
    return true;
  }

  string name() const { return name_; }
  string url() const  { return url_escaped_listen_path_; }
  string Info() const {
    return (name_ + "[http://" + http_server_->name() +
            url_escaped_listen_path_ + "]: ");
  }
  const TagSplitter* splitter() const { return splitter_; }

  bool AddCallback(streaming::Request* request,
                   streaming::ProcessingCallback* callback);
  void RemoveCallback(streaming::Request* request);

  // Causes the close of this import and the eventually deletion
  void Close();

 public:
  // The media tag type we produce (mainly :)
  const streaming::Capabilities& caps() const { return caps_; }

  // If non empty we can save as we publish
  const string& save_dir() const { return save_dir_; }

 private:
  //////////////////////////////////////////////////////////////////////
  //
  // Networking thread functions
  //

  // Request processing callback - comes from http server ..
  void ProcessServerRequest(http::ServerRequest* req);
  // Called when authorization from main thread is completed
  void ReqAuthorizationCompleted(http::ServerRequest* req,
                                 bool allowed);
  // Called internally to process more data
  void ProcessMoreData();
  // Called when a request gets closed for some reason
  void NotifyRequestClosed();
  // Closes the current http request (if it matches http_req_id)
  void CloseHttpRequest(http::HttpReturnCode return_code,
                        int64 http_req_id);
  void NetThreadPause(bool pause, int64 id);

  //////////////////////////////////////////////////////////////////////
  //
  // Media mapper thread functions
  //
  // Initializes the splitter_ for a new request
  bool InitializeSplitter(int64 http_req_id);
  void CloseSplitter(io::MemoryStream* http_transfer_buffer);

  void ProcessStreamData(int64 http_req_id,
                         io::MemoryStream* http_transfer_buffer,
                         bool finalized);
  void ProcessStreamDataInternal(bool flush_parsing,
                                 io::MemoryStream* http_transfer_buffer);
  void StartAuthorization(http::ServerRequest* req,
                          streaming::AuthorizerRequest* areq,
                          streaming::AuthorizerReply* areply);
  void AuthorizationCallback(http::ServerRequest* req,
                             streaming::Authorizer* auth,
                             streaming::AuthorizerRequest* areq,
                             streaming::AuthorizerReply* areply);
  void MaybeReregisterTagTimeout(bool force);
  void MaybeUnregisterTagTimeout();
  void TagTimeoutCallback(int64 splitter_req_id);
  void CloseRequestById(http::HttpReturnCode return_code,
                        int64 splitter_req_id);
  void FinalDecRef();
  bool Pause(bool pause);

  void SetNewRequest(http::ServerRequest* req);

  static const int kTagTimeoutMs = 20000;
  static const int kTagTimeoutRegistrationGracePeriodMs = 1000;

  streaming::ElementMapper* const mapper_;
  net::Selector* const selector_;
  http::Server* const http_server_;
  const string name_;
  const Capabilities caps_;
  const string save_dir_;
  const bool append_only_;
  const bool disable_preemption_;
  const int32 prefill_buffer_ms_;
  const int32 advance_media_ms_;
  const int32 buildup_interval_sec_;
  const int32 buildup_delay_sec_;
  const string url_escaped_listen_path_;
  const string authorizer_name_;

  static const int kMaxSizeBeforeProcessing = 500000;

  streaming::SplitterCreator* const splitter_creator_;
  TagSplitter* splitter_;
  int64 splitter_req_id_;
  TagDistributor distributor_;

  Saver* saver_;
  string publish_command_;

  bool is_first_tag_;
  int64 first_tag_time_;
  int64 start_decode_time_;
  int64 start_stream_time_;
  int64 last_tag_timeout_registration_time_;
  bool is_paused_;
  Closure* tag_timeout_callback_;

  ////////////////////
  //
  // Things accessed from both threads
  //
  synch::Mutex mutex_;

  Tag::Type stream_type_;
  io::MemoryStream* http_transfer_buffer_;
  http::ServerRequest* http_req_;
  int64 http_req_id_;
  bool http_req_authorized_;
  bool http_is_paused_;

  // Callbacks for networking thread
  Closure* notify_request_closed_callback_;

  bool is_closed_;

  io::MemoryStream parse_buffer_;

  friend class HttpServerImportDataController;

  DISALLOW_EVIL_CONSTRUCTORS(HttpServerImportData);
};

class HttpServerImportDataController : public ElementController {
 public:
  HttpServerImportDataController(HttpServerImportData* master)
      : master_(master), pause_count_(0) {
  }
  ~HttpServerImportDataController() {
  }
  virtual bool DisabledSeek() const {
    return true;
  }
  virtual bool SupportsPause() const {
    return master_->splitter_ != NULL;
  }
  virtual bool Pause(bool pause) {
    if ( master_->splitter_ != NULL ) {
      if ( pause ) {
        ++pause_count_;
        if ( pause_count_ == 1 ) {
          master_->Pause(true);
        }
      } else {
        --pause_count_;
        if ( pause_count_ < 1 ) {
          master_->Pause(false);
        }
      }
    }
    return true;
  }
 private:
  HttpServerImportData* const master_;
  int pause_count_;
};


//////////////////////////////////////////////////////////////////////

http::ServerParams MakeDefaultHttpServerParams() {
  http::ServerParams p;
  // TODO(cpopescu): sort this out ..
  // p.max_reply_buffer_size_ = ;
  // p.reply_write_timeout_ms_ = ;
  // p.request_read_timeout_ms_ = ;
  // p.dlog_level_ = true;
  p.max_concurrent_connections_ = 10;
  return p;
}
const http::ServerParams
HttpServerElement::http_server_default_params_ = MakeDefaultHttpServerParams();

const char HttpServerElement::kElementClassName[] = "http_server";

HttpServerElement::HttpServerElement(
    const string& name,
    const string& id,
    streaming::ElementMapper* mapper,
    const string& url_escaped_listen_path,
    net::Selector* selector,
    const string& media_dir,
    http::Server* http_server,
    const string& rpc_path,
    rpc::HttpServer* rpc_server,
    io::StateKeepUser* state_keeper,
    streaming::SplitterCreator* splitter_creator,
    const string& authorizer_name)
    : BaseClase(kElementClassName, name, id, mapper,
                selector, media_dir,
                rpc_path, rpc_server, state_keeper, splitter_creator,
                authorizer_name),
      url_escaped_listen_path_(url_escaped_listen_path),
      http_server_(http_server) {
}

HttpServerElement::~HttpServerElement() {
}

HttpServerImportData* HttpServerElement::CreateNewImport(
    const char* import_name,
    const char* full_name,
    Tag::Type tag_type,
    const char* save_dir,
    bool append_only,
    bool disable_preemption,
    int32 prefill_buffer_ms,
    int32 advance_media_ms,
    int32 buildup_interval_sec,
    int32 buildup_delay_sec) {
  const string full_path(url_escaped_listen_path_ + "/" + import_name);
  return new HttpServerImportData(mapper_, selector_, http_server_,
                                  full_name, tag_type, save_dir,
                                  full_path,
                                  authorizer_name_,
                                  splitter_creator_,
                                  append_only,
                                  disable_preemption,
                                  prefill_buffer_ms,
                                  advance_media_ms,
                                  buildup_interval_sec,
                                  buildup_delay_sec);
}

//////////////////////////////////////////////////////////////////////

HttpServerImportData::HttpServerImportData(
    streaming::ElementMapper* mapper,
    net::Selector* selector,
    http::Server* http_server,
    const string& name,
    Tag::Type tag_type,
    const char* save_dir,
    const string& url_escaped_listen_path,
    const string& authorizer_name,
    streaming::SplitterCreator* splitter_creator,
    bool append_only,
    bool disable_preemption,
    int32 prefill_buffer_ms,
    int32 advance_media_ms,
    int32 buildup_interval_sec,
    int32 buildup_delay_sec)
    : RefCounted(&mutex_),
      mapper_(mapper),
      selector_(selector),
      http_server_(http_server),
      name_(name),
      caps_(tag_type, kDefaultFlavourMask),
      save_dir_(save_dir != NULL ? save_dir : ""),
      append_only_(append_only),
      disable_preemption_(disable_preemption),
      prefill_buffer_ms_(prefill_buffer_ms),
      advance_media_ms_(advance_media_ms),
      buildup_interval_sec_(buildup_interval_sec),
      buildup_delay_sec_(buildup_delay_sec),
      url_escaped_listen_path_(url_escaped_listen_path),
      authorizer_name_(authorizer_name),

      splitter_creator_(splitter_creator),
      splitter_(NULL),
      splitter_req_id_(-1),

      distributor_(kDefaultFlavourMask, name),

      saver_(NULL),

      is_first_tag_(true),
      first_tag_time_(0),
      start_decode_time_(0),
      start_stream_time_(0),
      last_tag_timeout_registration_time_(0),

      tag_timeout_callback_(NULL),

      stream_type_(Tag::kInvalidType),
      http_transfer_buffer_(NULL),
      http_req_(NULL),
      http_req_id_(0),
      http_req_authorized_(false),
      http_is_paused_(false),
      notify_request_closed_callback_(
          NewPermanentCallback(this,
                               &HttpServerImportData::NotifyRequestClosed)),
      is_closed_(false) {
  http_server_->RegisterProcessor(url(),
      NewPermanentCallback(this, &HttpServerImportData::ProcessServerRequest),
      true, true);
  http_server_->RegisterClientStreaming(url(), true);
  IncRef();
}


HttpServerImportData::~HttpServerImportData() {
  delete notify_request_closed_callback_;
  notify_request_closed_callback_ = NULL;
  CHECK(splitter_ == NULL);
  delete saver_;
  saver_ = NULL;
  delete http_transfer_buffer_;
  http_transfer_buffer_ = NULL;
}

//////////////////////////////////////////////////////////////////////
//
// Networking thread
//

void HttpServerImportData::SetNewRequest(http::ServerRequest* req) {
  URL* const url = req->request()->url();
  /*
  if ( http_req_ != NULL ) {
    mutex_.Unlock();
    ILOG_INFO << "Closing new publisher per old stream "
              << " publishing stream already online from: "
              << http_req_->remote_address().ToString();
    req->ReplyWithStatus(http::CONFLICT);
    return;
  }
  */
  ++http_req_id_;
  http_req_ = req;
  http_req_authorized_ = false;
  http_req_->set_closed_callback(notify_request_closed_callback_);
  mutex_.Unlock();

  vector< pair<string, string> > comp;
  {
    synch::MutexLocker l(&mutex_);
    publish_command_.clear();
    if ( url->GetQueryParameters(&comp, true) ) {
      for ( int i = 0; i < comp.size(); ++i ) {
        if ( comp[i].first == streaming::kMediaUrlParam_PublishCommand &&
             (comp[i].second == "record" ||
              comp[i].second == "append" ||
              comp[i].second == "live") ) {
          publish_command_ = comp[i].second;
        }
      }
    }
    const string content_type = req->request()->client_header()->FindField(
        http::kHeaderContentType);
    stream_type_ = GetStreamTypeFromContentType(content_type);
  }
}

void HttpServerImportData::ProcessServerRequest(http::ServerRequest* req) {
  CHECK_NOT_NULL(req);
  URL* const url = req->request()->url();
  if ( url == NULL ) {
    ILOG_WARNING << "Not there yet: " << req->ToString();
    return;
  }
  mutex_.Lock();
  if ( http_req_ == req ) {
    mutex_.Unlock();
    // continue streaming
    if ( http_req_authorized_ ) {
      ProcessMoreData();
    }
    return;
  }
  if ( disable_preemption_ && http_req_ != NULL ) {
    mutex_.Unlock();
    ILOG_WARNING << "Preemption disabled - deny new publish request: "
                 << req->ToString();
    req->ReplyWithStatus(http::CONFLICT);
    return;
  }
  // SetNewRequest(req);
  mutex_.Unlock();
  //ref_counter_.IncRef();
  IncRef();

  if ( !authorizer_name_.empty() ) {
    streaming::AuthorizerRequest* areq = new AuthorizerRequest();
    streaming::AuthorizerReply* areply = new AuthorizerReply(false);
    URL* const url = req->request()->url();

    areq->net_address_ = req->remote_address().ToString();
    areq->resource_ = url->spec();
    areq->action_ = streaming::kActionPublish;
    areq->ReadFromUrl(*url);

    if ( areq->user_.empty() ) {
      req->request()->client_header()->GetAuthorizationField(
          &areq->user_, &areq->passwd_);
    }
    //ref_counter_.IncRef();
    IncRef();
    selector_->RunInSelectLoop(
        NewCallback(this, &HttpServerImportData::StartAuthorization,
                    req, areq, areply));
  } else {
    http_req_authorized_ = true;
    if ( http_req_ != NULL ) {
      CloseHttpRequest(http::CONFLICT, http_req_id_);
    }
    SetNewRequest(req);
    ProcessMoreData();
  }
}

void HttpServerImportData::ReqAuthorizationCompleted(http::ServerRequest* req,
                                                     bool allowed) {
  //if ( ref_counter_.DecRef() ) {
  if ( DecRef() ) {
    return;  // I'm gone...
  }
  if ( req->is_orphaned() ) {
    req->clear_closed_callback();
    req->ReplyWithStatus(http::OK);
    return;
  }
  if ( !allowed ) {
    req->clear_closed_callback();
    ILOG_ERROR << "Unauthorized request received from: "
               << req->remote_address().ToString();
    req->request()->server_data()->Write("Unauthorized");
    req->ReplyWithStatus(http::UNAUTHORIZED);
    return;
  }
  if ( http_req_ != NULL ) {
    CloseHttpRequest(http::CONFLICT, http_req_id_);
  }
  SetNewRequest(req);
  http_req_authorized_ = true;
  ILOG_INFO << "New stream uploading client from: "
            << req->remote_address().ToString();
  ProcessMoreData();
}

void HttpServerImportData::ProcessMoreData() {
  CHECK(http_req_ != NULL);
  DCHECK(http_req_->net_selector()->IsInSelectThread());
  if ( !http_req_authorized_ ) {
    return;   // possibly not authorized if it comes from a 'pause'
  }

  if ( http_req_->is_orphaned() ) {
    ILOG_WARNING << "Closing on orphaned request.";
    CloseHttpRequest(http::OK, http_req_id_);
    return;
  }
  const int32 size = http_req_->request()->client_data()->Size();
  if ( !size ) {
    return;
  }
  {
    synch::MutexLocker l(&mutex_);
    if ( http_transfer_buffer_ == NULL ) {
      http_transfer_buffer_ = new io::MemoryStream();
    }
    char* buffer = new char[size];
    const int32 len = http_req_->request()->client_data()->Read(buffer, size);
    http_transfer_buffer_->AppendRaw(buffer, len);
  }
  //ref_counter_.IncRef();
  IncRef();

  selector_->RunInSelectLoop(
      NewCallback(this, &HttpServerImportData::ProcessStreamData,
                  http_req_id_, http_transfer_buffer_,
                  http_req_->is_parsing_finished()));
  if ( http_req_->is_parsing_finished() ) {
    CloseHttpRequest(http::OK, http_req_id_);
  }
  if ( http_is_paused_ ) {
    http_req_->PauseReading();
  }
}

void HttpServerImportData::NotifyRequestClosed() {
  ILOG_INFO << "Client request closed.";
  CloseHttpRequest((http::HttpReturnCode)0, http_req_id_);
}

void ClearReq(http::ServerRequest* req,
              http::HttpReturnCode return_code) {
  req->clear_closed_callback();

  LOG_INFO << "Closing stream uploading client from: "
           << req->remote_address().ToString()
           << " with http_return_code: "
           << http::GetHttpReturnCodeName(return_code);
  if ( return_code != 0 ) {
    req->ReplyWithStatus(return_code);
  }
}

void HttpServerImportData::CloseHttpRequest(http::HttpReturnCode return_code,
                                            int64 http_req_id) {
  {
    synch::MutexLocker l(&mutex_);
    if ( http_req_id_ != http_req_id ) {
      return;   // somehow closed already
    }
    CHECK(http_req_ != NULL);
    http_req_->net_selector()->RunInSelectLoop(
        NewCallback(&ClearReq, http_req_, return_code));
    // DCHECK(http_req_->net_selector()->IsInSelectThread());
  }

  //ref_counter_.IncRef();
  IncRef();
  selector_->RunInSelectLoop(
      NewCallback(this,
                  &HttpServerImportData::ProcessStreamData,
                  int64(-1),
                  http_transfer_buffer_,
                  true));
  {
    synch::MutexLocker l(&mutex_);
    http_req_ = NULL;
    http_req_authorized_ = false;
    http_transfer_buffer_ = NULL;
    http_is_paused_ = false;
    ++http_req_id_;
  }
  //ref_counter_.DecRef();
  DecRef();
}
void HttpServerImportData::NetThreadPause(bool pause, int64 id) {
  //if ( ref_counter_.DecRef() ) {
  if ( DecRef() ) {
    return;
  }
  {
    synch::MutexLocker l(&mutex_);
    http_is_paused_ = pause;
    if ( id != http_req_id_ || id == -1 || http_req_ == NULL ) {
      return;
    }
  }
  if ( pause ) {
    http_req_->PauseReading();
  } else {
    http_req_->ResumeReading();
    ProcessMoreData();
  }
}

//////////////////////////////////////////////////////////////////////
//
// Media Mapper thread functions
//

bool HttpServerImportData::AddCallback(
    streaming::Request* request,
    streaming::ProcessingCallback* callback) {
  distributor_.add_callback(request, callback);
  return true;
}
void HttpServerImportData::RemoveCallback(streaming::Request* request) {
  distributor_.remove_callback(request);
}
bool HttpServerImportData::InitializeSplitter(int64 http_req_id) {
  CHECK(splitter_ == NULL);
  if ( stream_type_ == caps_.tag_type_ ||
       caps_.tag_type_ == Tag::TYPE_RAW ) {
    splitter_ = splitter_creator_->CreateSplitter(name_, caps_.tag_type_);
  }
  if ( splitter_ == NULL ) {
    ILOG_ERROR << "Failed to create splitter";
    return false;
  }
  splitter_req_id_ = http_req_id;
  ILOG_INFO << "Splitter " << splitter_ << " created";
  string publish_command;
  {
    synch::MutexLocker l(&mutex_);
    publish_command = publish_command_;
  }

  if ( !save_dir_.empty() && saver_ == NULL &&
       (publish_command == "record" || publish_command == "append") ) {
    timer::Date now(false);
    string dirname(string("publish_") + now.ToShortString());
    if ( append_only_ ) {
      dirname = "";
    } else if ( publish_command == "append" ) {
      re::RE refile("^publish_.*");
      vector<string> dirs;
      io::DirList(save_dir_, &dirs, true, &refile);
      if ( !dirs.empty() ) {
        dirname = dirs.back();
      }
    }
    saver_ = new Saver(
        Info(), NULL, caps_.tag_type_, name(),
        save_dir_ + "/" + dirname,
        now.GetTime(), false, NULL);
    saver_->CreateSignalingFile(".save_done",
                                strutil::StringPrintf("buildup\n%d\n%d",
                                                      buildup_interval_sec_,
                                                      buildup_delay_sec_));
    saver_->CreateSignalingFile(".buildup", "");
  }
  if ( saver_ != NULL ) {
    saver_->StartSaving();
    saver_->CreateSignalingFile(".save_done", "buildup");
  }
  start_stream_time_ = selector_->now();
  is_first_tag_ = true;
  start_decode_time_ = 0;
  first_tag_time_ = 0;

  MaybeReregisterTagTimeout(true);

  return true;
}

void HttpServerImportData::CloseSplitter(
    io::MemoryStream* http_transfer_buffer) {
  if ( http_transfer_buffer != NULL ) {
    // Flush current stuff..
    ProcessStreamDataInternal(true, http_transfer_buffer);
  }

  distributor_.Reset();

  delete splitter_;
  splitter_ = NULL;

  splitter_req_id_ = -1;
  MaybeUnregisterTagTimeout();
}

void HttpServerImportData::ProcessStreamData(
    int64 http_req_id,
    io::MemoryStream* http_transfer_buffer,
    bool finalized) {
  //if ( ref_counter_.DecRef() ) {
  if ( DecRef() ) {
    return;
  }
  if ( is_closed_ ) {
    return;
  }
  //
  // TODO: we may want to use another buffer ...
  //
  if ( splitter_ != NULL &&
       splitter_req_id_ != http_req_id ) {
    CloseSplitter(http_transfer_buffer);
  }
  if ( http_req_id < 0 ) {
    delete http_transfer_buffer;
    parse_buffer_.Clear();
    return;    // was just a close signal..
  }
  if ( splitter_ == NULL ) {
    if ( !InitializeSplitter(http_req_id) ) {
      CloseRequestById(http::NOT_ACCEPTABLE, http_req_id);
      return;
    }
  }
  CHECK_EQ(splitter_req_id_, http_req_id);
  ProcessStreamDataInternal(finalized, http_transfer_buffer);
}

bool HttpServerImportData::Pause(bool pause) {
  //ref_counter_.IncRef();
  IncRef();
  net::Selector* selector = NULL;
  int64 id = 0;
  {
    synch::MutexLocker l(&mutex_);
    if ( http_req_ != NULL ) {
      selector = http_req_->net_selector();
      id = http_req_id_;
    }
  }
  is_paused_ = pause;
  if ( selector ) {
    selector->RunInSelectLoop(
        NewCallback(this, &HttpServerImportData::NetThreadPause, pause, id));
  }
  return true;
}

void HttpServerImportData::ProcessStreamDataInternal(
    bool flush_parsing,
    io::MemoryStream* http_transfer_buffer) {
  {
    synch::MutexLocker l(&mutex_);
    parse_buffer_.AppendStream(http_transfer_buffer);
  }
  if ( start_decode_time_ == 0 ) {
    if ( !flush_parsing &&
         selector_->now() - start_stream_time_ < prefill_buffer_ms_ &&
         parse_buffer_.Size() < kMaxSizeBeforeProcessing ) {
      return;
    }
    start_decode_time_ = selector_->now();
  }

  while ( true ) {
    int64 timestamp_ms;
    scoped_ref<Tag> tag;
    streaming::TagReadStatus status =
        splitter_->GetNextTag(&parse_buffer_,
            &tag, &timestamp_ms, flush_parsing);

    if ( status == streaming::READ_NO_DATA ) {
      break;
    }
    if ( status == streaming::READ_SKIP ) {
      continue;
    }
    if ( status != streaming::READ_OK ) {
      ILOG_ERROR << "Error reading next tag, status: "
                 << streaming::TagReadStatusName(status);
      if ( !flush_parsing ) {
        CloseRequestById(http::UNSUPPORTED_MEDIA_TYPE, splitter_req_id_);
      }
      return;
    }

    distributor_.DistributeTag(tag.get(), timestamp_ms);

    if ( saver_ != NULL ) {
      saver_->ProcessTag(tag.get(), timestamp_ms);
    }

    if ( !flush_parsing ) {
      MaybeReregisterTagTimeout(false);
      if ( is_first_tag_ ) {
        is_first_tag_ = false;
        first_tag_time_ = timestamp_ms;
      } else {
        const int64 delta = (timestamp_ms - first_tag_time_) -
                            (selector_->now() - start_decode_time_);
        if ( delta > advance_media_ms_ ) {
          break;
        }
      }
    }

    if ( is_paused_ && !flush_parsing ) {
      break;
    }
  }
  if ( flush_parsing ) {
    parse_buffer_.Clear();
  }
}

//////////////////////////////////////////////////////////////////////

void HttpServerImportData::StartAuthorization(
    http::ServerRequest* req,
    streaming::AuthorizerRequest* areq,
    streaming::AuthorizerReply* areply) {
  streaming::Authorizer* const auth =
      mapper_->GetAuthorizer(authorizer_name_);
  if ( auth == NULL ) {
    delete areq;
    delete areply;
    req->net_selector()->RunInSelectLoop(
        NewCallback(this,
                    &HttpServerImportData::ReqAuthorizationCompleted,
                    req, true));
  } else {
    auth->IncRef();
    auth->Authorize(*areq, areply,
                    NewCallback(
                        this,
                        &HttpServerImportData::AuthorizationCallback,
                        req, auth, areq, areply));
  }
}

void HttpServerImportData::AuthorizationCallback(
    http::ServerRequest* req,
    streaming::Authorizer* auth,
    streaming::AuthorizerRequest* areq,
    streaming::AuthorizerReply* areply) {
  auth->DecRef();
  const bool allowed = (areply == NULL || areply->allowed_);
  delete areq;
  delete areply;
  req->net_selector()->RunInSelectLoop(
      NewCallback(this,  &HttpServerImportData::ReqAuthorizationCompleted,
                  req, allowed));
}

//////////////////////////////////////////////////////////////////////

void HttpServerImportData::MaybeReregisterTagTimeout(bool force) {
  CHECK_GE(splitter_req_id_, 0);
  if ( tag_timeout_callback_ == NULL ) {
    force = true;
    tag_timeout_callback_ = NewCallback(
        this, &HttpServerImportData::TagTimeoutCallback,
        splitter_req_id_);
  }
  if ( force
       || ((last_tag_timeout_registration_time_ +
            kTagTimeoutRegistrationGracePeriodMs) < selector_->now()) ) {
    last_tag_timeout_registration_time_ = selector_->now();
    selector_->RegisterAlarm(tag_timeout_callback_,
                               kTagTimeoutMs + prefill_buffer_ms_);
  }
}
void HttpServerImportData::MaybeUnregisterTagTimeout() {
  if ( tag_timeout_callback_ ) {
    selector_->UnregisterAlarm(tag_timeout_callback_);
    delete tag_timeout_callback_;
    tag_timeout_callback_ = NULL;
  }
}

void HttpServerImportData::TagTimeoutCallback(int64 splitter_req_id) {
  tag_timeout_callback_ = NULL;
  if ( splitter_req_id_ == splitter_req_id ) {
    CloseRequestById(http::REQUEST_TIME_OUT, splitter_req_id);
  }
}

void HttpServerImportData::CloseRequestById(http::HttpReturnCode return_code,
                                            int64 splitter_req_id) {
  net::Selector* selector = NULL;
  {
    synch::MutexLocker l(&mutex_);
    if ( splitter_req_id == http_req_id_ && http_req_ != NULL ) {
      selector = http_req_->net_selector();
    }
  }
  if ( selector ) {
    selector->RunInSelectLoop(
        NewCallback(this, &HttpServerImportData::CloseHttpRequest,
                    return_code, splitter_req_id));
  }
}

void HttpServerImportData::Close() {
  http_server_->UnregisterProcessor(url());
  is_closed_ = true;
  MaybeUnregisterTagTimeout();

  distributor_.CloseAllCallbacks(true);
  if ( splitter_req_id_ >= 0 ) {
    CloseRequestById(http::OK, splitter_req_id_);
  }
  // Run this after a while to allow any concurrent CloseHttpRequest to get
  // executed
  selector_->RegisterAlarm(NewCallback(this,
                                       &HttpServerImportData::FinalDecRef),
                           1000);
}
void HttpServerImportData::FinalDecRef() {
  CloseSplitter(NULL);
  //ref_counter_.DecRef();
  DecRef();
}

}
