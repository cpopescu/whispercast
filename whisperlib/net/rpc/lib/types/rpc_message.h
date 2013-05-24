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
#include <whisperlib/net/rpc/lib/codec/rpc_decoder.h>
#include <whisperlib/net/rpc/lib/codec/rpc_encoder.h>

namespace rpc {

enum MESSAGE_TYPE {
  RPC_CALL = 0,
  RPC_REPLY = 1,
};
const char* MessageTypeName(MESSAGE_TYPE type);

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
const char* ReplyStatusName(REPLY_STATUS status);
bool IsValidReplyStatus(uint32 reply_status);

class Message {
 public:
  class Header {
   public:
    Header();
    Header(int32 xid, MESSAGE_TYPE msgType);

    int32 xid() const { return xid_; }
    MESSAGE_TYPE msgType() const { return msgType_; }

    void set_xid(int32 xid) { xid_ = xid; }
    void set_msgType(MESSAGE_TYPE msgType) { msgType_ = msgType; }

    DECODE_RESULT SerializeLoad(Decoder& dec, io::MemoryStream& in);
    void SerializeSave(Encoder& enc, io::MemoryStream* out) const;
#ifdef _DEBUG
    bool operator==(const Header& other) const {
      return xid_ == other.xid_ && msgType_ == other.msgType_;
    }
#endif
    string ToString() const;
   private:
    // not needed here. The version is estabilished in initial handshake.
    // int32 versionHi_;
    // int32 versionLo_;

    // call ID. The response will have the same ID.
    int32 xid_;
    // CALL or REPLY
    MESSAGE_TYPE msgType_;
  };

  class CallBody {
   public:
    CallBody();
    CallBody(const string& service, const string& method,
             const io::MemoryStream* params = NULL);

    const string& service() const { return service_; }
    const string& method() const { return method_; }
    const io::MemoryStream& params() const { return params_; }

    void set_service(const string& service) { service_ = service; }
    void set_method(const string& method) { method_ = method; }
    io::MemoryStream* mutable_params() { return &params_; }

    DECODE_RESULT SerializeLoad(Decoder& dec, io::MemoryStream& in);
    void SerializeSave(Encoder& enc, io::MemoryStream* out) const;
#ifdef _DEBUG
    bool operator==(const CallBody& other) const {
      return service_ == other.service_ &&
             method_ == other.method_ &&
             params_.Equals(other.params_);
    }
#endif
    string ToString() const;
   private:
    string service_;
    string method_;
    io::MemoryStream params_;
  };
  class ReplyBody {
   public:
    ReplyBody();
    ReplyBody(REPLY_STATUS replyStatus, const io::MemoryStream* result);

    REPLY_STATUS replyStatus() const { return replyStatus_; }
    const io::MemoryStream& result() const { return result_; }

    void set_replyStatus(REPLY_STATUS replyStatus) {replyStatus_ = replyStatus;}
    io::MemoryStream* mutable_result() { return &result_; }

    DECODE_RESULT SerializeLoad(Decoder& dec, io::MemoryStream& in);
    void SerializeSave(Encoder& enc, io::MemoryStream* out) const;
#ifdef _DEBUG
    bool operator==(const ReplyBody& other) const {
      return replyStatus_ == other.replyStatus_ &&
             result_.Equals(other.result_);
    }
#endif
    string ToString() const;
   private:
    REPLY_STATUS replyStatus_;   // call status
    io::MemoryStream result_;    // return value
  };

  Message() {}
  Message(int32 xid, MESSAGE_TYPE msgType,
          const string& service, const string& method,
          const io::MemoryStream* params = NULL);
  Message(int32 xid, MESSAGE_TYPE msgType,
          REPLY_STATUS replyStatus,
          const io::MemoryStream* result = NULL);

  const Header& header() const { return header_; }
  const CallBody& cbody() const { return cbody_; }
  const ReplyBody& rbody() const { return rbody_; }

  Header* mutable_header() { return &header_; }
  CallBody* mutable_cbody() { return &cbody_; }
  ReplyBody* mutable_rbody() { return &rbody_; }

  DECODE_RESULT SerializeLoad(Decoder& dec, io::MemoryStream& in);
  void SerializeSave(Encoder& enc, io::MemoryStream* out) const;
#ifdef _DEBUG
  bool operator==(const Message& other) const {
    return  header_ == other.header_ &&
            ((header().msgType() == RPC_CALL && cbody_ == other.cbody()) ||
             (header().msgType() == RPC_REPLY && rbody_ == other.rbody()));
  }
#endif

  string ToString() const;

 private:
  Header header_;

  // These 2 below should be a union; only one of them must be used at a time.
  // But because every each of them contain attribs with constructors,
  //  they cannot be part of an union.
  CallBody cbody_;
  ReplyBody rbody_;
};

inline ostream& operator<<(ostream& os, const rpc::Message& m) {
  return os << m.ToString();
}
}

#endif  // __NET_RPC_LIB_TYPES_RPC_MESSAGE_H__
