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
#include "common/base/errno.h"
#include "common/base/timer.h"
#include "common/base/scoped_ptr.h"
#include "common/io/buffer/memory_stream.h"
#include "net/rpc/lib/client/rpc_client_connection_tcp.h"
#include "net/rpc/lib/codec/rpc_encoder.h"
#include "net/rpc/lib/codec/rpc_decoder.h"

// TODO(cpopescu): very important - all timeouts should be configurable

namespace rpc {

//////////////////////////////////////////////////////////////
//
//        Methods available to any external thread.
//
rpc::ClientConnectionTCP::ClientConnectionTCP(
    net::Selector& selector,
    net::NetFactory& net_factory,
    net::PROTOCOL net_protocol,
    const net::HostPort& remote_addr,
    rpc::CODEC_ID codec_id,
    int64 open_timeout_ms,
    uint32 max_paralel_queries)
    : IClientConnection(selector, rpc::CONNECTION_TCP, codec_id),
      selector_(selector),
      net_factory_(net_factory),
      net_connection_(net_factory.CreateConnection(net_protocol)),
      handshake_state_(NOT_INITIALIZED),
      remote_addr_(remote_addr),
      open_timeout_ms_(open_timeout_ms),
      max_paralel_queries_(max_paralel_queries),
      open_completed_(false, true),
      is_opening_(false),
      shutdown_is_executing_(false),
      queries_(),
      active_query_(NULL),
      open_callback_(NULL),
      timeouter_(&selector, NewPermanentCallback(
          this, &rpc::ClientConnectionTCP::TimeoutHandler)) {
  net_connection_->SetReadHandler(NewPermanentCallback(
      this, &rpc::ClientConnectionTCP::ConnectionReadHandler), true);
  net_connection_->SetWriteHandler(NewPermanentCallback(
      this, &rpc::ClientConnectionTCP::ConnectionWriteHandler), true);
  net_connection_->SetConnectHandler(NewPermanentCallback(
      this, &rpc::ClientConnectionTCP::ConnectionConnectHandler), true);
  net_connection_->SetCloseHandler(NewPermanentCallback(
      this, &rpc::ClientConnectionTCP::ConnectionCloseHandler), true);
  //  generate random data
  for ( uint32 i = 0; i < HANDSHAKE_RANDOM_SIZE; i++ ) {
    kHandshakeClientRandomData[i] = 'a' + i % ('z'-'a');
  }
}

ClientConnectionTCP::~ClientConnectionTCP() {
  Shutdown();
  CHECK_NULL(open_callback_);
  CHECK(queries_.empty());
  CHECK_NULL(active_query_);
  delete net_connection_;
  net_connection_ = NULL;
}

bool ClientConnectionTCP::Open(OpenCallback* open_callback) {
  // Already Opened or Open in progress.
  CHECK_EQ(net_connection_->state(), net::NetConnection::DISCONNECTED);
  CHECK_EQ(handshake_state_, NOT_INITIALIZED);

  open_callback_ = open_callback;

  // Start Open procedure
  StartOpen();

  if ( open_callback_ ) {
    // Asynchronous Open
    return true;
  }

  // Synchronous Open
  CHECK(!selector_.IsInSelectThread())
      << "Cannot wait for Open in selector thread. Use async Open instead.";

  // Wait for TCP connect + RPC handshake completion.
  // In the worst case a Timeout notification will wake us.
  open_completed_.Wait();

  if ( !IsOpen() ) {
    Shutdown();
    return false;
  }

  return true;
}

bool ClientConnectionTCP::IsOpen() const {
  return net_connection_->state() == net::NetConnection::CONNECTED &&
      handshake_state_ == CONNECTED;
}

void ClientConnectionTCP::Shutdown(synch::Event* signal_me_when_done) {
  // Run this function no mater what. Maybe the selector just closed the
  // connection, or a HandleError is in progress.. it doesn't matter.
  // Testing for socket alive does not assure that the selector is not executing
  // something in here.
  //  e.g. [selector] -> RPCClientConnectionTCP::Close which closes fd but does
  //                     not finish execution of Close()
  //       [external] -> destructor -> Close -> test fd_ is invalid
  //                     -> go on with destructor
  //                  ==> Crash
  if ( !selector_.IsInSelectThread() ) {
    synch::Event shutdown_completed(false, true);
    selector_.RunInSelectLoop(
        NewCallback(this, &ClientConnectionTCP::Shutdown, &shutdown_completed));
    shutdown_completed.Wait();
    return;
  }

  //
  // selector thread
  //

  // tells ConnectionCloseHandler it should not call Shutdown
  shutdown_is_executing_ = true;

  // Closes underlying TCP connection.
  // Triggers a call to ConnectionCloseHandler.
  net_connection_->ForceClose();

  shutdown_is_executing_ = false;

  CHECK_EQ(net_connection_->state(), net::NetConnection::DISCONNECTED);
  handshake_state_ = NOT_INITIALIZED;

  // if an asynchronous Open was in progress, clean it up.
  if ( is_opening_ ) {
    EndOpen("Disconnected");
  }
  CHECK_NULL(open_callback_);

  // clear pending queries
  if ( active_query_ ) {
    delete active_query_;
    active_query_ = NULL;
  }
  for ( ClientQueryMap::iterator it = queries_.begin();
        it != queries_.end(); ++it ) {
    ClientQuery* q = it->second;
    delete q;
  }
  queries_.clear();

  // Notify IClientConnection about us being broken
  NotifyConnectionClosed();

  LOG_DEBUG << "Disconnect from "
            << net_connection_->remote_address()
            << " completed";

  // Let Signal() be the last statement! The destructor may be waiting
  // for Shutdown completion.
  if ( signal_me_when_done != NULL ) {
    signal_me_when_done->Signal();
  }
}

//////////////////////////////////////////////////////////////////////
//
//                    RPC I/O methods
//

// [THREAD SAFE]
void ClientConnectionTCP::Send(const rpc::Message* p) {
  // IMPORTANT:
  //  The query result (or failure) MUST be delivered on a different call !
  //  Many times we synchronize the Call and HandleCallResult, so the Call
  //  should not call HandleCallResult right away.
  //  We also defend against stack-overflow:
  //   - call async function
  //     - call completion handler
  //       - call another async function
  //         - ...
  selector_.RunInSelectLoop(
      NewCallback(this, &ClientConnectionTCP::SendPacket, p));
}

void ClientConnectionTCP::Cancel(uint32 xid) {
  if ( !selector_.IsInSelectThread() ) {
    selector_.RunInSelectLoop(
        NewCallback(this, &ClientConnectionTCP::Cancel, xid));
    return;
  }
  ClientQueryMap::iterator it = queries_.find(xid);
  if ( it == queries_.end() ) {
    return;
  }
  ClientQuery* q = it->second;
  delete q;
  queries_.erase(it);
}

//////////////////////////////////////////////////////////////
//
//     Methods available only from the selector thread.
//
//////////////////////////////////////////////////////////////

void ClientConnectionTCP::StartOpen() {
  if ( !selector_.IsInSelectThread() ) {
    selector_.RunInSelectLoop(
        NewCallback(this, &ClientConnectionTCP::StartOpen));
    return;
  }
  is_opening_ = true;
  timeouter_.SetTimeout(kOpenEvent, open_timeout_ms_);
  StartConnect();
}

void ClientConnectionTCP::EndOpen(const char* err) {
  CHECK ( is_opening_ );
  // maybe set last error
  if ( err != NULL && error_.empty() ) {
    error_ = err;
  }

  if ( err ) {
    // RPC connection failed
    LOG_ERROR << "Open completed with error: " << Error();
  } else {
    // RPC connection established
    LOG_INFO << " Connected to: " << net_connection_->remote_address()
             << " Local address is: " << net_connection_->local_address()
             << " #" << queries_.size() << " queries pending on send.";
  }

  timeouter_.UnsetTimeout(kOpenEvent);
  is_opening_ = false;
  open_completed_.Signal();
  if ( open_callback_ ) {
    LOG_DEBUG << "Running open_callback(" << boolalpha << IsOpen() << ")";
    open_callback_->Run(IsOpen());
    open_callback_ = NULL;
  }
}

void ClientConnectionTCP::StartConnect() {
  if ( !net_connection_->Connect(remote_addr_) ) {
    // connection refused, or host unreachable
    LOG_ERROR << "Failed to connect to remote host: " << remote_addr_
              << ". Error: " << GetLastSystemErrorDescription();
    EndConnect(GetLastSystemErrorDescription());
  }
}

void ClientConnectionTCP::EndConnect(const char* err) {
  if ( err ) {
    EndOpen(err);
    return;
  }
  StartHandshake();
}

void ClientConnectionTCP::StartHandshake() {
  LOG_INFO << "Starting handshake";
  CHECK_EQ(handshake_state_, NOT_INITIALIZED);
  DoHandshake();
}

void ClientConnectionTCP::EndHandshake(
    ClientConnectionTCP::HANDSHAKE_STATE state, const char* err) {
  if ( handshake_state_ == FAILURE || handshake_state_ == CONNECTED ) {
    LOG_FATAL << "Handshake already ended with handshake_state_="
              << handshake_state_;
  }
  handshake_state_ = state;
  EndOpen(err);
}

void ClientConnectionTCP::DoHandshake() {
  // It's a 3 way handshake. The involved packets are as follows:
  //  1. client -> server
  //    - 3 bytes: "rpc"
  //    - 2 bytes: rpc-protocol-version (hi, lo)
  //    - 1 byte: codec identifier
  //    - 32 bytes: client random generated data.
  // 2. server -> client
  //    - 3 bytes: "rpc"
  //    - 2 bytes: rpc-protocol-version. Should match the client version.
  //               Otherwise don't send this packet and drop the handshake.
  //    - 1 byte: codec identifier. Should match the client codec.
  //    - 32 bytes: server random generated data. Different from the
  //                client data.
  //    - 32 bytes: repeat client data.
  // 3. client -> server
  //    - 3 bytes: "rpc"
  //    - 2 bytes: rpc-protocol-version. Should match the server version.
  //               Otherwise don't send this packet and drop the handshake.
  //    - 1 byte: codec id. Should be identical with the first packet.
  //    - 32 bytes: repeat server data.
  //
  CHECK_EQ(net_connection_->state(), net::NetConnection::CONNECTED);
  CHECK_NE(handshake_state_, CONNECTED);
  CHECK_NE(handshake_state_, FAILURE);

  // TODO(cosmin): take out all the inlined consts from here they look ugly
  switch ( handshake_state_ ) {
    case NOT_INITIALIZED: {  // initial state: handshake not started
      // 1. create client first handshake packet
      LOG_INFO << "Handshake step 1: sending client request";
      uint8 clientHand[3+2+1+HANDSHAKE_RANDOM_SIZE];
      memcpy(clientHand, "rpc", 3);        // "rpc" head
      clientHand[3] = RPC_VERSION_MAJOR;   // version HI
      clientHand[4] = RPC_VERSION_MINOR;   // version LO
      clientHand[5] = GetCodec().GetCodecID();
      memcpy(clientHand + 6,
             kHandshakeClientRandomData,
             HANDSHAKE_RANDOM_SIZE);  // 32 bytes client random data

      // Send it to server
      net_connection_->Write(clientHand, sizeof(clientHand));

      handshake_state_ = WAITING_RESPONSE;
      LOG_INFO << "Handshake step 1: completed. Client request sent";
      return;
    }
    case WAITING_RESPONSE: {  // request sent, waiting response
      // 2. wait for server reply ended, check reply
      LOG_INFO << "Handshake step 2: waiting server response";
      uint8 serverHand[3+2+1+HANDSHAKE_RANDOM_SIZE+HANDSHAKE_RANDOM_SIZE];
      if ( net_connection_->inbuf()->Size() < sizeof(serverHand) ) {
        return;   // not enough data received
      }

      uint32 read = net_connection_->inbuf()->Read(
          serverHand, sizeof(serverHand));
      CHECK_EQ(read, sizeof(serverHand));

      // decode server reply
      const uint8* replyMark = serverHand + 0;     // 3 bytes long
      const uint8 replyVersionHi = serverHand[3];  // 1 byte
      const uint8 replyVersionLo = serverHand[4];  // 1 byte
      const uint8 replyCodecID = serverHand[5];    // 1 byte
      const uint8* replyServerRandom = serverHand + 6;
                         // HANDSHAKE_RANDOM_SIZE bytes
      const uint8* replyClientRandom = serverHand + 6 + HANDSHAKE_RANDOM_SIZE;
                        // HANDSHAKE_RANDOM_SIZE bytes
      // check server reply for correctness.
      if ( ::memcmp(replyMark, "rpc", 3) ) {
        LOG_ERROR << "Handshake: bad mark. Server reply starts with: "
                  << strutil::PrintableDataBuffer(replyMark, 3);
        EndHandshake(FAILURE, "Bad handshake mark");
        return;
      }
      if ( replyVersionHi != RPC_VERSION_MAJOR ||
           replyVersionLo != RPC_VERSION_MINOR ) {
        LOG_ERROR << "Handshake: bad version. Server is "
                  << replyVersionHi << "." << replyVersionLo
                  << " while client is " << RPC_VERSION_STR;
        EndHandshake(FAILURE, "Bad handshake version");
        return;
      }
      if ( replyCodecID != GetCodec().GetCodecID() ) {
        LOG_ERROR << "Hanshake: bad codec. Server has codec_id=" << replyCodecID
                  << " while client has codec_id=" << GetCodec().GetCodecID();
        EndHandshake(FAILURE, "Bad codec id");
        return;
      }
      if ( ::memcmp(replyClientRandom, kHandshakeClientRandomData,
                    HANDSHAKE_RANDOM_SIZE) ) {
        LOG_ERROR << "Handshake: bad random";
        EndHandshake(FAILURE, "Bad handshake random");
        return;
      }
      LOG_INFO << "Handshake step 2: completed. Server response OK.";

      // 3. create client acknowledge packet
      LOG_INFO << "Handshake step 3: sending client acknowledge";
      uint8 clientHand[3+2+1+HANDSHAKE_RANDOM_SIZE];
      memcpy(clientHand, "rpc", 3);        // "rpc" head
      clientHand[3] = RPC_VERSION_MAJOR;   // version HI
      clientHand[4] = RPC_VERSION_MINOR;   // version LO
      clientHand[5] = GetCodec().GetCodecID();  // codec identifier
      memcpy(clientHand + 6, replyServerRandom,
             HANDSHAKE_RANDOM_SIZE);       // 32 bytes server random data

      // Send it to server
      net_connection_->Write(clientHand, sizeof(clientHand));

      LOG_INFO << "Handshake step 3: completed. Handshake successful";

      // Handshake successful
      EndHandshake(CONNECTED);
      return;
    }
    case CONNECTED:  // response received and verified. Connection estabilished.
      LOG_FATAL << "Handshake already succeeded.";
    case FAILURE:
      LOG_FATAL << "Handshake already failed.";
    default:
      LOG_FATAL << "No such handshake_state_ = " << handshake_state_;
  }
}

void ClientConnectionTCP::SendPacket(const rpc::Message* p) {
  if ( !selector_.IsInSelectThread() ) {
    selector_.RunInSelectLoop(
        NewCallback(this, &ClientConnectionTCP::SendPacket, p));
    return;
  }

  if ( queries_.size() >= max_paralel_queries_ ) {
    NotifySendFailed(p, RPC_TOO_MANY_QUERIES);
    return;
  }

  // create a new query which will store the message until sent
  ClientQuery* q = new ClientQuery(p);

  // encode the RPC message
  GetCodec().EncodePacket(q->buf_, *p);

  // Always keep a read marker on the stream begin,
  // to be able to rewind on write error.
  q->buf_.MarkerSet();

  // enqueue the packet for send
  pair<ClientQueryMap::iterator, bool>
      result = queries_.insert(make_pair(p->header_.xid_, q));
  CHECK(result.second) << "Duplicate XID";

  if ( !IsOpen() ) {
    if ( net_connection_->state() == net::NetConnection::DISCONNECTED ) {
      StartOpen();
    }
    return;
  }

  // enable write (will trigger write events which write queries_ to network)
  net_connection_->RequestWriteEvents(true);
}

//////////////////////////////////////////////////////////////////////
//
//             net::BufferedConnection methods
//
//
bool ClientConnectionTCP::ConnectionReadHandler() {
  CHECK(selector_.IsInSelectThread());

  // wrap connection's input buffer
  io::MemoryStream* const in = net_connection_->inbuf();

  if ( handshake_state_ != CONNECTED && handshake_state_ != FAILURE ) {
    // still on handshake stage
    DoHandshake();

    if ( handshake_state_ == FAILURE ) {
      LOG_ERROR << "Handshake failed, closing connection: " << Error();
      return false;
    }
    if ( handshake_state_ != CONNECTED ) {
      // handshake did not finish, more data to come.
      return true;
    }
  }
  CHECK_EQ(handshake_state_, CONNECTED);

  // read & decode & handle as many rpc::Message-s as possible
  while ( true ) {
    // create a new rpc::Message to fill up
    scoped_ptr<rpc::Message> msg(new rpc::Message());

    in->MarkerSet();
    DECODE_RESULT result = GetCodec().DecodePacket(*in, *msg);
    if ( result == DECODE_RESULT_ERROR ) {
      // no need to restore data. But do it for logging the "in" buffer.
      in->MarkerRestore();
      LOG_ERROR << "Failed to decode packet from...";
      error_ = "Bad data. Decoder error.";
      return false;            // close connection
    }
    if ( result == DECODE_RESULT_NOT_ENOUGH_DATA ) {
      in->MarkerRestore();  // restore read data.
      return true;            // not enough data, keep waiting.
    }

    // successfully received RPC message
    CHECK_EQ(result, DECODE_RESULT_SUCCESS);
    in->MarkerClear();

    // send the received packet to IClientConnection interface
    IClientConnection::HandleResponse(msg.release());
  }
}

bool ClientConnectionTCP::ConnectionWriteHandler() {
  if ( !net_connection_->outbuf()->IsEmpty() ) {
    //
    // wait for outbuf_ to be consumed
    //
    return true;
  }

  if ( handshake_state_ != CONNECTED ) {
    // handhshake incomplete and outbuf_ empty
    return true;
  }

  CHECK_EQ(handshake_state_, CONNECTED);
  CHECK(net_connection_->outbuf()->IsEmpty());

  //
  // consume queries_
  //
  while ( true ) {
    // maybe pop a new query
    if ( !active_query_ && !queries_.empty() ) {
      ClientQueryMap::iterator it = queries_.begin();
      active_query_ = it->second;
      queries_.erase(it);
      LOG_INFO << "Sending query xid=" << active_query_->msg_->header_.xid_;
    }

    // no active query? nothing to send
    if ( !active_query_ ) {
      break;
    }

    // send active query
    net_connection_->Write(&active_query_->buf_);

    // query sent, delete it
    active_query_->buf_.MarkerClear();
    delete active_query_;
    active_query_ = NULL;
  }

  return true;
}
void ClientConnectionTCP::ConnectionConnectHandler() {
  LOG_INFO << "Connected to: " << net_connection_->remote_address();
  EndConnect();   // connect process ended
}
void ClientConnectionTCP::ConnectionCloseHandler(
    int err, net::NetConnection::CloseWhat what) {
  if ( what != net::NetConnection::CLOSE_READ_WRITE ) {
    return;
  }
  LOG_ERROR << "ConnectionCloseHandler"
               " what=" << net::NetConnection::CloseWhatName(what)
            << " , err: " << err;
  if ( err != 0 ) {
    error_ = GetSystemErrorDescription(err);
  } else {
    error_ = "Connection closed.";
  }
  if ( !shutdown_is_executing_ ) {
    Shutdown();
  }
}

void ClientConnectionTCP::TimeoutHandler(int64 timeout_id) {
  switch ( timeout_id ) {
    case kOpenEvent:
      LOG_ERROR << "Timeout connecting to " << net_connection_->remote_address();
      EndOpen("Open Timeout");
      break;
    default:
      LOG_FATAL << "Unknown timeout_id: " << timeout_id;
      break;
  }
}

}
