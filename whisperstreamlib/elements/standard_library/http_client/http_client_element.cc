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
// Author: Catalin Popescu

#include <whisperlib/common/base/types.h>
#include WHISPER_HASH_MAP_HEADER
#include <whisperlib/net/http/http_header.h>
#include <whisperlib/net/http/http_request.h>
#include <whisperlib/common/base/timer.h>

#include <whisperstreamlib/base/tag_distributor.h>
#include <whisperstreamlib/elements/standard_library/http_client/http_client_element.h>

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

//////////////////////////////////////////////////////////////////////

namespace streaming {

class HttpClientElementData : public ElementController {
 public:
  HttpClientElementData(
    net::Selector* selector,
    const net::NetFactory& net_factory,
    net::PROTOCOL net_protocol,
    const string& name,
    const string& server_name,
    uint16 server_port,
    const string& url_escaped_query_path,
    bool should_reopen,
    bool fetch_only_on_request,
    const http::ClientParams* client_params,
    int32 prefill_buffer_ms,
    int32 advance_media_ms,
    int media_http_maximum_tag_size);

  ~HttpClientElementData();

  const TagSplitter* splitter() const { return splitter_; }
  TagSplitter* mutable_splitter() { return splitter_; }

  string name() const { return name_; }

  bool AddCallback(streaming::Request* req,
                   streaming::ProcessingCallback* callback);
  void RemoveCallback(streaming::Request* req);

  void SetRemoteUser(const string& user_name, const string& password) {
    remote_user_name_ = user_name;
    remote_password_ = password;
  }

  // Initiates a new request
  void Start();
  // Closes this element
  void Close(Closure* call_on_close);

 public:
  virtual bool SupportsPause() const { return true; }

  virtual bool Pause(bool pause) {
    LOG_ERROR << "HTTP PAUSE " << pause;
    if ( http_proto_ != NULL ) {
      DCHECK(http_req_ != NULL);
      if ( pause ) {
        ++pause_count_;
        if ( pause_count_ == 1 ) {
         http_proto_->PauseReading();
        }
      } else {
        --pause_count_;
        if ( pause_count_ < 1 ) {
          if ( !http_req_->is_finalized() ) {
            http_proto_->ResumeReading();
          } else {
            selector_->RunInSelectLoop(
                NewCallback(this,
                    &HttpClientElementData::ProcessFinalizedRequest));
          }
        }
      }
    }
    return true;
  }

 private:
  string Info() const {
    return strutil::StringPrintf("%s[http://%s:%d%s]: ",
             name_.c_str(),
             server_name_.c_str(),
             server_port_,
             url_escaped_query_path_.c_str());
  }

  // Callback when some event happened on the client request
  void ProcessMoreData();

  // Ran when the client request finishes and we may retry it
  void ProcessFinalizedRequest();

  // Finalizes a request, and maybe retries it after retry_ms
  // (possibly closing the splitter that we have under control)
  void CloseRequest(int retry_ms);
  // Terminates a request, and never retries
  // (possibly closing the splitter that we have under control)
  void TerminateRequest();

  // Creates the spliter based on the request header
  bool InitializeSplitter();

  // Verifies if the http status code looks OK and we can continue the
  // request
  bool VerifyHttpStatusCode(http::HttpReturnCode code);

  // Processes what's in the buffer currently..
  void ProcessStreamData();

  static const int kRetryGoodClose = 100;
  static const int kRetryOnUnreachable = 5000;
  static const int kRetryOnBadSplitter = 10000;
  static const int kRetryOnBadRequest = 20000;
  static const int kRetryOnWrongParams = 30000;
  static const int kRetryOn5xx = 5000;

  // We make this 3 seconds, and in whisperer we use a lingering time
  // for elements of 2 seconds (anyway, less)
  static const int kRetryOnErrorCorruptedData = 3000;

  static const int kMaxSizeBeforeProcessing = 500000;

  //////////////////////////////////////////////////////////////////////

  net::Selector* const selector_;            // runs us
  const net::NetFactory& net_factory_;       // creates TCP or SSL connections
                                             // as base for HTTP
  net::PROTOCOL net_protocol_;               // SSL or TCP
  const string name_;                        // our name
  const string server_name_;                 // we connect to this server
  const uint16 server_port_;                 // we connect on this port
  const string url_escaped_query_path_;      // and ask for this path
  const bool should_reopen_;                 // do we reopen URL on error ?
  const bool fetch_only_on_request_;         // fetch media only when we have
                                             // consumers.

