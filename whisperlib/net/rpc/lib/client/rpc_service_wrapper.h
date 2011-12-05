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

#ifndef __NET_RPC_LIB_CLIENT_RPC_SERVICE_WRAPPER_H__
#define __NET_RPC_LIB_CLIENT_RPC_SERVICE_WRAPPER_H__

#include <stdarg.h>
#include <string>
#include <map>
#include <whisperlib/common/base/callback.h>
#include <whisperlib/net/rpc/lib/types/rpc_all_types.h>
#include <whisperlib/net/rpc/lib/client/irpc_client_connection.h>

namespace rpc {
// The return of any RPC call.
template <typename RET>
struct CallResult {
  bool success_;      // success status
  string error_;      // error description (if success == false)
  RET result_;        // RPC return value
};
// an identifier for asynchronous RPC calls.
typedef int64 CALL_ID;

// A base class for calling remote service methods.
// It takes a IRPCClientConnection implementation (as transport layer) and
// it offers a 2 simple function:
//  - void Call(method, encoded_params, result) which
//      sends a RPC query to the underlying RPCConnection,
//      waits(with timeout) for response, decodes and returns the result.
//  - void AsyncCall(method, encoded_params, callback) which
//      sends a RPC query to the underlying RPCConnection,
//      and returns immediately. The SUCCESS or TIMEOUT result
//      is asynchronously delivered to the provided "callback".
//      You may also cancel an executing AsyncCall.
class ServiceWrapper {
 public:
  // input:
  //   rpcConnection: a previously created rpc client connection.
  //                  The wrapper uses this connection to invoke
  //                  remote service methods.
  //   serviceName: the name of the service you wish to wrap.
  ServiceWrapper(IClientConnection& rpc_connection,
                 const string& service_class_name,
                 const string& service_instance_name);

  virtual ~ServiceWrapper();

  //  Sets the timeout expressed in milliseconds for both synchronous and
  //  asynchronous calls.
  // input:
  //   timeout: milliseconds to wait for a remote call return.
  void SetCallTimeout(uint32 timeout);

  // Returns the current timeout value.
  uint32 GetCallTimeout() const;

  // Returns the instance name of the remote service.
  const std::string& GetServiceName() const;
  // Returns the class name of the remote service.
  const std::string& GetServiceClassName() const;

  // Returns the codec used in the underlying connection.
  Codec& GetCodec() const;

 private:
  CALL_ID MakeCallId(uint32 qid) {
    return static_cast<CALL_ID>(qid);
  }
  uint32 MakeQid(CALL_ID call_id) {
    return static_cast<uint32>(call_id);
  }

  void AddResultCallbackNoSync(CALL_ID call_id,
                              Callback* callback) {
    bool s = result_callbacks_.insert(make_pair(call_id, callback)).second;
    CHECK(s) << "Duplicate call_id: " << call_id;
  }
  Callback* PopResultCallbackSync(CALL_ID call_id) {
    synch::MutexLocker lock(&sync_);
    ResultCallbackMap::iterator it = result_callbacks_.find(call_id);
    if ( it == result_callbacks_.end() ) {
      return NULL;
    }
    Callback* result_handler = it->second;
    result_callbacks_.erase(it);
    return result_handler;
  }

  // callback: the connection deliver results here
  template <typename RET>
  void HandleCallResult(std::string method,
                        uint32 qid,
                        rpc::REPLY_STATUS status,
                        const io::MemoryStream& cresult) {
    ////////////////////////////////////////////////////////
    // Find result callback
    //
    const CALL_ID call_id = MakeCallId(qid);
    Callback* result_callback = PopResultCallbackSync(call_id);
    if ( result_callback == NULL ) {
      LOG_ERROR << "No ResultCallback for call_id: " << call_id;
      return;
    }
    typedef Callback1<const CallResult<RET>& > CallResultCallback;
    CallResultCallback* call_result_callback =
      reinterpret_cast<CallResultCallback*>(result_callback);

    ////////////////////////////////////////////////////////
    // decode result and deliver it to the client result callback
    //
    io::MemoryStream& result = const_cast<io::MemoryStream&>(cresult);
    result.MarkerSet();

    CallResult<RET> call_result;
    call_result.success_ = false;   // assume the worst

    do {
      if ( status != RPC_SUCCESS )  {
        call_result.success_ = false;
        call_result.error_ = ReplyStatusName(status);
        LOG_ERROR << "rpc::Call failed for method: '" << method << "'"
                  << " on service '" << GetServiceName() << "'."
                  << " Returned Status: " << ReplyStatusName(status);
        if ( result.Size() > 0 ) {
          string reason;
          if ( GetCodec().Decode(result, reason) == DECODE_RESULT_SUCCESS ) {
            call_result.error_ += string(" Hint: ") + reason;
            LOG_ERROR << "Call failed hint: " << reason;
          }
        }
        break;
      }

      // decode returned value
      //
      if ( GetCodec().Decode(result, call_result.result_) ==
           DECODE_RESULT_ERROR ) {
        DLOG_ERROR << "RPC cannot decode return value as " << __NAME_STRINGIZER(RET)
                   << ". Bytes: " << result.DebugString();
        call_result.success_ = false;
        call_result.error_ =
            "Error decoding data, the server returned a wrong type";
        break;
      }
      call_result.success_ = true;
    } while ( false );

    result.MarkerRestore();

    // call the external result handler
    call_result_callback->Run(call_result);
  }

