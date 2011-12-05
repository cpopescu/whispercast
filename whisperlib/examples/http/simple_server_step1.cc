// Copyright (c) 2010, Whispersoft s.r.l.
// All rights reserved.
//
// License Info: New BSD License.
//               Consult the LICENSE.BSD file distributed with this package
//
// Step 1:
//    Starts an http server that publishes a hello world
//
// $ simple_server_step1 --alsologtostderr
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

//////////////////////////////////////////////////////////////////////

void HelloWorld(http::ServerRequest* req);

int main(int argc, char* argv[]) {
  common::Init(argc, argv);

  // Drives the networking show:
  net::Selector selector;

  // Creates connections
  net::NetFactory net_factory(&selector);

  // Parameters for the http server (how requests from clients are processed)
  http::ServerParams params;
  params.max_reply_buffer_size_ = 1<<18;
  params.dlog_level_ = true;

  // The server that knows to talk http
  http::Server server("Test Server 1",       // the name of the server
                      &selector,             // drives networking
                      net_factory,           // parametes for connections
                      params);               // parameters for http

  // Register the fact that we accept connections over TPC transport
  // on the specified port for all local ips.
  server.AddAcceptor(net::PROTOCOL_TCP, net::HostPort(0, FLAGS_port));

  // Register a procesing function. Will process all requests that come under /
  server.RegisterProcessor("/", NewPermanentCallback(&HelloWorld), true, true);

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
