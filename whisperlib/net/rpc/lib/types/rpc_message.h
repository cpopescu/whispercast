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

#ifndef __NET_RPC_LIB_TYPES_RPC_MESSAGE_H__
#define __NET_RPC_LIB_TYPES_RPC_MESSAGE_H__

#include <string>
#include <whisperlib/common/base/types.h>
#include <whisperlib/common/io/buffer/io_memory_stream.h>
#include <whisperlib/net/rpc/lib/types/rpc_all_types.h>

namespace rpc {

enum MESSAGE_TYPE {
  RPC_CALL = 0,
  RPC_REPLY = 1,
};
const char* MessageTypeName(int32 type);

enum REPLY_STATUS {
  ///////////////////////////////////////////////////////////////////
  //
  // Server status codes
  //
  RPC_SUCCESS         = 0,  // RPC executed successfully.
  RPC_SERVICE_UNAVAIL = 1,  // No such service.
  RPC_PROC_UNAVAIL    = 2,  // No such procedure in the given service.
  RPC_GARBAGE_ARGS    = 3,  // Illegal number of params
                            //  or wrong type for at least one param.
  RPC_SYSTEM_ERR      = 4,  // Unpredictable errors inside server,
                            //  like memory allocation failure.
                            // Errors in remote method calls should be handled
                            //  and error codes should be returned as valid
                            //  rpc-types. (e.g. a method can return rpc::UInt32
                            //  with value 0=success or (!0)=error code)
  RPC_SERVER_BUSY     = 5,  // The server has to many calls in execution.
  RPC_UNAUTHORIZED    = 6,  // The user is not authorized to perform the call
  ///////////////////////////////////////////////////////////////////
  // Client status codes
  RPC_QUERY_TIMEOUT    = 101,  // The transport layer did not receive
                               //  a response in due time.
  RPC_TOO_MANY_QUERIES = 102,  // The transport layer has too many queries
                               //  pending for send and cannot accept
                               //  another one.
  RPC_CONN_CLOSED      = 103,  // The transport layer was closed while
                               //  waiting for reply.
  RPC_CONN_ERROR       = 104,  // The transport layer was unable to send
                               //  the query.
};
const char* ReplyStatusName(int32 status);

// TODO(cosmin): this (packet) mark has 4 bytes: "rpc\0", while the handshake
//               uses a 3 bytes mark: "rpc". Maybe unify convention?
const char kMessageMark[4] = {'r', 'p', 'c', 0 };

struct Message {
  struct Header {
    // not needed here. The version is estabilished in initial handshake.
    // int32 versionHi_;
    // int32 versionLo_;

    int32 xid_;              // call ID. The response will have the same ID.
    int32 msgType_;          // CALL or REPLY
    bool operator==(const Header& h) const;
    string ToString() const;
  };
  struct CallBody {
    string service_;
    string method_;
    io::MemoryStream params_;
    CallBody();
    ~CallBody();
    bool operator==(const CallBody& c) const;
    string ToString() const;
  };
  struct ReplyBody {
    int32 replyStatus_;         // call status
    io::MemoryStream result_;    // return value
    ReplyBody();
    ~ReplyBody();
    bool operator==(const ReplyBody& r) const;
    string ToString() const;
  };

  Header header_;

  // These 2 below should be a union; only one of them must be used at a time.
  // But because every each of them contain attribs with constructors,
  //  they cannot be part of an union.
  CallBody cbody_;
  ReplyBody rbody_;

  static const char* name() { return "rpc_message"; }

  bool operator==(const Message& msg) const;

  string ToString() const;
};

inline string ToString(const rpc::Message& val) {
  return val.ToString();
}
inline ostream& operator<<(ostream& os, const rpc::Message& m) {
  return os << m.ToString();
}
}

#endif  // __NET_RPC_LIB_TYPES_RPC_MESSAGE_H__
