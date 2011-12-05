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
//
// This is the main file of whispercast server..
//

#include <whisperlib/common/app/app.h>

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/system.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/base/strutil.h>
#include <whisperlib/common/sync/thread.h>
#include <whisperlib/common/base/callback.h>

#include <whisperlib/common/io/file/file_input_stream.h>
#include <whisperlib/net/base/selector.h>

#include <whisperlib/net/rpc/lib/server/rpc_http_server.h>

#include <whisperstreamlib/stats2/stats_collector.h>
#include <whisperstreamlib/stats2/log_stats_saver.h>
#include <whisperstreamlib/base/element.h>
#include <whisperlib/net/util/ipclassifier.h>

#include <whisperlib/net/base/user_authenticator.h>
#include <whisperlib/net/http/http_server_protocol.h>

#include <whisperstreamlib/rtmp/rtmp_connection.h>
#include <whisperstreamlib/elements/factory.h>
#include <whisperstreamlib/rtp/rtsp/rtsp_server.h>
#include <whisperstreamlib/rtp/rtsp/rtsp_element_mapper_media_interface.h>

#include <whisperlib/common/io/ioutil.h>
#include "media_mapper.h"
#include "stream_request.h"

//////////////////////////////////////////////////////////////////////

DEFINE_int32(http_port,
             8080,
             "The port on which we accept HTTP connections");
DEFINE_int32(rpc_http_port,
             9090,
             "The port on which we accept rpc (commands) HTTP connections");
DEFINE_string(http_local_address,
              "0.0.0.0",
              "The local address to bind to when accepting HTTP connections");
DEFINE_string(rpc_http_local_address,
              "0.0.0.0",
              "The local address to bind to "
              "when accepting rpc HTTP connections");
DEFINE_int32(https_port,
             8443,
             "The port on which we accept HTTPS connections");
DEFINE_int32(rpc_https_port,
             9443,
             "The port on which we accept rpc (commands) HTTP connections");
DEFINE_string(https_local_address,
              "0.0.0.0",
              "The local address to bind to when accepting HTTPS connections");
DEFINE_string(rpc_https_local_address,
              "0.0.0.0",
              "The local address to bind to when accepting HTTPS connections");
DEFINE_int32(http_connection_max_media_outbuf_size,
             3 * 65536,
             "Sending buffer per HTTP connection for all media data");
DEFINE_int32(http_connection_read_timeout,
             20000,
             "Timeout for reading the HTTP client request");
DEFINE_int32(http_connection_write_timeout,
             20000,
             "Timeout for a HTTP client connection before disconnecting");
DEFINE_int64(http_connection_write_ahead_ms,
             4000,
             "How much HTTP data to write ahead of the element time, "
             "in miliseconds");
DEFINE_bool(http_connection_dlog_level,
            false,
            "Turn on deep debugging messages on HTTP connections");
DEFINE_int32(http_max_num_connections,
             1000,
             "Accept at most these many concurrent HTTP connections");

DEFINE_string(http_default_content_type,
              "text/html",
              "We send this content type when we don't know what "
              "to send");
DEFINE_string(http_extensions_to_content_types_file,
             "",
             "If set we read mappings from extensions to "
             "content types from here");
DEFINE_string(http_ssl_certificate,
              "",
              "Path to an SSL certificate file. We need both certificate & key"
              " to use HTTP over SSL; else we use regular HTTP over TCP");
DEFINE_string(http_ssl_key,
              "",
              "Path to an SSL key file. We need both certificate & key"
              " to use HTTP over SSL; else we use regular HTTP over TCP");

//////////////////////////////////////////////////////////////////////

DEFINE_int32(rtmp_port,
             1935,
             "The port on which we accept RTMP connections");
DEFINE_string(rtmp_local_address,
              "0.0.0.0",
              "The local address to bind to when accepting RTMP connections");
DEFINE_string(rtmp_publishing_application,
              "live",
              "If this is set we allow client publishing under "
              "this rtmp application");

DEFINE_int32(client_networking_threads,
             4,
             "We run these many threads for talking w/ clients");

//////////////////////////////////////////////////////////////////////

DEFINE_string(ip_classifiers,
              "",
              "Comma sepparated list of classifiers - first class 0, etc");

//////////////////////////////////////////////////////////////////////


DEFINE_string(log_stats_dir,
              "",
              "The directory where the stats log files will be written.");
