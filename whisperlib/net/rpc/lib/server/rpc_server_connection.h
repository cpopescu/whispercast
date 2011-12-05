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

#ifndef __NET_RPC_LIB_SERVER_RPC_SERVER_CONNECTION_H__
#define __NET_RPC_LIB_SERVER_RPC_SERVER_CONNECTION_H__

#include <string>
#include <whisperlib/common/base/types.h>
#include <whisperlib/common/io/buffer/io_memory_stream.h>
#include <whisperlib/common/sync/mutex.h>
#include <whisperlib/net/base/connection.h>
#include <whisperlib/net/rpc/lib/types/rpc_all_types.h>
#include <whisperlib/net/rpc/lib/server/irpc_result_handler.h>
#include <whisperlib/net/rpc/lib/server/irpc_async_query_executor.h>

// This is the TCP connection from server side working as transport layer
// for RPC calls & results. Can send & receive rpc::Message packets.

namespace rpc {

class Server;
class ServerConnection
    : public rpc::IResultHandler {
 public:
  ServerConnection(net::Selector* selector,
                   bool auto_delete_on_close,
                   net::NetConnection* net_connection,
                   rpc::IAsyncQueryExecutor& queryExecutor);
  ~ServerConnection();

 protected:
  //  Helper methods for synchronized incrementing and decrementing of
  //  the expectedWriteReplyCalls_ .
  // The working pattern is:
  //   - an external thread(worker) sending a packet: increments
  //     the expectedWriteReplyCalls_ and queues a closure in selector
  //     (which does the actual send to network)
  //   - the closure: sends the packet, decrements the expectedWriteReplyCalls_,
  //     and if the connection is closed and there are no more
  //     expectedWriteReplyCalls it deletes the connection according
  //     to autoCloseOnDelete_ flag.
  void IncExpectedWriteReplyCalls();
  void DecExpectedWriteReplyCallsAndPossiblyDeleteConnection();

 public:
  //////////////////////////////////////////////////////////////
  //
  //  Methods available to any external thread (worker threads).
  //

  // create a reply rpc-packet, and queue a closure to send it.
  void WriteReply(uint32 xid,
                  rpc::REPLY_STATUS status,
                  const io::MemoryStream& result);

  //  Queue a closure to send the given packet to the network.
  //  The selector does the encoding.
  //  NOTE: this saves time in the calling thread, loading the selector.
  // input:
  //   p: the packet to be sent. Must be dynamically allocated.
  void WriteWithEncodeInSelector(const rpc::Message* p);

  //  Encode the packet in a dynamically allocate a memory stream,
  //  then queues a closure to send the stream from selector thread.
  //  NOTE: this saves time in selector, loading the current thread.
  void WriteWithEncodeNow(const rpc::Message& p);


 protected:
  //////////////////////////////////////////////////////////////
  //
  //     Methods available only from the selector thread.
  //

  //  Sends the given RPC packet to underlying BufferedConnection.
  //  Does not wait for a response, not event for a complete write
  //  (the data to send is put in BufferedConnection's output buffer).
  // input:
  //   msg: the rpc message to be sent. Must be dynamically allocated& will
  //        be automatically deleted.
  void CallbackSendRPCPacket(const rpc::Message* msg);

  //  Sends all the data in the given memory stream to the network
  // input:
  //   ms: the stream containing the data to be sent.
  //       Must be dynamically allocated & will be automatically deleted.
  void CallbackSendData(const io::MemoryStream* ms);

  //////////////////////////////////////////////////////////////////////
  //
  //             net::BufferedConnection methods
  //
  //  Handle incoming data.
  //  Use net::BufferedConnection::HandleRead() to buffer incoming data
  //  then process data in net::BufferedConnection::inbuf()
  // returns:
  //   true - data successfully read. Even if an incomplete packet was received.
  //   false - error reading data from network. The caller will close
  //           the connection immediately on return.
  virtual void XHandleRead() {
  }
  bool ConnectionReadHandler();

  virtual void XHandleWrite() {
  }
  bool ConnectionWriteHandler();
  virtual void XHandleConnect() {
  }
  virtual void XHandleError() {
  }
  void ConnectionCloseHandler(int err, net::NetConnection::CloseWhat what);
  virtual void XHandleTimeout() {
  }
  virtual void XHandleAccept() {
  }

  // Called by the execution system (possibly a worker) to announce
  // the result of a query.
  void HandleRPCResult(const rpc::Query& q);

  // Returns a description of this connection. Good for logging.
  string ToString() const;

 private:
  net::Selector * selector_;

  // we own the underlying TCP connection
  net::NetConnection * net_connection_;

  // Buffer used by the Write function to serialize every packet before
  // sending it to network.
  io::MemoryStream cachedPacketBuffer_;

  // Synchronize access to cachedPacketBuffer_ . The execution model is
  // asynchronous, so query results may arrive & be sent back to client
  // anytime anyorder.
  synch::Mutex syncCachedPacketBuffer_;

  enum HANDSHAKE_STATE {
    HS_WAITING_REQUEST = 0,
    HS_WAITING_RESPONSE = 1,
    HS_CONNECTED = 2,
    HS_FAILURE = 3,
  };

  HANDSHAKE_STATE handshakeState_;

  enum {
    HANDSHAKE_RANDOM_SIZE = 32,   // protocol constant
  };

  // random generated data, used in handshake.
  uint8 handshakeServerRandomData_[HANDSHAKE_RANDOM_SIZE];

  // the query executor. Estabilished in constructor.
  rpc::IAsyncQueryExecutor& asyncQueryExecutor_;

  // true if we're registered in the executor.
  bool registeredToQueryExecutor_;

  // the codec used by this connection. It is estabilished in the hanshake.
  rpc::Codec* codec_;

  // the number of WriteWith... closures queued in selector and not run yet
  uint32 expectedWriteReplyCalls_;

  // synchronize access expectedWriteReplyCalls_
  synch::Mutex accessExpectedWriteReplyCalls_;

  bool auto_delete_on_close_;

  // Does the initial handshake.
  // input:
  //  in: contains client data. There may be less than a full handshake message
  //      bytes in the input stream, in which case the method should do nothing.
  //      When more data arrives the method is called again, and the
  //      input stream will contain more (old+new) data.
  void ProtocolHandleHandshake(io::MemoryStream& in);

  // Execute client query and send back the result.
  void ProtocolHandleMessage(const rpc::Message& msg);

  DISALLOW_EVIL_CONSTRUCTORS(ServerConnection);
};
}
#endif   // __NET_RPC_LIB_SERVER_RPC_SERVER_CONNECTION_H__
