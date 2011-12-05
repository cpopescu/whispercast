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

#ifndef __NET_RPC_LIB_CLIENT_IRPC_CLIENT_CONNECTION_H__
#define __NET_RPC_LIB_CLIENT_IRPC_CLIENT_CONNECTION_H__

#include <string>
#include <map>
#include <whisperlib/common/base/callback.h>
#include <whisperlib/common/sync/mutex.h>
#include <whisperlib/common/sync/event.h>
#include <whisperlib/net/base/selector.h>
#include <whisperlib/net/base/timeouter.h>
#include <whisperlib/net/rpc/lib/types/rpc_message.h>
#include <whisperlib/net/rpc/lib/codec/rpc_codec.h>

// This is an interface for all client side RPC transport connections.
// The interface is capable of sending & receiving RPCMessage through
// implementation specific transports.

namespace rpc {

enum CONNECTION_TYPE {
  CONNECTION_TCP,
  CONNECTION_HTTP,
  CONNECTION_FAILSAFE_HTTP,
};
const char* ConnectionTypeName(CONNECTION_TYPE connection_type);

class IClientConnection {
 public:
  IClientConnection(net::Selector& selector,
                    rpc::CONNECTION_TYPE connection_type,
                    rpc::CODEC_ID codec_id);
  virtual ~IClientConnection();

  // Returns a description of the last error.
  const string& Error() const {
    return error_;
  }
  // Returns the transport type.
  rpc::CONNECTION_TYPE GetConnectionType() const {
    return connection_type_;
  }

  // Retrieve connection codec.
  rpc::Codec& GetCodec() const {
    return *codec_;
  }

 protected:
  typedef Callback3<uint32, rpc::REPLY_STATUS, const io::MemoryStream&>
    ResponseCallback;

  // [THREAD SAFE]
  //  Generates consecutive XIDs.
  uint32 GenerateNextXID();

  // [THREAD SAFE]
  //  Create a rpc::Message call from the given params,
  //  serialize it and send it over network.
  void SendQuery(uint32 xid,
                 const std::string& service,
                 const std::string& method,
                 io::MemoryStream& params);

  // [THREAD SAFE]
  //  Tells implementation to serialize and send given packet.
  //  "p" was dynamically allocated, implementation takes ownership of it.
  //  If Send fails, you should call HandleSendError(p).
  //  Call Error() for an error description.
  virtual void Send(const rpc::Message* p) = 0;

  // [THREAD SAFE]
  //  Tells implementation to cancel the Send of the corresponding packet.
  //  If the packet was sent already, or partially sent, leave it sent, and
  //  the answer will be ignored.
  //  This method is only an optimization. Called when a query timed out.
  virtual void Cancel(uint32 xid) = 0;

  // [THREAD SAFE]
  //  Called by the implementation, after a rpc::Message is received and
  //  decoded.
  // input:
  //  p: the received message. Dynamically allocated, will be automatically
  //      deleted here.
  void HandleResponse(const rpc::Message* p);

  // [THREAD SAFE]
  //  Called by the implementation if it's unable to send an rpc::Message.
  //  It may happen because of Send timeout or internal fail.
  // input:
  //  p: the message from Send. We take ownership of it.
  //  status: describes the failure.
  void NotifySendFailed(const rpc::Message* p, rpc::REPLY_STATUS status);

  // [THREAD SAFE]
  // Called by the implementation, when the underlying connection was closed.
  // Useful to notify all waiting RPC responses that there will be no response.
  // This is just an optimization, as the waiting queries will be completed
  // on timeout by default.
  void NotifyConnectionClosed();

  // [THREAD SAFE]
  //  Called by the timeouter_ (selector thread) when a timeout occurs.
  //  Here we complete the query (the given xid) by RPC_QUERY_TIMEOUT status.
  void NotifyTimeout(int64 xid);

 private:
  // [NOT Thread safe] Call this ONLY with sync_response_map_ LOCKED !
  void AddResponseCallbackNoSync(uint32 xid,
                                 ResponseCallback * response_callback);
  // [THREAD SAFE]
  ResponseCallback * PopResponseCallbackSync(uint32 xid);

  // [THREAD SAFE] All.
  //  These methods are needed because we have to synchronize with selector
  //  thread.
  void AddTimeout(int64 xid, int64 timeout);
  void ClearTimeout(int64 xid);
  void ClearTimeouts();