DEFINE_string(log_stats_file_base,
              "",
              "The base name for stats log files.");

//////////////////////////////////////////////////////////////////////

DEFINE_string(server_id,
              "",
              "The ID of the server, as it will be logged");

//////////////////////////////////////////////////////////////////////

DEFINE_string(media_config_dir,
              "",
              "Where to load / save the configs");
DEFINE_string(base_media_dir,
              "",
              "Media is under this directory");
DEFINE_string(http_base_media_path,
              "/media",
              "We export our media elements under this path for http");
DEFINE_string(media_state_dir,
              "",
              "Where we may save our state");
DEFINE_string(media_state_name,
              "whispercast",
              "We save the state under this name");
DEFINE_string(local_media_state_name,
              "local_whispercast",
              "We save non config state under this name");

//////////////////////////////////////////////////////////////////////

DEFINE_string(admin_passwd,
              "",
              "An admin password for server administration - put this in a flag file");
DEFINE_string(authorization_realm,
              "",
              "If you set an admin password, you also need this");
DEFINE_string(rpc_config_allow_classifier,
              "",
              "If not empty we allow config rpc-s only from guys who "
              "qualify in the classifier created by this IpClassifier (e.g. "
              "'OR(IPFILTER(201.1.2.240/26), "
              "IPFILTERFILE(/etc/whispercast/good_rpc_ips))' )");
DEFINE_string(rpc_stats_allow_classifier,
              "",
              "If not empty we allow stat rpc-s only from guys who qualify "
              "in the classifier created by this IpClassifier (e.g. "
              "'OR(IPFILTER(201.1.2.240/26), "
              "IPFILTERFILE(/etc/whispercast/good_rpc_ips))' )");

DECLARE_int32(rtmp_max_num_connections);

/////////////////////////////////////////////////////////////////////
DEFINE_int32(rtsp_port,
             0,
             "Port where RTSP server is listening. 0 disables RTSP server.");

//////////////////////////////////////////////////////////////////////

// We don't want to get crawled - disable all robots
void RobotsProcessor(http::ServerRequest* req) {
  req->request()->server_header()->AddField(
    http::kHeaderContentType, "text/plain", true);
  req->request()->server_data()->Write(
    "User-agent: *\n"
    "Disallow: /\n");
  req->Reply();
}

void QuickStats(
  streaming::StatsCollector* collector,
  http::ServerRequest* req) {
  req->request()->server_header()->AddField(
    http::kHeaderContentType, "text/plain", true);
  // TODO [cpopescu]:
  req->Reply();
}

string DecodeAdminPasswd(const string& s) {
  const string tmp = URL::UrlUnescape(s);
  string ret;
  for ( int i = 0; i < tmp.size(); ++i ) {
    ret += int8(tmp[i]) ^ 0xb6;
  }
  DLOG_INFO << " Decoded passwd: [" << ret << "]";
  return ret;
}

//////////////////////////////////////////////////////////////////////

class Whispercast : public app::App {
public:
  Whispercast(int &argc, char **&argv);
  virtual ~Whispercast();

protected:
  int Initialize();
  void Cleanup();

  void Run() {
    selector_->Loop();
  }

  void CloseCompleted() {
    // MediaMapper already closed all it's elements
    CHECK(selector_->IsInSelectThread());

    for ( int i = 0; i < client_threads_.size(); i++ ) {
      client_threads_[i]->CleanAndCloseAll();
    }

    // Stop the selector, Run() will return and then Shutdown()
    // is automatically called by App.
    selector_->set_call_on_close(NewCallback(this, &Whispercast::Cleanup));
    selector_->RunInSelectLoop(NewCallback(selector_,
                                           &net::Selector::MakeLoopExit));
  }
  void StopInSelectThread() {
    CHECK(selector_->IsInSelectThread());
    // stop the networking
    if ( rpc_http_server_ != NULL ) {
      rpc_http_server_->StopServing();
    }
    if ( rtmp_server_ != NULL ) {
      rtmp_server_->StopServing();
    }
    if ( http_server_ != NULL ) {
      http_server_->StopServing();
    }
    if ( rtsp_server_ != NULL ) {
      rtsp_server_->Stop();
    }

    // initialize elements close process
    media_mapper_->Close(NewCallback(this, &Whispercast::CloseCompleted));
  }
  void Stop() {
    CHECK(!selector_->IsInSelectThread());
    // delegate to selector (because we cannot stop the networking from outside)
    selector_->RunInSelectLoop(NewCallback(this,
        &Whispercast::StopInSelectThread));
  }
  void Shutdown();

