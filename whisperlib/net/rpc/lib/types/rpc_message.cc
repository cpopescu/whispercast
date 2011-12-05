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
#include "net/rpc/lib/types/rpc_message.h"
#include "net/rpc/lib/log_indent.h"

namespace rpc {

const char* MessageTypeName(int32 type) {
  switch ( type ) {
    case RPC_CALL:  return "Call";
    case RPC_REPLY: return "Reply";
    default:        return "Unknown";
  }
}

const char* ReplyStatusName(int32 status) {
  switch ( status ) {
    case RPC_SUCCESS:
      // RPC executed successfully.
      return "Success";
    case RPC_SERVICE_UNAVAIL:
      // Service not registered.
      return "ServiceNotFound";
    case RPC_PROC_UNAVAIL:
      // No such procedure in the given service.
      return "ProcedureNotFound";
    case RPC_GARBAGE_ARGS:
      // Illegal number of params or wrong type for at least 1 param.
      return "GarbageArguments";
    case RPC_SYSTEM_ERR:
      // RPC Server internal error
      return "SystemError";
    case RPC_SERVER_BUSY:
      // Too many parallel calls in execution.
      return "ServerBusy";
    case RPC_QUERY_TIMEOUT:
      // RPC query timeout.
      return "QueryTimeout";
    case RPC_TOO_MANY_QUERIES:
      // RPC Client transport flooded.
      return "TooManyQueries";
    case RPC_CONN_CLOSED:
      // RPC Client transport closed.
      return "ConnectionClosed";
    case RPC_CONN_ERROR:
      // RPC Client transport error.
      return "ConnectionError";
    default:
      return "Unknown";
  }
}


string rpc::Message::Header::ToString() const {
  return strutil::StringPrintf("rpc::Header: [xid:%d, type:%d] - %s",
                               int(xid_), int(msgType_),
                               MessageTypeName(msgType_));
}

rpc::Message::CallBody::CallBody()
    : service_(), method_(), params_() {
}

rpc::Message::CallBody::~CallBody() {
}
string rpc::Message::CallBody::ToString() const {
  return strutil::StringPrintf(
      "rpc::CallBody: [service:%s, method:%s]",
      service_.c_str(), method_.c_str())
#ifdef _DEBUG
      + strutil::StringPrintf(" - params: [%s]", params_.DebugString().c_str())
#endif
      ;
}

rpc::Message::ReplyBody::ReplyBody()
    : replyStatus_(), result_() {
}

rpc::Message::ReplyBody::~ReplyBody() {
}


string rpc::Message::ReplyBody::ToString() const {
  return strutil::StringPrintf(
      "rpc::ReplyBody: [status:%s]",
      rpc::ReplyStatusName(replyStatus_))
#ifdef _DEBUG
      + strutil::StringPrintf(" - result:  [%s]", result_.DebugString().c_str())
#endif
      ;
}

#ifdef _DEBUG
bool rpc::Message::Header::operator==(const rpc::Message::Header& h) const {
  return (xid_ == h.xid_ &&
          msgType_ == h.msgType_);
}
bool rpc::Message::CallBody::operator==(
    const rpc::Message::CallBody& c) const {
  return (service_ == c.service_ &&
          method_ == c.method_ &&
          strutil::StrTrim(params_.DebugString()) ==
          strutil::StrTrim(c.params_.DebugString()));
}
bool rpc::Message::ReplyBody::operator==(
    const rpc::Message::ReplyBody& r) const {
  return (replyStatus_ == r.replyStatus_ &&
          strutil::StrTrim(result_.DebugString()) ==
          strutil::StrTrim(r.result_.DebugString()));
  }
bool rpc::Message::operator==(const rpc::Message& msg) const {
  return  header_ == msg.header_ &&
      ((header_.msgType_ == RPC_CALL &&
        cbody_ == msg.cbody_) ||
       (header_.msgType_ == RPC_REPLY &&
        rbody_ == msg.rbody_));
}
#else
bool rpc::Message::Header::operator==(const rpc::Message::Header& h) const {
  LOG_ERROR << "=======>>> PROBLEM - do not compare messages in RELEASE MODE ";
  return true;
}
bool rpc::Message::CallBody::operator==(
    const rpc::Message::CallBody& c) const {
  LOG_ERROR << "=======>>> PROBLEM - do not compare messages in RELEASE MODE ";
  return true;
}
bool rpc::Message::ReplyBody::operator==(
    const rpc::Message::ReplyBody& r) const {
  LOG_ERROR << "=======>>> PROBLEM - do not compare messages in RELEASE MODE ";
  return true;
}
bool rpc::Message::operator==(const rpc::Message& msg) const {
  LOG_ERROR << "=======>>> PROBLEM - do not compare messages in RELEASE MODE ";
  return true;
}
#endif

string rpc::Message::ToString() const {
  ostringstream ss;
  const Header& header = header_;
  ss << "rpc::Message:" << endl
     << INDENT_RPCMESSAGE << header.ToString() << endl;
  if ( header_.msgType_ == RPC_CALL ) {
    ss << INDENT_RPCMESSAGE << cbody_.ToString();
  } else {
    CHECK_EQ(header_.msgType_, RPC_REPLY);
    ss << INDENT_RPCMESSAGE << rbody_.ToString();
  }
  // no newline! the caller will end it
  return ss.str();
}
}
