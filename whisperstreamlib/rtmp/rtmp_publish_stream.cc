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

#include <whisperstreamlib/base/stream_auth.h>
#include <whisperstreamlib/base/media_info_util.h>
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
      streaming::Publisher(),
      element_mapper_(element_mapper),
      importer_(NULL),
      pending_start_(false),
      publish_channel_id_(0),
      publish_invoke_id_(0),
      has_audio_(false),
      has_video_(false),
      media_info_extracted_(false) {
}

PublishStream::~PublishStream() {
  CHECK_NULL(importer_);
}

bool PublishStream::ProcessEvent(const Event* event, int64 timestamp_ms) {
  if ( event->event_type() == EVENT_INVOKE ) {
    const EventInvoke* invoke = static_cast<const EventInvoke *>(event);
    const string& method(invoke->call()->method_name());
    if ( method == kMethodPublish )   {
      return InvokePublish(event, invoke->invoke_id());
    }
    if ( method == kMethodUnpublish ) {
      return InvokeUnpublish(event, invoke->invoke_id());
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

      if ( method == kMethodPublish ) {
        return InvokePublish(event, invoke_id);
      }
      if ( method == kMethodUnpublish ) {
        return InvokeUnpublish(event, invoke_id);
      }
    }
    }

  if ( importer_ == NULL || pending_start_ ) {
    RTMP_LOG_ERROR << "Not publishing, ignoring event: " << event->ToString();
    return true;
  }

  vector<scoped_ref<streaming::FlvTag> > tags;
  ExtractFlvTags(*event, timestamp_ms, &tags);
  for ( uint32 i = 0; i < tags.size(); i++ ) {
    streaming::FlvTag* flv_tag = tags[i].get();
    ///////////////////////////////////////////////////
    // Update media_info...
    if ( !media_info_extracted_ ) {
      if ( first_audio_.get() == NULL &&
           flv_tag->body().type() == streaming::FLV_FRAMETYPE_AUDIO ) {
        first_audio_ = flv_tag;
      }
      if ( first_video_.get() == NULL &&
           flv_tag->body().type() == streaming::FLV_FRAMETYPE_VIDEO ) {
        first_video_ = flv_tag;
      }
      if ( first_metadata_.get() == NULL &&
           flv_tag->body().type() == streaming::FLV_FRAMETYPE_METADATA ) {
        has_audio_ = (flv_tag->metadata_body().values().Find("audiocodecid")
            != NULL);
        has_video_ = (flv_tag->metadata_body().values().Find("videocodecid")
            != NULL);
        first_metadata_ = flv_tag;
      }
      if ( first_metadata_.get() != NULL &&
           (!has_audio_ || first_audio_.get() != NULL) &&
           (!has_video_ || first_video_.get() != NULL ) ) {
        streaming::util::ExtractMediaInfoFromFlv(*first_metadata_.get(),
            first_audio_.get(), first_video_.get(), &media_info_);
        LOG_DEBUG << media_info_.ToString();
        media_info_extracted_ = true;

        // send MediaInfo downstream
        SendTag(scoped_ref<streaming::MediaInfoTag>(
            new streaming::MediaInfoTag(
                  streaming::Tag::ATTR_METADATA,
                  streaming::kDefaultFlavourMask,
                  media_info_)).get(),
            flv_tag->timestamp_ms());
      }
    }

    // eat Metadata
    if ( flv_tag->body().type() == streaming::FLV_FRAMETYPE_METADATA &&
         flv_tag->metadata_body().name().value() == streaming::kOnMetaData ) {
      continue;
    }

    // Send tag to importer
    SendTag(tags[i].get(), tags[i]->timestamp_ms());
  }

  return true;
}

//////////////////////////////////////////////////////////////////////

