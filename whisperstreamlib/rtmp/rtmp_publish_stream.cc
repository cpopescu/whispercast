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

#include "rtmp/rtmp_publish_stream.h"

#include <whisperstreamlib/raw/raw_tag_splitter.h>

namespace rtmp {

PublishStream::PublishStream(StreamManager* manager,
                   const StreamParams& params,
                   Protocol* const protocol)
    : Stream(params, protocol),
      manager_(manager),
      callback_(NULL),
      stream_offset_(0),
      closed_(false),
      serializer_(true) {
}

PublishStream::~PublishStream() {
  if ( !closed_ ) {
    Close();
  }
}

bool PublishStream::ProcessEvent(rtmp::Event* event, int64 timestamp_ms) {
  switch ( event->event_type() ) {
    case EVENT_INVOKE: {
      rtmp::EventInvoke* invoke =
          reinterpret_cast<rtmp::EventInvoke *>(event);
      const string method(invoke->call()->method_name());
      if ( method == kMethodPublish ) {
        return InvokePublish(invoke);
      } else  if ( method == kMethodUnpublish ) {
        return InvokeUnpublish(invoke);
      }
    }
    break;
    case EVENT_NOTIFY: {
      rtmp::EventNotify* notification =
          reinterpret_cast<rtmp::EventNotify*>(event);
      if ( notification->name().value() == "@setDataFrame" ) {
        return SetMetadata(notification, timestamp_ms);
      }
    }
    break;
    case EVENT_AUDIO_DATA:
      return ProcessData(static_cast<rtmp::BulkDataEvent*>(event),
                         streaming::FLV_FRAMETYPE_AUDIO, timestamp_ms);
    case EVENT_VIDEO_DATA:
      return ProcessData(static_cast<rtmp::BulkDataEvent*>(event),
                         streaming::FLV_FRAMETYPE_VIDEO, timestamp_ms);
    default:
      return true;
  }
  RTMP_LOG_WARNING << "Unhandled publishing stream event: "
      << event->ToString();
  return true;
}

void PublishStream::Close() {
  DCHECK(protocol_->media_selector()->IsInSelectThread() ||
         protocol_->media_selector()->IsExiting());
  closed_ = true;
  Stop(true);

  protocol_->NotifyStreamClose(this);
}

//////////////////////////////////////////////////////////////////////

bool PublishStream::InvokePublish(rtmp::EventInvoke* invoke) {
  if ( callback_ != NULL ) {
    RTMP_LOG_ERROR << "Already publishing, event: " << invoke->ToString();
    return false;
  }

  if ( invoke->call()->arguments().size() < 2 ||
       invoke->call()->arguments()[0]->object_type() != CObject::CORE_STRING ||
       invoke->call()->arguments()[1]->object_type() != CObject::CORE_STRING) {
    RTMP_LOG_ERROR << "Bad publish arguments, event: " << invoke->ToString();
    return false;
  }

  params_.command_ = reinterpret_cast<const CString*>(
      invoke->call()->arguments()[1])->value();

  URL stream_url(string("http://x/") + params_.application_name_ + "/" +
      reinterpret_cast<const CString*>(
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

  RTMP_LOG_WARNING << "Publish: [" << stream_name_ << "]";

  vector< pair<string, string> > comp;
  if ( stream_url.GetQueryParameters(&comp, true) ) {
    for ( int i = 0; i < comp.size(); ++i ) {
      if ( comp[i].first == streaming::kMediaUrlParam_UserName ) {
        params_.auth_req_.user_ = comp[i].second;
      } else if ( comp[i].first == streaming::kMediaUrlParam_UserPass ) {
        params_.auth_req_.passwd_ = comp[i].second;
      } else if ( comp[i].first == streaming::kMediaUrlParam_UserToken ) {
        params_.auth_req_.token_ = comp[i].second;
      } else if ( comp[i].first == streaming::kMediaUrlParam_PublishCommand ) {
        params_.command_ = comp[i].second;
      }
    }
  }
  params_.auth_req_.net_address_ = protocol_->remote_address().ToString();

  if ( strutil::StrStartsWith(stream_name_, "mp3:") ) {
    stream_name_ = stream_name_.substr(4);
  } else if ( strutil::StrStartsWith(stream_name_, "mp4:") ) {
    stream_name_ = stream_name_.substr(4); // skip
  }
  params_.auth_req_.resource_ = stream_name_;
  params_.auth_req_.action_ = streaming::kActionPublish;

  IncRef();
  manager_->CanPublish(stream_name_, &params_,
      NewCallback(this, &PublishStream::CanPublishCompleted,
                  static_cast<int>(invoke->header()->channel_id()),
                  invoke->invoke_id()));
  return true;
}
//////////////////////////////////////////////////////////////////////

bool PublishStream::InvokeUnpublish(rtmp::EventInvoke* invoke) {
  if ( callback_ == NULL ) {
    RTMP_LOG_ERROR << "Not publishing, event: " << invoke->ToString();
    return false;
  }
  if ( invoke->call()->arguments().empty() ||
       invoke->call()->arguments()[0]->object_type() != CObject::CORE_STRING ) {
    RTMP_LOG_ERROR << "Bad unpublish arguments, event: " << invoke->ToString();
    return false;
  }

  const string& stream_name(
      reinterpret_cast<const CString*>(
          invoke->call()->arguments()[0])->value());

  Stop(false);

  SendEvent(
      protocol_->CreateInvokeResultEvent(kMethodResult,
                                         stream_id(),
                                         invoke->header()->channel_id(),
                                         invoke->invoke_id()));
  SendEvent(
    protocol_->CreateStatusEvent(
        stream_id(), Protocol::kReplyChannel, 0,
        "NetStream.Unpublish.Success",
        stream_name.c_str(),
        stream_name.c_str()),
        -1, NULL, true);

  return true;
}

//////////////////////////////////////////////////////////////////////

void PublishStream::CanPublishCompleted(int channel_id,
                                        uint32 invoke_id,
                                        bool success) {
  if ( closed_ ) {
    DecRef();
    return;
  }

  if ( !success || (callback_ = manager_->StartedPublisher(this)) == NULL ) {
    RTMP_LOG_ERROR << "Cannot publish on: [ " << stream_name_ << " ]";

    if ( !protocol_->is_closed() ) {
      SendEvent(
          protocol_->CreateInvokeResultEvent(kMethodError,
                                             stream_id(),
                                             channel_id,
                                             invoke_id));
      SendEvent(NULL);
    }

    DecRef();
    return;
  }

  if ( !protocol_->is_closed() ) {
    SendEvent(
        protocol_->CreateInvokeResultEvent(kMethodResult,
                                           stream_id(),
                                           channel_id,
                                           invoke_id));

    // the pings are going through stream 0 (system stream)...
    protocol_->system_stream()->SendEvent(
        protocol_->CreateClearPing(0, stream_id()));

    SendEvent(
        protocol_->CreateStatusEvent(
            stream_id(), Protocol::kReplyChannel, 0,
            "NetStream.Publish.Start", "Start Publishing",
            stream_name_.c_str()),
            -1, NULL, true);
  }
  DecRef();
}

//////////////////////////////////////////////////////////////////////

void PublishStream::Stop(bool forced) {
  if ( callback_ != NULL ) {
    callback_->Run(scoped_ref<streaming::Tag>(new streaming::EosTag(0,
        streaming::kDefaultFlavourMask, 0, forced)).get());

    manager_->StoppedPublisher(this);
    callback_ = NULL;
  }

  stream_name_.clear();
  stream_offset_ = 0;
}

bool PublishStream::SetMetadata(rtmp::EventNotify* n, int64 timestamp_ms) {
  if ( callback_ ) {
    if ( n->values().size() == 2 &&
         n->values()[0]->object_type() == CObject::CORE_STRING &&
         n->values()[1]->object_type() == CObject::CORE_STRING_MAP ) {
      io::MemoryStream tag_content;
      n->values()[0]->WriteToMemoryStream(&tag_content,
                                          rtmp::AmfUtil::AMF0_VERSION);
      // We need to write this as a Mixed Map - not as a string map..
      rtmp::CStringMap* str_map =
          reinterpret_cast<rtmp::CStringMap*>(n->values()[1]);
      rtmp::CMixedMap mix_map;
      mix_map.SetAll(*str_map);
      mix_map.WriteToMemoryStream(&tag_content,
                                  rtmp::AmfUtil::AMF0_VERSION);
      return SendTag(&tag_content,
                     streaming::FLV_FRAMETYPE_METADATA,
                     n,
                     timestamp_ms);
    }
    RTMP_LOG_WARNING << "Unknown metadata: " << n->ToString();
    return true;
  }
  RTMP_LOG_ERROR << "Cannot process metadata when not publishing";
  return false;
}

bool PublishStream::ProcessData(rtmp::BulkDataEvent* bd,
                           streaming::FlvFrameType data_type,
                           int64 timestamp_ms) {
  if ( callback_ ) {
    if ( bd->data().Size() > 0 ) {
      return SendTag(bd->mutable_data(), data_type, bd, timestamp_ms);
    }
    RTMP_LOG_WARNING << "Skipping zero sized data event: " << bd->ToString();
    return true;
  }
  RTMP_LOG_ERROR << "Cannot process tags when not publishing";
  return true;
}

bool PublishStream::SendTag(io::MemoryStream* tag_content,
                       streaming::FlvFrameType data_type,
                       const rtmp::Event* event, int64 timestamp_ms) {
  if (event->header()->channel_id() >= kMaxNumChannels) {
    RTMP_LOG_ERROR << "Invalid event channel ID " << event->ToString();
    return false;
  }

  scoped_ref<streaming::FlvTag> flv_tag(new streaming::FlvTag(
      0, streaming::kDefaultFlavourMask, timestamp_ms, data_type));
  streaming::TagReadStatus result =
      flv_tag->mutable_body().Decode(*tag_content, tag_content->Size());
  if ( result != streaming::READ_OK ) {
    RTMP_LOG_ERROR << "Failed to decode Flv tag body, data_type: "
                   << streaming::FlvFrameTypeName(data_type)
                   << ", result: " << streaming::TagReadStatusName(result);
    return false;
  }

  scoped_ref<streaming::RawTag> raw_tag(new streaming::RawTag(
      0, streaming::kDefaultFlavourMask));
  if ( stream_offset_ == 0 ) {
    serializer_.Initialize(raw_tag->mutable_data());
  }
  serializer_.SerializeFlvTag(flv_tag.get(), 0, raw_tag->mutable_data());

  callback_->Run(raw_tag.get());
  stream_offset_ += raw_tag->data()->Size();
  return true;
}

} // namespace rtmp
