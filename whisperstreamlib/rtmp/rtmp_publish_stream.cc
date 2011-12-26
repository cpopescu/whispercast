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
// Author: Catalin Popescu

#include <whisperstreamlib/flv/flv_consts.h>
#include <whisperstreamlib/flv/flv_tag.h>
#include <whisperstreamlib/rtmp/rtmp_publish_stream.h>
#include <whisperstreamlib/rtmp/rtmp_util.h>

#define RTMP_LOG(level) if ( connection_->flags().log_level_ < level ); \
                        else LOG(INFO) << connection_->info() << ": "

#define RTMP_LOG_DEBUG   RTMP_LOG(LDEBUG)
#define RTMP_LOG_INFO    RTMP_LOG(LINFO)
#define RTMP_LOG_WARNING RTMP_LOG(LWARNING)
#define RTMP_LOG_ERROR   RTMP_LOG(LERROR)
#define RTMP_LOG_FATAL   RTMP_LOG(LFATAL)

namespace rtmp {

PublishStream::PublishStream(const StreamParams& params,
                             ServerConnection* connection,
                             streaming::ElementMapper* element_mapper)
    : Stream(params, connection),
      element_mapper_(element_mapper),
      importer_(NULL),
      callback_(NULL),
      stop_publisher_callback_(NewPermanentCallback(this,
          &PublishStream::StopPublisher)) {
}

PublishStream::~PublishStream() {
  CHECK_NULL(importer_);
  CHECK_NULL(callback_);
  delete stop_publisher_callback_;
  stop_publisher_callback_ = NULL;
}

bool PublishStream::ProcessEvent(Event* event, int64 timestamp_ms) {
  if ( event->event_type() == EVENT_INVOKE ) {
    EventInvoke* invoke = static_cast<EventInvoke *>(event);
    const string& method(invoke->call()->method_name());
    if ( method == kMethodPublish )   { return InvokePublish(invoke); }
    if ( method == kMethodUnpublish ) { return InvokeUnpublish(invoke); }
  }

  if ( callback_ == NULL ) {
    RTMP_LOG_ERROR << "Not publishing, ignoring event: " << event->ToString();
    return true;
  }

  vector<scoped_ref<streaming::FlvTag> > tags;
  ExtractFlvTags(*event, timestamp_ms, &tags);
  for ( uint32 i = 0; i < tags.size(); i++ ) {
    SendTag(tags[i].get(), tags[i]->timestamp_ms());
  }

  return true;
}

//////////////////////////////////////////////////////////////////////

bool PublishStream::InvokePublish(EventInvoke* invoke) {
  if ( importer_ != NULL ) {
    RTMP_LOG_ERROR << "Already publishing, event: " << invoke->ToString();
    return false;
  }

  if ( invoke->call()->arguments().size() < 2 ||
       invoke->call()->arguments()[0]->object_type() != CObject::CORE_STRING ||
       invoke->call()->arguments()[1]->object_type() != CObject::CORE_STRING) {
    RTMP_LOG_ERROR << "Bad publish arguments, event: " << invoke->ToString();
    return false;
  }

  params_.command_ = static_cast<const CString*>(
      invoke->call()->arguments()[1])->value();

  URL stream_url(string("http://x/") + params_.application_name_ + "/" +
      static_cast<const CString*>(
          invoke->call()->arguments()[0])->value());
  if ( stream_url.path().empty() ) {
    RTMP_LOG_ERROR << "Invalid stream URL, event: " << invoke->ToString();
    return false;
  }

  size_t n = stream_url.path().substr(1).find('/');
  if (n == string::npos) {
    RTMP_LOG_ERROR << "Invalid stream URL, event: " << invoke->ToString();
    return false;
  }
  stream_name_ = stream_url.path().substr(n+2);

  RTMP_LOG_INFO << "Incoming publish: [" << stream_name_ << "]";

  if ( strutil::StrStartsWith(stream_name_, "mp3:") ||
       strutil::StrStartsWith(stream_name_, "mp4:") ) {
    stream_name_ = stream_name_.substr(4); // skip "mp?:"
  }

  streaming::AuthorizerRequest auth("", "",
                                    connection_->remote_address().ToString(),
                                    stream_name_,
                                    streaming::kActionPublish);
  string command;

  vector< pair<string, string> > comp;
  if ( stream_url.GetQueryParameters(&comp, true) ) {
    for ( int i = 0; i < comp.size(); ++i ) {
      if ( comp[i].first == streaming::kMediaUrlParam_UserName ) {
        auth.user_ = comp[i].second;
      } else if ( comp[i].first == streaming::kMediaUrlParam_UserPass ) {
        auth.passwd_ = comp[i].second;
      } else if ( comp[i].first == streaming::kMediaUrlParam_UserToken ) {
        auth.token_ = comp[i].second;
      } else if ( comp[i].first == streaming::kMediaUrlParam_PublishCommand ) {
        command = comp[i].second;
      }
    }
  }

  StartPublishing(invoke->header()->channel_id(), invoke->invoke_id(),
      auth, command);

  return true;
}
//////////////////////////////////////////////////////////////////////

bool PublishStream::InvokeUnpublish(EventInvoke* invoke) {
  if ( importer_ == NULL ) {
    RTMP_LOG_ERROR << "Not publishing, event: " << invoke->ToString();
    return false;
  }
  CHECK_NOT_NULL(callback_);
  if ( invoke->call()->arguments().empty() ||
       invoke->call()->arguments()[0]->object_type() != CObject::CORE_STRING ) {
    RTMP_LOG_ERROR << "Bad unpublish arguments, event: " << invoke->ToString();
    return false;
  }

  const string& stream_name(static_cast<const CString*>(
      invoke->call()->arguments()[0])->value());

  SendEvent(
      connection_->CreateInvokeResultEvent(
          kMethodResult,
          stream_id(),
          invoke->header()->channel_id(),
          invoke->invoke_id()).get());
  SendEvent(
      connection_->CreateStatusEvent(
          stream_id(), kChannelReply, 0,
          "NetStream.Unpublish.Success", stream_name, stream_name).get(),
          -1, NULL, true);

  Stop(true, false);

  return true;
}

//////////////////////////////////////////////////////////////////////
void PublishStream::StartPublishing(int channel_id, uint32 invoke_id,
    streaming::AuthorizerRequest auth_req, string command, bool dec_ref) {
  if ( !connection_->media_selector()->IsInSelectThread() ) {
    IncRef();
    connection_->media_selector()->RunInSelectLoop(NewCallback(this,
      &PublishStream::StartPublishing, channel_id, invoke_id, auth_req,
      command, true));
    return;
  }
  AutoDecRef auto_dec_ref(dec_ref ? this : NULL);

  // because we're in media_selector, instead of using stream_name_
  // (which can change in net_selector), we're using auth_req.resource_
  const string& stream_name = auth_req.resource_;

  streaming::Importer* importer = element_mapper_->GetImporter(
      streaming::Importer::TYPE_RTMP, stream_name);
  if ( importer == NULL ) {
    RTMP_LOG_ERROR << "No importer for path: " << stream_name;
    Stop(false, false);
    return;
  }
  CHECK(importer->type() == streaming::Importer::TYPE_RTMP);
  importer_ = static_cast<streaming::RtmpImporter*>(importer);

  // verify authentication & start publishing
  IncRef();
  importer_->Start("", auth_req, command,
      stop_publisher_callback_, NewCallback(this,
          &PublishStream::StartPublishingCompleted, channel_id, invoke_id));

  // Next step: The importer_ calls StartPublishingCompleted(..)
}
void PublishStream::StartPublishingCompleted(int channel_id, uint32 invoke_id,
    streaming::ProcessingCallback* callback) {
  if ( !connection_->net_selector()->IsInSelectThread() ) {
    connection_->net_selector()->RunInSelectLoop(NewCallback(this,
        &PublishStream::StartPublishingCompleted, channel_id, invoke_id,
        callback));
    return;
  }
  // this a callback run by importer_, always DecRef().
  AutoDecRef auto_dec_ref(this);

  if ( connection_->is_closed() ) {
    Stop(true, false);
    return;
  }

  if ( callback == NULL ) {
    RTMP_LOG_ERROR << "Cannot publish on: [ " << stream_name_ << " ]";
    SendEvent(connection_->CreateInvokeResultEvent(kMethodError,
                                                   stream_id(),
                                                   channel_id,
                                                   invoke_id).get());
    Stop(false, false);
    return;
  }

  RTMP_LOG_INFO << "Publishing OK on [" << stream_name_ << "];";
  callback_ = callback;
  SendEvent(connection_->CreateInvokeResultEvent(kMethodResult,
                                                 stream_id(),
                                                 channel_id,
                                                 invoke_id).get());

  // the pings are going through stream 0 (system stream)...
  connection_->system_stream()->SendEvent(
      connection_->CreateClearPing(0, stream_id()).get());

  SendEvent(
      connection_->CreateStatusEvent(stream_id(),
          kChannelReply, 0,
          "NetStream.Publish.Start", "Start Publishing", stream_name_).get(),
      -1, NULL, true);
}

void PublishStream::StopPublisher() {
  CHECK(connection_->media_selector()->IsInSelectThread());
  // stop sending tags downstream
  callback_ = NULL;
  Stop(false, true);
}

//////////////////////////////////////////////////////////////////////

void PublishStream::Stop(bool send_eos, bool forced, bool dec_ref) {
  if ( !connection_->net_selector()->IsInSelectThread() ) {
    IncRef();
    connection_->net_selector()->RunInSelectLoop(NewCallback(this,
        &PublishStream::Stop, send_eos, forced, true));
    return;
  }
  AutoDecRef auto_dec_ref(dec_ref ? this : NULL);

  RTMP_LOG_INFO << "Stop stream_name: " << stream_name_
                << ", importer_: " << importer_
                << ", send_eos: " << strutil::BoolToString(send_eos)
                << ", forced: " << strutil::BoolToString(forced)
                << ", dec_ref: " << strutil::BoolToString(dec_ref);
  CHECK(connection_->net_selector()->IsInSelectThread());
  if ( importer_ != NULL ) {
    if ( send_eos ) {
      SendTag(scoped_ref<streaming::Tag>(new streaming::EosTag(
          0, streaming::kDefaultFlavourMask, forced)).get(), 0);
    }
    importer_ = NULL;
  }
  connection_->CloseConnection();
  stream_name_ = "";
  // NOTE: Don't set callback_ = NULL now, because SendTag(eos)
  //       may be asynchronous
}

void PublishStream::SendTag(scoped_ref<streaming::Tag> tag, int64 timestamp_ms,
    bool dec_ref) {
  if ( !connection_->media_selector()->IsInSelectThread() ) {
    IncRef();
    connection_->media_selector()->RunInSelectLoop(
        NewCallback(this, &PublishStream::SendTag, tag, timestamp_ms, true));
    return;
  }
  AutoDecRef auto_dec_ref(dec_ref ? this : NULL);

  if ( callback_ == NULL ) {
    // an old scheduled SendTag(), we have been Closed in the meanwhile
    return;
  }

  //  send it downstream
  callback_->Run(tag.get(), timestamp_ms);

  // EOS is the last tag sent downstream
  if ( tag->type() == streaming::Tag::TYPE_EOS ) {
    callback_ = NULL;
  }
}

} // namespace rtmp
