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

#ifndef __NET_RPC_LIB_CLIENT_RPC_CLIENT_CONNECTION_TCP_H__
#define __NET_RPC_LIB_CLIENT_RPC_CLIENT_CONNECTION_TCP_H__

#include <string>
#include <map>
#include <whisperlib/common/sync/event.h>
#include <whisperlib/net/base/selector.h>
#include <whisperlib/net/base/connection.h>
#include <whisperlib/net/rpc/lib/types/rpc_all_types.h>
#include <whisperlib/net/rpc/lib/types/rpc_message.h>
#include <whisperlib/net/rpc/lib/codec/rpc_codec.h>
#include <whisperlib/net/rpc/lib/rpc_version.h>
#include <whisperlib/net/rpc/lib/client/irpc_client_connection.h>

namespace rpc {
// The TCP connection from client side working as transport layer for
// RPC calls & results.
// Can send & receive rpc::Message packets.

class ClientConnectionTCP
    : public rpc::IClientConnection {
 private:
  enum HANDSHAKE_STATE {
    NOT_INITIALIZED,   // initial state: handshake not started
    WAITING_RESPONSE,  // request sent, waiting response
    CONNECTED,         // response received and verified.
                       // Connection estabilished.
    FAILURE,           // some error occured in handshake procedure.
                       // For more information call Error();
  };
 public:
  // @param: success state. Call Error()
  typedef Callback1<bool> OpenCallback;

  //////////////////////////////////////////////////////////////
  //
  //        Methods available to any external thread.
  //
  ClientConnectionTCP(net::Selector& selector,
                      net::NetFactory& net_factory,
                      net::PROTOCOL net_protocol,
                      const net::HostPort& remote_addr,
                      rpc::CODEC_ID codec_id,
                      int64 open_timeout_ms = 20000,
                      uint32 max_paralel_queries = 100);
  virtual ~ClientConnectionTCP();

  //////////////////////////////////////////////////////////////////////
  //
  //              Generic connection management
  //

  //  Connect to remote address (ctor param) and execute RPC handshake.
  //  If open_callback == NULL this method blocks untill TCP connect
  //   and RPC handshake are completed or the timeout expires.
  //  If open_callback != NULL this method returns immediately and the
  //   open_callback will be called after RPC handshake completes or
  //   the timeout expires.
  //
  //  //This is a one shot method. Calling Open a second time gives
  //  // unpredictable results.
  //
  //  In BLOCKING mode, while Open is in progress, you cannot delete
  //   the connection.
  //  In NON-BLOCKING mode, you can delete the connection anytime. And
  //   the completion callback will be immediately called with fail status.
  //
  // input:
  //  open_callback: callback to be run after Open completes or fails.
  //                 In selector thread context.
  //                 If you wish to destroy the connection in callback then
  //                 DO NOT call delete, use selector.DeleteInSelectLoop(..).
  //
  // return:
  //  If open_callback == NULL (i.e. BLOCKING):
  //   - true = success.
  //   - false = failure. Call Error() for aditional information.
  //  If open_callback != NULL (i.e. UNBLOCKING):
  //   - true = connect & handshake in progress, the open_callback will
  //            be called after connect & handshake completes/fails,
  //            or the timeout expires.
  //            The callback indicates the Open success status;
  //            you can also check IsOpen().
  //   - false = something went bad right from the begining,
  //             The open_callback won't be called, but will be destroyed if
  //             non-permanent. Check Error().
  //
  //  Possible failure reasons are:
  //   - unable to connect to remote host. SYSTEM_ERROR: ECONNREFUSED
  //   - handshake failed. RPC_HANDSHAKE_FAILED
  //     Bad version, bad codec, invalid data, or timeout waiting for hand
  //     reply.
  //   - open_timeout_ expired
  //
  bool Open(OpenCallback* open_callback = NULL);

  // Test if tcp is alive & RPC handshake is ok.
  bool IsOpen() const;

  // Close RPC connection.
  void Shutdown(synch::Event* signal_me_when_done = NULL);

  //////////////////////////////////////////////////////////////////////
  //
  //          rpc::IClientConnectin interface methods
  //

 protected:
  // [THREAD SAFE]
  //  Forward to SendPacket(..).
  // NOTE: "p" was dynamically allocated and must be deleted.
  virtual void Send(const rpc::Message* p);

  // [THREAD SAFE]
  //  Cancel the send of the corresponding packet.
  virtual void Cancel(uint32 xid);

 protected:
  //////////////////////////////////////////////////////////////
  //
  //     Methods available only from the selector thread.
  //

  //////////////////////////////////////////////////////////////////////
  //
  //             RPC connection control callbacks
  //

  // Initiates the opening operation.
  void StartOpen();
  // Open ended. This happens on Connect error or Handshake error
  // or Handshake succeeded.
  void EndOpen(const char* err = NULL);

  // starts a TCP connect to the given address
  void StartConnect();
  // Connect ended. If "err" not NULL, it will be set as last error_.
  void EndConnect(const char* err = NULL);

  // starts the handshake procedure. The connection must be connected.
  void StartHandshake();
  // End handshake process with the given error "err".
  void EndHandshake(HANDSHAKE_STATE state, const char* err = NULL);

  // Exchange RPC handshake on the internal connected BufferedConnection.
  // After this function returns, check handshake_state_ for completion,
  // in progress or error status.
  void DoHandshake();

  // [THREAD SAFE]
  //  Serialize and send given packet.
  //  If disconnected, this method starts connect and delays send until after
  //  connect completed. If Send fails, we call NotifySendFailed(p).
  // NOTE: "p" was dynamically allocated and we take ownership of it here.
  void SendPacket(const rpc::Message* p);

  //  net::Connection methods
  bool ConnectionReadHandler();
  bool ConnectionWriteHandler();
  void ConnectionConnectHandler();
  void ConnectionCloseHandler(int err, net::NetConnection::CloseWhat what);

 private:
  void TimeoutHandler(int64 timeout_id);

 private:
  net::Selector& selector_;

  net::NetFactory& net_factory_; // used to create the transport layer

  net::NetConnection* net_connection_; // the underlying transport layer

  static const int kOpenEvent = 1;
  static const int HANDSHAKE_RANDOM_SIZE = 32;  // protocol constant

  // random generated data, used in handshake.
  uint8 kHandshakeClientRandomData[HANDSHAKE_RANDOM_SIZE];

  HANDSHAKE_STATE handshake_state_;

  // we are connected to this guy:
  const net::HostPort remote_addr_;
  // Milliseconds timeout for the Open process (TCP connect + handshake)
  const int64 open_timeout_ms_;
  // queries exceeding this number will fail with RPC_TOO_MANY_QUERIES
  const uint32 max_paralel_queries_;

  synch::Event open_completed_;        // signal TCP connect & handshake done

  // True if open is in progress.
  bool is_opening_;

  // True if shutdown is executing.
  // Tells ConnectionCloseHandler he is invoked as a consequence of Shutdown,
  // so it should not call Shutdown recursively.
  bool shutdown_is_executing_;

  struct ClientQuery {
    const rpc::Message* msg_;    // the whole message to be sent, we own it
    io::MemoryStream buf_;     // contains the serialized message
    explicit ClientQuery(const rpc::Message* msg)
        : msg_(msg), buf_() {
    }
    ~ClientQuery() {
      delete msg_;
      msg_ = NULL;
    }
  };
  typedef map<uint32, ClientQuery*> ClientQueryMap;
  // Map of pending queries, to be sent to remote address.
  // While the connection is established these are popped one by one
  // in active_query_ and written to network.
  ClientQueryMap queries_;

  // The current query being written to network.
  ClientQuery* active_query_;

  // synchronization is achieved by accessing queries_ from selector thread only

 private:
  // called when asynchronous Open completes:
  //  - error in connect
  //  - error in handshake
  //  - handshake succeeded = SUCCESS
  //  - timeout
  OpenCallback* open_callback_;

  net::Timeouter timeouter_;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ClientConnectionTCP);
};
}
#endif  // __NET_RPC_LIB_CLIENT_RPC_CLIENT_CONNECTION_TCP_H__
