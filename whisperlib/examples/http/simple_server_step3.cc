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
// Step 3:
//    Serve files from memory, files from (and under) a base directory.
//
// $ simple_server_step3 --alsologtostderr
//                       --base_dir=<some non public dir to serve>
//
//////////////////////////////////////////////////////////////////////

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/system.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/io/file/file.h>
#include <whisperlib/common/io/file/file_input_stream.h>
#include <whisperlib/common/io/ioutil.h>

#include <whisperlib/net/base/selector.h>
#include <whisperlib/net/http/http_server_protocol.h>

//////////////////////////////////////////////////////////////////////

DEFINE_int32(port, 8980, "Serve on this port");
DEFINE_int32(num_threads, 10, "Serve with these may worker threads");
DEFINE_int64(max_file_size, 10 << 20, "Serve files of at most this size");
DEFINE_string(base_dir, "/tmp", "Server files under this directory");

//////////////////////////////////////////////////////////////////////

void InitExtensions();
void ProcessRequest(http::ServerRequest* req);

int main(int argc, char* argv[]) {
  common::Init(argc, argv);

  // Prepares the extensions map (extension to content type)
  InitExtensions();

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
  http::Server server("Test Server 3",       // the name of the server
                      &selector,             // drives networking
                      net_factory,           // parametes for connections
                      params);               // parameters for http

  // Register the fact that we accept connections over TPC transport
  // on the specified port for all local ips.
  server.AddAcceptor(net::PROTOCOL_TCP, net::HostPort(0, FLAGS_port));

  // Register a procesing function. Will process all requests that come under /
  server.RegisterProcessor("/", NewPermanentCallback(&ProcessRequest),
      true, true);

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

// Extension to content type mapping

static const char kExtensions[] =
    ".pdf:application/pdf,.gz:application/x-gzip,.zip:application/zip,"
    ".gif:image/gif,.jpg:image/jpeg,.jpeg:image/jpeg,.png:image/png,"
    ".css:text/css,.html:text/html,.htm:text/html,.js:text/javascript,"
    ".text:text/plain,.txt:text/plain,.xml:text/xml,"
    ".mpeg:video/mpeg,.mpg:video/mpeg,.flv:video/x-flv,.f4v:video/x-m4v,"
    ".mov:video/quicktime,.mp3:audio/mpeg,.ogg:application/ogg";
static map<string, string> glb_extensions;

void InitExtensions() {
  strutil::SplitPairs(kExtensions, ",", ":", &glb_extensions);
}

const string& GetContentType(const string& filename) {
  static const string kDefaultContentType = "application/octet-stream";
  const string ext = "." + strutil::Extension(filename);
  LOG_INFO << " EXTENSION: " << ext
           << " -- " << strutil::ToString(glb_extensions);
  const map<string, string>::const_iterator it = glb_extensions.find(ext);
  if ( it != glb_extensions.end() ) {
    return it->second;
  }
  return kDefaultContentType;
}

//////////////////////////////////////////////////////////////////////

// Lists a directory and generate some html that with links inside it.

void GenerateDirListing(http::ServerRequest* req,
                        const string& url_path, const string& dir_path) {
  vector<string> dir_names;
  if ( !io::DirList(dir_path, &dir_names, true) ) {
    // Dir listing not allowed
    req->ReplyWithStatus(http::FORBIDDEN);
    return;
  }
  LOG_INFO << " Serving directory request : " << url_path
           << " [" << dir_path << "]";
  req->request()->server_data()->Write(
      strutil::StringPrintf("<h1>Directory: %s</h1><ul>", url_path.c_str()));

  // Prepare the directory listing.
  for ( int i = 0; i < dir_names.size(); i++ ) {
    const string crt_path(dir_path + "/" + dir_names[i]);
    req->request()->server_data()->Write(
        strutil::StringPrintf(
            "\n<li><a href=\"%s\">%s</a></li>",
            URL::UrlEscape(
                strutil::JoinPaths(url_path, dir_names[i])).c_str(),
            URL::UrlEscape(dir_names[i].c_str()).c_str()));
  }
  req->Reply();
}

void ServeFile(http::ServerRequest* req,
               const string& file_path) {
  // Open (maybe) the file.
  io::File* const infile = io::File::TryOpenFile(file_path.c_str());
  if ( infile == NULL ) {
    // File not found
    req->request()->server_data()->Write("<h1>Not Found</h1>");
    req->ReplyWithStatus(http::NOT_FOUND);
  } else {
    // Determine the size of the file
    io::FileInputStream fis(infile);
    const int64 size = fis.Readable();
    if ( size > FLAGS_max_file_size ) {
      // File too large to serve
    req->request()->server_data()->Write("<h1>File too big</h1>");
      req->ReplyWithStatus(http::NOT_IMPLEMENTED);
    } else {
      // Read the file in memory and send it out.
      LOG_INFO << " Serving file per request : [" << file_path << "]";
      req->request()->server_header()->AddField(
          http::kHeaderContentType, GetContentType(file_path), true);
      char* const buffer = new char[size];
      const int64 cb = fis.Read(buffer, size);
      if (cb > 0) {
        req->request()->server_data()->AppendRaw(buffer, cb);
      }
      req->Reply();
    }
  }
}

//////////////////////////////////////////////////////////////////////

// Request processing function
void ProcessRequest(http::ServerRequest* req) {
  URL* const url = req->request()->url();
  const string url_path(url->UrlUnescape(url->path().c_str(),
                                         url->path().size()));

  // The file pointed by the request
  const string file_path(strutil::JoinPaths(FLAGS_base_dir, url_path));

  // Determine the validity of the request
  if (!strutil::StrStartsWith(file_path, FLAGS_base_dir)) {
    // They try .. / stuff
    req->ReplyWithStatus(http::NOT_FOUND);
    return;
  }

  // Treat directories and files differently
  if ( io::IsDir(file_path.c_str()) ) {
    GenerateDirListing(req, url_path, file_path);
  } else {
    ServeFile(req, file_path);
  }
}