  const http::ClientParams* const client_params_;
                                              // how to behave (HTTP wise)
  Closure* call_on_close_;
                                              // upon closure we call this
                                              // notification callback

  TagSplitter* splitter_;                     // splits media into tags for us
  TagDistributor distributor_;                // manages clients

  http::ClientRequest* http_req_;             // outstanding request
  http::ClientStreamReceiverProtocol* http_proto_;
                                              // does the protocol related stuff
                                              // on http_req_
  bool http_header_checked_;                  // did we pass the header OK ?
  Closure* streaming_callback_;               // processes the stream from the
                                              // remote server
  Closure* start_request_callback_;           // called upon conn establishment

  string remote_user_name_;                   // If set we use this user for
                                              // basic authentication
  string remote_password_;                    // If set we use this pass for
                                              // basic authentication


  // Parameters: how we behave - check standard_library.rpc

  const int32 prefill_buffer_ms_;
  const int32 advance_media_ms_;
  const int media_http_maximum_tag_size_;

  bool is_first_tag_;
  int64 first_tag_time_;
  int64 start_decode_time_;
  int64 start_stream_time_;

  int pause_count_;

  DISALLOW_EVIL_CONSTRUCTORS(HttpClientElementData);
};

//////////////////////////////////////////////////////////////////////

const char HttpClientElement::kElementClassName[] = "http_client";

HttpClientElement::HttpClientElement(
  const string& name,
  ElementMapper* mapper,
  net::Selector* selector,
  int32 prefill_buffer_ms,
  int32 advance_media_ms,
  int media_http_maximum_tag_size)
    : Element(kElementClassName, name, mapper),
      selector_(selector),
      net_factory_(selector_),
      prefill_buffer_ms_(prefill_buffer_ms),
      advance_media_ms_(advance_media_ms),
      media_http_maximum_tag_size_(media_http_maximum_tag_size),
      call_on_close_(NULL) {

  net::TcpConnectionParams tcp_params(
      net_factory_.tcp_connection_params().block_size_,
      net_factory_.tcp_connection_params().recv_buffer_size_,
      net_factory_.tcp_connection_params().send_buffer_size_,
      net_factory_.tcp_connection_params().shutdown_linger_timeout_ms_,
      //net_factory_.tcp_connection_params().read_limit_,
      8192,
      //net_factory_.tcp_connection_params().write_limit_
      8192
  );
  net_factory_.SetTcpConnectionParams(tcp_params);

  default_params_.max_chunk_size_ = 1 << 18;
}

HttpClientElement::~HttpClientElement() {
  DCHECK(elements_.empty());
}

bool HttpClientElement::AddRequest(const string& media, Request* req,
                                   ProcessingCallback* callback) {
  CHECK(callback->is_permanent());
  ElementMap::const_iterator it = elements_.find(media);
  if ( it == elements_.end() ) {
    ILOG_ERROR << "Cannot find media: [" << media << "], looking through: "
               << strutil::ToStringKeys(elements_);
    return false;
  }

  if ( it->second->AddCallback(req, callback) ) {
    requests_.insert(make_pair(req, it->second));
    return true;
  }
  ILOG_ERROR << "The subelement: [" << it->second->name() << "]"
                " refused callback on media: [" << media << "]";
  return false;
}

void HttpClientElement::RemoveRequest(streaming::Request* req) {
  RequestMap::iterator it = requests_.find(req);
  if ( it == requests_.end() ) {
    return;
  }
  HttpClientElementData* data = it->second;
  requests_.erase(it);

  data->RemoveCallback(req);
}

bool HttpClientElement::HasMedia(const string& media) {
  return elements_.find(media) != elements_.end();
}

void HttpClientElement::ListMedia(const string& media_dir,
                                  vector<string>* out) {
  for ( ElementMap::const_iterator it = elements_.begin();
        it != elements_.end(); ++it ) {
    if ( media_dir == "" || it->first == media_dir ) {
      out->push_back(it->first);
    }
  }
}

bool HttpClientElement::DescribeMedia(const string& media,
                                      MediaInfoCallback* callback) {
  ElementMap::const_iterator it = elements_.find(media);
  if ( it == elements_.end() ||
       it->second->splitter() == NULL ) {
    return false;
  }
  callback->Run(&(it->second->splitter()->media_info()));
  return true;
}

bool HttpClientElement::AddElement(const string& ename,
                                   const string& server_name,
                                   const uint16 server_port,
                                   const string& url_escaped_query_path,
                                   bool should_reopen,
                                   bool fetch_only_on_request,
                                   const http::ClientParams* client_params) {
  const string full_name = name() + "/" + ename;
  ElementMap::const_iterator it = elements_.find(full_name);
  if ( it != elements_.end() ) {
    return false;
  }
  HttpClientElementData* const src = new HttpClientElementData(
    selector_, net_factory_, net::PROTOCOL_TCP,
    full_name,
    server_name,
    server_port,
    url_escaped_query_path,
    should_reopen,
    fetch_only_on_request,
    client_params == NULL ? &default_params_ : client_params,
    prefill_buffer_ms_,
    advance_media_ms_,
    media_http_maximum_tag_size_);

  elements_.insert(make_pair(string(full_name), src));
  if ( !fetch_only_on_request ) {
    selector_->RunInSelectLoop(
        NewCallback(src, &HttpClientElementData::Start));
  }
  return true;
}

bool HttpClientElement::SetElementRemoteUser(const string& ename,
                                             const string& user_name,
                                             const string& password) {
  const string full_name = name() + "/" + ename;
  ElementMap::const_iterator it = elements_.find(full_name);
  if ( it == elements_.end() ) {
    return false;
  }
  it->second->SetRemoteUser(user_name, password);
  return true;
}

string HttpClientElement::Info() const {
  return name();
}

void HttpClientElement::ClosedElement(HttpClientElementData* data) {
  elements_.erase(data->name());
  selector_->DeleteInSelectLoop(data);

  if ( elements_.empty() && call_on_close_ != NULL ) {
    selector_->RunInSelectLoop(call_on_close_);
    call_on_close_ = NULL;
  }
}

bool HttpClientElement::DeleteElement(const string& ename) {
  const string full_name = name() + "/" + ename;
  ElementMap::const_iterator it = elements_.find(full_name);
  if ( it == elements_.end() ) {
    return false;
  }
  it->second->Close(NewCallback(this, &HttpClientElement::ClosedElement, it->second));
  return true;
}

void HttpClientElement::CloseAllElements() {
  vector<HttpClientElementData*> elements;
  for (ElementMap::const_iterator it = elements_.begin();
      it != elements_.end(); ++it) {
    elements.push_back(it->second);
  }

  for (vector<HttpClientElementData*>::const_iterator itv = elements.begin();
      itv != elements.end(); ++itv) {
    (*itv)->Close(NewCallback(this, &HttpClientElement::ClosedElement, *itv));
  }
}
void HttpClientElement::Close(Closure* call_on_close) {
  DCHECK(call_on_close_ == NULL);
  call_on_close_ = call_on_close;
  selector_->RunInSelectLoop(NewCallback(this,
      &HttpClientElement::CloseAllElements));
}


//////////////////////////////////////////////////////////////////////

HttpClientElementData::HttpClientElementData(
    net::Selector* selector,
    const net::NetFactory& net_factory,
    net::PROTOCOL net_protocol,
    const string& name,
    const string& server_name,
    uint16 server_port,
    const string& url_escaped_query_path,
    bool should_reopen,
    bool fetch_only_on_request,
    const http::ClientParams* client_params,
    int32 prefill_buffer_ms,
    int32 advance_media_ms,
    int media_http_maximum_tag_size)
    : selector_(selector),
      net_factory_(net_factory),
      net_protocol_(net_protocol),
      name_(name),
      server_name_(server_name),
      server_port_(server_port),
      url_escaped_query_path_(url_escaped_query_path),
      should_reopen_(should_reopen),
      fetch_only_on_request_(fetch_only_on_request),
      client_params_(client_params),
      call_on_close_(NULL),
      splitter_(NULL),
      distributor_(kDefaultFlavourMask, name),
      http_req_(NULL),
      http_proto_(NULL),
      http_header_checked_(false),
      streaming_callback_(
          NewPermanentCallback(
              this, &HttpClientElementData::ProcessMoreData)),
      start_request_callback_(
          NewPermanentCallback(
              this, &HttpClientElementData::Start)),
      prefill_buffer_ms_(prefill_buffer_ms),
      advance_media_ms_(advance_media_ms),
      media_http_maximum_tag_size_(media_http_maximum_tag_size),
      is_first_tag_(true),
      first_tag_time_(0),
      start_decode_time_(0),
      start_stream_time_(0),
      pause_count_(0) {
}

HttpClientElementData::~HttpClientElementData() {
  DCHECK(call_on_close_ == NULL);

  delete start_request_callback_;
  delete streaming_callback_;
  delete splitter_;
  delete http_proto_;
  delete http_req_;
}

void HttpClientElementData::CloseRequest(int retry_ms) {
  ILOG_DEBUG << "Closing w/ " << distributor_.count()
             << " callbacks, retry in: " << retry_ms << " ms";

  bool closing = (call_on_close_ != NULL);
  if ( !should_reopen_ || closing || (retry_ms == 0) ) {
    distributor_.CloseAllCallbacks(closing);
  } else {
    distributor_.Reset();
  }

  if ( http_proto_ != NULL ) {
    DCHECK(http_req_ != NULL);

    selector_->DeleteInSelectLoop(http_proto_);
    http_proto_ = NULL;

    selector_->DeleteInSelectLoop(http_req_);
    http_req_ = NULL;
  }

  http_header_checked_ = false;

  delete splitter_;
  splitter_ = NULL;

  if ( should_reopen_ && !closing && (retry_ms > 0) ) {
    if ( !fetch_only_on_request_ ||
         (fetch_only_on_request_ && (distributor_.count() > 0)) ) {
      ILOG_DEBUG << "Restarting the request in " << retry_ms << " ms";
      selector_->RegisterAlarm(start_request_callback_, retry_ms);
      return;
    }
    ILOG_DEBUG << "Closed - waiting for callbacks to restart the request";
  }

  selector_->UnregisterAlarm(start_request_callback_);

  if ( closing && (distributor_.count() == 0) ) {
    selector_->RunInSelectLoop(call_on_close_);
    call_on_close_ = NULL;
  }
}
void HttpClientElementData::TerminateRequest() {
  if ( http_proto_ != NULL && http_proto_->connection() != NULL ) {
    http_proto_->connection()->ForceClose();
  } else {
    CloseRequest(0);   // should not restart actually...
  }
}

void HttpClientElementData::ProcessMoreData() {
  CHECK(http_req_ != NULL);
  if ( http_req_->is_finalized() ) {
    ProcessFinalizedRequest();
    return;
  }
  if ( !http_header_checked_ ) {
    if ( !VerifyHttpStatusCode(
           http_req_->request()->server_header()->status_code()) ) {
      return;
    }
    http_header_checked_ = true;
  }

  if ( splitter_ == NULL ) {
    if ( !InitializeSplitter() ) {
      return;
    }
  }
  ProcessStreamData();
}

void HttpClientElementData::ProcessFinalizedRequest() {
  if ( http_req_ != NULL) {
    if ( !VerifyHttpStatusCode(
           http_req_->request()->server_header()->status_code()) ) {
      return;
    }
    if ( splitter_ != NULL ) {
      ProcessStreamData();
    }
  }
}


bool HttpClientElementData::InitializeSplitter() {
  CHECK(splitter_ == NULL);
  const string content_type =
    http_req_->request()->server_header()->FindField(http::kHeaderContentType);
  MediaFormat media_format;
  if ( !MediaFormatFromContentType(content_type, &media_format) ) {
    LOG_ERROR << "Unrecognized content_type: [" << content_type << "]";
    distributor_.CloseAllCallbacks(false);
    CloseRequest(kRetryOnBadSplitter);
    return false;
  }
  ILOG_DEBUG << "Stream format: [" << MediaFormatName(media_format) << "]";
  splitter_ = CreateSplitter(name_, media_format);
  is_first_tag_ = true;
  start_decode_time_ = 0;
  first_tag_time_ = 0;

  return true;
}

bool HttpClientElementData::VerifyHttpStatusCode(http::HttpReturnCode code) {
  if ( code >= 200 && code < 300 ) {
    return true;
  }
  ILOG_ERROR << "Response received, code: " << code << " ("
             << http::GetHttpReturnCodeName(code) << ") on uri: ["
             << http_req_->request()->client_header()->uri() << "]";
  if ( code == 0 ) {
    CloseRequest(kRetryOnUnreachable);
    return false;
  }
  if ( code >= 500 ) {
    CloseRequest(kRetryOn5xx);
    return false;
  }
  CloseRequest(kRetryOnBadRequest);
  return false;
}

void HttpClientElementData::ProcessStreamData() {
  DCHECK(splitter_ != NULL);
  if ( start_decode_time_ == 0 ) {
    if ( (selector_->now() - start_stream_time_) < prefill_buffer_ms_ &&
         http_req_->request()->server_data()->Size() < kMaxSizeBeforeProcessing ) {
      if ( !http_req_->is_finalized() ||
          ( (http_req_->request()->server_data()->Size() > 0) &&
              ( ( distributor_.count() > 0 ) || !fetch_only_on_request_) ) ) {
        return;
      }
      CloseRequest(kRetryGoodClose);
      return;
    }
    start_decode_time_ = selector_->now();
  }

  while ( true ) {
    int64 timestamp_ms;
    scoped_ref<Tag> tag;
    TagReadStatus status = splitter_->GetNextTag(
        http_req_->request()->server_data(),
        &tag, &timestamp_ms, http_req_->is_finalized());
    if ( status == streaming::READ_EOF ||
         status == streaming::READ_CORRUPTED ) {
      ILOG_ERROR << "Error reading tag, status: " << TagReadStatusName(status)
                 << ", http error code: " << http_req_->error_name();
      CloseRequest(kRetryOnErrorCorruptedData);
      return;
    }
    if ( status == streaming::READ_NO_DATA ) {
      if ( http_req_->request()->server_data()->Size()  >
           media_http_maximum_tag_size_ ) {
        ILOG_ERROR << "Oversized buffer size: "
                   << http_req_->request()->server_data()->Size();
        CloseRequest(kRetryOnErrorCorruptedData);
        return;
      }
      break;
    }
    if ( status == streaming::READ_SKIP ) {
      ILOG_DEBUG << "Skipped a tag..";
      continue;
    }
    if ( status != streaming::READ_OK ) {
      ILOG_ERROR << "Error reading tag, status: " << TagReadStatusName(status);
      continue;
    }

    distributor_.DistributeTag(tag.get(), timestamp_ms);

    if ( status == streaming::READ_OK ) {
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
  }

  if ( !http_req_->is_finalized() ||
      ( (http_req_->request()->server_data()->Size() > 0) &&
          ( ( distributor_.count() > 0 ) || !fetch_only_on_request_) ) ) {
    return;
  }
  CloseRequest(kRetryGoodClose);
}

bool HttpClientElementData::AddCallback(streaming::Request* req,
                 streaming::ProcessingCallback* callback) {
  distributor_.add_callback(req, callback);

  if ( http_req_ == NULL && (distributor_.count() == 1) ) {
    selector_->RegisterAlarm(start_request_callback_, 0);
  }

  req->set_controller(this);
  return true;
}
void HttpClientElementData::RemoveCallback(streaming::Request* req) {
  distributor_.remove_callback(req);

  if ( distributor_.count() == 0 ) {
    if ( call_on_close_ != NULL || fetch_only_on_request_ ) {
      TerminateRequest();
    }
  }
}

void HttpClientElementData::Start() {
  CHECK(http_req_ == NULL);
  CHECK(http_proto_ == NULL);
  selector_->UnregisterAlarm(start_request_callback_);

  ILOG_DEBUG << "Starting request, server_name_: [" << server_name_
           << "], host: " << server_name_ << ":" << server_port_
           << ", url: [" << url_escaped_query_path_
           << "], callbacks: " << distributor_.count();

  http_proto_ = new http::ClientStreamReceiverProtocol(
      client_params_,
      new http::SimpleClientConnection(selector_, net_factory_, net_protocol_),
      net::HostPort(server_name_, server_port_));
  http_req_ = new http::ClientRequest(http::METHOD_GET,
                                      url_escaped_query_path_);
  if ( !remote_user_name_.empty() || !remote_password_.empty() ) {
    if ( !http_req_->request()->client_header()->SetAuthorizationField(
             remote_user_name_, remote_password_) ) {
      ILOG_WARNING << "Error seting remote user/pass in http request";
    }
  }

  pause_count_ = 0;

  http_header_checked_ = false;
  http_proto_->BeginStreamReceiving(http_req_, streaming_callback_);
}

void HttpClientElementData::Close(Closure* call_on_close) {
  DCHECK(call_on_close_ == NULL);
  call_on_close_ = call_on_close;

  TerminateRequest();
}
}