  void ProcessMediaRequest(http::ServerRequest* req);
  void ProcessMediaRequestCallback(http::ServerRequest* req);

  void LoadMediaConfig();
 private:
  net::Selector* selector_;
  MediaMapper* media_mapper_;

  SSL_CTX* http_ssl_ctx_;
  net::NetFactory* http_net_factory_;
  net::NetFactory* rpc_http_net_factory_;
  http::Server* http_server_;
  http::Server* rpc_http_server_;

  vector<net::SelectorThread*> client_threads_;

  rtmp::StandardStreamManager* rtmp_stream_manager_;
  rtmp::ServerAcceptor* rtmp_server_;
  rtmp::ProtocolFlags rtmp_flags_;

  net::SimpleUserAuthenticator* admin_authenticator_;
  rpc::HttpServer* rpc_stat_processor_;
  rpc::HttpServer* rpc_processor_;

  streaming::rtsp::MediaInterface* rtsp_media_interface_;
  streaming::rtsp::Server* rtsp_server_;

  //////////////////////////////////////////////////////////////////////

  streaming::StatsCollector* stats_collector_;

  map<string, string> ext2ct_;   // maps extensions to content type

  vector<const net::IpClassifier*> classifiers_;
  int64 connection_id_;
};

//////////////////////////////////////////////////////////////////////

Whispercast::Whispercast(int &argc, char **&argv)
  : app::App(argc, argv),
    selector_(NULL),
    media_mapper_(NULL),
    http_ssl_ctx_(NULL),
    http_net_factory_(NULL),
    rpc_http_net_factory_(NULL),
    http_server_(NULL),
    rpc_http_server_(NULL),
    rtmp_stream_manager_(NULL),
    rtmp_server_(NULL),
    admin_authenticator_(NULL),
    rpc_stat_processor_(NULL),
    rpc_processor_(NULL),
    rtsp_media_interface_(NULL),
    rtsp_server_(NULL),
    stats_collector_(NULL),
    classifiers_(),
    connection_id_(0) {
  google::SetUsageMessage("Whispercast - stream broadcasting server.");
}

Whispercast::~Whispercast() {
  CHECK_NULL(selector_);
  CHECK_NULL(media_mapper_);
  CHECK_NULL(http_ssl_ctx_);
  CHECK_NULL(http_net_factory_);
  CHECK_NULL(rpc_http_net_factory_);
  CHECK_NULL(http_server_);
  CHECK_NULL(rpc_http_server_);
  CHECK(client_threads_.empty());
  CHECK_NULL(rtmp_stream_manager_);
  CHECK_NULL(rtmp_server_);
  CHECK_NULL(rpc_stat_processor_);
  CHECK_NULL(rpc_processor_);
  CHECK_NULL(rtsp_media_interface_);
  CHECK_NULL(rtsp_server_);
  CHECK_NULL(stats_collector_);
  CHECK(classifiers_.empty());
}

