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

#include "common/base/log.h"
#include "net/url/url.h"
#include "net/rpc/lib/rpc_util.h"

namespace {

// These constants are used to encode/decode RPC URI.
// Many of these URIs are used in configuration files, so do NOT change
// these values!!
const char kRpcConnectionTypeTcp[] = "tcp";
const char kRpcConnectionTypeHttp[] = "http";
const char kRpcConnectionTypeUnknown[] = "unknown";

const char kRpcCodecIdBinary[] = "binary";
const char kRpcCodecIdJson[] = "json";
const char kRpcCodecIdUnknown[] = "unknown";

const char* RpcConnectionTypeToString(
    rpc::CONNECTION_TYPE connection_type) {
  switch ( connection_type ) {
    case rpc::CONNECTION_TCP:  return kRpcConnectionTypeTcp;
    case rpc::CONNECTION_HTTP: return kRpcConnectionTypeHttp;
    default: {
      LOG_FATAL << "no such connection_type: " << connection_type;
      return kRpcConnectionTypeUnknown;
    }
  }
}

rpc::CONNECTION_TYPE RpcConnectionTypeFromString(const string& str) {
  if ( str == kRpcConnectionTypeTcp ) {
    return rpc::CONNECTION_TCP;
  } else if ( str == kRpcConnectionTypeHttp ) {
    return rpc::CONNECTION_HTTP;
  } else {
    return (rpc::CONNECTION_TYPE)(-1);
  }
}

const char* RpcCodecIdToString(rpc::CODEC_ID codec_id) {
  switch ( codec_id ) {
    case rpc::CID_BINARY: return kRpcCodecIdBinary;
    case rpc::CID_JSON:   return kRpcCodecIdJson;
    default: {
      LOG_FATAL << "no such codec_id: " << codec_id;
      return kRpcCodecIdUnknown;
    }
  }
}

rpc::CODEC_ID RpcCodecIdFromString(const string& str) {
  if ( str == kRpcCodecIdBinary ) {
    return rpc::CID_BINARY;
  } else if ( str == kRpcCodecIdJson ) {
    return rpc::CID_JSON;
  } else {
    return (rpc::CODEC_ID)(-1);
  }
}

}  // namespace


namespace rpc {

string CreateUri(const net::HostPort& host_port,
                 const string& path,
                 const string& auth_user,
                 const string& auth_pswd,
                 rpc::CONNECTION_TYPE connection_type,
                 rpc::CODEC_ID codec_id) {
  string str = strutil::StringPrintf(
      "%s://%s/%s?codec=%s",
      RpcConnectionTypeToString(connection_type),
      host_port.ToString().c_str(),
      path.c_str()[0] == '/' ? path.c_str() + 1 : path.c_str(),
      RpcCodecIdToString(codec_id));
  if ( auth_user != "" ) {
    str += strutil::StringPrintf("&user=%s&pswd=%s",
        auth_user.c_str(), auth_pswd.c_str());
  }
  return str;
}

bool ParseUri(const string& address,
              net::HostPort& host_port,
              string& path,
              string& auth_user,
              string& auth_pswd,
              rpc::CONNECTION_TYPE& connection_type,
              rpc::CODEC_ID& codec_id) {
  URL url(address);
  if ( !url.is_valid() ) {
    LOG_ERROR << "invalid address: " << address;
    return false;
  }
  if ( url.port().empty() ) {
    LOG_ERROR << "port not specified in address: " << address;
    return false;
  }
  host_port = net::HostPort(url.host(), url.IntPort());
  if ( host_port.port() == 0 ) {
    LOG_ERROR << "invalid port in address: " << address;
    return false;
  }
  path = url.path();
  connection_type = RpcConnectionTypeFromString(url.scheme());
  if ( connection_type == -1 ) {
    LOG_ERROR << "invalid connection_type in address: " << address;
    return false;
  }
  vector< pair<string, string> > qp;
  url.GetQueryParameters(&qp, true);
  bool codec_id_found = false;
  for ( vector< pair<string, string> >::iterator it = qp.begin();
        it != qp.end(); ++it ) {
    const string& param = it->first;
    const string& value = it->second;
    if ( param == "codec" ) {
      codec_id = RpcCodecIdFromString(value);
      if ( codec_id == -1 ) {
        LOG_ERROR << "invalid codec_id in address: " << address;
        return false;
      }
      codec_id_found = true;
      continue;
    }
    if ( param == "user" ) {
      auth_user = value;
      continue;
    }
    if ( param == "pswd" ) {
      auth_pswd = value;
      continue;
    }
    LOG_ERROR << "Unknown URI parameter, name: [" << param << "]"
                 ", value: [" << value << "], ignoring..";
  }
  if ( !codec_id_found ) {
    LOG_ERROR << "missing codec_id in address: " << address;
    return false;
  }
  return true;
}
}
