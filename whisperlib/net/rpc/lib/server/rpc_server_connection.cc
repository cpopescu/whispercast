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

#include <whisperlib/common/base/common.h>
#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/scoped_ptr.h>

#include <whisperlib/net/rpc/lib/rpc_version.h>
#include <whisperlib/net/rpc/lib/rpc_constants.h>
#include <whisperlib/net/rpc/lib/server/rpc_server_connection.h>
#include <whisperlib/net/rpc/lib/codec/binary/rpc_binary_encoder.h>
#include <whisperlib/net/rpc/lib/codec/binary/rpc_binary_decoder.h>
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_encoder.h>
#include <whisperlib/net/rpc/lib/codec/json/rpc_json_decoder.h>

namespace rpc {

void rpc::ServerConnection::ProtocolHandleHandshake(io::MemoryStream& in) {
  // It's a 3 way handshake. The involved packets are as follows:
  //  1. client -> server
  //    - 3 bytes: "rpc"
  //    - 2 bytes: rpc-protocol-version (hi,lo)
  //    - 1 byte: codec identifier.
  //    - 32 bytes: client random generated data.
  // 2. server -> client
  //    - 3 bytes: "rpc"
  //    - 2 bytes: rpc-protocol-version. Should match the client version.
  //               Otherwise don't send this packet and drop the handshake.
  //    - 1 byte: codec identifier. Should be identical with the client
  //               indicated value. Otherwise don't send this packet and
  //               drop the handshake.
  //    - 32 bytes: server random generated data. Different from the
  //                client data.
  //    - 32 bytes: repeat client data.
  // 3. client -> server
  //    - 3 bytes: "rpc"
  //    - 2 bytes: rpc-protocol-version. Should match the server version.
  //               Otherwise don't send this packet and drop the handshake.
  //    - 1 byte: codec identifier. Identical with the first packet.
  //    - 32 bytes: repeat server data.
  //

  CHECK(handshakeState_ != HS_CONNECTED) << "Handshake already done!";
  CHECK(handshakeState_ != HS_FAILURE) << "Handshake already failed!";

  // protocol constant = 3 + 2 + 1 + HANDSHAKE_RANDOM_SIZE
  if ( in.Size() < 38 ) {
    // not enough data. Wait for more.
    return;
  }

  // decode the handshake
  char mark[4] = { 0, };
  in.Read(mark, 3);
  const uint8 versHi = io::NumStreamer::ReadByte(&in);
  const uint8 versLo = io::NumStreamer::ReadByte(&in);
  const uint8 codecID = io::NumStreamer::ReadByte(&in);
  char data[HANDSHAKE_RANDOM_SIZE] = { 0, };
  uint32 readSize = in.Read(data, HANDSHAKE_RANDOM_SIZE);
  CHECK_EQ(readSize, uint32(HANDSHAKE_RANDOM_SIZE));

  if ( !strutil::StrEql(mark, "rpc") ) {
    // the handshake packet should start with "rpc"
    LOG_DEBUG << "ERROR: Handshake does not start with \"rpc\" but with: "
              << strutil::PrintableDataBuffer(mark, 3);
    handshakeState_ = HS_FAILURE;
    return;
  }

  if ( versHi != RPC_VERSION_MAJOR ||
       versLo != RPC_VERSION_MINOR ) {
    // different version
    LOG_WARNING << "handshake attempt"
                    " from " << net_connection_->remote_address()
                << " with version " << versHi << "." << versLo
                << "; Server version is " << RPC_VERSION_STR
                << ";";
    handshakeState_ = HS_FAILURE;
    return;
  }

  // create codec according to client codecID
  //
  if ( encoder_ == NULL ) {
    CHECK_NULL(decoder_);
    if ( codecID == kCodecIdBinary ) {
      encoder_ = new rpc::BinaryEncoder();
      decoder_ = new rpc::BinaryDecoder();
    } else if ( codecID == kCodecIdJson ) {
      encoder_ = new rpc::JsonEncoder();
      decoder_ = new rpc::JsonDecoder();
    } else {
      LOG_WARNING << "handshake attempt"
                     " from " << net_connection_->remote_address()
                  << " with invalid codecID: " << (int)codecID;
      handshakeState_ = HS_FAILURE;
    };
  }

  switch ( handshakeState_ ) {
    case HS_WAITING_REQUEST: {
      // compose handshake response
      uint8 handResponse[3+2+1+HANDSHAKE_RANDOM_SIZE+HANDSHAKE_RANDOM_SIZE];
      memcpy(handResponse, "rpc", 3);          // "rpc" head
      handResponse[3] = RPC_VERSION_MAJOR;     // version HI
      handResponse[4] = RPC_VERSION_MINOR;     // version LO
      handResponse[5] = codecID;               // codec identifier
      memcpy(handResponse + 6,
             handshakeServerRandomData_,
             HANDSHAKE_RANDOM_SIZE);           // 32 bytes server random data
      memcpy(handResponse + 6 + HANDSHAKE_RANDOM_SIZE,
             data,
             HANDSHAKE_RANDOM_SIZE);     // 32 bytes the received client data
      // send response
      net_connection_->Write(
          reinterpret_cast<const void*>(handResponse), sizeof(handResponse));
      handshakeState_ = HS_WAITING_RESPONSE;
      return;
    }
      break;
    case HS_WAITING_RESPONSE: {
      // check replied data. The client should have replied with our
      // random data.
      if ( memcmp(data, handshakeServerRandomData_, HANDSHAKE_RANDOM_SIZE) ) {
        LOG_WARNING << "handshake failed: client replied different random data";
        handshakeState_ = HS_FAILURE;
        return;
      }
      CHECK_NOT_NULL(encoder_);
      CHECK_NOT_NULL(decoder_);
      if ( codecID != encoder_->type() ) {
        LOG_WARNING << "handshake failed: client replied different codecid."
                       " First: " << encoder_->type()
                    << ", second: " << codecID;
        handshakeState_ = HS_FAILURE;
        return;
      }
      handshakeState_ = HS_CONNECTED;
      return;
    }
      break;
    case HS_FAILURE: {
      // hanshake failed already
      return;
    }
      break;
    default:
      LOG_FATAL << "Invalid Handshake state "
                << (static_cast<uint32>(handshakeState_));
      return;
  }
}

void rpc::ServerConnection::ProtocolHandleMessage(const rpc::Message& p) {
  LOG_DEBUG << "Handle received packet: " << p;

  if ( p.header().msgType() != RPC_CALL ) {
    LOG_ERROR << "Received a non-CALL message! ignoring: " << p;
    return;
  }

  // extract transport
  rpc::Transport transport(rpc::Transport::TCP,
                           net_connection_->local_address(),
                           net_connection_->remote_address());
  // extract call: service, method and arguments
  CHECK_EQ(p.header().msgType(), RPC_CALL);
  const uint32 xid = p.header().xid();
  const string& service = p.cbody().service();
  const string& method = p.cbody().method();
  const io::MemoryStream& params = p.cbody().params();

  // create an internal query. Use xid as qid, because inside a connection
  //  xid s are unique.
  rpc::Query* query = new rpc::Query(transport, xid, service, method,
                                     params, encoder_->type(),
                                     GetResultHandlerID());

  // send the query to the executor. Specify us as the result collector.
  //
  if ( !asyncQueryExecutor_.QueueRPC(query) ) {
    delete query;
    query = NULL;
    LOG_ERROR << "Async queue execution failed:"
              << " service=" << service
              << " method=" << method
              << " reason=" << GetLastSystemErrorDescription();
    io::MemoryStream ms;
    encoder_->Encode(GetLastSystemErrorDescription(), &ms);
    WriteReply(xid, RPC_SYSTEM_ERR, ms);
    return;
  }
}

rpc::ServerConnection::ServerConnection(
    net::Selector* selector,
    bool auto_delete_on_close,
    net::NetConnection * net_connection,
    rpc::IAsyncQueryExecutor& queryExecutor)
    : selector_(selector),
      net_connection_(net_connection),
      handshakeState_(HS_WAITING_REQUEST),
      asyncQueryExecutor_(queryExecutor),
      registeredToQueryExecutor_(false),
      encoder_(NULL),
      decoder_(NULL),
      expectedWriteReplyCalls_(0),
      accessExpectedWriteReplyCalls_(),
      auto_delete_on_close_(auto_delete_on_close) {
  net_connection_->SetReadHandler(
      NewPermanentCallback(
          this, &ServerConnection::ConnectionReadHandler), true);
  net_connection_->SetWriteHandler(
      NewPermanentCallback(
          this, &ServerConnection::ConnectionWriteHandler), true);
  net_connection_->SetCloseHandler(
      NewPermanentCallback(
          this, &ServerConnection::ConnectionCloseHandler), true);
  // generate random data
  for ( uint32 i = 0; i < HANDSHAKE_RANDOM_SIZE; i++ ) {
    handshakeServerRandomData_[i] = '0' + i % 10;
  }
}

rpc::ServerConnection::~ServerConnection() {
  // stop RPC result receiving
  if ( registeredToQueryExecutor_ ) {
    asyncQueryExecutor_.UnregisterResultHandler(*this);
    registeredToQueryExecutor_ = false;
  }
  delete encoder_;
  encoder_ = NULL;
  delete decoder_;
  decoder_ = NULL;
  delete net_connection_;
  net_connection_ = NULL;
  CHECK_EQ(expectedWriteReplyCalls_, 0);
}

void rpc::ServerConnection::IncExpectedWriteReplyCalls() {
  synch::MutexLocker lock(&accessExpectedWriteReplyCalls_);
  expectedWriteReplyCalls_++;
}
void
rpc::ServerConnection::DecExpectedWriteReplyCallsAndPossiblyDeleteConnection() {
  bool deleteConnection = false;
  {
    synch::MutexLocker lock(&accessExpectedWriteReplyCalls_);
    CHECK_GT(expectedWriteReplyCalls_, 0);
    expectedWriteReplyCalls_--;
    deleteConnection =
        (expectedWriteReplyCalls_ == 0) &&
        (net_connection_->state() == net::NetConnection::DISCONNECTED) &&
        auto_delete_on_close_;
  }
  if ( deleteConnection ) {
    selector_->DeleteInSelectLoop(this);
  }
}

//////////////////////////////////////////////////////////////
//
//   Methods available to any external thread (worker threads).
//
void rpc::ServerConnection::WriteReply(uint32 xid,
                                       rpc::REPLY_STATUS status,
                                       const io::MemoryStream& result) {
  rpc::Message* p = new rpc::Message(xid, RPC_REPLY, status, &result);

  LOG_DEBUG << "WriteReply sending packet: " << p->ToString();

  WriteWithEncodeInSelector(p);
}

void rpc::ServerConnection::WriteWithEncodeInSelector(const rpc::Message* p) {
  CHECK_NOT_NULL(p);

  IncExpectedWriteReplyCalls();
  selector_->RunInSelectLoop(
      NewCallback(this, &rpc::ServerConnection::CallbackSendRPCPacket, p));
}

//////////////////////////////////////////////////////////////
//
//     Methods available only from the selector thread.
//
void rpc::ServerConnection::CallbackSendRPCPacket(const rpc::Message* p) {
  CHECK_NOT_NULL(p);
  // the message was dynamically allocated
  scoped_ptr<const rpc::Message> autoDelP(p);
  if ( net_connection_->state() != net::NetConnection::CONNECTED ) {
    LOG_DEBUG << "Bad connection state: " << net_connection_->StateName()
              << " Cannot SendRPCPacket: " << *p;
    DecExpectedWriteReplyCallsAndPossiblyDeleteConnection();
    return;
  }

  LOG_DEBUG << "SendRPCPacket: " << *p;

  // send reply over network
  //
  encoder_->Encode(p, net_connection_->outbuf());
  net_connection_->RequestWriteEvents(true);
  DecExpectedWriteReplyCallsAndPossiblyDeleteConnection();
}

//////////////////////////////////////////////////////////////////////
//
//             net::BufferedConnection methods
//
bool rpc::ServerConnection::ConnectionReadHandler() {
  // get the BufferedConnection's internal memory stream
  io::MemoryStream* in = net_connection_->inbuf();

  if ( handshakeState_ != HS_CONNECTED ) {
    ProtocolHandleHandshake(*in);
    if ( handshakeState_ == HS_FAILURE ) {
      LOG_ERROR << "Handshake failed. Closing connection.";
      return false;
    }
    if ( handshakeState_ != HS_CONNECTED ) {
      // handshake not finished yet
      return true;
    }

    // handshake completed, register to the executor to be able to
    // receive RPC results (as we're about to receive RPC queries)
    //
    asyncQueryExecutor_.RegisterResultHandler(*this);
    registeredToQueryExecutor_ = true;
  }

  // the codec is estabilished in handshake. It should be known by now.
  CHECK_NOT_NULL(encoder_);
  CHECK_NOT_NULL(decoder_);

  while ( !in->IsEmpty() ) {
    // set a marker, to be able to restore read data on incomplete packets.
    in->MarkerSet();

    // try decoding an RPC message
    rpc::Message p;
    DECODE_RESULT result = decoder_->Decode(*in, &p);

    if ( result == DECODE_RESULT_NOT_ENOUGH_DATA ) {
      // not enough data to read the entire packet.
      // restore read data
      in->MarkerRestore();
      return true;
    }
    if ( result == DECODE_RESULT_SUCCESS ) {
      // RPC packet read & decoded successfully
      // a complete packet was read, cancel the marker.
      in->MarkerClear();

      // handle message (queue RPC query for execution)
      ProtocolHandleMessage(p);

      // try read & handle next packet in input stream
      continue;
    }
    if ( result == DECODE_RESULT_ERROR ) {
      // decoder error. Data is not a packet.
      // no need to restore data, we're closing the connection.
      in->MarkerClear();
      LOG_ERROR << "Invalid data. Closing Connection.";
      net_connection_->FlushAndClose();
      return false;
    }

    // no such result
    in->MarkerClear();
    LOG_FATAL << "Illegal result from Decode: " << result;
    return false;
  }
  return true;
}
bool rpc::ServerConnection::ConnectionWriteHandler() {
  return true;
}
void rpc::ServerConnection::ConnectionCloseHandler(
    int err, net::NetConnection::CloseWhat what) {
  if ( what != net::NetConnection::CLOSE_READ_WRITE ) {
    net_connection_->FlushAndClose();
    return;
  }
  LOG_INFO << "Closed connection to " << net_connection_->remote_address()
           << " err: " << GetSystemErrorDescription(err);

  // stop RPC result receiving
  if ( registeredToQueryExecutor_ ) {
    asyncQueryExecutor_.UnregisterResultHandler(*this);
    registeredToQueryExecutor_ = false;
  }
}

void rpc::ServerConnection::HandleRPCResult(const rpc::Query& q) {
  WriteReply(q.qid(), q.status(), q.result());
}

// Returns a description of this connection. Good for logging.
string rpc::ServerConnection::ToString() const {
  std::ostringstream oss;
  oss << "rpc::ServerConnection to " << net_connection_->remote_address();
  return oss.str();
}
}
