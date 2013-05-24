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
// Author: Catalin Popescu & Cosmin Tudorache & Mihai Ianculescu

#include <whisperlib/common/base/timer.h>
#include <whisperlib/common/base/date.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/net/url/url.h>
#include <whisperlib/net/util/ipclassifier.h>
#include <whisperstreamlib/rtmp/events/rtmp_call.h>
#include <whisperstreamlib/rtmp/events/rtmp_event_invoke.h>
#include <whisperstreamlib/rtmp/events/rtmp_event_notify.h>
#include <whisperstreamlib/rtmp/rtmp_connection.h>
#include <whisperstreamlib/rtmp/rtmp_play_stream.h>
#include <whisperstreamlib/rtmp/rtmp_publish_stream.h>
#include <whisperstreamlib/rtmp/rtmp_handshake.h>
#include <whisperstreamlib/rtmp/rtmp_stream.h>

#define RTMP_LOG(level) if ( flags().log_level_ > level ); \
                        else LOG(INFO) << info() << ": "

#define RTMP_LOG_DEBUG   RTMP_LOG(LDEBUG)
#define RTMP_LOG_INFO    RTMP_LOG(LINFO)
#define RTMP_LOG_WARNING RTMP_LOG(LWARNING)
#define RTMP_LOG_ERROR   RTMP_LOG(LERROR)
#define RTMP_LOG_FATAL   RTMP_LOG(LFATAL)

namespace rtmp {

class SystemStream : public Stream {
 public:
  SystemStream(const StreamParams& params, ServerConnection* const connection)
    : Stream(params, connection) {}
  virtual ~SystemStream() {}

  virtual void NotifyOutbufEmpty(int32 outbuf_size) {
    LOG_FATAL << "SystemStream has no outbuf";
  }

  virtual bool ProcessEvent(const Event* event, int64 timestamp_ms) {
    LOG_FATAL << "SystemStream can only send events";
    return false;
  }

  virtual void Close() {}

  virtual bool IsPublishing(const string& stream_name) { return false; }

