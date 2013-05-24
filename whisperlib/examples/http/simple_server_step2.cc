// Copyright (c) 2010, Whispersoft s.r.l.
// All rights reserved.
//
// License Info: New BSD License.
//               Consult the LICENSE.BSD file distributed with this package
//
// Step 1:
//    Starts an http server that publishes a hello world
// Step 2:
//    Serve the request multiple threads
//
// $ simple_server_step2 --alsologtostderr
//
//////////////////////////////////////////////////////////////////////

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/system.h>
#include <whisperlib/common/base/gflags.h>

#include <whisperlib/net/base/selector.h>
#include <whisperlib/net/http/http_server_protocol.h>

//////////////////////////////////////////////////////////////////////

DEFINE_int32(port, 8980, "Serve on this port");
DEFINE_int32(num_threads, 10, "Serve with these may worker threads");

//////////////////////////////////////////////////////////////////////

void HelloWorld(http::ServerRequest* req);

int main(int argc, char* argv[]) {
  common::Init(argc, argv);

  // Drives the networking show:
  net::Selector selector;

  // Prepare the serving therads
  vector<net::SelectorThread*> working_threads;
  for ( int i = 0; i < FLAGS_num_threads; ++i ) {
    working_threads.push_back(new net::SelectorThread());
    working_threads.back()->Start();
  }

  // Setup connection worker threads
  net::NetFactory net_factory(&selector);
  net::TcpAcceptorParams acceptor_params;
  acceptor_params.set_client_threads(&working_threads);
  net_factory.SetTcpParams(acceptor_params);

  // Parameters for the http server (how requests from clients are processed)
  http::ServerParams params;
  params.max_reply_buffer_size_ = 1<<18;
  params.dlog_level_ = true;

  // The server that knows to talk http
  http::Server server("Test Server 2",       // the name of the server
                      &selector,             // drives networking
                      net_factory,           // parametes for connections
                      params);               // parameters for http

  // Register the fact that we accept connections over TPC transport
  // on the specified port for all local ips.
  server.AddAcceptor(net::PROTOCOL_TCP, net::HostPort(0, FLAGS_port));

  // Register a procesing function. Will process all requests that come under /
  server.RegisterProcessor("", NewPermanentCallback(&HelloWorld), true, true);

  // Start the server as soon as networking starts
  selector.RunInSelectLoop(NewCallback(&server, &http::Server::StartServing));

  // Write a message for the user:
  LOG_INFO << " ==========> Starting. Point your browser to: "
           << "http://localhost:" << FLAGS_port << "/";

  // Start networking loop
  selector.Loop();

  LOG_INFO << "DONE";
}

//////////////////////////////////////////////////////////////////////

void HelloWorld(http::ServerRequest* req) {
  // Prepare some message:
  const string reply(strutil::StringPrintf(
                         "<h1>Hello World</h1>\n"
                         "<p>Welcome from: <b>%s</b> on url: <b>%s</b></p>\n",
                         req->remote_address().ToString().c_str(),
                         req->request()->url()->spec().c_str()));

  // Write the message out:
  req->request()->server_data()->Write(reply);

  // Done with this request - send the answer back
  req->Reply();
}
