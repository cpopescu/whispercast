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

#ifndef __MEDIA_RTP_RTSP_SERVER_PROCESSOR_H__
#define __MEDIA_RTP_RTSP_SERVER_PROCESSOR_H__

#include <whisperlib/net/base/selector.h>
#include <whisperstreamlib/rtp/rtsp/rtsp_processor.h>
#include <whisperstreamlib/rtp/rtsp/rtsp_session.h>
#include <whisperstreamlib/rtp/rtsp/rtsp_media_interface.h>

namespace streaming {
namespace rtsp {

// This class stands by a RTSP server. It receives all RTSP requests,
// resolves them and replies accordingly.
// It manages RTSP sessions, starts and stops rtp::Broadcaster objects.
class ServerProcessor : public Processor, public SessionListener {
public:
  ServerProcessor(net::Selector* selector, MediaInterface* media_interface);
  virtual ~ServerProcessor();

private:
  //////////////////////////////////////////////////////////////////////////
  // Processor methods
  virtual void HandleConnectionConnected(Connection* conn);
  virtual void HandleRequest(Connection* conn, const Request* req);
  virtual void HandleResponse(Connection* conn, const Response* rsp);
  virtual void HandleInterleavedFrame(Connection* conn,
      const InterleavedFrame* frame);
  virtual void HandleConnectionClose(Connection* conn);

  //////////////////////////////////////////////////////////////////////////
  // SessionListener methods
  virtual void HandleSessionSetupCompleted(Session* session);
  virtual void HandleSessionClosed(Session* session);

private:
  void HandleOptions(const Request* req);
  void HandleDescribe(const Request* req);
  void ContinueDescribeCallback(const Request* req, const MediaInfo* info);
  void HandleSetup(const Request* req);
  void ContinueSetup(const Request* req, Session* session, bool setup_success);
  void HandlePlay(const Request* req);
  void HandlePause(const Request* req);
  void HandleTeardown(const Request* req);
  void HandleRecord(const Request* req);
  void HandleRedirect(const Request* req);
  void HandleAnnounce(const Request* req);
  void HandleGetParameter(const Request* req);
  void HandleSetParameter(const Request* req);

  // generates session IDs
  string GenerateSessionID();

  // conn: the connection which sent the rtsp SETUP which created the 'session'
  void AddSession(Session* session, Connection* conn);
  void DeleteSession(Session* session);
  void DeleteSession(const string& session_id);
  Session* FindSession(const string& session_id);

  // Sends a response back to the client.
  // The 'req' is deleted!
  void Reply(const Request* req, const Response& rsp);
  void SimpleReply(const Request* req, STATUS_CODE status,
      const CSeqHeaderField* cseq);
private:
  net::Selector* selector_;
  MediaInterface* media_interface_;

  // session ID -> Session
  typedef map<string, Session*> SessionMap;
  SessionMap sessions_;

  typedef set<const Request*> RequestSet;
  map<Connection*,  RequestSet> active_requests_;
};

} // namespace rtsp
} // namespace streaming

#endif // __MEDIA_RTP_RTSP_SERVER_PROCESSOR_H__
