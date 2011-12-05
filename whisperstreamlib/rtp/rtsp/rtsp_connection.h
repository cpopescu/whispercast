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

#ifndef __MEDIA_RTP_RTSP_CONNECTION_H__
#define __MEDIA_RTP_RTSP_CONNECTION_H__

#include <whisperlib/common/base/types.h>
#include <whisperlib/net/base/selector.h>
#include <whisperlib/net/base/connection.h>

#include <whisperstreamlib/rtp/rtp_sender.h>
#include <whisperstreamlib/rtp/rtsp/rtsp_encoder.h>
#include <whisperstreamlib/rtp/rtsp/rtsp_decoder.h>

namespace streaming {
namespace rtsp {

class Request;
class Response;
class Processor;

// Implements a RTSP connection capable of sending & receiving RTSP messages
// through the underlying TCP connection. It can be used both on server and
// client side, as it has 2 mechanisms to establish TCP connection:
// - the server uses the Wrap method to pass an already established TCP connection
// - the client uses the Connect method with an asynchronous completion callback
class Connection : public rtp::Sender {
public:
  static const rtp::Sender::Type kType;
  static const uint32 kOutbufMaxSize;
  typedef Callback1<bool> ConnectCallback;
  Connection(net::Selector* selector,
             Processor* processor);
  virtual ~Connection();

  // use the given TCP connection as is. The 'tcp_connection' must be connected.
  // This methods completes immediately, and this Connection is ready to use.
  // The processor_->HandleConnectionConnected() is NOT called.
  void Wrap(net::TcpConnection* tcp_connection);
  // Creates a TCP connection and starts connecting to the given address.
  // If TCP connect succeeds, processor_->HandleConnectionConnected() is called.
  // If TCP connect fails, processor_->HandleConnectionClosed() is called.
  void Connect(const net::HostPort& addr);

  // Begins the TCP close.
  // When TCP close is completed, processor_->HandleConnectionClosed() is called
  void Close();

  const net::HostPort& local_address() const;
  const net::HostPort& remote_address() const;
  const string& session_id() const;

  void set_rtp_audio_ch(uint8 rtp_audio_ch);
  void set_rtp_video_ch(uint8 rtp_video_ch);
  void set_rtp_ch(uint8 rtp_ch, bool is_audio);
  void set_session_id(const string& session_id);

  // Serialize & send through the underlying tcp_connection_
  void Send(const Request& request);
  void Send(const Response& response);

  ///////////////////////////////////////////////////////////////
  // rtp::Sender methods
  // send RTP packet as RTSP interleaved frame
  virtual bool SendRtp(const io::MemoryStream& rtp_packet, bool is_audio);
  // query tcp_connection's output buffer space. Returns the number of RTP
  // packets you can safely send, without overloading this connection.
  // Serves as flow control for RTP through RTSP streaming.
  virtual uint32 OutQueueSpace() const;
  // when OutQueueSpace() becomes > 0, call the 'closure'.
  virtual void SetCallOnOutQueueSpace(Closure* closure);
  virtual void CloseSender();
  virtual string ToString() const;

private:
  // TCP connection handlers
  void ConnectionConnectHandler();
  bool ConnectionReadHandler();
  bool ConnectionWriteHandler();
  void ConnectionCloseHandler(int err, net::NetConnection::CloseWhat what);

private:
  net::Selector* selector_;
  net::TcpConnection* tcp_connection_;

  // Decodes RTSP Requests and Responses
  Decoder decoder_;

  // handles all incoming RTSP Requests and Responses
  Processor* processor_;

  // call this closure when OutQueueSpace() > 0
  Closure* call_on_out_queue_space_;

  // for intearleaved RTP frames
  uint8 rtp_audio_ch_;
  uint8 rtp_video_ch_;

  // The associated session id.
  // Used only externally by the ServerProcessor to keep track of which
  // Connection created which Session.
  string session_id_;
};

} // namespace rtsp
} // namespace streaming

#endif // __MEDIA_RTP_RTSP_CONNECTION_H__
