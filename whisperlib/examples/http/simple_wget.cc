// Copyright (c) 2010, Whispersoft s.r.l.
// All rights reserved.
//
// License Info: New BSD License.
//               Consult the LICENSE.BSD file distributed with this package
//
// For a simple run:
// $ simple_wget --url=http://www.google.com/
//
// To get more debug info in stderr:
// $ simple_wget --url=http://www.google.com/ --alsologtostderr
//
//////////////////////////////////////////////////////////////////////

#include "common/base/types.h"
#include "common/base/log.h"
#include "common/base/system.h"
#include "common/base/gflags.h"
#include "net/base/selector.h"
#include "net/http/http_client_protocol.h"
#include "net/util/resolver.h"

//////////////////////////////////////////////////////////////////////

DEFINE_string(url, "", "URL to download");

void RequestDone(net::Selector* selector, http::ClientRequest* req);

//////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
  // Parse command line, initialize log etc.
  common::Init(argc, argv);

  // Parse and check the url parameter
  CHECK(!FLAGS_url.empty()) << " Please specify a --url";
  URL url(FLAGS_url);
  CHECK(url.is_valid()) << " Invalid url given !";
  CHECK(url.SchemeIs("http") || url.SchemeIs("https")) << "Unknown URL scheme";

  // Resolve the url host
  net::HostPort server(net::util::DnsResolve(url.host(), url.scheme()));
  CHECK(!server.IsInvalid()) << " Cannot resolve: " << url.host();
  if (url.IntPort() != -1) {
    server.set_port(url.IntPort());   // override the port if set in the URL
  }

  // Prepare networking components: Selector wrapps ove an epoll loop.
  net::Selector selector;

  // Parameters for http protocol
  http::ClientParams params;
  // Set some parameters, just as an example:
  params.user_agent_ = "whisperlib quick_wget 0.1";
  params.dlog_level_ = true;

  // Initialize protocol parameters. We can use TCP or SSL under HTTP
  net::NetFactory net_factory(&selector);
  SSL_CTX* ctx = NULL;
  net::PROTOCOL protocol = net::PROTOCOL_TCP;
  // Deal with https (use SSL -> initialize and set parameters to use it)
  if (url.SchemeIsSecure()) {
    ctx = net::SslConnection::SslCreateContext();
    net_factory.SetSslConnectionParams(net::SslConnectionParams(ctx));
    protocol = net::PROTOCOL_SSL;
  }

  // Construct the connection
  http::SimpleClientConnection* const conn =
      new http::SimpleClientConnection(&selector, net_factory, protocol);

  // This drives the concersation w/ the server over conn (takes over conn)
  http::ClientProtocol proto(&params, conn, server);

  // The request for the server (can have multiple requests w/ the same
  // client protocol i.e. on the same connection). Use just one now.
  http::ClientRequest req(http::METHOD_GET, &url);

  // Prepare a functor that will be called when request is done. It will
  // get deleted automatically when called.
  Closure* const done_callback = NewCallback(&RequestDone, &selector, &req);

  // Records a functor to be run when selector starts. It will call
  // SendRequest for the given request (req), with the providede completion
  // callback.
  selector.RunInSelectLoop(NewCallback(&proto,
                                       &http::ClientProtocol::SendRequest,
                                       &req, done_callback));

  // Loop through networking events
  selector.Loop();

  // At the end, clear ssl context - if initialized
  if ( ctx != NULL ) {
    net::SslConnection::SslDeleteContext(ctx);
  }
}

//////////////////////////////////////////////////////////////////////
//
// Function that will be called when request is completed.
// We print some info and force an exit.
//
void RequestDone(net::Selector* selector, http::ClientRequest* req) {
  printf("Request %s finished w/ error: %s\n",
         req->name().c_str(), req->ClientErrorName());
  printf("Response received from server\n"
         "Header:\n%s\n"
         "==============================="
         "Body:\n%s\n",
         req->request()->server_header()->ToString().c_str(),
         req->request()->server_data()->ToString().c_str());
  // Force exit from the networking loop.
  selector->MakeLoopExit();
}