void Whispercast::LoadMediaConfig() {
  const MediaMapper::LoadError err = media_mapper_->Load();
  if ( err != MediaMapper::LOAD_OK ) {
    //CHECK(load_err == MediaMapper::LOAD_FILE_ERROR)
    LOG_WARNING << "We encountered some data consistency error loading"
                   " configuration: " << MediaMapper::LoadErrorName(err);
  }
}
int Whispercast::Initialize() {
  common::Init(argc_, argv_);

  //////////////////////////////////////////////////////////////////////

  selector_ = new net::Selector();

  //////////////////////////////////////////////////////////////////////

  // Create the classifiers
  if ( !FLAGS_ip_classifiers.empty() ) {
      vector<string> components;
      CHECK(strutil::SplitBracketedString(FLAGS_ip_classifiers.c_str(),
                                          ',', '(', ')',  &components))
                                            << " Invalid classifier spec: ["
                                            << FLAGS_ip_classifiers << "]";
      for ( int i = 0; i < components.size(); ++i ) {
        classifiers_.push_back(
          net::IpClassifier::CreateClassifier(components[i]));
      }
    }

  if ( FLAGS_client_networking_threads > 0 ) {
    for ( int i = 0; i < FLAGS_client_networking_threads; ++i ) {
      client_threads_.push_back(new net::SelectorThread());
      client_threads_.back()->Start();
    }
  }
  vector<net::SelectorThread*>* client_threads =
      client_threads_.empty() ? NULL : &client_threads_;

  //////////////////////////////////////////////////////////////////////

  // Initialize the stats collection
  vector<streaming::StatsSaver*> stats_savers;
  if(FLAGS_log_stats_dir != "") {
    stats_savers.push_back(new streaming::LogStatsSaver(
                                FLAGS_log_stats_dir.c_str(),
                                FLAGS_log_stats_file_base.c_str()));
  }
  stats_collector_ = new streaming::StatsCollector(
      selector_, FLAGS_server_id, timer::Date::Now(), &stats_savers);
  CHECK(stats_collector_->Start());

  //////////////////////////////////////////////////////////////////////

  // Create the HTTP server

  // TODO : add http_connection_write_ahead
  http_net_factory_ = new net::NetFactory(selector_);
  rpc_http_net_factory_ = new net::NetFactory(selector_);

  if ( FLAGS_http_ssl_certificate != "" &&
       FLAGS_http_ssl_key != "" ) {
    http_ssl_ctx_ = net::SslConnection::SslCreateContext(
        FLAGS_http_ssl_certificate, FLAGS_http_ssl_key);
    if ( http_ssl_ctx_ != NULL ) {
      net::SslConnectionParams ssl_connection_params;
      ssl_connection_params.ssl_context_ = http_ssl_ctx_;
      net::SslAcceptorParams ssl_acceptor_params(ssl_connection_params);
      rpc_http_net_factory_->SetSslParams(ssl_acceptor_params,
                                          ssl_connection_params);
      // With possible client threads
      if ( client_threads != NULL ) {
        ssl_acceptor_params.set_client_threads(client_threads);
      }
      http_net_factory_->SetSslParams(ssl_acceptor_params,
                                      ssl_connection_params);
    }
  }
  if ( client_threads != NULL ) {
    net::TcpConnectionParams tcp_connection_params;
    net::TcpAcceptorParams tcp_acceptor_params(tcp_connection_params);
    if ( client_threads ) {
      tcp_acceptor_params.set_client_threads(client_threads);
    }
    http_net_factory_->SetTcpParams(tcp_acceptor_params,
                                    tcp_connection_params);
  }

  http::ServerParams p;
  p.max_reply_buffer_size_ = FLAGS_http_connection_max_media_outbuf_size;
  p.reply_write_timeout_ms_ = FLAGS_http_connection_write_timeout;
  p.request_read_timeout_ms_ = FLAGS_http_connection_read_timeout;
  p.dlog_level_ = FLAGS_http_connection_dlog_level;
  p.max_concurrent_connections_ = FLAGS_http_max_num_connections;
  // Is essential to have the keep-alive active as our rpc-s should keep-alive
  // The loss for streaming is not big deal..
  //
  // p.keep_alive_timeout_sec_ = 0;  // no keep-alive for us

  http_server_ = new http::Server("Whispercast 0.03",
      selector_, *http_net_factory_, p);
  http_server_->RegisterProcessor(FLAGS_http_base_media_path,
      NewPermanentCallback(this, &Whispercast::ProcessMediaRequest),
      true, true);
  rpc_http_server_ = new http::Server("Whispercast 0.03",
      selector_, *rpc_http_net_factory_, p);

  //////////////////////////////////////////////////////////////////////

  // Add RPC statistics mapping to HTTP server

  if ( !FLAGS_authorization_realm.empty() ) {
    admin_authenticator_ =
        new net::SimpleUserAuthenticator(FLAGS_authorization_realm);
    admin_authenticator_->set_user_password(string("admin"),
                                            FLAGS_admin_passwd);
  } else {
    admin_authenticator_ = NULL;
  }
  rpc_stat_processor_ = new rpc::HttpServer(selector_,
                                            rpc_http_server_,
                                            NULL,  // authenticator_,
                                            "/rpc-stats", true,
                                            true, 10,
                                            FLAGS_rpc_stats_allow_classifier);
  rpc_processor_ = new rpc::HttpServer(selector_,
                                       rpc_http_server_,
                                       admin_authenticator_,
                                       "/rpc-config", true,
                                       true, 10,
                                       FLAGS_rpc_config_allow_classifier);

  rtmp_stream_manager_ = new rtmp::StandardStreamManager(
      selector_,
      NULL,  // will set later..
      stats_collector_,
      FLAGS_rtmp_publishing_application);

  //////////////////////////////////////////////////////////////////////

  CHECK(!FLAGS_media_config_dir.empty() &&
        io::IsDir(FLAGS_media_config_dir.c_str()))
    << " [" << FLAGS_media_config_dir << "]";
  CHECK(FLAGS_base_media_dir.empty() ||
        io::IsDir(FLAGS_base_media_dir.c_str()))
    << " [" << FLAGS_base_media_dir << "]";
  media_mapper_ = new MediaMapper(selector_,
                                  http_server_,
                                  rpc_http_server_,
                                  rpc_processor_,
                                  rtmp_stream_manager_,
                                  admin_authenticator_,
                                  FLAGS_media_config_dir,
                                  FLAGS_base_media_dir,
                                  FLAGS_media_state_dir,
                                  FLAGS_media_state_name,
                                  FLAGS_local_media_state_name);

  rtmp_stream_manager_->set_element_mapper(media_mapper_->mapper());

  //////////////////////////////////////////////////////////////////////

  // TODO: make private
  CHECK(rpc_processor_->RegisterService(media_mapper_));
  CHECK(rpc_stat_processor_->RegisterService(stats_collector_));

  //////////////////////////////////////////////////////////////////////

  // TODO: Add Stream elements
  //       Add exported streams

  //////////////////////////////////////////////////////////////////////

  // Create the RTMP server
  rtmp::GetProtocolFlags(&rtmp_flags_);

  rtmp_server_ = new rtmp::ServerAcceptor(
      selector_,
      client_threads,
      rtmp_stream_manager_,
      "rtmp",
      stats_collector_,
      &classifiers_,
      &rtmp_flags_);

  //////////////////////////////////////////////////////////////////////

  // Register various path mappings

  // Register robots porcessor
  http_server_->RegisterProcessor("/robots.txt",
      NewPermanentCallback(&RobotsProcessor), true, true);
  rpc_http_server_->RegisterProcessor("/robots.txt",
      NewPermanentCallback(&RobotsProcessor), true, true);
  // Register stats processor
  rpc_http_server_->RegisterProcessor("/__stats__",
      NewPermanentCallback(QuickStats,  stats_collector_), true, true);

  //////////////////////////////////////////////////////////////////////

  if ( !FLAGS_http_extensions_to_content_types_file.empty() ) {
    const string s = io::FileInputStream::ReadFileOrDie(
        FLAGS_http_extensions_to_content_types_file.c_str());
    vector< pair<string, string> > exts;
    strutil::SplitPairs(s, "\n",  ":", &exts);
    for ( int i = 0; i < exts.size(); ++i ) {
      ext2ct_[strutil::StrTrim(exts[i].first)] =
          strutil::StrTrim(exts[i].second);
    }
  }

  // Start http serving
  if ( FLAGS_http_port != 0 ) {
    http_server_->AddAcceptor(net::PROTOCOL_TCP,
                              net::HostPort(FLAGS_http_local_address.c_str(),
                                            FLAGS_http_port));
  }
  if ( FLAGS_rpc_http_port != 0 ) {
    rpc_http_server_->AddAcceptor(net::PROTOCOL_TCP,
                                  net::HostPort(
                                      FLAGS_rpc_http_local_address.c_str(),
                                      FLAGS_rpc_http_port));
  }
  if ( FLAGS_https_port != 0 && http_ssl_ctx_ != NULL ) {
    http_server_->AddAcceptor(net::PROTOCOL_SSL,
                              net::HostPort(FLAGS_https_local_address.c_str(),
                                            FLAGS_https_port));
  }
  if ( FLAGS_rpc_https_port != 0 && http_ssl_ctx_ != NULL ) {
    rpc_http_server_->AddAcceptor(
        net::PROTOCOL_SSL,
        net::HostPort(FLAGS_rpc_https_local_address.c_str(),
                      FLAGS_rpc_https_port));
  }
  selector_->RunInSelectLoop(NewCallback(this,
                                         &Whispercast::LoadMediaConfig));
  selector_->RunInSelectLoop(NewCallback(http_server_,
                                         &http::Server::StartServing));
  selector_->RunInSelectLoop(NewCallback(rpc_http_server_,
                                         &http::Server::StartServing));
  // Start rtmp serving
  selector_->RunInSelectLoop(NewCallback(rtmp_server_,
                                         &rtmp::ServerAcceptor::StartServing,
                                         FLAGS_rtmp_port,
                                         FLAGS_rtmp_local_address.c_str()));

  // Create RTSP server
  if ( FLAGS_rtsp_port != 0 ) {
    rtsp_media_interface_ = new streaming::rtsp::ElementMapperMediaInterface(
        media_mapper_->mapper());
    rtsp_server_ = new streaming::rtsp::Server(selector_,
        net::NetFactory(selector_).CreateAcceptor(net::PROTOCOL_TCP),
        net::HostPort(0, FLAGS_rtsp_port), rtsp_media_interface_);
    if ( !rtsp_server_->Start() ) {
      LOG_ERROR << "Failed to start the RTSP server.";
      return -1;
    }
  }

  return 0;
}

