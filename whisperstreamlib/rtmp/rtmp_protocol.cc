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
// Author: Cosmin Tudorache & Catalin Popescu
//
// TODO(cpopescu): this needs to be reworked as it was developed and pathched
//    multiple times - the current state is quite messy ..
//

#include <whisperlib/common/base/timer.h>
#include <whisperlib/net/url/url.h>
#include <whisperlib/common/base/date.h>
#include <whisperlib/common/base/scoped_ptr.h>

#include <whisperstreamlib/rtmp/rtmp_protocol.h>
#include <whisperstreamlib/rtmp/rtmp_connection.h>
#include <whisperstreamlib/rtmp/events/rtmp_call.h>
#include <whisperstreamlib/rtmp/events/rtmp_event_notify.h>
#include <whisperstreamlib/rtmp/rtmp_handshake.h>
#include <whisperstreamlib/base/consts.h>

//////////////////////////////////////////////////////////////////////

namespace rtmp {

#define RTMP_PROTO_LOG(level) if ( flags()->log_level_ < level ); \
  else LOG(INFO) << info() << ": "
#define RTMP_PROTO_LOG_DEBUG   RTMP_PROTO_LOG(LDEBUG)
#define RTMP_PROTO_LOG_INFO    RTMP_PROTO_LOG(LINFO)
#define RTMP_PROTO_LOG_WARNING RTMP_PROTO_LOG(LWARNING)
#define RTMP_PROTO_LOG_ERROR   RTMP_PROTO_LOG(LERROR)
#define RTMP_PROTO_LOG_FATAL   RTMP_PROTO_LOG(LFATAL)


//////////////////////////////////////////////////////////////////////
class SystemStream : public Stream {
 public:
  SystemStream(const StreamParams& params, Protocol* const protocol)
    : Stream(params, protocol) {}
  virtual ~SystemStream() {}

  virtual void NotifyOutbufEmpty(int32 outbuf_size) {
    LOG_FATAL << "SystemStream has no outbuf";
  }

  virtual bool ProcessEvent(rtmp::Event* event, int64 timestamp_ms) {
    LOG_FATAL << "SystemStream can only send events";
    return false;
  }

  virtual void Close() {}