  // Helper for synchronous calls.
  // Basically we use an asynchronous call, catch the result here, and signal
  // the waiting thread.
  template <typename RET>
  void ReceiveSynchronousResult(synch::Event* received,
                                CallResult<RET>* out,
                                const CallResult<RET>& in) {
    *out = in;
    received->Signal();
  }

 public:
  //  Call a remote method on the enclosed service. This method blocks at most
  //  "call_timeout_" milliseconds waiting for remote call result.
  // input:
  //   [OUT] returnValue: the return value is copied here.
  //   method: remote method name. Plain text.
  //   params: contains remote method parameters encoded.
  // return:
  //   Success state. In order for the remote call to succeed
  //   these requirements must be met:
  //     - the wrapper must be connected (IsConnected() == true);
  //     - the method name must exist;
  //     - the parameters number and types must be correct;
  //     - the return type must be the one expected.
  //         The remote call must return the same object type as typename RET.
  template <typename RET>
  void Call(const std::string& method,
            io::MemoryStream& params,
            CallResult<RET>& return_value) {
    // we wait call result on this event
    synch::Event response_received(false, true);

    AsyncCall(method, params,
        NewCallback(this, &ServiceWrapper::ReceiveSynchronousResult<RET>,
                          &response_received,
                          &return_value));

    // A response should be delivered either on success or on timeout.
    response_received.Wait();
  }

  //  Call a remote method on the enclosed service. This method returns
  //  immediately and the remote call result shall be asynchronously delivered
  //  to the "handle_return" callback. The call has a timeout of "call_timeout_"
  //  milliseconds.
  //  You will get an asynchronous result either if call succeeds(server
  //  responds) or if a timeout occurs.
  // input:
  //  method: remote method name. Plain text.
  //  params: contains remote method parameters encoded according GetCodec()
  //  handle_return: callback to be called when the server response arrives
  //                 or a timeout occurs.
  // return:
  //  An identifier for this call. You may cancel the call using CancelCall
  //  and providing the same id.
  template <typename RET>
  CALL_ID AsyncCall(const std::string& method,
                    io::MemoryStream& params,
                    Callback1<const CallResult<RET>&>* handle_return) {
    CALL_ID call_id = 0;
    {
      synch::MutexLocker lock(&sync_);
      const uint32 qid = rpc_connection_.AsyncQuery(
                           GetServiceName(),
                           method, params,
                           GetCallTimeout(),
                           NewCallback(this,
                                       &ServiceWrapper::HandleCallResult<RET>,
                                       std::string(method)));
      call_id = MakeCallId(qid);
      AddResultCallbackNoSync(call_id, handle_return);
    }

    return call_id;
  }

  // Cancel an asynchronous call based on call ID.
  void CancelCall(CALL_ID call_id);

  // Cancels all asynchronous call.
  void CancelAllCalls();

 private:
  // the rpc-connection used by this service wrapper
  IClientConnection& rpc_connection_;

  // the name of the remote service for this wrapper
  const string service_class_name_;
  const string service_instance_name_;

  // milliseconds timeout on calling a remote method
  uint32 call_timeout_;

  // asynchronous executing calls
  // map: call id -> client callback
  typedef std::map<CALL_ID, Callback*> ResultCallbackMap;
  ResultCallbackMap result_callbacks_;

  // synchronize result_callbacks_
  synch::Mutex sync_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ServiceWrapper);
};
}
#endif  // __NET_RPC_LIB_CLIENT_RPC_SERVICE_WRAPPER_H__
