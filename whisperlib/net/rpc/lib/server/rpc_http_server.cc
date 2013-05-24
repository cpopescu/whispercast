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

#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/io/file/file_input_stream.h>
#include <whisperlib/net/rpc/lib/server/rpc_http_server.h>
#include <whisperlib/net/rpc/lib/codec/rpc_codec.h>

//////////////////////////////////////////////////////////////////////

// TODO(cpopescu): make this a parameter rather than a flag ...
DEFINE_string(rpc_js_form_path,
              "",
              "If specified, we export RPC accesing via auto-generated forms "
              "and we read the extra js sources from here (we need files "
              "from //depot/.../libs/net/rpc/js/");

DEFINE_bool(rpc_enable_http_get,
            false,
            "If enabled we process HTTP GET requests. By default only"
            " HTTP POST requests are enabled.");

//////////////////////////////////////////////////////////////////////

namespace rpc {
HttpServer::HttpServer(net::Selector* selector,
                       http::Server* server,
                       net::UserAuthenticator* authenticator,
                       const string& path,
                       bool auto_js_forms,
                       bool is_public,
                       int max_concurrent_requests,
                       const string& ip_class_restriction)
    : selector_(selector),
      authenticator_(authenticator),
      accepted_clients_(
          ip_class_restriction.empty()
          ? NULL
          : net::IpClassifier::CreateClassifier(ip_class_restriction)),
      path_(path),
      max_concurrent_requests_(max_concurrent_requests),
      current_requests_(0),
      http_server_(server) {
  if ( auto_js_forms && !FLAGS_rpc_js_form_path.empty() ) {
    const string rpc_base_js = io::FileInputStream::ReadFileOrDie(
        strutil::JoinPaths(FLAGS_rpc_js_form_path, "rpc_base.js"));
    const string rpc_standard_js = io::FileInputStream::ReadFileOrDie(
        strutil::JoinPaths(FLAGS_rpc_js_form_path, "rpc_standard.js"));
    js_prolog_ = ("<script language=\"JavaScript1.1\">\n" +
                  rpc_base_js + "\n" +
                  rpc_standard_js + "\n" +
                  "</script>\n");
    auto_forms_log_ = io::FileInputStream::ReadFileOrDie(
        strutil::JoinPaths(FLAGS_rpc_js_form_path, "auto_forms_log.html"));
  }
  http_server_->RegisterProcessor(path_,
      NewPermanentCallback(this, &HttpServer::ProcessRequest),
      is_public, true);
}

HttpServer::~HttpServer() {
  http_server_->UnregisterProcessor(path_);
  for ( MirrorMap::const_iterator it_mirror = mirror_map_.begin();
        it_mirror != mirror_map_.end();
        ++it_mirror ) {
    delete it_mirror->second;
  }
  delete accepted_clients_;
  LOG_INFO_IF(current_requests_ > 0)
      << " Exiting the RPC server with current_requests_ "
      << "requests in processing !!";
}

void HttpServer::RegisterServiceMirror(const string& sub_path_src,
                                       const string& sub_path_dest) {
  string normalized_sub_path_src(strutil::NormalizePath(sub_path_src + '/'));
  string normalized_sub_path_dest(strutil::NormalizePath(sub_path_dest + '/'));
  MirrorMap::iterator it_mirror = mirror_map_.find(normalized_sub_path_src);
  if ( it_mirror != mirror_map_.end() ) {
    it_mirror->second->insert(normalized_sub_path_dest);
  } else {
    hash_set<string>* s = new hash_set<string>;
    s->insert(normalized_sub_path_dest);
    mirror_map_.insert(make_pair(normalized_sub_path_src, s));
  }
  reverse_mirror_map_[normalized_sub_path_dest] = normalized_sub_path_src;
}

bool HttpServer::RegisterService(const string& sub_path,
                                 rpc::ServiceInvoker* invoker) {
  const string full_path(strutil::JoinPaths(sub_path, invoker->GetName()));
  ServicesMap::const_iterator it = services_.find(full_path);
  if ( it != services_.end() ) {
    LOG_WARNING << " Tried to double register " << full_path;
    return false;
  }
  LOG_INFO << " Registering: " << invoker->ToString() << " on path: "
           << full_path;
  services_.insert(make_pair(full_path, invoker));
  return true;
}
bool HttpServer::UnregisterService(const string& sub_path,
                                   rpc::ServiceInvoker* invoker) {
  const string full_path(strutil::JoinPaths(sub_path, invoker->GetName()));
  ServicesMap::iterator it = services_.find(full_path);
  if ( it == services_.end() ) {
    LOG_WARNING << " Tried to unregister " << full_path
                << " not found.";
    return false;
  }
  if ( invoker != it->second ) {
    LOG_WARNING << " Different service registered under " << full_path
                << invoker->ToString() << " vs. " << it->second->ToString()
                << " --> " << invoker << " vs. " << it->second;
    return false;
  }
  string normalized_sub_path(strutil::NormalizePath(sub_path + '/'));
  MirrorMap::iterator it_mirror = mirror_map_.find(normalized_sub_path);
  if ( it_mirror != mirror_map_.end() ) {
    for ( hash_set<string>::const_iterator it_rev = it_mirror->second->begin();
          it_rev != it_mirror->second->end(); ++it_rev ) {
      reverse_mirror_map_.erase(*it_rev);
    }
    delete it_mirror->second;
    mirror_map_.erase(it_mirror);
    LOG_INFO << " RPC forward mapping removed for " << normalized_sub_path;
  }
  ReverseMirrorMap::iterator
      it_rev = reverse_mirror_map_.find(normalized_sub_path);
  if ( it_rev != reverse_mirror_map_.end() ) {
    it_mirror = mirror_map_.find(it_rev->second);
    if ( it_mirror != mirror_map_.end() ) {
      it_mirror->second->erase(normalized_sub_path);
      if ( it_mirror->second->empty() ) {
        delete it_mirror->second;
        mirror_map_.erase(it_mirror);
      }
    }
    LOG_INFO << " RPC backward mapping removed for " << normalized_sub_path
             << " from " << it_rev->second;
    reverse_mirror_map_.erase(it_rev);
  }
  services_.erase(full_path);
  return true;
}
bool HttpServer::RegisterService(rpc::ServiceInvoker* invoker) {
  return RegisterService("", invoker);
}

bool HttpServer::UnregisterService(rpc::ServiceInvoker* invoker) {
  return UnregisterService("", invoker);
}

void HttpServer::ProcessRequest(http::ServerRequest* req) {
  const string req_id =
      req->request()->client_header()->FindField(http::kHeaderXRequestId);
  if ( !req_id.empty() ) {
    req->request()->server_header()->AddField(
        http::kHeaderXRequestId, req_id, true);
  }
  if ( current_requests_ >= max_concurrent_requests_ ) {
    LOG_ERROR << " Too many concurrent requests: " << current_requests_;
    // TODO(cpopescu): stats
    req->ReplyWithStatus(http::SERVICE_UNAVAILABLE);
    return;
  }
  if ( accepted_clients_ != NULL &&
       !accepted_clients_->IsInClass(req->remote_address().ip_object()) ) {
    // TODO(cpopescu): stats
    req->request()->server_data()->Write("Bad IP");
    req->ReplyWithStatus(http::UNAUTHORIZED);
    return;
  }


  ///////////////////////////////////////////////////////////////
  //
  // Split url into: service_path / service_name / method_name
  //
  const string url_path = URL::UrlUnescape(req->request()->url()->path());

  std::string sub_path = strutil::StrTrimChars(
      url_path.substr(path_.size()), "/");
  LOG_DEBUG << "url: [" << url_path << "] , path_: [" << path_
            << "], sub_path: [" << sub_path << "]";

  // sub_path should be "a/b/c/service_name/method_name"
  //
  // We extract:
  //   service_full_path = "a/b/c/service_name"
  //   service_path = "a/b/c";
  //   service_name = "service_name";
  //   method_name = "method_name";
  //
  pair<string,string> p = strutil::SplitLast(sub_path, '/');
  const string service_full_path = p.first;
  const string method_name = p.second;
  p = strutil::SplitLast(p.first, '/');
  const string service_path = p.first;
  const string service_name = p.second;
  //if ( service_name == "" || method_name == "" ) {
  //  LOG_ERROR << "Invalid request path: [" << sub_path << "]";
  //  req->ReplyWithStatus(http::BAD_REQUEST);
  //  return;
  //}

  ////////////////////////////////////////////////////////////////////

  // maybe process auto-forms request
  do {
    if ( js_prolog_.empty() ||
         url_path.length() <= path_.length() ) {
      break;
    }

    if ( !strutil::StrStartsWith(method_name, "__form") ) {
      break;
    }

    ServicesMap::const_iterator it = services_.find(service_full_path);
    rpc::ServiceInvoker* service = (it == services_.end() ? NULL : it->second);
    if ( !service ) {
      req->request()->server_data()->Write(
          "<h1>404 Not Found</h1>"
          " could not find RPC service: [" + service_full_path + "] on "
          "suburl: [" + sub_path +"]");
      req->request()->server_data()->Write(
          "<br/> Known services are: {");
      for ( ServicesMap::const_iterator it = services_.begin();
            it != services_.end(); ) {
        req->request()->server_data()->Write(it->first.c_str());
        ++it;
        if ( it != services_.end() ) {
          req->request()->server_data()->Write(", ");
        }
      }
      req->request()->server_data()->Write("}");
      req->ReplyWithStatus(http::NOT_FOUND);
      return;
    }

    const string auto_form_data = service->GetTurntablePage(
        strutil::JoinPaths(path_, service_path), method_name);
    req->request()->server_data()->Write("<html>\n");
    req->request()->server_data()->Write(js_prolog_);
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

  req->AuthenticateRequest(
      authenticator_,
      NewCallback(this, &HttpServer::ProcessAuthenticatedRequest,
                  req, service_name, method_name));
}

void HttpServer::ProcessAuthenticatedRequest(http::ServerRequest* req,
    string service_name, string method_name,
    net::UserAuthenticator::Answer auth_answer) {
  if ( auth_answer != net::UserAuthenticator::Authenticated ) {
    LOG_INFO << "Unauthenticated request: " << req->ToString()
             << " answer: " << auth_answer;
    req->AnswerUnauthorizedRequest(authenticator_);
    return;
  }
  // Authenticated - OK !

  // receives the RPC packet
  rpc::Message p;

  // will be set to appropriate codec
  rpc::CodecId codec = rpc::kCodecIdJson;

  URL * url = req->request()->url();
  const string url_path = URL::UrlUnescape(req->request()->url()->path());
  std::string sub_path = url_path.substr(path_.size());
  sub_path = strutil::StrTrimChars(sub_path, "/");

  ////////////////////////////////////////////////////////////////////
  //
  // read RPC packet from HTTP GET
  //
  if ( req->request()->client_header()->method() == http::METHOD_GET ) {
    if ( !FLAGS_rpc_enable_http_get ) {
      LOG_ERROR << "Bad request method: "
                << req->request()->client_header()->method()
                << ". RPC HTTP Get not enabled.";
      req->ReplyWithStatus(http::BAD_REQUEST);
      return;
    }

    // on HTTP GET the format is always JSON
    //
    codec = rpc::kCodecIdJson;

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
          + string(rpc::kHttpFieldParams) + "] in url: [" + url->path() + "]");
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
  // handle RPC message
  //
  LOG_DEBUG << "Handle: " << p;

  if ( p.header().msgType() != RPC_CALL ) {
    // TODO(cpopescu): stats
    LOG_ERROR << "Received a non-CALL message! ignoring: " << p;
    req->ReplyWithStatus(http::BAD_REQUEST);
    return;
  }

  // extract transport information
  //
  rpc::Transport transport(rpc::Transport::HTTP,
                           net::HostPort(),
                           req->remote_address());
  if ( authenticator_ != NULL ) {
    string user, passwd;
    if ( req->request()->client_header()->GetAuthorizationField(&user,
                                                                &passwd) ) {
      transport.set_user_passwd(user, passwd);
    }
  }

  // Any timeouts ?
  string timeout_str;
  int timeout = 0;
  if ( req->request()->client_header()->FindField("Timeout", &timeout_str) ) {
    errno = 0;   // essential as strtol would not set a 0 errno
    timeout = strtol(timeout_str.c_str(), NULL, 10);
    if ( timeout < 0 ) {
      timeout = 0;
    }
  }
  // TODO(cosmin): support timeouts in the rpc processing !

  // Now see if secondary paths are involved
  string normalized_sub_path(strutil::NormalizePath(sub_path + '/'));
  MirrorMap::const_iterator it_mirror =  mirror_map_.find(normalized_sub_path);
  if ( it_mirror != mirror_map_.end() ) {
    // WELL -- this is importnant - the called mirror request should not
    //         cause the removal of the service
    for ( hash_set<string>::const_iterator it_dup = it_mirror->second->begin();
          it_dup != it_mirror->second->end(); ++it_dup ) {
      LOG_DEBUG << " RPC mirror processing for : " << *it_dup << " / "
                << p.cbody().service()
                << " per request on " << sub_path;
      ProcessRequestCore(NULL, *it_dup, transport, p, codec, timeout);
    }
  }

  ProcessRequestCore(req, sub_path, transport, p, codec, timeout);
}


void HttpServer::ProcessRequestCore(http::ServerRequest* req,
                                    const string& sub_path,
                                    const rpc::Transport& transport,
                                    const rpc::Message& p,
                                    rpc::CodecId codec,
                                    int timeout) {
  // extract call: service, method and arguments
  //
  const uint32 xid = p.header().xid();
  string service = p.cbody().service();
  const string& method = p.cbody().method();

  ServicesMap::const_iterator it =
      services_.find(strutil::JoinPaths(sub_path, service));
  if ( it == services_.end() ) {
    LOG_ERROR << "Cannot find service: " << service;
    if ( req ) {
      req->ReplyWithStatus(http::NOT_FOUND);
    }
    return;
  }
  rpc::ServiceInvoker* const invoker = it->second;
  if ( service != invoker->GetName() ) {
    service = invoker->GetName();  // may miss a '/'
  }

  // create an internal query.
  if ( req ) {
    if ( req->is_orphaned() ) {
      return;
    }
    // TODO(cpopescu): parametrize this one !!
    req->request()->set_server_use_gzip_encoding(false);
  }

  rpc::Query* const query = new rpc::Query(
      transport, xid, service, method, p.cbody().params(), codec, 0);
  query->SetCompletionCallback(NewCallback(this, &HttpServer::RpcCallback, req));

  current_requests_++;
  if ( !invoker->Call(query) ) {
    current_requests_--;
    delete query;
    if ( req ) {
      // TODO(cpopescu): stats
      req->ReplyWithStatus(http::INTERNAL_SERVER_ERROR);
    }
    return;
  }
}

void HttpServer::RpcCallback(http::ServerRequest* req,
                             const rpc::Query& query) {
  CHECK(selector_->IsInSelectThread());
  current_requests_--;
  if ( req == NULL ) {
    return;
  }

  // On several errors we want to reply HTTP directly
  if ( query.status() == RPC_UNAUTHORIZED ) {
    // This is safe from any thread..
    if ( !authenticator_->realm().empty() ) {
      req->request()->server_header()->AddField(
          http::kHeaderWWWAuthenticate,
          strutil::StringPrintf("Basic realm=\"%s\"",
                                authenticator_->realm().c_str()),
          true);
    }
    req->ReplyWithStatus(http::UNAUTHORIZED);
    return;
  }
  // create a RPC result message, and serialize it in
  // http request -> server_data
  //
  rpc::Message p(query.qid(), RPC_REPLY, query.status(), &query.result());
  EncodeBy(query.codec(), p, req->request()->server_data());
  switch ( query.codec() ) {
    case rpc::kCodecIdJson: req->request()->server_header()->AddField(
        http::kHeaderContentType, "application/json", true);
        break;
    case rpc::kCodecIdBinary: req->request()->server_header()->AddField(
        http::kHeaderContentType, "application/octet-stream", true);
        break;
  }
  req->Reply();

  // TODO(cpopescu): stats
  // -- including req->is_orphaned()
  // -- TODO(cpopescu): break request on orphaned requests
}
}
