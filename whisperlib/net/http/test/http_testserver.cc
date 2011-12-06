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

#include <utility>
#include <vector>
#include <string>

#include "common/base/types.h"
#include "common/base/log.h"
#include "common/base/system.h"
#include "common/base/timer.h"
#include "common/base/gflags.h"
#include "common/base/scoped_ptr.h"
#include "common/io/file/file.h"
#include "common/io/file/file_input_stream.h"
#include "common/io/ioutil.h"
#include "common/sync/event.h"
#include "common/sync/mutex.h"

#include "net/http/http_server_protocol.h"
#include "net/http/http_client_protocol.h"
#include "net/base/selector.h"

//
// Server that can be used for testing:
//
//   /request?<query>
//   in query we accept:
//       rc= desired return code  [default 200]
//       size= desired body size (default 1024) - if cunking is enabled
//             this is the size of a chunk.
//       num_chunks= desired number of chunks (default 1)
//       hsize= desired extra 100-char header lines (default 0)
//       hdelay= desired delay before sending header in ms (default 0)
//       bdelay= desired delay for body (in ms) - or per chunck (default 0)
//       chunk= enable chunking
//
//   /restricted_request  (w/ the same parameters) but restricted..
//
//   /closeonme?<quesry>
//   on this request server closes the connection unexpectedly after
//   some bytes sent
//       size= this is the reported content length
//       close_after= closes the connection after these many bytes
//

//////////////////////////////////////////////////////////////////////

DEFINE_int32(port,
             8081,
             "Serve on this port");

DEFINE_bool(dlog_level,
            false,
            "Log more stuff ?");

#define LOG_TEST LOG(INFO)

//////////////////////////////////////////////////////////////////////
//
// Server side /request
//
int PrepareBuffer(char* buffer, int body_size, int id) {
  int num_print = (body_size - 1) / 8;
  for ( int i = 0; i < num_print; i++ ) {
    snprintf(buffer + i * 8, 9, "%07d\n", id++);
  }
  num_print = (body_size - 1) % 8;
  char* p = buffer + body_size - 1 - num_print;
  for ( int i = 0; i < num_print; i++ ) {
    *p++ = ' ';
  }
  *p = 'X';
  return id;
}

void ContinueStreaming(http::ServerRequest* req,
                       int body_size, int body_delay, int id, int i,
                       int num_chunks) {
  if ( req->is_orphaned() || i >= num_chunks ) {
    req->EndStreamingData();
    return;
  }
  if ( body_delay ) {
    timer::SleepMsec(body_delay);
  }
  char* buffer = new char[body_size];
  id = PrepareBuffer(buffer, body_size, id);
  req->request()->server_data()->AppendRaw(buffer, body_size);
  LOG_TEST << "ProcessRequest: sending chunk " << i << "/" << num_chunks;
  req->set_ready_callback(NewCallback(&ContinueStreaming, req,
       body_size, body_delay, id, i+1, num_chunks), 32000);
  req->ContinueStreamingData();
  LOG_TEST << "ProcessRequest: done chunk " << i << "/" << num_chunks;
}

