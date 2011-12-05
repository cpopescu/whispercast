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

#ifndef __NET_RPC_LIB_SERVER_RPC_HTTP_SERVER_H__
#define __NET_RPC_LIB_SERVER_RPC_HTTP_SERVER_H__

#include <map>
#include <string>
#include <whisperlib/common/base/types.h>

#include WHISPER_HASH_SET_HEADER

#include <whisperlib/common/base/log.h>
#include <whisperlib/net/rpc/lib/rpc_constants.h>
#include <whisperlib/net/rpc/lib/codec/binary/rpc_binary_codec.h>
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_codec.h>
#include <whisperlib/net/rpc/lib/server/rpc_core_types.h>
#include <whisperlib/net/rpc/lib/server/rpc_service_invoker.h>
#include <whisperlib/net/http/http_server_protocol.h>
#include <whisperlib/net/base/user_authenticator.h>
#include <whisperlib/net/util/ipclassifier.h>

namespace rpc {

class HttpServer {
 public:
  // Starts serving RPC requests under the provided path
  // Makes the processing calls to be issued in the given select server
  // IMPORTANT: this rpc::HttpServer *must* live longer then the
  //            underlining http::Server
  // We don't own any of the provided objects..
  HttpServer(net::Selector* selector,
             http::Server* server,
             net::UserAuthenticator* authenticator,
             const string& path,
             bool auto_js_forms,
             bool is_public,
             int max_concurrent_requests,
             const string& ip_class_restriction);
  ~HttpServer();

  // Registers a service with this server (will be provided
  // under /..path../
  bool RegisterService(rpc::ServiceInvoker* invoker);
  bool UnregisterService(rpc::ServiceInvoker* invoker);
  // Registers a service with this name (will be provided
  // under /..path../..sub_path../
  // NOTE: sub_path should contain a final slash (if you wish !)
  bool RegisterService(const string& sub_path, rpc::ServiceInvoker* invoker);
  bool UnregisterService(const string& sub_path, rpc::ServiceInvoker* invoker);

  // A special mapping - all rpc callbacks on sub_path_src will be mirrored
  // also on sub_path_dest.
  void RegisterServiceMirror(const string& sub_path_src,
                             const string& sub_path_dest);

  const string& path() const {
    return path_;
  }
 protected:
  // Actual request processing.
  void ProcessRequest(http::ServerRequest* req);

  // The actual processing - if req == NULL - it is a mirrored request..
  void ProcessRequestCore(http::ServerRequest* req,
                          const string& sub_path,
                          const rpc::Transport& transport,
                          const rpc::Message& p,
                          rpc::Codec* codec,
                          int timeout);

  // Request processing continuation (after authentication)
  void ProcessAuthenticatedRequest(
    http::ServerRequest* req,
    string* service_name, string* method_name,
    net::UserAuthenticator::Answer auth_answer);

  // Callback upon query completion
  // The query is about to be deleted after this call,
  // so don't use it for a new scheduled callback.
  void RpcCallback(http::ServerRequest* req,
                   const rpc::Query& query);

  // Callback completion for secondary requests
  void RpcSecondaryCallback(const io::MemoryStream* params,
                            const rpc::Query& query);

  // Callback to send http reply from selector context.
  void RpcReply(http::ServerRequest* req, io::MemoryStream* reply);

  // We run your processing callbacks in the select loop of this selector
  net::Selector* const selector_;

  // If this is set empty we require for our users to be authenticated
  net::UserAuthenticator* const authenticator_;

  // If not null accept clients only from guys identified under this class
  net::IpClassifier* const accepted_clients_;

  // On which path we serve ?
  const string path_;

  // We do not accept more then these concurrent requests
  int max_concurrent_requests_;

  // Current statistics
  int current_requests_;

  // Used for encoding / decoding of stuff
  rpc::BinaryCodec binary_codec_;
  rpc::JsonCodec json_codec_;

  // What services we provide ..
  typedef map<string, rpc::ServiceInvoker*> ServicesMap;
  ServicesMap services_;

  // Services Mirror - forward map -
  // i.e. when we receive a request on given first path we invoke the
  // same function on all seconds
  typedef map<string, hash_set<string>*> MirrorMap;
  MirrorMap mirror_map_;
  // The revers for above
  typedef map<string, string> ReverseMirrorMap;
  ReverseMirrorMap reverse_mirror_map_;

  // Used to keep some javascript that needs included in http forms
  string js_prolog_;
  string auto_forms_log_;

  http::Server* http_server_;

  // TODO(cpopescu): stats !
  //
  // - per each service / method -
  //    - num received (min etc)
  //    - processing time
  //
  // - stats per error case
  // - recent request browsing
  //
 private:
  DISALLOW_EVIL_CONSTRUCTORS(HttpServer);
};
}
#endif  // __NET_RPC_LIBS_SERVER_RPC_HTTP_SERVER_H__
