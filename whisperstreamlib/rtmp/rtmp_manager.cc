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
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Author: Catalin Popescu

#include <whisperstreamlib/rtmp/rtmp_manager.h>

#include <whisperlib/common/io/ioutil.h>

#include <whisperstreamlib/rtmp/rtmp_play_stream.h>
#include <whisperstreamlib/rtmp/rtmp_publish_stream.h>

namespace rtmp {

StandardStreamManager::StandardStreamManager(
    net::Selector* selector,
    streaming::ElementMapper* element_mapper,
    streaming::StatsCollector* stats_collector,
    const string& publishing_application)
    : StreamManager(),
      selector_(selector),
      element_mapper_(element_mapper),
      stats_collector_(stats_collector),
      publishing_application_(publishing_application) {
}
StandardStreamManager::~StandardStreamManager() {
  for ( StreamMap::const_iterator it = publishing_streams_.begin();
        it != publishing_streams_.end(); ++it ) {
    it->second->DecRef();
  }
  publishing_streams_.clear();
  for ( RegStructMap::const_iterator it = registered_streams_.begin();
        it != registered_streams_.end(); ++it ) {
// TODO: WTF !?
//    delete it->second;
  }
  registered_streams_.clear();
}

Stream* StandardStreamManager::CreateStream(
    const StreamParams& params,
    Protocol* const protocol,
    bool publishing) {
  if ( publishing ) {
    return new PublishStream(this,
                        params,
                        protocol);
  }
  return new PlayStream(element_mapper_,
                       stats_collector_,
                       params,
                       protocol);
}
bool StandardStreamManager::RegisterPublishingStream(
    StreamPublisher* publisher) {
  if ( registered_streams_.find(publisher->stream_prefix()) !=
       registered_streams_.end() ) {
    return false;
  }
  LOG_INFO << " RTMP Manager registering publisher for: "
           << publisher->stream_prefix();
  registered_streams_[publisher->stream_prefix()] = publisher;
  return true;
}
bool StandardStreamManager::UnregisterPublishingStream(
    StreamPublisher* publisher) {
  RegStructMap::iterator it = registered_streams_.find(
      publisher->stream_prefix());
  if ( it == registered_streams_.end() ) {
    return false;
  }
  LOG_INFO << " RTMP Manager unregistring publisher for: "
           << publisher->stream_prefix();
  StreamMap::iterator begin, end;
  strutil::GetBounds(publisher->stream_prefix(), &publishing_streams_,
                     &begin, &end);
  for ( StreamMap::iterator it_stream = begin;
        it_stream != end; ) {
    Stream* crt = it_stream->second;
    ++it_stream;
    crt->Close();
    // Causes: crt->DecRef();
    //         StoppedPublisher(crt);
    //         publishing_streams_.erase(it);
  }
  return true;
}
streaming::ProcessingCallback*
StandardStreamManager::StartedPublisher(Stream* stream) {
  string path = stream->stream_name();
  StreamPublisher* const publisher =
      io::FindPathBased(&registered_streams_, path);
  if ( publisher == NULL ) {
    LOG_ERROR << " RTMP Manager cannot find any handler for: " << path;
    return NULL;
  }
  StreamMap::iterator it = publishing_streams_.find(
      stream->stream_name());
  if ( it != publishing_streams_.end() ) {
    Stream* old_stream = it->second;
    CHECK(stream != old_stream);
    old_stream->Close();
    // Causes: it->second->DecRef();
    //         publishing_streams_.erase(it);

    CHECK(publishing_streams_.find(stream->stream_name()) ==
          publishing_streams_.end());
  }
  streaming::ProcessingCallback* callback =
      publisher->StartPublisher(stream->stream_name(), &stream->params());
  if ( callback != NULL ) {
    stream->IncRef();
    publishing_streams_.insert(
        make_pair(stream->stream_name(), stream));
  }
  return callback;
}
void StandardStreamManager::StoppedPublisher(Stream* stream) {
  StreamMap::iterator it =
      publishing_streams_.find(stream->stream_name());
  if ( it != publishing_streams_.end() && it->second == stream ) {
    stream->DecRef();
    publishing_streams_.erase(it);
  }
}

//////////////////////////////////////////////////////////////////////

void StandardStreamManager::DeletePublisher(string stream_name,
    const StreamParams* params, bool force,
    Callback1<bool>* completion_callback) {
  if ( !force && publishing_application_ != params->application_name_ ) {
    completion_callback->Run(false);
  } else {
    StreamMap::iterator it = publishing_streams_.find(stream_name);
    if ( it == publishing_streams_.end() ) {
      completion_callback->Run(true);
    } else {
      if ( !force ) {
        string path(stream_name);
        StreamPublisher* const publisher =
            io::FindPathBased(&registered_streams_, path);
        if ( publisher != NULL) {
          publisher->CanStopPublishing(
              stream_name, params, NewCallback(
                  this, &StandardStreamManager::DeletePublisherChecked,
                  stream_name, params, completion_callback));
          return;
        }
      }
      it->second->Close();
      // Causes: it->second->DecRef();
      //         publishing_streams_.erase(it);
      CHECK(publishing_streams_.find(stream_name) == publishing_streams_.end());

      completion_callback->Run(true);
    }
  }
}

void StandardStreamManager::DeletePublisherChecked(
    string stream_name,
    const StreamParams* params,
    Callback1<bool>* completion_callback,
    bool success) {
  if ( success ) {
    StreamMap::iterator it = publishing_streams_.find(stream_name);
    if ( it != publishing_streams_.end() ) {
      it->second->Close();
      // Causes: it->second->DecRef();
      //         publishing_streams_.erase(it);
      CHECK(publishing_streams_.find(stream_name) == publishing_streams_.end());
    }
  }
  completion_callback->Run(success);
}

//////////////////////////////////////////////////////////////////////

void StandardStreamManager::CanPublish(string stream_name,
    const StreamParams* params,
    Callback1<bool>* completion_callback) {
    string path(stream_name);
    StreamPublisher* const publisher =
        io::FindPathBased(&registered_streams_, path);
    if ( publisher == NULL ) {
      completion_callback->Run(true);
    } else {
      publisher->CanStartPublishing(stream_name, params, completion_callback);
    }
}

}
