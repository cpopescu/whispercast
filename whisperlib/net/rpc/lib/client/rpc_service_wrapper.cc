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

#include "common/base/callback.h"
#include "net/rpc/lib/client/rpc_service_wrapper.h"

namespace rpc {

ServiceWrapper::ServiceWrapper(IClientConnection& rpc_connection,
                               const string& service_class_name,
                               const string& service_instance_name)
  : rpc_connection_(rpc_connection),
    service_class_name_(service_class_name),
    service_instance_name_(service_instance_name),
    call_timeout_(5000),
    result_callbacks_(),
    sync_() {
}

ServiceWrapper::~ServiceWrapper() {
  CancelAllCalls();
}

void ServiceWrapper::SetCallTimeout(uint32 timeout) {
  call_timeout_ = timeout;
  LOG_INFO << "CallTimeout modified to: " << call_timeout_ << " ms";
}

uint32 ServiceWrapper::GetCallTimeout() const {
  return call_timeout_;
}

const std::string& ServiceWrapper::GetServiceName() const {
  return service_instance_name_;
}
const std::string& ServiceWrapper::GetServiceClassName() const {
  return service_class_name_;
}

CodecId ServiceWrapper::GetCodec() const {
  return rpc_connection_.codec();
}

void ServiceWrapper::CancelCall(CALL_ID call_id) {
  Callback * result_handler = PopResultCallbackSync(call_id);
  if ( result_handler == NULL ) {
    return;
  }
  delete result_handler;
  const uint32 qid = MakeQid(call_id);
  // WARNING: do NOT call rpc_connection_.CancelQuery with sync_ locked !
  //          Because CancelQuery synchronizes on selector, and the selector
  //          may be trying to deliver a response on HandleCallResult witch
  //          also locks sync_.
  rpc_connection_.CancelQuery(qid);
}

void ServiceWrapper::CancelAllCalls() {
  // NOTE: do NOT use rpc_connection_.CancelAllQueries() because the
  //       rpc_connection may be shared between multiple RPCServiceWrappers.

  // Make a temporary map or ResultHandlers, because we don't want to
  // call rpc_connection_.CancelQuery with sync_ locked.
  ResultCallbackMap tmp;
  {
    synch::MutexLocker lock(&sync_);
    tmp = result_callbacks_;
    result_callbacks_.clear();
  }

  for ( ResultCallbackMap::iterator it = tmp.begin(); it != tmp.end(); ++it) {
    CALL_ID call_id = it->first;
    Callback * result_handler = it->second;
    const uint32 qid = MakeQid(call_id);
    delete result_handler;
    rpc_connection_.CancelQuery(qid);
  }
}
}
