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

#include "common/base/errno.h"
#include "common/base/scoped_ptr.h"
#include "common/base/gflags.h"
#include "common/io/file/file_input_stream.h"
#include "net/rpc/lib/server/rpc_http_processor.h"
#include "net/rpc/lib/server/rpc_service_invoker.h"
#include "net/rpc/lib/rpc_constants.h"

// Defined in rpc_http_server.cc
DECLARE_string(rpc_js_form_path);
// Defined in rpc_http_server.cc
DECLARE_bool(rpc_enable_http_get);

DEFINE_bool(rpc_enable_http_gzip, true,
            "If true, RPC Http Responses are gzipped, taking less bandwidth.\n"
            "False is useful for debugging.");

namespace rpc {

//////////////////////////////////////////////////////////////////////

HttpProcessor::ExecutingRequest::ExecutingRequest(
    http::ServerRequest* req, Codec& codec, uint32 xid)
    : req_(req),
      codec_(codec),
      xid_(xid) {
  CHECK_NOT_NULL(req);
}
HttpProcessor::ExecutingRequest::~ExecutingRequest() {
}

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

  // will be set to appropriate codec
  Codec* codec = NULL;

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

    // codec is always JSON
    //
    codec = &json_codec_;

    // service, method and params are encode inside URL
    //
    URL * url = req->request()->url();
    CHECK_NOT_NULL(url) << "NULL url on http request";

    // receives call parameters
    io::MemoryStream& params = p.cbody_.params_;

    // GET parameters should be: ?params=[...]
    //
    std::vector< std::pair<std::string, std::string> > http_get_params;
    url->GetQueryParameters(&http_get_params, true);
    for ( std::vector< std::pair<std::string, std::string> >::const_iterator
        it = http_get_params.begin(); it != http_get_params.end(); ++it) {
      const std::string& key = (*it).first;
      const std::string& value = (*it).second;
      if ( key == RPC_HTTP_FIELD_PARAMS ) {
        params.Write(value);
        continue;
      }
      LOG_ERROR << "Ignoring unknown parameter: [" << key << "]"
                   " , value: [" << value << "]";
    }
    if ( params.IsEmpty() ) {
      LOG_ERROR << "Cannot find parameter: [" << RPC_HTTP_FIELD_PARAMS << "]."
                   " in url: [" << url->path() << "]";
      req->request()->server_data()->Write("Cannot find parameter: ["
          + string(RPC_HTTP_FIELD_PARAMS) + "] in url: [" + url->path() + "]");
      req->ReplyWithStatus(http::BAD_REQUEST);
      return;
    }

    p.header_.msgType_ = RPC_CALL;
    p.header_.xid_ = 0;
    p.cbody_.service_ = service_name;
    p.cbody_.method_ = method_name;
    // p.cbody_.params_ were filled directly
  }

  /////////////////////////////////////////////////////////////////
  //
  // read RPC packet from HTTP POST
  //
  if ( req->request()->client_header()->method() == http::METHOD_POST ) {
    // find codec ID in HTTP header (every request should specify the rpc codec)
    //
    do {
      string strCodecID;
      bool success = req->request()->client_header()->FindField(
          string(RPC_HTTP_FIELD_CODEC_ID), &strCodecID);
      if ( !success ) {
        //  LOG_ERROR << "Cannot find field '" << RPC_HTTP_FIELD_CODEC_ID
        //            << "'. Cannot decode rpc query from http request.";
        //  req->ReplyWithStatus(http::BAD_REQUEST);
        //  return;
        LOG_ERROR << "Cannot find field '" << RPC_HTTP_FIELD_CODEC_ID
                  << "'. Assuming CID_JSON(" << CID_JSON << ").";
        codec = &json_codec_;
        break;
      }
      // create codec
      //
      errno = 0; // required, to detect ::strtol failure
      uint32 nCodecID = ::strtol(strCodecID.c_str(), NULL, 10);
      if ( errno != 0 ) {
        LOG_ERROR << "invalid codec_id, not a number: [" << strCodecID << "]";
        req->ReplyWithStatus(http::BAD_REQUEST);
        return;
      }
      switch ( nCodecID ) {
        case CID_BINARY:
          codec = &binary_codec_;
          break;
        case CID_JSON:
          codec = &json_codec_;
          break;
        default:
          LOG_ERROR << "invalid codec_id value: [" << strCodecID << "]";
          req->ReplyWithStatus(http::BAD_REQUEST);
          return;
      }
    } while( false );

    // HTTP client data should contain the RPC packet
    //
    // decode rpc message
    //
    DECODE_RESULT result = codec->DecodePacket(*req->request()->client_data(),
                                               p);
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

    if ( p.header_.msgType_ != RPC_CALL ) {
      LOG_ERROR << "Received a non-CALL message! ignoring: " << p;
      req->request()->server_data()->Write("Ignoring no-CALL message!");
      req->ReplyWithStatus(http::BAD_REQUEST);
      return;
    }
  }


  // "codec" should be set
  CHECK_NOT_NULL(codec);
  // "p" should be filled
  CHECK_EQ(p.header_.msgType_, RPC_CALL);


  ///////////////////////////////////////////////////////////
  //
  // handle rpc message
  //
  LOG_DEBUG << "Handle received packet: " << p;

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
  CHECK_EQ(p.header_.msgType_, RPC_CALL);
  const uint32 xid = p.header_.xid_;
  const string& service = p.cbody_.service_;
  const string& method = p.cbody_.method_;
  const io::MemoryStream& params = p.cbody_.params_;

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
                                     *codec, GetResultHandlerID());

  // put the request on waiting list
  // RACE NOTE: ! do this before queueing the query for execution. Because the
  //              execution may happen extremely fast, returning and finding
  //              no waiting request.
  //
  pair<MapOfRequests::iterator, bool> bit;
  {
    synch::MutexLocker lock(&access_requests_in_execution_);
    bit = requests_in_execution_.insert(
        make_pair(qid, new ExecutingRequest(req, *codec, xid)));
    codec = NULL;
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
  scoped_ptr<ExecutingRequest> ereq;
  {
    synch::MutexLocker lock(&access_requests_in_execution_);
    MapOfRequests::iterator it = requests_in_execution_.find(qid);
    CHECK(it != requests_in_execution_.end());
    ereq.reset(it->second);
    requests_in_execution_.erase(it);
  }
  CHECK_NOT_NULL(ereq.get());
  CHECK_NOT_NULL(ereq->req_);

  http::ServerRequest* req = ereq->req_;
  Codec& codec = ereq->codec_;
  uint32 xid = ereq->xid_;

  // create a RPC result message, and serialize it in http
  // request -> server_data
  Message p;

  Message::Header& header = p.header_;
  header.xid_ = xid;
  header.msgType_ = RPC_REPLY;

  Message::ReplyBody& body = p.rbody_;
  body.replyStatus_ = status;
  body.result_.AppendStreamNonDestructive(&result);

  LOG_DEBUG << "WriteReply sending packet: " << p;

  // encode the RPC result message
  //
  codec.EncodePacket(*req->request()->server_data(), p);

  // enable/disable gzip compression
  req->request()->set_server_use_gzip_encoding(FLAGS_rpc_enable_http_gzip);

  // queue a closure that sends the response by replying to the http request
  //
  http_server_->selector()->RunInSelectLoop(
      NewCallback(this, &HttpProcessor::CallbackReplyToHTTPRequest,
                  req, http::OK));
}

void HttpProcessor::HandleRPCResult(const Query& q) {
  WriteReply(q.qid(), q.status(), q.result());
}
}