void Whispercast::Cleanup() {
  //
  // We crash normally on closing these .. too hard to debug and no real
  // incentive, however, if you want to enable profiling or other things
  // you may want a clean exit, so uncomment this:
  //
  // exit(0);
  //

  for ( int i = 0; i < client_threads_.size(); ++i ) {
    delete client_threads_[i];
  }
  client_threads_.clear();

  //////////////////////////////////////////////////////////////////////

  if ( rtsp_server_ != NULL ) {
    rtsp_server_->Stop();
    delete rtsp_server_;
    rtsp_server_ = NULL;
    delete rtsp_media_interface_;
    rtsp_media_interface_ = NULL;
  }

  //////////////////////////////////////////////////////////////////////

  delete media_mapper_;
  media_mapper_ = NULL;

  //////////////////////////////////////////////////////////////////////

  // Stop RPC
  delete rpc_processor_;
  rpc_processor_ = NULL;
  delete rpc_stat_processor_;
  rpc_stat_processor_ = NULL;
  delete admin_authenticator_;
  admin_authenticator_ = NULL;

  //////////////////////////////////////////////////////////////////////

  // Delete the HTTP server
  delete http_server_;
  http_server_ = NULL;
  delete http_net_factory_;
  http_net_factory_ = NULL;

  delete rpc_http_server_;
  rpc_http_server_ = NULL;
  delete rpc_http_net_factory_;
  rpc_http_net_factory_ = NULL;

  net::SslConnection::SslDeleteContext(http_ssl_ctx_);
  http_ssl_ctx_ = NULL;

  //////////////////////////////////////////////////////////////////////

  // Close the RTMP server

  delete rtmp_stream_manager_;
  rtmp_stream_manager_ = NULL;
  rtmp_server_ = NULL;  // auto-deleted...

  //////////////////////////////////////////////////////////////////////

  // Delete the classifiers
  for ( int i = 0; i < classifiers_.size(); ++i ) {
    delete classifiers_[i];
  }
  classifiers_.clear();

  //////////////////////////////////////////////////////////////////////

  if ( stats_collector_ != NULL ) {
    stats_collector_->Stop();
    delete stats_collector_;
    stats_collector_ = NULL;
  }
}

void Whispercast::Shutdown() {
  // call Cleanup() in case Initialize did not succeed.
  Cleanup();

  delete selector_;
  selector_ = NULL;
}

void Whispercast::ProcessMediaRequest(http::ServerRequest* request) {
  selector_->RunInSelectLoop(
      NewCallback(this, &Whispercast::ProcessMediaRequestCallback, request));
}

void Whispercast::ProcessMediaRequestCallback(http::ServerRequest* request) {
  StreamRequest* srequest = new StreamRequest(connection_id_++,
      selector_, media_mapper_->mapper(), stats_collector_, request);

  const string extension = "." + strutil::Extension(
      request->request()->url()->path());

  map<string, string>::const_iterator it_ext = ext2ct_.find(extension);
  if ( it_ext != ext2ct_.end() ) {
    srequest->Play(it_ext->second.c_str());
  } else {
    srequest->Play("video/x-flv");
  }
}

int main(int argc, char* argv[]) {
  Whispercast app(argc, argv);
  return app.Main();
}
