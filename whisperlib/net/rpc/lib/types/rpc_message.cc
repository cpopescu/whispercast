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

#include <whisperlib/common/base/log.h>
#include <whisperlib/net/rpc/lib/types/rpc_message.h>
#include <whisperlib/net/rpc/lib/codec/rpc_encoder.h>
#include <whisperlib/net/rpc/lib/codec/rpc_decoder.h>
#include <whisperlib/net/rpc/lib/log_indent.h>

namespace rpc {

const char* MessageTypeName(MESSAGE_TYPE type) {
  switch ( type ) {
    case RPC_CALL:  return "Call";
    case RPC_REPLY: return "Reply";
  }
  LOG_FATAL << "Illegal MessageType: " << type;
}

const char* ReplyStatusName(REPLY_STATUS status) {
  switch ( status ) {
    case RPC_SUCCESS:          return "Success";
    case RPC_SERVICE_UNAVAIL:  return "ServiceNotFound";
    case RPC_PROC_UNAVAIL:     return "ProcedureNotFound";
    case RPC_GARBAGE_ARGS:     return "GarbageArguments";
    case RPC_SYSTEM_ERR:       return "SystemError";
    case RPC_SERVER_BUSY:      return "ServerBusy";
    case RPC_UNAUTHORIZED:     return "Unauthorized";
    case RPC_QUERY_TIMEOUT:    return "QueryTimeout";
    case RPC_TOO_MANY_QUERIES: return "TooManyQueries";
    case RPC_CONN_CLOSED:      return "ConnectionClosed";
    case RPC_CONN_ERROR:       return "ConnectionError";
  }
  LOG_FATAL << "Illegal reply status: " << status;
  return "Unknown";
}
bool IsValidReplyStatus(uint32 reply_status) {
  switch ( static_cast<REPLY_STATUS>(reply_status) ) {
    case RPC_SUCCESS:
    case RPC_SERVICE_UNAVAIL:
    case RPC_PROC_UNAVAIL:
    case RPC_GARBAGE_ARGS:
    case RPC_SYSTEM_ERR:
    case RPC_SERVER_BUSY:
    case RPC_UNAUTHORIZED:
    case RPC_QUERY_TIMEOUT:
    case RPC_TOO_MANY_QUERIES:
    case RPC_CONN_CLOSED:
    case RPC_CONN_ERROR:
      return true;
  }
  return false;
}

Message::Header::Header()
  : xid_(0), msgType_(RPC_CALL) {}
Message::Header::Header(int32 xid, MESSAGE_TYPE msgType)
  : xid_(xid), msgType_(msgType) {}

DECODE_RESULT Message::Header::SerializeLoad(Decoder& dec, io::MemoryStream& in) {
  DECODE_VERIFY(dec.DecodeStructStart(in));
  bool found_xid = false;
  bool found_msg_type = false;
  while ( true ) {
    DECODE_VERIFY(dec.DecodeStructAttribStart(in));
    string attr_name;
    DECODE_VERIFY(dec.Decode(in, &attr_name));
    DECODE_VERIFY(dec.DecodeStructAttribMiddle(in));
    if ( attr_name == "xid" ) {
      if ( found_xid ) {
        LOG_ERROR << "Duplicate attribute: xid";
        return DECODE_RESULT_ERROR;
      }
      found_xid = true;
      DECODE_VERIFY(dec.Decode(in, &xid_));
    } else if (attr_name == "msgType" ) {
      if ( found_msg_type ) {
        LOG_ERROR << "Duplicate attribute: msgType";
        return DECODE_RESULT_ERROR;
      }
      found_msg_type = true;
      uint32 msg_type = 0;
      DECODE_VERIFY(dec.Decode(in, &msg_type));
      if ( msg_type != RPC_CALL && msg_type != RPC_REPLY ) {
        LOG_ERROR << "Invalid message type: " << msg_type;
        return DECODE_RESULT_ERROR;
      }
      msgType_ = static_cast<MESSAGE_TYPE>(msg_type);
    } else {
      LOG_ERROR << "Unknown attribute name: [" << attr_name << "]";
      return DECODE_RESULT_ERROR;
    }
    bool more_attribs = false;
    DECODE_VERIFY(dec.DecodeStructContinue(in, &more_attribs));
    if ( !more_attribs ) {
      break;
    }
  }
  if ( !found_xid ) {
    LOG_ERROR << "Missing attribute: xid";
    return DECODE_RESULT_ERROR;
  }
  if ( !found_msg_type ) {
    LOG_ERROR << "Missing attribute: msgType";
    return DECODE_RESULT_ERROR;
  }
  return DECODE_RESULT_SUCCESS;
}
void Message::Header::SerializeSave(Encoder& enc, io::MemoryStream* out) const {
  enc.EncodeStructStart(2, out);
  enc.EncodeStructAttrib("xid", xid_, out);
  enc.EncodeStructContinue(out);
  enc.EncodeStructAttrib("msgType", (uint32)msgType_, out);
  enc.EncodeStructEnd(out);
}

string Message::Header::ToString() const {
  return strutil::StringPrintf("Header{xid: %d, type: %s}",
      int(xid_), MessageTypeName(msgType_));
}

/////////////////////////////////////////////////////////////////////////////

Message::CallBody::CallBody() {}
Message::CallBody::CallBody(const string& service, const string& method,
                            const io::MemoryStream* params)
    : service_(service), method_(method) {
  if ( params != NULL ) {
    params_.AppendStreamNonDestructive(params);
  }
}

DECODE_RESULT Message::CallBody::SerializeLoad(Decoder& dec, io::MemoryStream& in) {
  DECODE_VERIFY(dec.DecodeStructStart(in));
  bool found_service = false;
  bool found_method = false;
  bool found_params = false;
  while ( true ) {
    DECODE_VERIFY(dec.DecodeStructAttribStart(in));
    string attr_name;
    DECODE_VERIFY(dec.Decode(in, &attr_name));
    DECODE_VERIFY(dec.DecodeStructAttribMiddle(in));
    if ( attr_name == "service" ) {
      if ( found_service ) {
        LOG_ERROR << "Duplicate attribute: service";
        return DECODE_RESULT_ERROR;
      }
      found_service = true;
      DECODE_VERIFY(dec.Decode(in, &service_));
    } else if (attr_name == "method" ) {
      if ( found_method ) {
        LOG_ERROR << "Duplicate attribute: method";
        return DECODE_RESULT_ERROR;
      }
      found_method = true;
      DECODE_VERIFY(dec.Decode(in, &method_));
    } else if (attr_name == "params" ) {
      if ( found_params ) {
        LOG_ERROR << "Duplicate attribute: params";
        return DECODE_RESULT_ERROR;
      }
      found_params = true;
      DECODE_VERIFY(dec.ReadRawObject(in, &params_));
    } else {
      LOG_ERROR << "Unknown attribute name: [" << attr_name << "]";
      return DECODE_RESULT_ERROR;
    }
    bool more_attribs = false;
    DECODE_VERIFY(dec.DecodeStructContinue(in, &more_attribs));
    if ( !more_attribs ) {
      break;
    }
  }
  if ( !found_service ) {
    LOG_ERROR << "Missing attribute: service";
    return DECODE_RESULT_ERROR;
  }
  if ( !found_method ) {
    LOG_ERROR << "Missing attribute: method";
    return DECODE_RESULT_ERROR;
  }
  if ( !found_params ) {
    LOG_ERROR << "Missing attribute: params";
    return DECODE_RESULT_ERROR;
  }
  return DECODE_RESULT_SUCCESS;
}
void Message::CallBody::SerializeSave(Encoder& enc, io::MemoryStream* out) const {
  enc.EncodeStructStart(3, out);
  enc.EncodeStructAttrib("service", service_, out);
  enc.EncodeStructContinue(out);
  enc.EncodeStructAttrib("method", method_, out);
  enc.EncodeStructContinue(out);
  enc.EncodeStructAttribStart(out);
  enc.Encode("params", out);
  enc.EncodeStructAttribMiddle(out);
  enc.WriteRawObject(params_, out);
  enc.EncodeStructAttribEnd(out);
  enc.EncodeStructEnd(out);
}

string Message::CallBody::ToString() const {
  ostringstream oss;
  oss << "CallBody{service_: " << service_
      << ", method_: " << method_;
#ifdef _DEBUG
  oss << ", params_: " << params_.DebugString();
#endif
  oss << "}";
  return oss.str();
}

/////////////////////////////////////////////////////////////////////////

Message::ReplyBody::ReplyBody()
    : replyStatus_(RPC_SUCCESS) {
}
Message::ReplyBody::ReplyBody(REPLY_STATUS replyStatus, const io::MemoryStream* result)
    : replyStatus_(replyStatus) {
  if ( result != NULL ) {
    result_.AppendStreamNonDestructive(result);
  }
}

DECODE_RESULT Message::ReplyBody::SerializeLoad(Decoder& dec, io::MemoryStream& in) {
  DECODE_VERIFY(dec.DecodeStructStart(in));
  bool found_reply_status = false;
  bool found_result = false;
  while ( true ) {
    DECODE_VERIFY(dec.DecodeStructAttribStart(in));
    string attr_name;
    DECODE_VERIFY(dec.Decode(in, &attr_name));
    DECODE_VERIFY(dec.DecodeStructAttribMiddle(in));
    if ( attr_name == "replyStatus" ) {
      if ( found_reply_status ) {
        LOG_ERROR << "Duplicate attribute: replyStatus";
        return DECODE_RESULT_ERROR;
      }
      found_reply_status = true;
      uint32 reply_status = 0;
      DECODE_VERIFY(dec.Decode(in, &reply_status));
      if ( !IsValidReplyStatus(reply_status) ) {
        LOG_ERROR << "Invalid reply status: " << reply_status;
        return DECODE_RESULT_ERROR;
      }
      replyStatus_ = static_cast<REPLY_STATUS>(reply_status);
    } else if (attr_name == "result" ) {
      if ( found_result ) {
        LOG_ERROR << "Duplicate attribute: result";
        return DECODE_RESULT_ERROR;
      }
      found_result = true;
      DECODE_VERIFY(dec.ReadRawObject(in, &result_));
    } else {
      LOG_ERROR << "Unknown attribute name: [" << attr_name << "]";
      return DECODE_RESULT_ERROR;
    }
    bool more_attribs = false;
    DECODE_VERIFY(dec.DecodeStructContinue(in, &more_attribs));
    if ( !more_attribs ) {
      break;
    }
  }
  if ( !found_reply_status ) {
    LOG_ERROR << "Missing attribute: replyStatus";
    return DECODE_RESULT_ERROR;
  }
  if ( !found_result ) {
    LOG_ERROR << "Missing attribute: result";
    return DECODE_RESULT_ERROR;
  }
  return DECODE_RESULT_SUCCESS;
}
void Message::ReplyBody::SerializeSave(Encoder& enc, io::MemoryStream* out) const {
  enc.EncodeStructStart(2, out);
  enc.EncodeStructAttrib("replyStatus", replyStatus_, out);
  enc.EncodeStructContinue(out);
  enc.EncodeStructAttribStart(out);
  enc.Encode("result", out);
  enc.EncodeStructAttribMiddle(out);
  enc.WriteRawObject(result_, out);
  enc.EncodeStructAttribEnd(out);
  enc.EncodeStructEnd(out);
}
string Message::ReplyBody::ToString() const {
  ostringstream oss;
  oss << "ReplyBody{replyStatus_: " << ReplyStatusName(replyStatus_);
#ifdef _DEBUG
  oss << ", result_: " << result_.DebugString();
#endif
  oss << "}";
  return oss.str();
}

///////////////////////////////////////////////////////////////////////////////

Message::Message(int32 xid, MESSAGE_TYPE msgType,
                 const string& service, const string& method,
                 const io::MemoryStream* params)
  : header_(xid, msgType),
    cbody_(service, method, params),
    rbody_() {
}
Message::Message(int32 xid, MESSAGE_TYPE msgType,
                 REPLY_STATUS replyStatus, const io::MemoryStream* result)
  : header_(xid, msgType),
    cbody_(),
    rbody_(replyStatus, result) {
}

DECODE_RESULT Message::SerializeLoad(Decoder& dec, io::MemoryStream& in) {
  DECODE_VERIFY(dec.DecodeStructStart(in));
  bool found_header = false;
  bool found_cbody = false;
  bool found_rbody = false;
  while ( true ) {
    DECODE_VERIFY(dec.DecodeStructAttribStart(in));
    string attr_name;
    DECODE_VERIFY(dec.Decode(in, &attr_name));
    DECODE_VERIFY(dec.DecodeStructAttribMiddle(in));
    if ( attr_name == "header" ) {
      if ( found_header ) {
        LOG_ERROR << "Duplicate attribute: header";
        return DECODE_RESULT_ERROR;
      }
      found_header = true;
      DECODE_VERIFY(header_.SerializeLoad(dec, in));
    } else if (attr_name == "cbody" ) {
      if ( found_cbody ) {
        LOG_ERROR << "Duplicate attribute: cbody";
        return DECODE_RESULT_ERROR;
      }
      found_cbody = true;
      DECODE_VERIFY(cbody_.SerializeLoad(dec, in));
    } else if (attr_name == "rbody" ) {
      if ( found_rbody ) {
        LOG_ERROR << "Duplicate attribute: rbody";
        return DECODE_RESULT_ERROR;
      }
      found_rbody = true;
      DECODE_VERIFY(rbody_.SerializeLoad(dec, in));
    } else {
      LOG_ERROR << "Unknown attribute name: [" << attr_name << "]";
      return DECODE_RESULT_ERROR;
    }
    bool more_attribs = false;
    DECODE_VERIFY(dec.DecodeStructContinue(in, &more_attribs));
    if ( !more_attribs ) {
      break;
    }
  }
  if ( !found_header ) {
    LOG_ERROR << "Missing attribute: header";
    return DECODE_RESULT_ERROR;
  }


  return DECODE_RESULT_SUCCESS;
}
void Message::SerializeSave(Encoder& enc, io::MemoryStream* out) const {
  enc.EncodeStructStart(2, out);
  enc.EncodeStructAttribStart(out);
  enc.Encode("header", out);
  enc.EncodeStructAttribMiddle(out);
  header_.SerializeSave(enc, out);
  enc.EncodeStructAttribEnd(out);
  enc.EncodeStructContinue(out);
  enc.EncodeStructAttribStart(out);
  enc.Encode(header_.msgType() == RPC_CALL ? "cbody" : "rbody", out);
  enc.EncodeStructAttribMiddle(out);
  if ( header_.msgType() == RPC_CALL ) {
    cbody_.SerializeSave(enc, out);
  } else if ( header_.msgType() == RPC_REPLY ) {
    rbody_.SerializeSave(enc, out);
  } else {
    LOG_FATAL << "Illegal msgType: " << header_.msgType();
  }
  enc.EncodeStructAttribEnd(out);
  enc.EncodeStructEnd(out);
}

string Message::ToString() const {
  ostringstream oss;
  oss << "rpc::Message{header_: " << header_.ToString() << ", ";
  if ( header_.msgType() == RPC_CALL ) {
    oss << "cbody_: " << cbody_.ToString();
  } else if ( header_.msgType() == RPC_REPLY ) {
    oss << "rbody_: " << rbody_.ToString();
  } else {
    LOG_FATAL << "Illegal msgType: " << header_.msgType();
  }
  oss << "}";
  return oss.str();
}
}
