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

#ifndef __NET_RTMP_RTMP_MANAGER_H__
#define __NET_RTMP_RTMP_MANAGER_H__

#include <whisperstreamlib/base/element_mapper.h>

#include <whisperstreamlib/rtmp/rtmp_stream.h>

namespace rtmp {

class StreamPublisher {
 public:
  StreamPublisher(const string& stream_prefix)
      : stream_prefix_(stream_prefix) {
  }
  virtual ~StreamPublisher() {
  }
  virtual streaming::ProcessingCallback* StartPublisher(string stream_name,
      const StreamParams* params) = 0;
  virtual void CanStopPublishing(string stream_name,
      const StreamParams* params, Callback1<bool>* completion_callback) = 0;
  virtual void CanStartPublishing(string stream_name,
      const StreamParams* params, Callback1<bool>* completion_callback) = 0;
  const string& stream_prefix() const {
    return stream_prefix_;
  }
 private:
  const string stream_prefix_;
  DISALLOW_EVIL_CONSTRUCTORS(StreamPublisher);
};

class StreamManager {
 public:
  StreamManager() {
  }
  virtual ~StreamManager() {
  }

  virtual Stream* CreateStream(
      const StreamParams& params,  // only some parameters may be set..
      Protocol* const protocol,
      bool publish) = 0;
  // Registers a publisher for a path. We don't own the publisher pointer,
  // but we hold it, and should be available until after the destruction of
  // the manager of unregistration.
  virtual bool RegisterPublishingStream(StreamPublisher* publisher) = 0;
  virtual bool UnregisterPublishingStream(StreamPublisher* publisher) = 0;

  virtual streaming::ProcessingCallback* StartedPublisher(Stream* stream) = 0;
  virtual void StoppedPublisher(Stream* stream) = 0;
  virtual void DeletePublisher(string stream_name,
      const StreamParams* params, bool force,
      Callback1<bool>* completion_callback) = 0;
  virtual void CanPublish(string stream_name,
      const StreamParams* params, Callback1<bool>* completion_callback) = 0;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(StreamManager);
};

class StandardStreamManager : public StreamManager {
 public:
  StandardStreamManager(net::Selector* selector,
                        streaming::ElementMapper* element_mapper,
                        streaming::StatsCollector* stats_collector,
                        const string& publishing_application);
  virtual ~StandardStreamManager();
  // So we can create element mapper later..
  void set_element_mapper(streaming::ElementMapper* element_mapper) {
    element_mapper_ = element_mapper;
  }
  virtual Stream* CreateStream(
      const StreamParams& params,
      Protocol* const protocol,
      bool publishing);
  // Functions to be called by the framework to register / unregister
  // streams that are published (accept input streams);
  virtual bool RegisterPublishingStream(StreamPublisher* publisher);
  virtual bool UnregisterPublishingStream(StreamPublisher* publisher);

  // Internal functions to be called by the stream (for notification when the
  // stream starts / stops publishing
  virtual streaming::ProcessingCallback* StartedPublisher(Stream* stream);
  virtual void StoppedPublisher(Stream* stream);

  // To be called by anybody to stop a current publishing stream.
  virtual void DeletePublisher(string stream_name,
      const StreamParams* params, bool force,
      Callback1<bool>* completion_callback);

  // To be called by anybody to figure out if publishing is possible now
  // on that stream / application
  virtual void CanPublish(string stream_name,
      const StreamParams* params, Callback1<bool>* completion_callback);

 private:
  void DeletePublisherChecked(string stream_name, const StreamParams* params,
                               Callback1<bool>* completion_callback,
                               bool success);

  net::Selector* const selector_;
  streaming::ElementMapper* element_mapper_;
  streaming::StatsCollector* stats_collector_;
  const string publishing_application_;

  typedef map<string, StreamPublisher*> RegStructMap;
  RegStructMap registered_streams_;
  typedef map<string, Stream*> StreamMap;
  StreamMap publishing_streams_;

  DISALLOW_EVIL_CONSTRUCTORS(StandardStreamManager);
};

}

#endif  // __NET_RTMP_RTMP_MANAGER_H__
