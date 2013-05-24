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

#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/base/scoped_ptr.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/io/file/file_input_stream.h>
#include <whisperlib/net/rpc/lib/codec/rpc_codec.h>
#include <whisperlib/net/rpc/lib/server/rpc_http_processor.h>
#include <whisperlib/net/rpc/lib/server/rpc_service_invoker.h>
#include <whisperlib/net/rpc/lib/rpc_constants.h>

// Defined in rpc_http_server.cc
DECLARE_string(rpc_js_form_path);
// Defined in rpc_http_server.cc
DECLARE_bool(rpc_enable_http_get);

DEFINE_bool(rpc_enable_http_gzip, true,
            "If true, RPC Http Responses are gzipped, taking less bandwidth.\n"
            "False is useful for debugging.");

namespace rpc {

//////////////////////////////////////////////////////////////////////

HttpProcessor::HttpProcessor(ServicesManager& manager,
                             IAsyncQueryExecutor& executor,
                             bool enable_auto_forms)
    : IResultHandler(),
      http_server_(NULL),
      http_path_(),
      authenticator_(NULL),
      services_manager_(manager),
      async_query_executor_(executor),
      registered_to_query_executor_(false),
      requests_in_execution_(),
      access_requests_in_execution_(),
      auto_forms_js_(),
      auto_forms_log_() {
  async_query_executor_.RegisterResultHandler(*this);
  registered_to_query_executor_ = true;

  if ( enable_auto_forms && !FLAGS_rpc_js_form_path.empty() ) {
    const string rpc_base_js = io::FileInputStream::ReadFileOrDie(
        strutil::JoinPaths(FLAGS_rpc_js_form_path, "rpc_base.js"));
    const string rpc_standard_js = io::FileInputStream::ReadFileOrDie(
        strutil::JoinPaths(FLAGS_rpc_js_form_path, "rpc_standard.js"));
    auto_forms_js_ = ("<script language=\"JavaScript1.1\">\n" +
                      rpc_base_js + "\n" +
                      rpc_standard_js + "\n" +
                      "</script>\n");
    auto_forms_log_ = io::FileInputStream::ReadFileOrDie(
        strutil::JoinPaths(FLAGS_rpc_js_form_path, "auto_forms_log.html"));
  }
}

HttpProcessor::~HttpProcessor() {
  if ( http_server_ != NULL ) {
    http_server_->UnregisterProcessor(http_path_);
  }
  if ( registered_to_query_executor_ ) {
    async_query_executor_.UnregisterResultHandler(*this);
    registered_to_query_executor_ = false;
  }
}

// TODO: a mechanism to detach is also needed
bool HttpProcessor::AttachToServer(http::Server* server,
                                        const string& process_path,
                                        net::UserAuthenticator* authenticator) {
  CHECK_NULL(http_server_) << "Duplicate attach to http server";
  http_server_ = server;
  http_server_->RegisterProcessor(process_path,
      NewPermanentCallback(this, &HttpProcessor::CallbackProcessHttpRequest),
      true, true);

  if ( strutil::StrEndsWith(process_path, "/") ) {
    LOG_WARNING << "AttachToServer on a path ending with '/'";
    http_path_ = process_path.substr(0, process_path.length() - 1);
  } else {
    http_path_ = process_path;
  }

  authenticator_ = authenticator;
  return true;
}


//////////////////////////////////////////////////////////////////////
//
//     Methods available only from the selector thread.
//
//
// TODO(cosmin): this function is too long - split it ..
void HttpProcessor::CallbackProcessHttpRequest(http::ServerRequest* req) {
  req->AuthenticateRequest(authenticator_,
      NewCallback(this,
          &HttpProcessor::CallbackProcessHttpRequestAuthorized, req));
}

void HttpProcessor::CallbackProcessHttpRequestAuthorized(
    http::ServerRequest* req, net::UserAuthenticator::Answer auth_answer) {
  if ( auth_answer != net::UserAuthenticator::Authenticated ) {
    LOG_DEBUG << "Unauthenticated request: " << req->ToString()
              << " answer: " << auth_answer;
    req->AnswerUnauthorizedRequest(authenticator_);
    return;
  }

  // We accept all requests here.
  // Parallel requests count is limited in IRPCAsyncQueryExecutor !

  ///////////////////////////////////////////////////////////////
  //
  // Split url into: service_path / service_name / method_name
  //
  const string url_path = URL::UrlUnescape(req->request()->url()->path());
  LOG_DEBUG << "url: [" << url_path << "]"
               " , method: " << GetHttpMethodName(
                   req->request()->client_header()->method())
            << " , path we're listening: [" << http_path_ << "]";

  std::string sub_path = url_path.substr(http_path_.size());
  sub_path = strutil::StrTrimChars(sub_path, "/");
  LOG_DEBUG << "sub_path = " << sub_path;

  // sub_path should be "a/b/c/service_name/method_name"
  //
  // We extract:
  //   service_full_path = "a/b/c/service_name"
  //   service_path = "a/b/c";
  //   service_name = "service_name";
  //   method_name = "method_name";
  //
  string service_full_path;
  string method_name;
  int last_slash_index = sub_path.rfind('/');
  if ( last_slash_index == std::string::npos ) {
    service_full_path = sub_path;
    method_name = "";
  } else {
    service_full_path = sub_path.substr(0, last_slash_index);
    method_name = sub_path.substr(last_slash_index + 1);
  }

  string service_path;
  string service_name;
  last_slash_index = service_full_path.rfind('/');
  if ( last_slash_index == std::string::npos ) {
    service_path = "";
    service_name = service_full_path;
  } else {
    service_path = service_full_path.substr(0, last_slash_index);
    service_name = service_full_path.substr(last_slash_index + 1);
  }

  LOG_DEBUG << "service_path: [" << service_path << "] , "
               "service_name: [" << service_name << "] , "
               "method_name = [" << method_name << "]";
  ////////////////////////////////////////////////////////////////////

  // maybe process auto-forms request
  do {
    if ( auto_forms_js_.empty() ||
         url_path.length() <= http_path_.length() ) {
      break;
    }

    if ( !strutil::StrStartsWith(method_name, "__form") ) {
      break;
    }

    ServiceInvoker* service = services_manager_.FindService(service_name);
    if ( !service ) {
      req->request()->server_data()->Write(
          "<h1>404 Not Found</h1>"
          " could not find RPC service: [" + service_name + "] on "
          "suburl: [" + sub_path +"]");
      req->request()->server_data()->Write(
          "<br/> Known services are: {" +
          services_manager_.StrListServices(", ") + "}");
      req->ReplyWithStatus(http::NOT_FOUND);
      return;
    }

    const string auto_form_data =
        service->GetTurntablePage(http_path_, method_name);
    req->request()->server_data()->Write("<html>\n");
    req->request()->server_data()->Write(auto_forms_js_);
    req->request()->server_data()->Write("\n<body>\n");
    req->request()->server_data()->Write(auto_form_data);
    if ( strutil::StrStartsWith(method_name, "__form_") ) {
      // Draw the "Log" just for service calls
      req->request()->server_data()->Write(auto_forms_log_);
    }
    req->request()->server_data()->Write("\n</body></html>\n");
    req->Reply();
    return;
  } while ( false );

  // receives the RPC packet
  Message p;

  // the decoder used to read the RPC Message from HTTP Data
  CodecId codec = kCodecIdJson;

  ////////////////////////////////////////////////////////////////////
  //
  // read RPC packet from HTTP GET
  //
  if ( req->request()->client_header()->method() == http::METHOD_GET ) {

    // is HTTP GET enabled for rpc ?
    //
    if ( !FLAGS_rpc_enable_http_get ) {
      LOG_ERROR << "Bad request method: "
                << req->request()->client_header()->method()
                << ". RPC HTTP Get not enabled.";
      req->ReplyWithStatus(http::BAD_REQUEST);
      return;
    }

    // on HTTP GET the decoder is always JSON
    //
    codec = kCodecIdJson;

    // service, method and params are encode inside URL
    //
    URL * url = req->request()->url();
    CHECK_NOT_NULL(url) << "NULL url on http request";

    // fill in message
    p.mutable_header()->set_msgType(RPC_CALL);
    p.mutable_header()->set_xid(0);
    p.mutable_cbody()->set_service(service_name);
    p.mutable_cbody()->set_method(method_name);
    // receives call parameters
    io::MemoryStream* params = p.mutable_cbody()->mutable_params();

    // GET parameters should be: ?params=[...]
    //
    std::vector< std::pair<std::string, std::string> > http_get_params;
    url->GetQueryParameters(&http_get_params, true);
    for ( std::vector< std::pair<std::string, std::string> >::const_iterator
        it = http_get_params.begin(); it != http_get_params.end(); ++it) {
      const std::string& key = (*it).first;
      const std::string& value = (*it).second;
      if ( key == rpc::kHttpFieldParams ) {
        params->Write(value);
        continue;
      }
      LOG_ERROR << "Ignoring unknown parameter: [" << key << "]"
                   " , value: [" << value << "]";
    }
    if ( params->IsEmpty() ) {
      LOG_ERROR << "Cannot find parameter: [" << rpc::kHttpFieldParams << "]."
                   " in url: [" << url->path() << "]";
      req->request()->server_data()->Write("Cannot find parameter: ["
          + rpc::kHttpFieldParams + "] in url: [" + url->path() + "]");
      req->ReplyWithStatus(http::BAD_REQUEST);
      return;
    }
  }

  /////////////////////////////////////////////////////////////////
  //
  // read RPC packet from HTTP POST
  //
  if ( req->request()->client_header()->method() == http::METHOD_POST ) {
    // find codec ID in HTTP header (every request should specify the rpc codec)
    //
    string strCodecID;
    bool success = req->request()->client_header()->FindField(
        rpc::kHttpFieldCodec, &strCodecID);
    if ( !success ) {
      LOG_ERROR << "Cannot find field '" << rpc::kHttpFieldCodec
                << "'. Assuming JSON";
      strCodecID = rpc::kCodecNameJson;
    }
    if ( !GetCodecIdFromName(strCodecID, &codec) ) {
      LOG_ERROR << "Invalid codec_id value: [" << strCodecID << "]"
                   ", assuming JSON";
      strCodecID = rpc::kCodecNameJson;
    }

    // HTTP client data should contain the RPC packet
    //
    // decode rpc message
    //
    DECODE_RESULT result = DecodeBy(codec, *req->request()->client_data(), &p);
    if ( result == DECODE_RESULT_ERROR ) {
      LOG_ERROR << "Error decoding RPC message. Bad data.";
      req->ReplyWithStatus(http::BAD_REQUEST);
      return;
    }
    if ( result == DECODE_RESULT_NOT_ENOUGH_DATA ) {
      LOG_ERROR << "Incomplete RPC message. Bad data.";
      req->ReplyWithStatus(http::BAD_REQUEST);
      return;
    }
    CHECK_EQ(result, DECODE_RESULT_SUCCESS);

    if ( p.header().msgType() != RPC_CALL ) {
      LOG_ERROR << "Received a non-CALL message! ignoring: " << p;
      req->request()->server_data()->Write("Ignoring no-CALL message!");
      req->ReplyWithStatus(http::BAD_REQUEST);
      return;
    }
  }

  ///////////////////////////////////////////////////////////
  //
  // handle rpc message
  //
  LOG_DEBUG << "Handle: " << p;

  // extract transport
  //
  net::HostPort local_address(req->local_address());
  net::HostPort remote_address(req->remote_address());
  // If there is a HTTP proxy, the X-Forwarded-For (XFF) HTTP header is
  // de facto standard for identifying the originating IP address.
  string originating_ip;
  bool success = req->request()->client_header()->FindField(
      string("X-Forwarded-For"), &originating_ip);
  if ( success ) {
    net::IpAddress orig_ip(originating_ip);
    if ( !orig_ip.IsInvalid() ) {
      LOG_DEBUG << "X-Forwarded-For: " << originating_ip;
      remote_address = net::HostPort(orig_ip, net::HostPort::kInvalidPort);
    }
  }
  Transport transport(Transport::HTTP, local_address, remote_address);

  // extract call: service, method and arguments
  //
  CHECK_EQ(p.header().msgType(), RPC_CALL);
  const uint32 xid = p.header().xid();
  const string& service = p.cbody().service();
  const string& method = p.cbody().method();
  const io::MemoryStream& params = p.cbody().params();

  // create an internal query.
  //
  // DO NOT use xid as qid. Because HttpProcessor manages multiple
  // connections(being unique on server).. so xid s may be identical in
  // different connections.
  //
  // TODO(cosmin) : ** possible int64 trouble ** !!
  intptr_t qid = intptr_t(req);
  // const int32 qid = static_cast<int32>();
  Query* query = new Query(transport, qid, service, method, params,
                           codec, GetResultHandlerID());

  // put the request on waiting list
  // RACE NOTE: ! do this before queueing the query for execution. Because the
  //              execution may happen extremely fast, returning and finding
  //              no waiting request.
  //
  pair<MapOfRequests::iterator, bool> bit;
  {
    synch::MutexLocker lock(&access_requests_in_execution_);
    bit = requests_in_execution_.insert(
        make_pair(qid, new RpcRequest(req, codec, xid)));
  }
  CHECK(bit.second);   // the qid must not be already in execution.
  MapOfRequests::iterator it = bit.first;

  // send the query to the executor. Specify us as the result collector.
  //
  success = async_query_executor_.QueueRPC(query);
  if ( !success ) {
    delete query;
    query = NULL;

    LOG_ERROR << "Async queue execution failed:"
              << " service=" << service
              << " method=" << method
              << " reason=" << GetLastSystemErrorDescription();

    // on error, remove & complete the waiting request
    //
    delete it->second;   // delete executing request
    {
      synch::MutexLocker lock(&access_requests_in_execution_);
      requests_in_execution_.erase(it);
    }
    req->ReplyWithStatus(http::INTERNAL_SERVER_ERROR);
    return;
  }

  // Query execution is now in progress. We will be notified about
  // result in HandleRPCResult(..).
}

void HttpProcessor::CallbackReplyToHTTPRequest(
    http::ServerRequest* req,
    http::HttpReturnCode rCode) {
  CHECK_NOT_NULL(req);
  req->ReplyWithStatus(rCode);
}

//////////////////////////////////////////////////////////////
//
//   Methods available to any external thread (worker threads).
//
void HttpProcessor::WriteReply(uint32 qid,
                               REPLY_STATUS status,
                               const io::MemoryStream& result) {
  // [external/worker thread here]
  scoped_ptr<RpcRequest> ereq;
  {
    synch::MutexLocker lock(&access_requests_in_execution_);
    MapOfRequests::iterator it = requests_in_execution_.find(qid);
    CHECK(it != requests_in_execution_.end());
    ereq.reset(it->second);
    requests_in_execution_.erase(it);
  }
  CHECK_NOT_NULL(ereq.get());
  CHECK_NOT_NULL(ereq->req_);

  // create a RPC result message, and serialize it in http
  // request -> server_data
  Message p(ereq->xid_, RPC_REPLY, status, &result);

  LOG_DEBUG << "WriteReply sending packet: " << p;

  // encode the RPC result message
  //
  EncodeBy(ereq->codec_, p, ereq->req_->request()->server_data());

  // enable/disable gzip compression
  ereq->req_->request()->set_server_use_gzip_encoding(
      FLAGS_rpc_enable_http_gzip);

  switch ( ereq->codec_ ) {
    case rpc::kCodecIdJson: ereq->req_->request()->server_header()->AddField(
        http::kHeaderContentType, "application/json", true);
        break;
    case rpc::kCodecIdBinary: ereq->req_->request()->server_header()->AddField(
        http::kHeaderContentType, "application/octet-stream", true);
        break;
  }

  // queue a closure that sends the response by replying to the http request
  //
  http_server_->selector()->RunInSelectLoop(
      NewCallback(this, &HttpProcessor::CallbackReplyToHTTPRequest,
                  ereq->req_, http::OK));
}

void HttpProcessor::HandleRPCResult(const Query& q) {
  WriteReply(q.qid(), q.status(), q.result());
}
}
