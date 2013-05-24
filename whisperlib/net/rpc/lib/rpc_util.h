// Copyright (c) 2009, Whispersoft s.r.l. rights reserved.
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

#ifndef __NET_RPC_LIB_RPC_UTIL_H__
#define __NET_RPC_LIB_RPC_UTIL_H__

#include <string>
#include <vector>

#include <whisperlib/net/base/address.h>
#include <whisperlib/net/rpc/lib/types/rpc_all_types.h>
#include <whisperlib/net/rpc/lib/client/irpc_client_connection.h>

namespace rpc {
//  Creates an all-in-one address containing all the given parameters.
//  e.g.: "http://192.168.2.3:8081/rpc/manager/master?codec=json"
string CreateUri(const net::HostPort& host_port,
                 const string& path,
                 const string& auth_user,
                 const string& auth_pswd,
                 rpc::CONNECTION_TYPE connection_type,
                 rpc::CodecId codec);

//  The reverse of CreateUri.
// input:
//  [IN]  address: the all-in-one master address to be parsed
//  [OUT] host_port: receives the ip & port
//  [OUT] path: receives the request path (useful only for HTTP)
//  [OUT] auth_user: receives the authentication username (useful only for HTTP)
//  [OUT] auth_pswd: receives the authentication password (useful only for HTTP)
//  [OUT] connection_type: receives the connection type (HTTP, TCP)
//  [OUT] codec_id: receives the codec id (RPC_CID_BINARY, RPC_CID_JSON)
// returns:
//   success state.
//
// e.g.:
//    for uri:
// http://192.168.2.3:8081/rpc/manager/master?codec=json&user=admin&pswd=abcdef
//  returns:
//      host_port: 192.168.2.3:8081
//      path: "/rpc/manager/master"
//      auth_user: "admin"
//      auth_pswd: "abcdef"
//      connection_type: rpc::CONNECTION_HTTP
//      codec_id: RPC_CID_JSON
//
//    for uri: "tcp://192.168.2.3:1234?codec=binary" returns:
//      host_port: 192.168.2.3:1234
//      path: ""
//      auth_user: ""
//      auth_pswd: ""
//      connection_type: rpc::CONNECTION_TCP
//      codec_id: RPC_CID_BINARY
bool ParseUri(const string& uri,
              net::HostPort* out_host_port,
              string* out_path,
              string* out_auth_user,
              string* out_auth_pswd,
              rpc::CONNECTION_TYPE* out_connection_type,
              rpc::CodecId* out_codec);
}

#endif   // __NET_RPC_LIB_RPC_UTIL_H__