void ProcessRequest(http::ServerRequest* req) {
  LOG_TEST << "ProcessRequest: " << req->ToString();
  URL* const url = req->request()->url();
  vector< pair<string, string> > query;
  url->GetQueryParameters(&query, true);
  int ret_code = 200;
  int body_size = 1024;
  int num_chunks = 1;
  int head_size = 0;
  int head_delay = 0;
  int body_delay = 0;
  bool chunk = false;
  for ( int i = 0; i < query.size(); i++ ) {
    if ( query[i].first == "rc" ) {
      ret_code = atoi(query[i].second.c_str());
    } else if ( query[i].first == "size" ) {
      body_size = atoi(query[i].second.c_str());
    } else if ( query[i].first == "num_chunks" ) {
      num_chunks = atoi(query[i].second.c_str());
    } else if ( query[i].first == "hsize" ) {
      head_size = atoi(query[i].second.c_str());
    } else if ( query[i].first == "hdelay" ) {
      head_delay = atoi(query[i].second.c_str());
    } else if ( query[i].first == "bdelay" ) {
      body_delay = atoi(query[i].second.c_str());
    } else if ( query[i].first == "chunk" ) {
      chunk = true;
    }
  }
  chunk = chunk || (num_chunks > 0 ||
                    body_size > req->free_output_bytes());
  int32 id = 0;
  req->request()->server_header()->AddField(
      http::kHeaderContentType, "text/plain", true);
  // req->request()->set_server_use_gzip_encoding(false);
  if ( head_size > 0 ) {
    for ( int i = 0; i < head_size; ++i ) {
      req->request()->server_header()->AddField(
          strutil::StringPrintf("X-Extra-Header-%d", i),
          "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx"
          "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx",
          true);
    }
  }
  if ( !chunk ) {
    if ( body_delay ) {
      timer::SleepMsec(body_delay);
    }
    char* buffer = new char[body_size];
    PrepareBuffer(buffer, body_size, id);
    req->request()->server_data()->AppendRaw(buffer, body_size);
    req->ReplyWithStatus(http::HttpReturnCode(ret_code));
    LOG_TEST << "ProcessRequest: reply sent";
  } else {
    if ( head_delay ) {
      timer::SleepMsec(head_delay);
    }
    LOG_TEST << "ProcessRequest: BeginStreamingData"
                ", num_chunks: " << num_chunks;
    req->BeginStreamingData(http::HttpReturnCode(ret_code),
                            NULL, false);
    ContinueStreaming(req, body_size, body_delay, id, 0, num_chunks);
  }
}

//////////////////////////////////////////////////////////////////////
//
// Server side /closeonme
//

void ProcessCloseOnMe(net::Selector* selector,
                      http::ServerRequest* req) {
  if ( !selector->IsInSelectThread() ) {
    selector->RunInSelectLoop(NewCallback(&ProcessCloseOnMe,
                                          selector, req));
    return;
  }
  int size = 2048;
  int close_after = 1024;
  URL* const url = req->request()->url();
  vector< pair<string, string> > query;
  url->GetQueryParameters(&query, true);
  for ( int i = 0; i < query.size(); i++ ) {
    if ( query[i].first == "size" ) {
      size = atoi(query[i].second.c_str());
    } else if ( query[i].first == "close_after" ) {
      close_after = atoi(query[i].second.c_str());
    }
  }
  // TODO(cosmin): fix, the original code is commented
  // const int fd = req->DetachFromFd();
  net::NetConnection * conn = req->DetachFromFd();
  LOG_TEST << "req->DetachFromFd() => " << conn;
  delete req;
  // if ( fd != INVALID_FD_VALUE ) {
  if ( conn ) {
    // ClosingConnection* conn = new ClosingConnection(selector);
    // conn->Initialize(fd);
    conn->outbuf()->Write(
        strutil::StringPrintf("HTTP/1.1 200 OK\r\n"
                              "Keep-Alive: 15\r\n"
                              "Connection: Keep-Alive\r\n"
                              "Content-Type: text/plain\r\n"
                              "Content-Length: %d\r\n"
                              "\r\n", size));
    char* buffer = new char[close_after];
    PrepareBuffer(buffer, close_after, 0);
    conn->outbuf()->AppendRaw(buffer, close_after);
    conn->FlushAndClose();
    LOG_TEST << "About to FlushAndClose connection";
  }
}

//////////////////////////////////////////////////////////////////////