 public:
  // [THREAD SAFE]
  //  Send a remote RPC call packet, wait for RPC reply packet with timeout,
  //  return call result and status. If timeout expires a RPC_TIMEOUT status
  //  is returned. This methods implements a synchronous rpc.
  // input:
  //  [IN]  service: remote service whose method is to be invoked. Plain text.
  //  [IN]  method:  call method name. Plain text.
  //  [IN]  params:  encoded list of parameters to be passed to the
  //                 remote invoke.
  //  [IN]  timeout: milliseconds to wait for server reply.
  //  [OUT] status:  Call status. A successfully call returns RPC_SUCCESS.
  //                 The rest are errors: errors that appear on the server side
  //                 during call execution (like server out of memory,
  //                 or bad parameters), or errors in transport layer
  //                 (connection error, timeout waiting for server reply).
  //                 In case of error the return value isn't what you expected:
  //                 it may be rpc::Void or a rpc::String containing a
  //                 description of the error.
  //  [OUT] result: receives the encoded return value of the remote method call
  //              only if the call succeeds (i.e. status == RPC_SUCCESS).
  //              If the call failed, the result is either empty or contains
  //              a RPC::String description of the error.
  //              The caller should know what type of return value is expected.
  //   Internal errors:
  //         - failure of the RPC system to send the query,
  //         - invalid answer from server,
  //         - timeout in server response
  void Query(const std::string& service,
             const std::string& method,
             io::MemoryStream& params,
             uint32 timeout,
             rpc::REPLY_STATUS& status,
             io::MemoryStream& result);

  // [THREAD SAFE]
  //  Sends a remote RPC call packet, and does not wait for the result.
  //  The result will be asynchronously delivered to the the
  //  response_callback callback.
  //  If timeout expires a RPC_TIMEOUT status is sent to the
  //  response_callback callback.
  //  This methods implements an asynchronous rpc.
  // returns:
  //  query ID. Useful for CancelQuery.
  //
  uint32 AsyncQuery(const std::string& service,
                    const std::string& method,
                    io::MemoryStream& params,
                    uint32 timeout,
                    ResponseCallback* response_callback);

  // [THREAD SAFE]
  //  Complete synchronous or asynchronous query.
  //  Calls the ResultCallback delivering the given status & result.
  void CompleteQuery(uint32 qid, rpc::REPLY_STATUS status,
                     const io::MemoryStream & result);

  // [THREAD SAFE]
  //  Complete all synchronous and asynchronous queries.
  //  Calls the ResultCallback delivering the given status & an empty result.
  void CompleteAllQueries(rpc::REPLY_STATUS status);

  // [THREAD SAFE]
  //  Cancel synchronous or asynchronous query.
  //  The ResultCallback is NOT called, but just deleted.
  void CancelQuery(uint32 qid);

  // [THREAD SAFE]
  //  Cancel all pending queries, both synchronous and asynchronous.
  //  The ResultCallback is NOT called, but just deleted.
  void CancelAllQueries();

 protected:
  net::Selector& selector_;

  rpc::CONNECTION_TYPE connection_type_;  // identifies the transport layer
  rpc::Codec* codec_;                     // the codec used. Allocated locally.

  string error_;                          // description of the last error

  uint32 next_xid_;              // for generating consecutive transaction IDs
  synch::Mutex sync_next_xid_;   // synchronize access to next_xid_

  // map of pending queries response_callbacks, by xid
  typedef map<uint32, ResponseCallback*> ResponseCallbackMap;
  ResponseCallbackMap response_map_;
  synch::Mutex sync_response_map_;

  // used to set timeouts on Query and AsyncQuery
  // WARNING: the Timeouter MUST be used in Selector thread ONLY!
  net::Timeouter timeouter_;
  synch::Event done_clear_timeouts_;

  // signal the completion of CancelQuery or CancelAllQueries.
  // This must be auto-reset events, as multiple threads may call CancelQuery.
  synch::Event done_cancel_query_;
  synch::Event done_cancel_all_queries_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(IClientConnection);
};
}
#endif   // __NET_RPC_LIB_CLIENT_IRPC_CLIENT_CONNECTION_H__