  virtual bool IsPublishing(const string& stream_name) { return false; }
};

const char* Protocol::StateName(State state) {
  switch ( state ) {
    CONSIDER(HANDSHAKE_WAIT_INIT);
    CONSIDER(HANDSHAKE_WAIT_REPLY);
    CONSIDER(HANDSHAKED);
    CONSIDER(CONNECTED);
    CONSIDER(CLOSED);
  }
  LOG_FATAL << "Illegal state: " << state;
  return "Unknown";
}

Protocol::Protocol(
    net::Selector* net_selector,
    net::Selector* media_selector,
    ServerConnection* connection,
    StreamManager* rtmp_stream_manager,
    const rtmp::ProtocolFlags* flags)
    : net_selector_(net_selector),
      media_selector_(media_selector),
      connection_(connection),
      info_(strutil::StringPrintf("RTMP(%s->%s)",
                                  remote_address().ToString().c_str(),
                                  local_address().ToString().c_str())),
      stream_manager_(rtmp_stream_manager),
      flags_(flags),
      ref_count_(0),
      state_(HANDSHAKE_WAIT_INIT),
      rtmp_coder_(&protocol_data_, flags_->decoder_mem_limit_),
      connect_timeout_alarm_(*net_selector_),
      pause_timeout_alarm_(*net_selector_),
      last_pause_registration_time_(0),
      pending_stream_id_(-1),
      system_stream_(new SystemStream(StreamParams(), this)),
      bytes_read_(0),
      bytes_read_reported_(0) {
  CHECK(net_selector_->IsInSelectThread());
  next_stream_params_.stream_id_ = 1;

  connect_timeout_alarm_.Set(NewPermanentCallback(this,
      &Protocol::CloseConnection), true, flags_->pause_timeout_ms_, false, true);
  pause_timeout_alarm_.Set(NewPermanentCallback(this,
      &Protocol::CloseConnection), true, flags_->pause_timeout_ms_, false, false);

  MayBeReregisterPauseTimeout(true);    // force
}

//////////////////////////////////////////////////////////////////////

Protocol::~Protocol() {
  DCHECK_EQ(ref_count_, -1);

  // Make sure the system_stream is deleted!
  // It keeps the protocol alive through IncRef().
  CHECK_NULL(system_stream_);

  CHECK_NOT_NULL(connection_);
  delete connection_;
  connection_ = NULL;
}

const net::HostPort& Protocol::local_address() const {
  return connection_->local_address();
}
const net::HostPort& Protocol::remote_address() const {
  return connection_->remote_address();
}
Protocol::State Protocol::state() const {
  synch::MutexLocker l(&mutex_);
  return state_;
}
void Protocol::set_state(Protocol::State state) {
  synch::MutexLocker l(&mutex_);
  state_ = state;
}
const char* Protocol::state_name() const {
  return StateName(state());
}
bool Protocol::is_closed() const {
  return state() >=  CLOSED;
}
const ConnectionBegin& Protocol::connection_begin_stats() const {
  return connection_->connection_begin_stats();
}
const ConnectionEnd& Protocol::connection_end_stats() const {
  return connection_->connection_end_stats();
}
const rtmp::ProtocolFlags* Protocol::flags() const {
  return flags_;
}
const ProtocolData& Protocol::protocol_data() const {
  return protocol_data_;
}
ProtocolData* Protocol::mutable_protocol_data() {
  return &protocol_data_;
}
Stream* Protocol::system_stream() {
  return system_stream_;
}

void Protocol::CloseProtocol() {
  {
    // synchronize, because CloseProtocol may come from net & media concurrently
    synch::MutexLocker l(&mutex_);
    if ( state_ == CLOSED ) {
      return;
    }
    state_ = CLOSED;
  }

  IncRef();

  if ( net_selector_->IsInSelectThread() ) {
    CloseInNetSelector();
  } else {
    net_selector_->RunInSelectLoop(NewCallback(
        this, &Protocol::CloseInNetSelector));
  }
}
void Protocol::CloseInNetSelector() {
  CHECK(net_selector_->IsInSelectThread());
  pause_timeout_alarm_.Stop();
  connect_timeout_alarm_.Stop();
  CloseConnection();
  media_selector_->RunInSelectLoop(NewCallback(this, &Protocol::CloseInMediaSelector));
}
void Protocol::CloseInMediaSelector() {
  CHECK(media_selector_->IsInSelectThread());
  StreamMap tmp;
  {
    synch::MutexLocker l(&mutex_);
    tmp = streams_;
    streams_.clear();
  }
  for ( StreamMap::iterator it = tmp.begin(); it != tmp.end(); ++it ) {
    it->second->Close();  // -> DecRef comes via NotifyStreamClose
  }
  delete system_stream_;
  system_stream_ = NULL;
  DecRef();
}

//////////////////////////////////////////////////////////////////////
//
// NET SELECTOR STUFF
//
//////////////////////////////////////////////////////////////////////

int32 Protocol::outbuf_size() const {
  DCHECK(net_selector_->IsInSelectThread());
  return connection_->outbuf_size();
}

void Protocol::CloseConnection() {
  DCHECK(net_selector_->IsInSelectThread());
  connection_->CloseConnection();
}

void Protocol::NotifyConnectionClose() {
  DCHECK(net_selector_->IsInSelectThread());
  RTMP_PROTO_LOG_INFO << "Connection closed underneath, runtime: "
                << (timer::Date::Now() -
                      connection_begin_stats().timestamp_utc_ms_.get())
                << " ms";
  CloseProtocol();
}

void Protocol::NotifyConnectionWrite() {
  DCHECK(net_selector_->IsInSelectThread());

  if ( outbuf_size() > flags_->max_outbuf_size_ ) {
    CloseProtocol();
    return;
  }
  if ( outbuf_size() < flags_->max_outbuf_size_/2 ) {
    NotifyOutbufEmpty(outbuf_size());
  }
}

void Protocol::HandleClientData(io::MemoryStream* in) {
  DCHECK(net_selector_->IsInSelectThread());
  if ( is_closed() ) {
    in->Clear();
    return;
  }
  while ( true ) {
    Result result = state() == HANDSHAKE_WAIT_INIT ? DoHandshakeInit(in) :
                    state() == HANDSHAKE_WAIT_REPLY ? DoHandshakeReply(in) :
                    ProcessConnectionData(in);
    switch ( result ) {
      case RESULT_SUCCESS:
        break;
      case RESULT_NO_DATA:
        return;
      case RESULT_ERROR:
        CloseProtocol();
        return;
    }
  }
}

Protocol::Result Protocol::DoHandshakeInit(io::MemoryStream* in) {
  DCHECK(net_selector_->IsInSelectThread());
  CHECK_EQ(state(), HANDSHAKE_WAIT_INIT);
  if ( in->Size() < kHandshakeSize+1 ) {
    return RESULT_NO_DATA;  // wait for more data ..
  }
  uint32 first_byte = io::NumStreamer::ReadByte(in);
  if ( first_byte != kHandshakeLeadByte ) {
    RTMP_PROTO_LOG_ERROR << "Invalid handshake first byte: "
                   << strutil::StringPrintf("0x%02x", first_byte)
                   << ", expected: "
                   << strutil::StringPrintf("0x%02x", kHandshakeLeadByte);
    return RESULT_ERROR;
  }

  // get the handshake response
  char* client_data = new char[kHandshakeSize];
  CHECK(in->Read(client_data, kHandshakeSize));

  const char* server_response;
  if ( !PrepareServerHandshake(client_data, kHandshakeSize, &server_response) ) {
    delete [] client_data;

    RTMP_PROTO_LOG_ERROR << "Cannot generate server response on RTMP handshake";
    return RESULT_ERROR;
  }

  io::NumStreamer::WriteByte(connection_->outbuf(), kHandshakeLeadByte);
  connection_->outbuf()->Write(server_response, 2*kHandshakeSize);
  connection_->SendOutbufData();

  delete [] client_data;
  delete [] server_response;

  set_state(HANDSHAKE_WAIT_REPLY);
  return RESULT_SUCCESS;
}

Protocol::Result Protocol::DoHandshakeReply(io::MemoryStream* in) {
  DCHECK(net_selector_->IsInSelectThread());
  CHECK_EQ(state(), HANDSHAKE_WAIT_REPLY);
  if ( in->Size() < kHandshakeSize ) {
    return RESULT_NO_DATA;  // wait for more data ..
  }

  // drop the junk received from the client
  in->Skip(kHandshakeSize);

  set_state(HANDSHAKED);
  return RESULT_SUCCESS;
}

Protocol::Result Protocol::ProcessConnectionData(io::MemoryStream* in) {
  DCHECK(net_selector_->IsInSelectThread());

  while ( true ) {
    int32 initial_size = in->Size();
    rtmp::Event* event = NULL;
    AmfUtil::ReadStatus err = rtmp_coder_.Decode(
        in, protocol_data_.amf_version(), &event);
    bytes_read_ += (initial_size - in->Size());
    if ( bytes_read_ - bytes_read_reported_ > ( 1 << 20 ) ) {  // 1 MB
      bytes_read_reported_ = bytes_read_;
      // Stupid enough, this event contains an uint32 .. suckers..
      system_stream()->SendEvent(new rtmp::EventBytesRead(
                                 static_cast<uint32>(bytes_read_),
                                 &protocol_data_, kPingChannel, 0),
                                 -1, NULL, true);
    }

    if ( err == AmfUtil::READ_NO_DATA ) {
      return RESULT_NO_DATA;
    }
    if ( err != AmfUtil::READ_OK ) {
      RTMP_PROTO_LOG_ERROR << "Error decoding event: "
                           << AmfUtil::ReadStatusName(err)
                           << ", state_: " << state_name();
      return RESULT_ERROR;
    }

    CHECK_EQ(err, AmfUtil::READ_OK);
    CHECK_NOT_NULL(event) << " on ReadStatus: " << AmfUtil::ReadStatusName(err);

    //LOG(-1) << this << " ReceivedEvent: " << event->ToString();

    MayBeReregisterPauseTimeout(false); // no force

    // Handle EVENT_CHUNK_SIZE here, for performance
    if ( event->event_type() == rtmp::EVENT_CHUNK_SIZE ) {
      const int chunk_size =
          static_cast<rtmp::EventChunkSize*>(event)->chunk_size();
      if ( chunk_size < kMaxChunkSize ) {
        //synch::MutexLocker l(&mutex_);
        RTMP_PROTO_LOG_INFO << "Setting chunk size to: " << chunk_size;
        protocol_data_.set_read_chunk_size(chunk_size);
      }
      continue;
    }

    // On CONNECT event -> stop connect_timeout_alarm_
    if ( event->event_type() == rtmp::EVENT_INVOKE ) {
       rtmp::EventInvoke* invoke = static_cast<rtmp::EventInvoke*>(event);
       if ( invoke->call()->method_name() == kMethodConnect ) {
         connect_timeout_alarm_.Stop();
       }
    }

    // delegate event processing to media_selector
    if ( !media_selector_->IsInSelectThread() ) {
      IncRef();
      media_selector_->RunInSelectLoop(
          NewCallback(this, &Protocol::ProcessEvent, event, true));
    } else {
      ProcessEvent(event, false);
    }
  }
  return RESULT_SUCCESS;
}


//////////////////////////////////////////////////////////////////////
//
// MEDIA SELECTOR STUFF
//
//////////////////////////////////////////////////////////////////////

void Protocol::NotifyStreamClose(Stream* stream) {
  DCHECK(media_selector_->IsInSelectThread());
  {
    synch::MutexLocker l(&mutex_);
    streams_.erase(stream->stream_id());
  }
  stream->DecRef();
}

void Protocol::NotifyOutbufEmpty(int32 outbuf_size) {
  CHECK(net_selector_->IsInSelectThread());

  // RACE: Any stream in "streams_" may go away in media_selector, while
  //       we iterate them in net_selector.
  //

  // make a temporary copy of all streams and IncRef them,
  // so they cannot go away when the mutex_ is released.
  StreamMap tmp;
  {
    synch::MutexLocker l(&mutex_);
    tmp = streams_;
    for ( StreamMap::iterator it = tmp.begin();
          it != tmp.end(); ++it) {
      it->second->IncRef();
    }
  }

  // without mutex_ lock: call the notification in each stream + DecRef
  for ( StreamMap::iterator it = tmp.begin();
      it != tmp.end() && !is_closed(); ++it) {
    it->second->NotifyOutbufEmpty(outbuf_size);
    it->second->DecRef();
  }
}

void Protocol::ProcessEvent(rtmp::Event* event, bool dec_ref) {
  DCHECK(media_selector_->IsInSelectThread());
  scoped_ptr<rtmp::Event> auto_del_event(event);
  if ( state() == CLOSED ) {
    if ( dec_ref ) {
      DecRef();
    }
    return;
  }

  bool is_ok = false;
  bool is_handled = false;

  if ( event->event_type() == EVENT_INVOKE ) {
    rtmp::EventInvoke* invoke = static_cast<rtmp::EventInvoke *>(event);

    const string& method = invoke->call()->method_name();
    if ( method == kMethodConnect ) {
      is_handled = true;
      is_ok = InvokeConnect(invoke);
    } else if ( method == kMethodCreateStream ) {
      is_handled = true;
      is_ok = InvokeCreateStream(invoke);
    } else if ( method == kMethodDeleteStream ) {
      is_handled = true;
      is_ok = InvokeDeleteStream(invoke);
    } else if ( method == kMethodPublish ) {
      is_handled = true;
      is_ok = InvokePublish(invoke);
    } else if ( method == kMethodPlay ) {
      is_handled = true;
      is_ok = InvokePlay(invoke);
    }
  } else if (event->event_type() == EVENT_BYTES_READ ) {
    RTMP_PROTO_LOG_DEBUG << "Ignoring event: " << *event;

    is_handled = true;
    is_ok = true;
  }

  if ( !is_handled ) {
    StreamMap::const_iterator it = streams_.find(event->header()->stream_id());
    if ( it != streams_.end() ) {
      is_handled = true;
      is_ok = it->second->ReceiveEvent(event);
    }
  }

  if ( !is_handled ) {
    if ( event->event_type() == EVENT_INVOKE ) {
      is_handled = true;
      is_ok = InvokeUnhandled(static_cast<rtmp::EventInvoke *>(event));
    } else {
      RTMP_PROTO_LOG_WARNING << "Unhandled event: " << event->ToString();
      is_ok = true;
    }
  }

  if ( !is_ok ) {
    RTMP_PROTO_LOG_ERROR << "Error processing event: " << event->ToString()
                   << ", closing protocol...";
    CloseProtocol();
  }

  if ( dec_ref ) {
    DecRef();
  }
}

////////////////////////////////////////////////////////////////////////////////

bool Protocol::InvokeConnect(rtmp::EventInvoke* invoke) {
  //LOG(-1) << this << " InvokeConnect: " << invoke->ToString();

  const uint32 stream_id = invoke->header()->stream_id();
  const uint32 channel_id = invoke->header()->channel_id();
  const int invoke_id = invoke->invoke_id();

  int32 object_encoding = -1;
  if ( invoke->connection_params() != NULL &&
       invoke->connection_params()->object_type() ==
       CObject::CORE_STRING_MAP ) {
    const CStringMap* params =
        static_cast<const CStringMap*>(invoke->connection_params());
    RTMP_PROTO_LOG_INFO << "RTMP Connect params: "
                  << (params == NULL ? "NULL" : params->ToString().c_str());
    CStringMap::Map::const_iterator it = params->data().find("objectEncoding");
    if ( it != params->data().end() && it->second != NULL &&
         it->second->object_type() == CObject::CORE_NUMBER ) {
      object_encoding =
          static_cast<const CNumber*>(it->second)->int_value();
      if ( object_encoding != 0 &&
           object_encoding != 3 ) {
        RTMP_PROTO_LOG_WARNING << "Invalid object encoding: [ " << object_encoding
            << " ], event: " << invoke->ToString();
        object_encoding = -1;
      }
    }
    it = params->data().find("tcUrl");
    if ( it != params->data().end() && it->second != NULL &&
         it->second->object_type() == CObject::CORE_STRING ) {
      const string app_url(
          static_cast<const CString*>(it->second)->value());
      URL tc_url(app_url);
      if ( tc_url.path().empty() ) {
        RTMP_PROTO_LOG_ERROR << "Invalid tcUrl: [ " << app_url
            << " ], event: " << invoke->ToString();
        return false;
      }
      next_stream_params_.application_name_ = tc_url.path().substr(1);
      next_stream_params_.application_url_ = app_url;
    }
  }

  system_stream()->SendEvent(CreateClearPing(0, stream_id));

  // Send Invoke result
  rtmp::EventInvoke* reply = new rtmp::EventInvoke(
      &protocol_data_, channel_id, stream_id);

  reply->set_call(new rtmp::PendingCall("", kMethodResult));
  reply->set_invoke_id(invoke_id);

  reply->mutable_call()->set_status(rtmp::Call::CALL_STATUS_PENDING);

  CStringMap* const connection_params = new  CStringMap();
  connection_params->Set("capabilities", new CNumber(252));
  connection_params->Set("fmsVer", new CString("whispercast/0,3,0,0"));
  connection_params->Set("mode", new CNumber(1));

  reply->mutable_call()->set_connection_params(connection_params);

  CStringMap* const arg0 = new CStringMap();
  arg0->Set("level", new CString("status"));
  arg0->Set("code", new CString("NetConnection.Connect.Success"));
  arg0->Set("description", new CString("Connected"));

  if ( object_encoding >= 0 ) {
    arg0->Set("objectEncoding", new CNumber(object_encoding));
  }

  CMixedMap* data_map = new CMixedMap();
  data_map->Set("type", new CString("whispercast"));
  data_map->Set("version", new CString("0,3,0,0"));
  arg0->Set("data", data_map);

  reply->mutable_call()->AddArgument(arg0);

  system_stream()->SendEvent(reply, -1, NULL, true);

  set_state(CONNECTED);
  return true;
}

//////////////////////////////////////////////////////////////////////

bool Protocol::InvokeCreateStream(rtmp::EventInvoke* invoke) {
  if ( state() != CONNECTED ) {
    RTMP_PROTO_LOG_ERROR << "Not connected, event: " << invoke->ToString();
    return false;
  }

  const char* reason = "";

  bool succeeded = true;
  if ( next_stream_params_.stream_id_ >= flags_->max_num_connection_streams_ ) {
    reason = "Too many streams";
    succeeded = false;
  }
  if ( pending_stream_id_ != -1 ) {
    reason = "Duplicate call";
    succeeded = false;
  }

  rtmp::EventInvoke* reply = new rtmp::EventInvoke(
      &protocol_data_,
      invoke->header()->channel_id(),
      invoke->header()->stream_id());

  if (succeeded) {
    pending_stream_id_ = next_stream_params_.stream_id_;

    reply->set_call(new rtmp::PendingCall("", kMethodResult));
    reply->set_invoke_id(invoke->invoke_id());

    reply->mutable_call()->set_status(rtmp::Call::CALL_STATUS_PENDING);
    reply->mutable_call()->set_connection_params(NULL);
    reply->mutable_call()->AddArgument(new CNumber(pending_stream_id_));
  } else {
    RTMP_PROTO_LOG_ERROR << reason << ", event: " << invoke->ToString();

    reply->set_call(new rtmp::PendingCall("", kMethodError));
    reply->set_invoke_id(invoke->invoke_id());

    reply->mutable_call()->set_status(rtmp::Call::CALL_STATUS_PENDING);

    CStringMap* const arg = new CStringMap();
    arg->Set("level", new CString("error"));
    arg->Set("code", new CString("NetConnection.Call.Failed"));
    arg->Set("description", new CString(reason));

    reply->mutable_call()->AddArgument(arg);
  }

  system_stream()->SendEvent(reply, -1, NULL, true);
  return true;
}

//////////////////////////////////////////////////////////////////////

bool Protocol::InvokeDeleteStream(rtmp::EventInvoke* invoke) {
  if ( state() != CONNECTED ) {
    RTMP_PROTO_LOG_ERROR << "Not connected, event: " << invoke->ToString();
    return false;
  }

  if ( invoke->call()->arguments().empty() ||
       invoke->call()->arguments()[0]->object_type() != CObject::CORE_NUMBER ) {
    RTMP_PROTO_LOG_ERROR << "Bad arguments, event: " << invoke->ToString();
    return false;
  }

  int stream_id = static_cast<const CNumber*>(
          invoke->call()->arguments()[0])->int_value();

  StreamMap::const_iterator it = streams_.find(stream_id);
  if ( it != streams_.end() ) {
    it->second->Close();
    return true;
  }
  RTMP_PROTO_LOG_ERROR << "Stream not found, event: " << invoke->ToString();
  return false;
}

//////////////////////////////////////////////////////////////////////

bool Protocol::InvokePublish(rtmp::EventInvoke* invoke) {
  if ( state() != CONNECTED ) {
    RTMP_PROTO_LOG_ERROR << "Not connected, event: " << invoke->ToString();
    return false;
  }

  if ( pending_stream_id_ == -1 ) {
    RTMP_PROTO_LOG_ERROR << "No stream was created, event: "
                         << invoke->ToString();
    return false;
  }

  Stream* const stream = stream_manager_->CreateStream(
    next_stream_params_, this, true);
  if ( stream == NULL ) {
    RTMP_PROTO_LOG_ERROR << "Cannot create publishing stream, event: "
                   << invoke->ToString();
    return false;
  }

  stream->IncRef();
  {
    synch::MutexLocker l(&mutex_);
    streams_.insert(make_pair(pending_stream_id_, stream));
  }

  next_stream_params_.stream_id_++;
  pending_stream_id_ = -1;

  return stream->ReceiveEvent(invoke);
}

//////////////////////////////////////////////////////////////////////

bool Protocol::InvokePlay(rtmp::EventInvoke* invoke) {
  if ( state() != CONNECTED ) {
    RTMP_PROTO_LOG_ERROR << "Not connected, event: " << invoke->ToString();
    return false;
  }

  if ( pending_stream_id_ == -1 ) {
    RTMP_PROTO_LOG_ERROR << "No stream was created, event: "
                         << invoke->ToString();
    return false;
  }

  Stream* const stream = stream_manager_->CreateStream(
          next_stream_params_, this, false);
  if ( stream == NULL ) {
    RTMP_PROTO_LOG_ERROR << "Cannot create play stream, event: "
        << invoke->ToString();
    return false;
  }

  stream->IncRef();
  {
    synch::MutexLocker l(&mutex_);
    streams_.insert(make_pair(pending_stream_id_, stream));
  }

  next_stream_params_.stream_id_++;
  pending_stream_id_ = -1;

  return stream->ReceiveEvent(invoke);
}

//////////////////////////////////////////////////////////////////////

bool Protocol::InvokeUnhandled(rtmp::EventInvoke* invoke) {
  if ( state() != CONNECTED ) {
    RTMP_PROTO_LOG_ERROR << "Not connected, event: " << invoke->ToString();
    return false;
  }

  RTMP_PROTO_LOG_WARNING << "Unhandled event: " << invoke->ToString();

  rtmp::EventInvoke* reply = new rtmp::EventInvoke(
      &protocol_data_, invoke->header()->channel_id(),
      invoke->header()->stream_id());

  reply->set_call(new rtmp::PendingCall("", kMethodError));
  reply->set_invoke_id(invoke->invoke_id());

  reply->mutable_call()->set_status(rtmp::Call::CALL_STATUS_PENDING);

  CStringMap* const arg = new CStringMap();
  arg->Set("level", new CString("error"));
  arg->Set("code", new CString("NetConnection.Call.Failed"));
  arg->Set("description",
      new CString((invoke->call()->method_name() + " not found").c_str()));

  reply->mutable_call()->AddArgument(arg);

  system_stream()->SendEvent(reply, -1, NULL, true);
  return true;
}

//////////////////////////////////////////////////////////////////////
// Helpers to create specific events

rtmp::Event* Protocol::CreateInvokeResultEvent(const char* method,
                                              int stream_id, int channel_id,
                                              int invoke_id) {
  rtmp::EventInvoke* reply = new rtmp::EventInvoke(
      &protocol_data_, channel_id, stream_id);
  reply->set_invoke_id(invoke_id);

  rtmp::PendingCall* call_reply = new rtmp::PendingCall("", method);
  call_reply->set_invoke_id(invoke_id);

  reply->set_call(call_reply);
  reply->mutable_call()->set_result(rtmp::Call::CALL_STATUS_PENDING,
                                   new CNull());
  return reply;
}

rtmp::Event* Protocol::CreateStatusEvent(int stream_id, int channel_id,
                                        int invoke_id,
                                        const char* code,
                                        const char* description,
                                        const char* detail,
                                        const char* method,
                                        const char* level) {
  rtmp::EventInvoke* reply = new rtmp::EventInvoke(
      &protocol_data_, channel_id, stream_id);

  rtmp::PendingCall* const on_status =
      new rtmp::PendingCall(rtmp::Call::CALL_STATUS_PENDING);
  if ( method == NULL )  {
    on_status->set_method_name("onStatus");
  } else {
    on_status->set_method_name(method);
  }

  CStringMap* const arg = new  CStringMap();
  arg->Set("level", (level != NULL) ? new CString(level):new CString("status"));
  arg->Set("code", new CString(code));
  arg->Set("description", new  CString(description));
  arg->Set("details", new CString(detail));
  on_status->AddArgument(arg);

  reply->set_call(on_status);
  reply->set_invoke_id(invoke_id);

  return reply;
}

void Protocol::SendEvent(rtmp::Event* event,
                         const io::MemoryStream* buffer,
                         bool force_write,
                         bool dec_ref) {
  if ( !net_selector_->IsInSelectThread() ) {
    CHECK_NULL(buffer);
    IncRef();
    net_selector_->RunInSelectLoop(NewCallback(this, &Protocol::SendEvent,
        event, (const io::MemoryStream*)NULL, force_write, true));
    return;
  }
  DCHECK(net_selector_->IsInSelectThread());

  if ( event == NULL ) {
    CloseConnection();
    return;
  }

  scoped_ptr<rtmp::Event> auto_del_event(event);

  //if ( event->event_type() != EVENT_VIDEO_DATA &&
  //     event->event_type() != EVENT_AUDIO_DATA &&
  //     event->event_type() != EVENT_MEDIA_DATA ) {
  //  LOG(-1) << this << " SendEvent: " << event->ToString();
  //}

  if ( buffer == NULL ) {
    rtmp_coder_.Encode(connection_->outbuf(), AmfUtil::AMF0_VERSION, event);
  } else {
    rtmp_coder_.EncodeWithAuxBuffer(connection_->outbuf(),
                                    AmfUtil::AMF0_VERSION,
                                    event, buffer);
  }

  if ( force_write || (outbuf_size() > flags_->min_outbuf_size_to_send_) ) {
    connection_->SendOutbufData();
  }
  MayBeReregisterPauseTimeout(force_write);

  if ( dec_ref ) {
    DecRef();
  }
}

void Protocol::SendChunkSize(int stream_id) {
  SendEvent(new rtmp::EventChunkSize(
      flags()->chunk_size_,
      &protocol_data_, kPingChannel, stream_id), NULL, true);
  protocol_data_.set_write_chunk_size(flags()->chunk_size_);
}

//////////////////////////////////////////////////////////////////////

void Protocol::MayBeReregisterPauseTimeout(bool force) {
  DCHECK(net_selector_->IsInSelectThread());
  if ( force || (last_pause_registration_time_ + (flags_->pause_timeout_ms_ / 2)
      < net_selector_->now()) ) {
    last_pause_registration_time_ = net_selector_->now();
    pause_timeout_alarm_.Start();
  }
}

}