bool PublishStream::InvokePublish(const Event* event, int invoke_id) {
  if ( importer_ != NULL ) {
    RTMP_LOG_ERROR << "Already publishing, event: " << event->ToString();
    return false;
  }

  string path;

  if ( event->event_type() == EVENT_INVOKE ) {
    const EventInvoke* invoke = static_cast<const EventInvoke *>(event);
    if ( invoke->call()->arguments().size() < 2 ||
         invoke->call()->arguments()[0]->object_type() != CObject::CORE_STRING ||
         invoke->call()->arguments()[1]->object_type() != CObject::CORE_STRING) {
      RTMP_LOG_ERROR << "Bad publish arguments, event: " << event->ToString();
      return false;
    }

    path = static_cast<const CString*>(
        invoke->call()->arguments()[0])->value();
    publish_command_ = static_cast<const CString*>(
        invoke->call()->arguments()[1])->value();
    publish_channel_id_ = invoke->header().channel_id();
    publish_invoke_id_ = invoke->invoke_id();
  } else
  if ( event->event_type() == EVENT_FLEX_MESSAGE ) {
    const EventFlexMessage* efm = static_cast<const EventFlexMessage *>(event);
    if (efm->data().size() < 5 ||
        efm->data()[1]->object_type() != CObject::CORE_NUMBER ||
        efm->data()[3]->object_type() != CObject::CORE_STRING ||
        efm->data()[4]->object_type() != CObject::CORE_STRING) {
      RTMP_LOG_ERROR << "Bad publish arguments, event: " << event->ToString();
      return false;
    }

    path = static_cast<const CString*>(efm->data()[3])->value();
    publish_command_ = static_cast<const CString*>(efm->data()[4])->value();
    publish_channel_id_ = event->header().channel_id();
    publish_invoke_id_ = static_cast<const CNumber*>(
        efm->data()[1])->int_value();
  }

  URL stream_url("http://x/" + params().application_name_ + "/" + path);
  if ( stream_url.path().empty() ) {
    RTMP_LOG_ERROR << "Invalid stream URL, event: " << event->ToString();
    return false;
  }

  size_t n = stream_url.path().substr(1).find('/');
  if (n == string::npos) {
    RTMP_LOG_ERROR << "Invalid stream URL, event: " << event->ToString();
    return false;
  }
  stream_name_ = stream_url.path().substr(n+2);

  RTMP_LOG_INFO << "Incoming publish: [" << stream_name_ << "]";

  if ( strutil::StrStartsWith(stream_name_, "mp3:") ||
       strutil::StrStartsWith(stream_name_, "mp4:") ) {
    stream_name_ = stream_name_.substr(4); // skip "mp?:"
  }

  streaming::AuthorizerRequest auth("", "", "",
                                    connection_->remote_address().ToString(),
                                    stream_name_,
                                    streaming::kActionPublish,
                                    0);
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
        publish_command_ = comp[i].second;
      }
    }
  }

  StartPublishing(auth);
  return true;
}

bool PublishStream::InvokeUnpublish(const Event* event, int invoke_id) {
  if ( importer_ == NULL ) {
    RTMP_LOG_ERROR << "Not publishing, event: " << event->ToString();
    return false;
  }

  string stream_name;

  if ( event->event_type() == EVENT_INVOKE ) {
    const EventInvoke* invoke = static_cast<const EventInvoke *>(event);

    if ( invoke->call()->arguments().empty() ||
         invoke->call()->arguments()[0]->object_type() != CObject::CORE_STRING ) {
      RTMP_LOG_ERROR << "Bad unpublish arguments, event: " << invoke->ToString();
      return false;
    }
    stream_name = static_cast<const CString*>(
        invoke->call()->arguments()[0])->value();
    invoke_id = invoke->invoke_id();
  }
  else
  if ( event->event_type() == EVENT_FLEX_MESSAGE ) {
    const EventFlexMessage* efm = static_cast<const EventFlexMessage *>(event);
    if ( efm->data().size() < 4 ||
        efm->data()[3]->object_type() != CObject::CORE_STRING ) {
      RTMP_LOG_ERROR << "Bad unpublish arguments, event: " << event->ToString();
      return false;
    }

    stream_name = static_cast<const CString*>(
        efm->data()[3])->value();
  }

  SendEvent(
      connection_->CreateInvokeResultEvent(
          kMethodResult,
          stream_id(),
          event->header().channel_id(),
          invoke_id).get());
  SendEvent(
      connection_->CreateStatusEvent(
          stream_id(), kChannelReply, 0,
          "NetStream.Unpublish.Success", stream_name, stream_name).get(),
          -1, NULL, true);

  InternalStop(true, false);
  return true;
}

