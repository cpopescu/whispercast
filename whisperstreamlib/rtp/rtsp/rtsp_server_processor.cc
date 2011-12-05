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

#include <whisperlib/common/base/date.h>
#include <whisperlib/common/base/scoped_ptr.h>
#include <whisperstreamlib/rtp/rtsp/rtsp_server_processor.h>
#include <whisperstreamlib/rtp/rtsp/rtsp_common.h>
#include <whisperstreamlib/rtp/rtsp/rtsp_encoder.h>
#include <whisperstreamlib/rtp/rtsp/rtsp_connection.h>
#include <whisperstreamlib/rtp/rtp_sdp.h>
#include <whisperstreamlib/rtp/rtp_util.h>

namespace streaming {
namespace rtsp {

ServerProcessor::ServerProcessor(net::Selector* selector,
    MediaInterface* media_interface)
  : Processor(),
    SessionListener(),
    selector_(selector),
    media_interface_(media_interface),
    sessions_(),
    active_requests_() {
}
ServerProcessor::~ServerProcessor() {
}

void ServerProcessor::HandleConnectionConnected(Connection* conn) {
  // on server side we use rtsp::Connection::Wrap,
  // so we should not receive a "connect completed" event
  RTSP_LOG_FATAL << "HandleConnectionConnected on server side!";
}
void ServerProcessor::HandleRequest(Connection* conn, const Request* req) {
  RTSP_LOG_INFO << "Handle request: " << req << " " << req->ToString();
  active_requests_[conn].insert(req);
  switch ( req->header().method() ) {
    case METHOD_DESCRIBE: HandleDescribe(req); return;
    case METHOD_ANNOUNCE: HandleAnnounce(req); return;
    case METHOD_GET_PARAMETER: HandleGetParameter(req); return;
    case METHOD_OPTIONS: HandleOptions(req); return;
    case METHOD_PAUSE: HandlePause(req); return;
    case METHOD_PLAY: HandlePlay(req); return;
    case METHOD_RECORD: HandleRecord(req); return;
    case METHOD_REDIRECT: HandleRedirect(req); return;
    case METHOD_SETUP: HandleSetup(req); return;
    case METHOD_SET_PARAMETER: HandleSetParameter(req); return;
    case METHOD_TEARDOWN: HandleTeardown(req); return;
  }
  RTSP_LOG_FATAL << "Illegal method: " << req->header().method();

}
void ServerProcessor::HandleResponse(Connection* conn, const Response* rsp) {
  // Don't know how to handle response on server side
  RTSP_LOG_ERROR << "Ignoring RTSP response: " << rsp->ToString();
}
void ServerProcessor::HandleInterleavedFrame(Connection* conn,
    const InterleavedFrame* frame) {
  // Don't know how to handle an InterleavedFrame on server side
  RTSP_LOG_ERROR << "Ignoring RTSP InterleavedFrame: " << frame->ToString();
}
void ServerProcessor::HandleConnectionClose(Connection* conn) {
  RequestSet& reqs = active_requests_[conn];
  for ( RequestSet::const_iterator it = reqs.begin(); it != reqs.end(); ++it ) {
    const Request* req = *it;
    req->set_connection(NULL);
  }
  active_requests_.erase(conn);

  // TEARDOWN the session created by this connection
  Session* session = FindSession(conn->session_id());
  if ( session != NULL ) {
    RTSP_LOG_INFO << "Teardown session: " << session->id() << " because the"
                     " corresponding rtsp connection: " << conn->ToString()
                  << " has been closed";
    session->Teardown();
  }

  // corresponding retain: rtsp::Server::AcceptorAcceptHandler
  conn->Release();
}

void ServerProcessor::HandleSessionSetupCompleted(Session* session) {
  // Setup succeeded
  CHECK_NOT_NULL(session->active_request());
  ContinueSetup(session->active_request(), session, true);
  session->set_active_request(NULL);
}
void ServerProcessor::HandleSessionClosed(Session* session) {
  if ( session->active_request() != NULL ) {
    // this close is a Setup fail
    ContinueSetup(session->active_request(), session, false);
    session->set_active_request(NULL);
  }
  RTSP_LOG_INFO << "Deleting closed session: " << session->ToString();
  DeleteSession(session);
}

void ServerProcessor::HandleOptions(const Request* req) {
  const CSeqHeaderField* cseq = req->header().tget_field<CSeqHeaderField>();
  if ( cseq == NULL ) {
    RTSP_LOG_ERROR << "Missing CSeq, in req: " << req->ToString();
    SimpleReply(req, STATUS_BAD_REQUEST, cseq);
    return;
  }
  Response response(STATUS_OK);
  response.mutable_header()->add_field(new ServerHeaderField(
      FLAGS_rtsp_server_name));
  response.mutable_header()->add_field(new CSeqHeaderField(cseq->value()));
  response.mutable_header()->add_field(new PublicHeaderField(
      "DESCRIBE, SETUP, TEARDOWN, PLAY, PAUSE, GET_PARAMETER"));
  Reply(req, response);
}
void ServerProcessor::HandleDescribe(const Request* req) {
  const CSeqHeaderField* cseq = req->header().tget_field<CSeqHeaderField>();
  if ( cseq == NULL ) {
    RTSP_LOG_ERROR << "Missing CSeq, in req: " << req->ToString();
    SimpleReply(req, STATUS_BAD_REQUEST, cseq);
    return;
  }
  const string media = req->header().media();
  if ( !media_interface_->Describe(media, NewCallback(this,
      &ServerProcessor::ContinueDescribeCallback, req)) ) {
    RTSP_LOG_ERROR << "Cannot obtain MediaInfo for media: [" << media << "]";
    SimpleReply(req, STATUS_NOT_FOUND, cseq);
    return;
  }
  return;
}
void ServerProcessor::ContinueDescribeCallback(const Request* req,
    const MediaInfo* info) {
  CHECK_NOT_NULL(info);
  const CSeqHeaderField* cseq = req->header().tget_field<CSeqHeaderField>();
  CHECK_NOT_NULL(cseq) << " NULL CSeq should be handled in HandleDescribe()";
  const string media = req->header().media();

  RTSP_LOG_INFO << "Describe media: [" << media << "] => " << info->ToString();

  rtp::Sdp sdp;
  rtp::util::ExtractSdpFromMediaInfo(0, 0, *info, &sdp);

  // It's OK to use the RTSP local address. Because the RTP stream will be sent
  // to the same client IP (as the RTSP client IP), using the same local IP.
  sdp.set_local_addr(req->connection()->local_address().ip_object());

  const string rtsp_host = req->connection()->local_address().ToString();
  if ( sdp.mutable_media(true) ) {
    sdp.mutable_media(true)->attributes_.push_back("control:rtsp://" + rtsp_host
        + "/" + media + "?" + kTrackIdName + "=" + kTrackIdAudio);
  }
  if ( sdp.media(false) ) {
    sdp.mutable_media(false)->attributes_.push_back("control:rtsp://"
        + rtsp_host + "/" + media + "?" + kTrackIdName + "=" + kTrackIdVideo);
  }
  RTSP_LOG_INFO << "Describe media: [" << media << "] => " << sdp.ToString();

  int64 now = timer::Date::Now();
  Response response(STATUS_OK);
  response.mutable_header()->add_field(new CSeqHeaderField(cseq->value()));
  response.mutable_header()->add_field(new DateHeaderField(now));
  response.mutable_header()->add_field(new ExpiresHeaderField(now));
  response.mutable_header()->add_field(new ServerHeaderField(
      FLAGS_rtsp_server_name));
  response.mutable_header()->add_field(new CacheControlHeaderField("no-cache"));
  sdp.WriteStream(response.mutable_data());
  response.UpdateContent("application/sdp");
  Reply(req, response);
}

void ServerProcessor::HandleSetup(const Request* req) {
  const CSeqHeaderField* cseq = req->header().tget_field<CSeqHeaderField>();
  if ( cseq == NULL ) {
    RTSP_LOG_ERROR << "Missing CSeq, in req: " << req->ToString();
    SimpleReply(req, STATUS_BAD_REQUEST, cseq);
    return;
  }
  const TransportHeaderField* transport = req->header().tget_field<
      TransportHeaderField>();
  if ( transport == NULL ) {
    RTSP_LOG_ERROR << "Missing TransportHeaderField, in req: "
                   << req->ToString();
    SimpleReply(req, STATUS_BAD_REQUEST, cseq);
    return;
  }
  if ( transport->transport() != "RTP" ||
       transport->profile() != "AVP" ||
       (transport->lower_transport().is_set() &&
        transport->lower_transport().get() != "TCP" &&
        transport->lower_transport().get() != "UDP") ) {
    RTSP_LOG_ERROR << "Unsupported transport: " << transport->ToString()
                   << " we support only RTP/AVP over UDP/TCP";
    SimpleReply(req, STATUS_UNSUPPORTED_TRANSPORT, cseq);
    return;
  }
  if ( transport->mode().is_set() &&
       transport->mode().get() != "PLAY" ) {
    RTSP_LOG_ERROR << "Unsupported transport: " << transport->ToString()
                   << " we support only mode=\"PLAY\"";
    SimpleReply(req, STATUS_UNSUPPORTED_TRANSPORT, cseq);
    return;
  }
  const string media = req->header().media();
  bool is_interleaved = transport->lower_transport().is_set() &&
                        transport->lower_transport().get() == "TCP";

  // Either: kTrackIdAudio / kTrackIdVideo
  const string track_id = req->header().GetQueryParam(kTrackIdName);
  const bool is_audio = (track_id == kTrackIdAudio);

  if ( is_interleaved ) {
    // RTP through RTSP
    uint8 media_ch = 0;
    uint8 rtcp_ch = 1;
    if ( transport->interleaved().is_set() ) {
      if ( transport->interleaved().get().first < 0 ||
           transport->interleaved().get().first > kMaxUInt8 ||
           transport->interleaved().get().second < 0 ||
           transport->interleaved().get().second > kMaxUInt8 ) {
        RTSP_LOG_ERROR << "Illegal interleaved channels: "
                       << strutil::ToString(transport->interleaved().get());
        SimpleReply(req, STATUS_BAD_REQUEST, cseq);
        return;
      }
      media_ch = static_cast<uint8>(transport->interleaved().get().first);
      rtcp_ch = static_cast<uint8>(transport->interleaved().get().second);
    }

    Session* session = NULL;
    const SessionHeaderField* session_field =
        req->header().tget_field<SessionHeaderField>();
    if ( session_field != NULL ) {
      // SETUP on existing session. We got the other channel (usually: first
      // audio, second video). Configure into existing session.
      const string& session_id = session_field->id();
      session = FindSession(session_id);
      if ( session == NULL ) {
        RTSP_LOG_ERROR << "No such session: " << session_field->ToString();
        SimpleReply(req, STATUS_SESSION_NOT_FOUND, cseq);
        return;
      }
    }
    if ( session == NULL ) {
      // SETUP creates a new Session. Containing only AUDIO channel.
      session = new Session(selector_, media_interface_, *this,
          GenerateSessionID());
      RTSP_LOG_INFO << "Created session: " << session->ToString();
      AddSession(session, req->connection());
    }

    RTSP_LOG_INFO << "Setup session: " << session->ToString()
        << ", media: [" << media << "], interleaved: " << media_ch << "-"
        << rtcp_ch;
    session->set_active_request(req);
    session->Setup(media, req->connection(), media_ch, is_audio);
    return;
  }

  // Regular RTP/UDP
  if ( !transport->client_port().is_set() ) {
    RTSP_LOG_ERROR << "Unsupported transport: " << transport->ToString()
                   << " no client port specified.";
    SimpleReply(req, STATUS_UNSUPPORTED_TRANSPORT, cseq);
    return;
  }
  net::HostPort udp_media_dst(req->connection()->remote_address().ip_object(),
      transport->client_port().get().first);
  net::HostPort udp_control_dst(req->connection()->remote_address().ip_object(),
      transport->client_port().get().second);

  Session* session = NULL;
  const SessionHeaderField* session_field =
      req->header().tget_field<SessionHeaderField>();
  if ( session_field != NULL ) {
    // SETUP on existing session. We probably got VIDEO params.
    // Configure into existing session.
    const string& session_id = session_field->id();
    session = FindSession(session_id);
    if ( session == NULL ) {
      RTSP_LOG_ERROR << "No such session: " << session_field->ToString();
      SimpleReply(req, STATUS_SESSION_NOT_FOUND, cseq);
      return;
    }
  }
  if ( session == NULL ) {
    // SETUP creates a new Session. Usually contains only AUDIO params.
    session = new Session(selector_, media_interface_, *this,
        GenerateSessionID());
    RTSP_LOG_INFO << "Created session: " << session->ToString();
    AddSession(session, req->connection());
  }

  RTSP_LOG_INFO << "Setup session: " << session->ToString()
      << ", media: [" << media << "], udp_media_dst: " << udp_media_dst
      << ", udp_control_dst: " << udp_control_dst
      << ", is_audio: " << std::boolalpha << is_audio;
  session->set_active_request(req);
  session->Setup(media, udp_media_dst, udp_control_dst, is_audio);
}
void ServerProcessor::ContinueSetup(const Request* req,
    Session* session, bool success) {
  const CSeqHeaderField* cseq = req->header().tget_field<CSeqHeaderField>();
  const TransportHeaderField* transport = req->header().tget_field<
      TransportHeaderField>();
  CHECK_NOT_NULL(cseq);
  CHECK_NOT_NULL(transport);

  bool is_interleaved = transport->lower_transport().is_set() &&
                        transport->lower_transport().get() == "TCP";

  if ( !success ) {
    RTSP_LOG_ERROR << "Failed to Setup session: " << session->ToString();
    SimpleReply(req, STATUS_INTERNAL_SERVER_ERROR, cseq);
    return;
  }

  int64 now = timer::Date::Now();
  Response response(STATUS_OK);
  response.mutable_header()->add_field(new ServerHeaderField(
      FLAGS_rtsp_server_name));
  response.mutable_header()->add_field(new CSeqHeaderField(cseq->value()));
  response.mutable_header()->add_field(new SessionHeaderField(session->id(),
      kSessionTimeoutSec));
  response.mutable_header()->add_field(new DateHeaderField(now));
  response.mutable_header()->add_field(new ExpiresHeaderField(now));
  response.mutable_header()->add_field(new CacheControlHeaderField("no-cache"));
  response.mutable_header()->add_field(new ContentBaseHeaderField(
      req->header().media()));
  response.UpdateContent("text");
  TransportHeaderField* t = new TransportHeaderField();
  t->set_transport("RTP");
  t->set_profile("AVP");
  if ( is_interleaved ) {
    // interleaved RTP through RTSP
    t->set_lower_transport("TCP");
    if ( transport->interleaved().is_set() ) {
      t->set_interleaved(transport->interleaved().get());
    }
  } else {
    // regular RTP through UDP
    t->set_lower_transport("UDP");
    t->set_client_port(transport->client_port().get());
  }
  t->set_transmission_type(TransportHeaderField::TT_UNICAST);
  t->set_source(req->connection()->local_address().ip_object().ToString());
  uint16 local_port = session->local_port();
  if ( local_port != 0 ) {
    t->set_server_port(make_pair(local_port, local_port+1));
  }

  response.mutable_header()->add_field(t);
  Reply(req, response);
}
void ServerProcessor::HandlePlay(const Request* req) {
  const CSeqHeaderField* cseq = req->header().tget_field<CSeqHeaderField>();
  if ( cseq == NULL ) {
    RTSP_LOG_ERROR << "Missing CSeq, in req: " << req->ToString();
    SimpleReply(req, STATUS_BAD_REQUEST, cseq);
    return;
  }
  const SessionHeaderField* s = req->header().tget_field<SessionHeaderField>();
  if ( s == NULL ) {
    RTSP_LOG_ERROR << "Missing SessionHeaderField, in req: " << req->ToString();
    SimpleReply(req, STATUS_BAD_REQUEST, cseq);
    return;
  }
  Session* session = FindSession(s->id());
  if ( session == NULL ) {
    RTSP_LOG_ERROR << "No such session: " << s->ToString();
    SimpleReply(req, STATUS_BAD_REQUEST, cseq);
    return;
  }
  session->Play(true);

  Response response(STATUS_OK);
  response.mutable_header()->add_field(new ServerHeaderField(
      FLAGS_rtsp_server_name));
  response.mutable_header()->add_field(new CSeqHeaderField(cseq->value()));
  response.mutable_header()->add_field(new SessionHeaderField(session->id(),
      kSessionTimeoutSec));
  response.mutable_header()->add_field(new DateHeaderField(timer::Date::Now()));
  response.mutable_header()->add_field(new CacheControlHeaderField("no-cache"));
  response.mutable_header()->add_field(new RtpInfoHeaderField(
      strutil::StringPrintf("url=%s", req->header().url()->spec().c_str())));
  Reply(req, response);
}
void ServerProcessor::HandlePause(const Request* req) {
  const CSeqHeaderField* cseq = req->header().tget_field<CSeqHeaderField>();
  if ( cseq == NULL ) {
    RTSP_LOG_ERROR << "Missing CSeq, in req: " << req->ToString();
    SimpleReply(req, STATUS_BAD_REQUEST, cseq);
    return;
  }
  const SessionHeaderField* s = req->header().tget_field<SessionHeaderField>();
  if ( s == NULL ) {
    RTSP_LOG_ERROR << "Missing SessionHeaderField, in req: " << req->ToString();
    SimpleReply(req, STATUS_BAD_REQUEST, cseq);
    return;
  }
  Session* session = FindSession(s->id());
  if ( session == NULL ) {
    RTSP_LOG_ERROR << "No such session: " << s->ToString();
    SimpleReply(req, STATUS_BAD_REQUEST, cseq);
    return;
  }
  session->Play(false);

  Response response(STATUS_OK);
  response.mutable_header()->add_field(new CSeqHeaderField(cseq->value()));
  response.mutable_header()->add_field(new ServerHeaderField(
      FLAGS_rtsp_server_name));
  response.mutable_header()->add_field(new CacheControlHeaderField("no-cache"));
  Reply(req, response);
}
void ServerProcessor::HandleTeardown(const Request* req) {
  const CSeqHeaderField* cseq = req->header().tget_field<CSeqHeaderField>();
  if ( cseq == NULL ) {
    RTSP_LOG_ERROR << "Missing field: CSeq, in req: " << req->ToString();
    SimpleReply(req, STATUS_BAD_REQUEST, cseq);
    return;
  }
  const SessionHeaderField* s = req->header().tget_field<SessionHeaderField>();
  if ( s == NULL ) {
    RTSP_LOG_ERROR << "Missing SessionHeaderField, in req: " << req->ToString();
    SimpleReply(req, STATUS_BAD_REQUEST, cseq);
    return;
  }
  Session* session = FindSession(s->id());
  if ( session == NULL ) {
    RTSP_LOG_ERROR << "No such session: " << s->ToString();
    SimpleReply(req, STATUS_BAD_REQUEST, cseq);
    return;
  }
  RTSP_LOG_INFO << "Teardown session: " << session->ToString();
  session->Teardown();

  Response response(STATUS_OK);
  response.mutable_header()->add_field(new CSeqHeaderField(cseq->value()));
  Reply(req, response);
}
void ServerProcessor::HandleRecord(const Request* req) {
  const CSeqHeaderField* cseq = req->header().tget_field<CSeqHeaderField>();
  if ( cseq == NULL ) {
    RTSP_LOG_ERROR << "Missing CSeq, in req: " << req->ToString();
    SimpleReply(req, STATUS_BAD_REQUEST, cseq);
    return;
  }
  Response response(STATUS_NOT_IMPLEMENTED);
  response.mutable_header()->add_field(new CSeqHeaderField(cseq->value()));
  Reply(req, response);
}
void ServerProcessor::HandleRedirect(const Request* req) {
  const CSeqHeaderField* cseq = req->header().tget_field<CSeqHeaderField>();
  if ( cseq == NULL ) {
    RTSP_LOG_ERROR << "Missing CSeq, in req: " << req->ToString();
    SimpleReply(req, STATUS_BAD_REQUEST, cseq);
    return;
  }
  Response response(STATUS_NOT_IMPLEMENTED);
  response.mutable_header()->add_field(new CSeqHeaderField(cseq->value()));
  Reply(req, response);
}
void ServerProcessor::HandleAnnounce(const Request* req) {
  const CSeqHeaderField* cseq = req->header().tget_field<CSeqHeaderField>();
  if ( cseq == NULL ) {
    RTSP_LOG_ERROR << "Missing CSeq, in req: " << req->ToString();
    SimpleReply(req, STATUS_BAD_REQUEST, cseq);
    return;
  }
  Response response(STATUS_NOT_IMPLEMENTED);
  response.mutable_header()->add_field(new CSeqHeaderField(cseq->value()));
  Reply(req, response);
}
void ServerProcessor::HandleGetParameter(const Request* req) {
  const CSeqHeaderField* cseq = req->header().tget_field<CSeqHeaderField>();
  if ( cseq == NULL ) {
    RTSP_LOG_ERROR << "Missing CSeq, in req: " << req->ToString();
    SimpleReply(req, STATUS_BAD_REQUEST, cseq);
    return;
  }
  // extract parameter names from req body (one parameter each line)
  vector<string> params;
  io::MemoryStream tmp;
  tmp.AppendStreamNonDestructive(&req->data());
  while ( !tmp.IsEmpty() ) {
    string line;
    tmp.ReadLine(&line);
    if ( line == "" ) {
      continue;
    }
    params.push_back(line);
  }
  if ( params.empty() ) {
    // No parameters => this is like a PING, just return OK
    Response response(STATUS_OK);
    response.mutable_header()->add_field(new CSeqHeaderField(cseq->value()));
    response.mutable_header()->add_field(new ServerHeaderField(
        FLAGS_rtsp_server_name));
    response.mutable_header()->add_field(new CacheControlHeaderField(
        "no-cache"));
    Reply(req, response);
    return;
  }
  // TODO(cosmin): parameters are specific to implementation.
  //               No implementation for now.
  RTSP_LOG_ERROR << "Cannot handle GET_PARAMETER: "
                 << strutil::ToString(params);
  Response response(STATUS_NOT_IMPLEMENTED);
  response.mutable_header()->add_field(new CSeqHeaderField(cseq->value()));
  Reply(req, response);
}
void ServerProcessor::HandleSetParameter(const Request* req) {
  const CSeqHeaderField* cseq = req->header().tget_field<CSeqHeaderField>();
  if ( cseq == NULL ) {
    RTSP_LOG_ERROR << "Missing CSeq, in req: " << req->ToString();
    SimpleReply(req, STATUS_BAD_REQUEST, cseq);
    return;
  }
  SimpleReply(req, STATUS_NOT_IMPLEMENTED, cseq);
}
string ServerProcessor::GenerateSessionID() {
  char text[13] = {0,};
  for ( uint32 i = 0; i < sizeof(text) - 1; i++ ) {
    text[i] = '0' + (::rand() % 10);
  }
  return text;
}

void ServerProcessor::AddSession(Session* session, Connection* conn) {
  SessionMap::iterator it = sessions_.find(session->id());
  CHECK(it == sessions_.end()) << " Duplicate session, old: "
      << it->second->ToString() << ", new: " << session->ToString();
  sessions_[session->id()] = session;
  conn->set_session_id(session->id());
}
void ServerProcessor::DeleteSession(Session* session) {
  DeleteSession(session->id());
}
void ServerProcessor::DeleteSession(const string& session_id) {
  SessionMap::iterator it = sessions_.find(session_id);
  CHECK(it != sessions_.end()) << " Cannot find session_id: " << session_id;
  Session* session = it->second;
  selector_->DeleteInSelectLoop(session);
  sessions_.erase(it);
}
Session* ServerProcessor::FindSession(const string& session_id) {
  SessionMap::iterator it = sessions_.find(session_id);
  return it == sessions_.end() ? NULL : it->second;
}

void ServerProcessor::Reply(const Request* req, const Response& rsp) {
  scoped_ptr<const Request> auto_del_req(req);
  if ( req->connection() == NULL ) {
    RTSP_LOG_WARNING << "Orphan request " << req << ", ignoring response";
    return;
  }
  RTSP_LOG_INFO << "Reply to request: " << req
                << " with: " << rsp.ToString();
  active_requests_[req->connection()].erase(req);
  req->connection()->Send(rsp);
}
void ServerProcessor::SimpleReply(const Request* req, STATUS_CODE status,
    const CSeqHeaderField* cseq) {
  Response response(status);
  if ( cseq != NULL ) {
    response.mutable_header()->add_field(new CSeqHeaderField(cseq->value()));
  }
  Reply(req, response);
}


} // namespace rtsp
} // namespace streaming