class TestRequest {
 public:
  TestRequest(net::Selector* selector,
              const net::NetFactory& net_factory,
              net::PROTOCOL net_protocol,
              http::ClientParams* params,
              http::HttpMethod http_method,
              const string& escaped_query_path,
              Closure* run_on_end,
              http::ClientError expected_error,
              http::HttpReturnCode expected_code)
      : selector_(selector),
        net_factory_(net_factory),
        req_(http_method, escaped_query_path),
        proto_(new http::ClientProtocol(
                   params,
                   new http::SimpleClientConnection(selector_, net_factory_,
                                                    net_protocol),
                   net::HostPort("127.0.0.1", FLAGS_port))),
        run_on_end_(run_on_end),
        expected_error_(expected_error),
        expected_code_(expected_code) {
    req_.request()->set_server_use_gzip_encoding(false);
  }
  TestRequest(net::Selector* selector,
              const net::NetFactory& net_factory,
              net::PROTOCOL net_protocol,
              http::ClientProtocol* proto,
              http::HttpMethod http_method,
              const string& escaped_query_path,
              Closure* run_on_end,
              http::ClientError expected_error,
              http::HttpReturnCode expected_code)
      : selector_(selector),
        net_factory_(net_factory),
        req_(http_method, escaped_query_path),
        proto_(proto),
        run_on_end_(run_on_end),
        expected_error_(expected_error),
        expected_code_(expected_code) {
    req_.request()->set_server_use_gzip_encoding(false);
  }
  virtual ~TestRequest() {
    delete proto_;
  }
  void Start() {
    LOG_TEST << "Start TestRequest escaped_query_path=["
            << req_.request()->client_header()->uri() << "]";
    proto_->SendRequest(&req_,
                        NewCallback(this, &TestRequest::RequestDone));
  }
  void RequestDone() {
    const http::ClientError cli_error = req_.error();
    const http::HttpReturnCode ret_code =
        req_.request()->server_header()->status_code();
    LOG_TEST  << "Request finished: " << req_.name()
             << " - Client Error : "
             << http::ClientErrorName(cli_error)
             << " - HTTP Error : "
             << http::GetHttpReturnCodeName(ret_code);
    CHECK_EQ(cli_error, expected_error_);
    CHECK_EQ(ret_code, expected_code_);
    if ( run_on_end_ ) {
      run_on_end_->Run();
    }
  }

 protected:
  net::Selector* selector_;
  const net::NetFactory& net_factory_;
  http::ClientRequest req_;
  http::ClientProtocol* proto_;
  Closure* run_on_end_;
  http::ClientError expected_error_;
  http::HttpReturnCode expected_code_;
};

//////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
  common::Init(argc, argv);
  http::ServerParams params;
  params.max_reply_buffer_size_ = 1<<18;
  params.dlog_level_ = FLAGS_dlog_level;
  net::Selector selector;
  net::NetFactory net_factory(&selector);
  net::PROTOCOL net_protocol = net::PROTOCOL_TCP;
  http::Server server("Test Server", &selector, net_factory, params);
  server.RegisterProcessor("/request",
                           NewPermanentCallback(&ProcessRequest),
                           true, true);
  server.RegisterProcessor("/restricted_request",
                           NewPermanentCallback(&ProcessRequest),
                           false, true);
  server.RegisterProcessor("/closeonme",
                           NewPermanentCallback(&ProcessCloseOnMe,
                                                &selector),
                           true, true);
  server.AddAcceptor(net_protocol, net::HostPort(0, FLAGS_port));
  selector.RunInSelectLoop(NewCallback(&server,
                                       &http::Server::StartServing));

  http::ClientParams cli_params;
  cli_params.dlog_level_ = FLAGS_dlog_level;
  cli_params.max_header_size_ = 256;
  cli_params.max_body_size_ = 1024;
  cli_params.max_chunk_size_ = 1024;
  cli_params.max_num_chunks_ = 5;
  cli_params.default_request_timeout_ms_ = 4000;
  cli_params.connect_timeout_ms_ = 100;
  cli_params.write_timeout_ms_ = 100;
  cli_params.read_timeout_ms_ = 1000;
  cli_params.keep_alive_sec_ = 0;

  TestRequest t0(&selector, net_factory, net_protocol,
                 &cli_params, http::METHOD_GET,
                 "/request?hsize=450",
                 NewCallback(&selector, &net::Selector::MakeLoopExit),
                 http::CONN_HTTP_PARSING_ERROR,
                 http::OK);
  TestRequest t1(&selector, net_factory, net_protocol,
                 &cli_params, http::METHOD_GET,
                 "/request?size=1025",
                 NewCallback(&t0, &TestRequest::Start),
                 http::CONN_OK,
                 http::OK);
  TestRequest t2(&selector, net_factory, net_protocol,
                 &cli_params, http::METHOD_GET,
                 "/request?num_chunks=6&size=50",
                 NewCallback(&t1, &TestRequest::Start),
                 http::CONN_CONNECTION_CLOSED,
                 http::OK);

  TestRequest* last = &t2;
  selector.RunInSelectLoop(NewCallback(last, &TestRequest::Start));

  selector.Loop();
  LOG_INFO << "========= PASS =========";
}
