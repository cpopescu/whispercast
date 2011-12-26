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

#include "rtmp/events/rtmp_call.h"
#include "rtmp/rtmp_consts.h"
#include "rtmp/objects/amf/amf0_util.h"


// namespace {
// static const char kActionConnect[] = "connect";
// static const char kActionDisconnect[] = "disconnect";
// static const char kActionCreateStream[] = "createStream";
// static const char kActionDeleteStream[] = "deleteStream";
// static const char kActionCloseStream[] = "closeStream";
// static const char kActionReleaseStream[] = "releaseStream";
// static const char kActionPublish[] = "publish";
// static const char kActionPause[] = "pause";
// static const char kActionSeek[] = "seek";
// static const char kActionPlay[] = "play";
// static const char kActionStop[] = "disconnect";
// static const char kActionReceiveVideo[] = "receiveVideo";
// static const char kActionReceiveAudio[] = "receiveAudio";
// // TODO(cpopescu): - get smarter about this .. (a hash_set or something)
// bool RtmpIsStreamCommand(const string& cmd) {
//   return (cmd == rtmp::kActionCreateStream ||
//           cmd == rtmp::kActionDeleteStream ||
//           cmd == rtmp::kActionCloseStream ||
//           cmd == rtmp::kActionPublish ||
//           cmd == rtmp::kActionPause ||
//           cmd == rtmp::kActionSeek ||
//           cmd == rtmp::kActionPlay  ||
//           cmd == rtmp::kActionReceiveVideo ||
//           cmd == rtmp::kActionReceiveAudio);
// }
// }

namespace rtmp {

const char* Call::CallStatusName(CallStatus status) {
  switch ( status ) {
    CONSIDER(CALL_STATUS_PENDING);
    CONSIDER(CALL_STATUS_SUCCESS_RESULT);
    CONSIDER(CALL_STATUS_SUCCESS_NULL);
    CONSIDER(CALL_STATUS_SUCCESS_VOID);
    CONSIDER(CALL_STATUS_SERVICE_NOT_FOUND);
    CONSIDER(CALL_STATUS_METHOD_NOT_FOUND);
    CONSIDER(CALL_STATUS_ACCESS_DENIED);
    CONSIDER(CALL_STATUS_INVOCATION_EXCEPTION);
    CONSIDER(CALL_STATUS_GENERAL_EXCEPTION);
  }
  LOG_FATAL << "Illegal CallStatus: " << status;
  return "CALL_STATUS_UNKOWN";   // keep g++ happy
}
string Call::ToString() const {
  ostringstream oss;
  oss << " status: " << status_name()
      << ", service: " << service_name_
      << ", method: " << method_name_
      << ", invoke_id: " << invoke_id_
      << ", connection_params: ";
  if ( connection_params_ != NULL ) {
    oss << connection_params_->ToString();
  } else {
    oss << "NULL";
  }
  for ( int i = 0; i < arguments_.size(); i++ ) {
    oss << "\n\tArg " << i << ": ";
    if ( arguments_[i] != NULL ) {
      oss << arguments_[i]->ToString();
    } else {
      oss << "NULL";
    }
  }
  return oss.str();
}

AmfUtil::ReadStatus Call::ReadCallData(io::MemoryStream* in,
                                       int stream_id,
                                       bool is_notify,
                                       AmfUtil::Version version) {
  CHECK_EQ(version, AmfUtil::AMF0_VERSION);
  CString action;
  AmfUtil::ReadStatus err = action.ReadFromMemoryStream(in, version);
  if ( err != AmfUtil::READ_OK )
    return err;

  // TODO(cpopescu): check action value (size at least)..
  in->MarkerSet();
  CNumber invoke_id;
  err = invoke_id.ReadFromMemoryStream(in, version);
  if ( err == AmfUtil::READ_OK ) {
    invoke_id_ = static_cast<uint32>(invoke_id.int_value());
    in->MarkerClear();
  } else {
    invoke_id_ = 0;
    in->MarkerRestore();
  }

  size_t pos_dot = action.value().find_last_of(".");
  if ( pos_dot == string::npos ) {
    method_name_.assign(action.value());
  } else {
    service_name_.assign(action.value().substr(0, pos_dot));
    method_name_.assign(action.value().substr(pos_dot + 1));
  }
  CObject* obj = NULL;
  bool first = true;
  do {
    obj = NULL;
    err = Amf0Util::ReadNextObject(in, &obj);
    if ( err != AmfUtil::READ_OK ) {
      if ( err != AmfUtil::READ_NO_DATA ) {
        delete obj;
        return err;
      }
    } else {
      // Before the actual parameters we sometimes (especially on "connect")
      // get a map of parameters; this is usually null, but if set it should be
      // passed to the connection object.
      if ( first ) {
        set_connection_params(obj);
        first = false;
      } else {
        AddArgument(obj);
      }
    }
  } while ( err == AmfUtil::READ_OK );
  return AmfUtil::READ_OK;
}

void Call::WriteCallData(io::MemoryStream* out,
                         bool write_invoke_id,
                         AmfUtil::Version version) {
  CHECK_EQ(version, AmfUtil::AMF0_VERSION);
  // !! IMPORTANT !!
  // the result is written only if
  // status() == Call::CALL_STATUS_PENDING
  // see org.red5.server.net.rtmp.codec.RTMPProtocolEncoder.
  //                                             encodeNotifyOrInvoke:502
  if ( service_name_.empty() ) {
    Amf0Util::WriteString(out, method_name_);
  } else {
    Amf0Util::WriteString(out, service_name_ + "." + method_name_);
  }
  if ( write_invoke_id ) {
    CNumber(invoke_id_).WriteToMemoryStream(out, version);
  }
  if ( connection_params_ != NULL ) {
    connection_params_->WriteToMemoryStream(out, version);
  } else {
    CNull null;
    null.WriteToMemoryStream(out, version);
  }
  // TODO(cpopescu): verify - is this true ???
  // if ( status() == Call::CALL_STATUS_PENDING ) {
  for ( int i = 0; i < arguments_.size(); i++ ) {
    arguments_[i]->WriteToMemoryStream(out, version);
  //  }
  }
}
}