  virtual void NotifyConnectionClosed() {}
};

const char* ServerConnection::StateName(State state) {
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

// Constructor for serving connection
ServerConnection::ServerConnection(
    net::Selector* net_selector,
    net::Selector* media_selector,
    streaming::ElementMapper* element_mapper,
    net::NetConnection* net_connection,
    const string& connection_id,
    streaming::StatsCollector* stats_collector,
    Closure* delete_callback,
    const vector<const net::IpClassifier*>* classifiers,
    const ProtocolFlags* flags,
    MissingStreamCache* missing_stream_cache)
  : net_selector_(net_selector),
    media_selector_(media_selector),
    element_mapper_(element_mapper),
    connection_(net_connection),
    classifiers_(classifiers),
    delete_callback_(delete_callback),
    info_(strutil::StringPrintf("RTMP(%s->%s)",
                                remote_address().ToString().c_str(),
                                local_address().ToString().c_str())),
    state_(HANDSHAKE_WAIT_INIT),
    flags_(flags),
    missing_stream_cache_(missing_stream_cache),
    coder_(flags_->decoder_mem_limit_),
    next_stream_params_(),
    invoke_create_stream_called_(false),
    system_stream_(NULL),
    streams_(),
    bytes_read_(0),
    bytes_read_reported_(0),
    pause_timeout_alarm_(*net_selector_),
    last_pause_registration_time_(0),
    stats_collector_(stats_collector),
    connection_id_(connection_id),
    timeouter_(net_selector, NewPermanentCallback(this,
        &ServerConnection::TimeoutHandler)),
    ref_count_(0) {
  CHECK(net_selector_->IsInSelectThread());
  connection_->SetReadHandler(NewPermanentCallback(
      this, &ServerConnection::ConnectionReadHandler), true);
  connection_->SetWriteHandler(NewPermanentCallback(
      this, &ServerConnection::ConnectionWriteHandler), true);
  connection_->SetCloseHandler(NewPermanentCallback(
      this, &ServerConnection::ConnectionCloseHandler), true);

  if ( flags_->send_buffer_size_ > 0 ) {
    connection_->SetSendBufferSize(flags_->send_buffer_size_);
  }

  timeouter_.SetTimeout(kConnectTimeoutID, flags_->pause_timeout_ms_);
  timeouter_.SetTimeout(kWriteTimeoutID, flags_->write_timeout_ms_);

  connection_begin_stats_.connection_id_ = connection_id_;
  connection_begin_stats_.timestamp_utc_ms_ = timer::Date::Now();
  connection_begin_stats_.remote_host_ =
      connection_->remote_address().ip_object().ToString();
  connection_begin_stats_.remote_port_ =
      connection_->remote_address().port();
  connection_begin_stats_.local_host_ =
      connection_->local_address().ip_object().ToString(),
  connection_begin_stats_.local_port_ =
      connection_->local_address().port();
  connection_begin_stats_.protocol_ = "rtmp";

  connection_end_stats_.connection_id_ = connection_id_;
  connection_end_stats_.timestamp_utc_ms_ =
      connection_begin_stats_.timestamp_utc_ms_;
  connection_end_stats_.bytes_up_ = 0;
  connection_end_stats_.bytes_down_ = 0;
  connection_end_stats_.result_ = ConnectionResult("ACTIVE");

  stats_collector_->StartStats(&connection_begin_stats_,
      &connection_end_stats_);

  next_stream_params_.stream_id_ = 0;
  pause_timeout_alarm_.Set(NewPermanentCallback(this,
      &ServerConnection::CloseConnection), true, flags_->pause_timeout_ms_,
      false, false);

  // NOTE: SystemStream must be created after ref_count_ is initialized to 0!
  //       If you want to use initialization list, then move ref_count_ up.
  system_stream_ = new SystemStream(StreamParams(), this);
  system_stream_->IncRef(); // will DecRef on TCP disconnect
  // Bug trap: we should be at 1 because SystemStream has IncRef()ed us
  CHECK_EQ(ref_count_, 1);

  MayBeReregisterPauseTimeout(true);    // force
}

ServerConnection::~ServerConnection() {
  CHECK(net_selector_->IsInSelectThread());
  CHECK(state_ == CLOSED);
  CHECK(connection_->state() == net::NetConnection::DISCONNECTED);
  if ( delete_callback_ != NULL ) {
    media_selector_->RunInSelectLoop(delete_callback_);
    delete_callback_ = NULL;
  }
  if ( connection_end_stats_.result_.get().result_.get() == "ACTIVE" ) {
    connection_end_stats_.timestamp_utc_ms_ = timer::Date::Now();
    connection_end_stats_.result_ =
        connection_ != NULL && connection_->last_error_code() != 0
        ? ConnectionResult("ERROR") : ConnectionResult("SERVER CLOSE");
    stats_collector_->EndStats(&connection_end_stats_);
  }
  CHECK_NOT_NULL(connection_);
  delete connection_;
  connection_ = NULL;
}

void ServerConnection::CloseConnection() {
  RTMP_LOG_DEBUG << "CloseConnection";
  CHECK(net_selector_->IsInSelectThread());
  connection_->FlushAndClose();
}

scoped_ref<Event> ServerConnection::CreateInvokeResultEvent(
    const string& method, int stream_id, int channel_id, int invoke_id) {
  return new EventInvoke(
      Header(channel_id, stream_id, rtmp::EVENT_INVOKE, 0, false),
      new PendingCall("", method, invoke_id,
            Call::STATUS_PENDING, NULL, new CNull()));
}

scoped_ref<Event> ServerConnection::CreateStatusEvent(
    int stream_id, int channel_id, int invoke_id, const string& code,
    const string& description, const string& detail, const char* method,
    const char* level) {
  CStringMap* const arg = new  CStringMap();
  arg->Set("level", (level != NULL) ? new CString(level):new CString("status"));
  arg->Set("code", new CString(code));
  arg->Set("description", new  CString(description));
  arg->Set("details", new CString(detail));

  return new EventInvoke(
      Header(channel_id, stream_id, rtmp::EVENT_INVOKE, 0, false),
      new PendingCall("", method != NULL ? method : "onStatus",
          invoke_id, Call::STATUS_PENDING, NULL, arg));
}

void ServerConnection::SendEvent(const Event& event,
    const io::MemoryStream* data, bool force_write) {
  CHECK(net_selector_->IsInSelectThread());

  //if ( event.event_type() != EVENT_VIDEO_DATA &&
  //     event.event_type() != EVENT_AUDIO_DATA &&
  //     event.event_type() != EVENT_MEDIA_DATA ) {
  //  LOG(-1) << this << " SendEvent: " << event.ToString();
  //}

  if ( data == NULL ) {
    coder_.Encode(event, AmfUtil::AMF0_VERSION, connection_->outbuf());
  } else {
    coder_.EncodeWithAuxBuffer(event, data, AmfUtil::AMF0_VERSION,
        connection_->outbuf());
  }

  if ( force_write || (outbuf_size() > flags_->min_outbuf_size_to_send_) ) {
    SendOutbufData();
  }
  MayBeReregisterPauseTimeout(force_write);
}

void ServerConnection::SendOutbufData() {
  if ( connection_->state() != net::NetConnection::CONNECTED ||
       connection_->outbuf()->IsEmpty() ) {
    return;
  }
  timeouter_.SetTimeout(kWriteTimeoutID, flags_->write_timeout_ms_);
  connection_->RequestWriteEvents(true);
}

bool ServerConnection::ConnectionReadHandler() {
  connection_end_stats_.bytes_down_ = connection_->count_bytes_read();

  io::MemoryStream* in = connection_->inbuf();
  if ( is_closed() ) {
    in->Clear();
    return false;
  }
  while ( true ) {
    Result result = state() == HANDSHAKE_WAIT_INIT ? DoHandshakeInit(in) :
                    state() == HANDSHAKE_WAIT_REPLY ? DoHandshakeReply(in) :
                    ProcessData(in);
    switch ( result ) {
      case RESULT_SUCCESS:
        break;
      case RESULT_NO_DATA:
        return true;
      case RESULT_ERROR:
        CloseConnection();
        return false;
    }
  }
  LOG_FATAL << "Unreachable code line";
  return true;
}

bool ServerConnection::ConnectionWriteHandler() {
  connection_end_stats_.bytes_up_ = connection_->count_bytes_written();
  if ( connection_->outbuf()->IsEmpty() ) {
    timeouter_.UnsetTimeout(kWriteTimeoutID);
  } else {
    timeouter_.SetTimeout(kWriteTimeoutID, flags_->write_timeout_ms_);
  }
  if ( outbuf_size() > flags_->max_outbuf_size_ ) {
    LOG_ERROR << "outbuf too large: " << outbuf_size() << " vs "
                 "flags_->max_outbuf_size_: " << flags_->max_outbuf_size_
                 << ", closing connection";
    return false;
  }
  if ( outbuf_size() < flags_->max_outbuf_size_/2 ) {
    for ( StreamMap::iterator it = streams_.begin();
          it != streams_.end(); ++it) {
      it->second->NotifyOutbufEmpty(outbuf_size());
    }
  }
  return true;
}
void ServerConnection::ConnectionCloseHandler(
    int err, net::NetConnection::CloseWhat what) {
  if ( what != net::NetConnection::CLOSE_READ_WRITE ) {
    connection_->FlushAndClose();
    return;
  }
  // TCP connection completely closed.
  // Now close the application.
  RTMP_LOG_DEBUG << " Connection Close in state: " << state_name()
                 << ", net_connection state: " << connection_->StateName()
                 << ", left in outbuf: " << connection_->outbuf()->Size();
  state_ = CLOSED;

  // close stats
  connection_end_stats_.timestamp_utc_ms_ = timer::Date::Now();
  connection_end_stats_.result_.ref().result_ = "CLOSE";
  stats_collector_->EndStats(&connection_end_stats_);

  // Notify all streams of our disconnect
  while ( !streams_.empty() ) {
    Stream* stream = streams_.begin()->second;
    streams_.erase(streams_.begin());
    stream->NotifyConnectionClosed();
    stream->DecRef();
  }
  Stream* stream = system_stream_;
  system_stream_ = NULL;
  stream->NotifyConnectionClosed();
  stream->DecRef(); // nothing after DecRef(), because DecRef() may delete us
}

ServerConnection::Result ServerConnection::DoHandshakeInit(
    io::MemoryStream* in) {
  DCHECK(net_selector_->IsInSelectThread());
  CHECK_EQ(state(), HANDSHAKE_WAIT_INIT);
  if ( in->Size() < kHandshakeSize+1 ) {
    return RESULT_NO_DATA;  // wait for more data ..
  }
  uint32 first_byte = io::NumStreamer::ReadByte(in);
  if ( first_byte != kHandshakeLeadByte ) {
    RTMP_LOG_ERROR << "Invalid handshake first byte: "
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

    RTMP_LOG_ERROR << "Cannot generate server response on RTMP handshake";
    return RESULT_ERROR;
  }

  io::NumStreamer::WriteByte(connection_->outbuf(), kHandshakeLeadByte);
  connection_->outbuf()->Write(server_response, 2*kHandshakeSize);
  SendOutbufData();

  delete [] client_data;
  delete [] server_response;

  state_ = HANDSHAKE_WAIT_REPLY;
  return RESULT_SUCCESS;
}

ServerConnection::Result ServerConnection::DoHandshakeReply(
    io::MemoryStream* in) {
  DCHECK(net_selector_->IsInSelectThread());
  CHECK_EQ(state(), HANDSHAKE_WAIT_REPLY);
  if ( in->Size() < kHandshakeSize ) {
    return RESULT_NO_DATA;  // wait for more data ..
  }

  // drop the junk received from the client
  in->Skip(kHandshakeSize);

  state_ = HANDSHAKED;
  return RESULT_SUCCESS;
}

ServerConnection::Result ServerConnection::ProcessData(io::MemoryStream* in) {
  DCHECK(net_selector_->IsInSelectThread());

  while ( true ) {
    if ( is_closed() ) {
      in->Clear();
      return RESULT_SUCCESS;
    }
    int32 initial_size = in->Size();
    scoped_ref<Event> event = NULL;
    AmfUtil::ReadStatus err = coder_.Decode(
        in, AmfUtil::AMF0_VERSION, &event);
    bytes_read_ += (initial_size - in->Size());
    if ( bytes_read_ - bytes_read_reported_ > ( 1 << 20 ) ) {  // 1 MB
      bytes_read_reported_ = bytes_read_;
      // Stupid enough, this event contains an uint32 .. suckers..
      system_stream_->SendEvent(scoped_ref<EventBytesRead>(new EventBytesRead(
          Header(kChannelPing, 0, rtmp::EVENT_BYTES_READ, 0, false),
          static_cast<uint32>(bytes_read_))).get(), -1, NULL, true);
    }

    if ( err == AmfUtil::READ_NO_DATA ) {
      return RESULT_NO_DATA;
    }
    if ( err != AmfUtil::READ_OK ) {
      RTMP_LOG_ERROR << "Error decoding event: " << AmfUtil::ReadStatusName(err)
                     << ", state_: " << state_name();
      return RESULT_ERROR;
    }

    CHECK_EQ(err, AmfUtil::READ_OK);
    CHECK_NOT_NULL(event.get()) << " on ReadStatus: "
        << AmfUtil::ReadStatusName(err);

    //LOG(-1) << this << " ReceivedEvent: " << event.ToString();

    MayBeReregisterPauseTimeout(false); // no force

    // On CONNECT event -> stop connect timeout
    if ( event->event_type() == EVENT_INVOKE ) {
       EventInvoke* invoke = static_cast<EventInvoke*>(event.get());
       if ( invoke->call()->method_name() == kMethodConnect ) {
         timeouter_.UnsetTimeout(kConnectTimeoutID);
       }
    }

    if ( !ProcessEvent(event.get()) ) {
      RTMP_LOG_ERROR << "Error processing event: " << event.ToString();
      return RESULT_ERROR;
    }
  }
  return RESULT_SUCCESS;
}

bool ServerConnection::ProcessEvent(const Event* event) {
  CHECK(!is_closed());

  if ( event->event_type() == EVENT_INVOKE ) {
    const EventInvoke* invoke = static_cast<const EventInvoke *>(event);
    const string& method = invoke->call()->method_name();
    if ( method == kMethodConnect )      { return InvokeConnect(invoke); }
    if ( method == kMethodCreateStream ) {
      return InvokeCreateStream(event, invoke->invoke_id());
    }
    if ( method == kMethodDeleteStream ) {
      return InvokeDeleteStream(event, invoke->invoke_id());
    }
    if ( method == kMethodPublish ) {
      return InvokePublish(event, invoke->invoke_id());
    }
    if ( method == kMethodPlay ) {
      return InvokePlay(event, invoke->invoke_id());
    }
  } else
  if ( event->event_type() == EVENT_FLEX_MESSAGE ) {
    const EventFlexMessage* efm = static_cast<const EventFlexMessage *>(event);
    if (efm->data().size() >= 2 &&
        efm->data()[0]->object_type() == CObject::CORE_STRING &&
        efm->data()[1]->object_type() == CObject::CORE_NUMBER) {
      const string& method = static_cast<const CString*>(
          efm->data()[0])->value();
      const int invoke_id = static_cast<const CNumber*>(
          efm->data()[1])->int_value();

      if ( method == kMethodCreateStream ) {
        return InvokeCreateStream(event, invoke_id);
      }
      if ( method == kMethodDeleteStream ) {
        return InvokeDeleteStream(event, invoke_id);
      }
      if ( method == kMethodPublish ) {
        return InvokePublish(event, invoke_id);
      }
      if ( method == kMethodPlay ) {
        return InvokePlay(event, invoke_id);
      }
    }
  }

  if (event->event_type() == EVENT_BYTES_READ ) {
    RTMP_LOG_DEBUG << "Ignoring event: " << *event;
    return true;
  }

  StreamMap::const_iterator it = streams_.find(event->header().stream_id());
  if ( it != streams_.end() ) {
    return it->second->ReceiveEvent(event);
  }

  if ( event->event_type() == EVENT_INVOKE ) {
    return Unhandled(event, static_cast<const EventInvoke *>(event)->invoke_id());
  }
  if ( event->event_type() == EVENT_FLEX_MESSAGE ) {
    const EventFlexMessage* efm = static_cast<const EventFlexMessage *>(event);
    if (efm->data().size() >= 2 &&
        efm->data()[0]->object_type() == CObject::CORE_STRING &&
        efm->data()[1]->object_type() == CObject::CORE_NUMBER) {
      const int invoke_id = static_cast<const CNumber*>(
          efm->data()[1])->int_value();
      return Unhandled(event, invoke_id);
    }
  }

  RTMP_LOG_ERROR << "Unhandled event: " << event->ToString();
  return true;
}

bool ServerConnection::InvokeConnect(const EventInvoke* invoke) {
  //LOG(-1) << this << " InvokeConnect: " << invoke->ToString();

  const uint32 stream_id = invoke->header().stream_id();
  const uint32 channel_id = invoke->header().channel_id();
  const int invoke_id = invoke->invoke_id();

  int32 object_encoding = -1;
  if ( invoke->connection_params() != NULL &&
       invoke->connection_params()->object_type() ==
           CObject::CORE_STRING_MAP ) {
    const CStringMap* params =
        static_cast<const CStringMap*>(invoke->connection_params());
    RTMP_LOG_INFO << "RTMP Connect params: "
                  << (params == NULL ? "NULL" : params->ToString().c_str());
    CStringMap::Map::const_iterator it = params->data().find("objectEncoding");
    if ( it != params->data().end() && it->second != NULL &&
         it->second->object_type() == CObject::CORE_NUMBER ) {
      object_encoding =
          static_cast<const CNumber*>(it->second)->int_value();
      if ( object_encoding != 0 &&
           object_encoding != 3 ) {
        RTMP_LOG_WARNING << "Invalid object encoding: [ " << object_encoding
            << " ], event: " << invoke->ToString();
        object_encoding = -1;
      }
    }
    it = params->data().find("tcUrl");
    if ( it != params->data().end() && it->second != NULL &&
         it->second->object_type() == CObject::CORE_STRING ) {
      const string& app_url =
          static_cast<const CString*>(it->second)->value();
      URL tc_url(app_url);
      if ( tc_url.path().empty() ) {
        RTMP_LOG_ERROR << "Invalid tcUrl: [ " << app_url
            << " ], event: " << invoke->ToString();
        return false;
      }
      next_stream_params_.application_name_ = tc_url.path().substr(1);
      next_stream_params_.application_url_ = app_url;
    }
  }
  if ( next_stream_params_.application_url_ == "" ) {
    next_stream_params_.application_url_ = "rtmp://internal/";
  }

  system_stream_->SendEvent(CreateClearPing(0, stream_id).get());

  CStringMap* const connection_params = new CStringMap();
  connection_params->Set("capabilities", new CNumber(252));
  connection_params->Set("fmsVer", new CString("whispercast/0,3,0,0"));
  connection_params->Set("mode", new CNumber(1));

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

  scoped_ref<EventInvoke> reply = new EventInvoke(
      Header(channel_id, stream_id, EVENT_INVOKE, 0, false),
      new PendingCall("", kMethodResult, invoke_id,
          Call::STATUS_PENDING, connection_params, arg0));

  system_stream_->SendEvent(reply.get(), -1, NULL, true);

  state_ = CONNECTED;
  return true;
}

namespace {
scoped_ref<EventInvoke> ReplyError(const Event* event, int invoke_id,
                                         const string& reason) {
  CStringMap* const arg = new CStringMap();
  arg->Set("level", new CString("error"));
  arg->Set("code", new CString("NetConnection.Call.Failed"));
  arg->Set("description", new CString(reason));
  return new EventInvoke(
                Header(event->header().channel_id(),
                       event->header().stream_id(),
                       EVENT_INVOKE, 0, false),
                new PendingCall("", kMethodError,
                       invoke_id, Call::STATUS_PENDING, NULL, arg));
}
}
bool ServerConnection::InvokeCreateStream(const Event* event, int invoke_id) {
  if ( state() != CONNECTED ) {
    RTMP_LOG_ERROR << "Not connected, event: " << event->ToString();
    return false;
  }

  if ( next_stream_params_.stream_id_ >= flags_->max_num_connection_streams_ ) {
    system_stream_->SendEvent(
        ReplyError(event, invoke_id, "Too many streams").get(), -1, NULL, true);
    return false;
  }
  if ( invoke_create_stream_called_ ) {
    system_stream_->SendEvent(
        ReplyError(event, invoke_id, "Duplicate Call").get(), -1, NULL, true);
    return false;
  }
  invoke_create_stream_called_ = true;
  next_stream_params_.stream_id_++;

  scoped_ref<EventInvoke> reply = new EventInvoke(
      Header(event->header().channel_id(),
             event->header().stream_id(),
             EVENT_INVOKE, 0, false),
      new PendingCall("", kMethodResult,
              invoke_id, Call::STATUS_PENDING, NULL, new CNumber(
              next_stream_params_.stream_id_)));
  system_stream_->SendEvent(reply.get(), -1, NULL, true);
  return true;
}

bool ServerConnection::InvokeDeleteStream(const Event* event, int invoke_id) {
  if ( state() != CONNECTED ) {
    RTMP_LOG_ERROR << "Not connected, event: " << event->ToString();
    return false;
  }

  int stream_id;

  if ( event->event_type() == EVENT_INVOKE ) {
    const EventInvoke* invoke = static_cast<const EventInvoke *>(event);
    if ( invoke->call()->arguments().empty() ||
         invoke->call()->arguments()[0]->object_type() != CObject::CORE_NUMBER ) {
      RTMP_LOG_ERROR << "Bad arguments, event: " << invoke->ToString();
      return false;
    }
    stream_id = static_cast<const CNumber*>(
      invoke->call()->arguments()[0])->int_value();
  } else
  if ( event->event_type() == EVENT_FLEX_MESSAGE ) {
    const EventFlexMessage* efm = static_cast<const EventFlexMessage *>(event);
    if (efm->data().size() < 3 ||
        efm->data()[2]->object_type() == CObject::CORE_NUMBER) {
      RTMP_LOG_ERROR << "Cannot determine stream, event: " << event->ToString();
      return false;
    }
    stream_id = static_cast<const CNumber*>(
        efm->data()[2])->int_value();
  } else {
    RTMP_LOG_ERROR << "Cannot determine stream, event: " << event->ToString();
    return false;
  }

  StreamMap::const_iterator it = streams_.find(stream_id);
  if ( it != streams_.end() ) {
    it->second->Close();
    return true;
  }
  RTMP_LOG_ERROR << "Stream not found, event: " << event->ToString();
  return false;
}

bool ServerConnection::InvokePublish(const Event* event, int invoke_id) {
  if ( state() != CONNECTED ) {
    RTMP_LOG_ERROR << "Not connected, event: " << event->ToString();
    return false;
  }

  if ( !invoke_create_stream_called_ ) {
    RTMP_LOG_ERROR << "No stream was created, event: " << event->ToString();
    return false;
  }
  invoke_create_stream_called_ = false;

  Stream* stream = new PublishStream(next_stream_params_, this,
      element_mapper_);
  stream->IncRef();
  bool success = streams_.insert(make_pair(stream->stream_id(), stream)).second;
  CHECK(success) << "Duplicate stream id: " << stream->stream_id();

  return stream->ReceiveEvent(event);
}

bool ServerConnection::InvokePlay(const Event* event, int invoke_id) {
  if ( state() != CONNECTED ) {
    RTMP_LOG_ERROR << "Not connected, event: " << event->ToString();
    return false;
  }

  if ( !invoke_create_stream_called_ ) {
    RTMP_LOG_ERROR << "No stream was created, event: " << event->ToString();
    return false;
  }
  invoke_create_stream_called_ = false;

  Stream* const stream = new PlayStream(next_stream_params_, this,
      element_mapper_, stats_collector_);
  stream->IncRef();
  bool success = streams_.insert(make_pair(stream->stream_id(), stream)).second;
  CHECK(success) << "Duplicate stream id: " << stream->stream_id();

  return stream->ReceiveEvent(event);
}

bool ServerConnection::Unhandled(const Event* event, int invoke_id) {
  if ( state() != CONNECTED ) {
    RTMP_LOG_ERROR << "Not connected, event: " << event->ToString();
    return false;
  }

  RTMP_LOG_ERROR << "Unhandled event: " << event->ToString();
  // TODO: disable this for now...
  return true;

  /*
  CStringMap* const arg = new CStringMap();
  arg->Set("level", new CString("error"));
  arg->Set("code", new CString("NetConnection.Call.Failed"));
  arg->Set("description", new CString(invoke->call()->method_name() +
      " not found"));

  scoped_ref<EventInvoke> reply = new EventInvoke(
      Header(invoke->header().channel_id(),
             invoke->header().stream_id(),
             EVENT_INVOKE, 0, false),
      new PendingCall("", kMethodError, invoke->invoke_id(),
             Call::STATUS_PENDING, NULL, arg));

  system_stream_->SendEvent(reply.get(), -1, NULL, true);
  return true;
  */
}

void ServerConnection::TimeoutHandler(int64 timeout_id) {
  LOG_ERROR << "Timeout id: " << timeout_id << " , closing connection...";
  connection_->ForceClose();
}

void ServerConnection::MayBeReregisterPauseTimeout(bool force) {
  DCHECK(net_selector_->IsInSelectThread());
  if ( force || (last_pause_registration_time_ + (flags_->pause_timeout_ms_ / 2)
      < net_selector_->now()) ) {
    last_pause_registration_time_ = net_selector_->now();
    pause_timeout_alarm_.Start();
  }
}

}