//////////////////////////////////////////////////////////////////////
void PublishStream::StartPublishing(streaming::AuthorizerRequest auth_req,
    bool dec_ref) {
  if ( !connection_->media_selector()->IsInSelectThread() ) {
    IncRef();
    connection_->media_selector()->RunInSelectLoop(NewCallback(this,
      &PublishStream::StartPublishing, auth_req, true));
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
    InternalStop(false, false);
    return;
  }
  CHECK(importer->type() == streaming::Importer::TYPE_RTMP);
  importer_ = static_cast<streaming::RtmpImporter*>(importer);

  // verify authentication & start publishing
  pending_start_ = true;
  IncRef();
  importer_->Start("", auth_req, publish_command_, this);

  // Next step: The importer_ asynchronously calls StartCompleted(..)
}
void PublishStream::StartCompleted(bool success) {
  if ( !connection_->net_selector()->IsInSelectThread() ) {
    connection_->net_selector()->RunInSelectLoop(NewCallback(this,
        &PublishStream::StartCompleted, success));
    return;
  }
  // this a callback run by importer_, always DecRef().
  AutoDecRef auto_dec_ref(this);
  pending_start_ = false;

  if ( connection_->is_closed() ) {
    InternalStop(true, false);
    return;
  }

  if ( !success ) {
    RTMP_LOG_ERROR << "Cannot publish on: [ " << stream_name_ << " ]";
    SendEvent(connection_->CreateInvokeResultEvent(kMethodError,
                                                   stream_id(),
                                                   publish_channel_id_,
                                                   publish_invoke_id_).get());
    InternalStop(false, false);
    return;
  }

  RTMP_LOG_WARNING << "Publishing OK on [" << stream_name_ << "];";
  SendEvent(connection_->CreateInvokeResultEvent(kMethodResult,
                                                 stream_id(),
                                                 publish_channel_id_,
                                                 publish_invoke_id_).get());

  // the pings are going through stream 0 (system stream)...
  connection_->system_stream()->SendEvent(
      connection_->CreateClearPing(0, stream_id()).get());

  SendEvent(connection_->CreateStatusEvent(stream_id(),
      kChannelReply, 0, "NetStream.Publish.Start",
      "Start Publishing", stream_name_).get(), -1, NULL, true);
}

//////////////////////////////////////////////////////////////////////

void PublishStream::InternalStop(bool send_eos, bool forced, bool dec_ref) {
  if ( !connection_->net_selector()->IsInSelectThread() ) {
    IncRef();
    connection_->net_selector()->RunInSelectLoop(NewCallback(this,
        &PublishStream::InternalStop, send_eos, forced, true));
    return;
  }
  AutoDecRef auto_dec_ref(dec_ref ? this : NULL);

  RTMP_LOG_INFO << "InternalStop stream_name: " << stream_name_
                << ", importer_: " << importer_
                << ", send_eos: " << strutil::BoolToString(send_eos)
                << ", forced: " << strutil::BoolToString(forced)
                << ", dec_ref: " << strutil::BoolToString(dec_ref);
  CHECK(connection_->net_selector()->IsInSelectThread());
  if ( send_eos ) {
    SendTag(scoped_ref<streaming::Tag>(new streaming::EosTag(
        0, streaming::kDefaultFlavourMask, forced)).get(), 0);
  }
  connection_->CloseConnection();
  stream_name_ = "";
  // NOTE: Don't set importer_ = NULL now, because SendTag(eos)
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

  if ( importer_ == NULL ) {
    // an old scheduled SendTag(), we have been Closed in the meanwhile
    return;
  }

  //  send it downstream
  importer_->ProcessTag(tag.get(), timestamp_ms);

  // EOS is the last tag sent downstream
  if ( tag->type() == streaming::Tag::TYPE_EOS ) {
    importer_ = NULL;
  }
}

} // namespace rtmp
