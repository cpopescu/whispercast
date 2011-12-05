
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

#ifndef __MEDIA_RTP_RTSP_ELEMENT_MAPPER_MEDIA_INTERFACE_H__
#define __MEDIA_RTP_RTSP_ELEMENT_MAPPER_MEDIA_INTERFACE_H__

#include <whisperlib/common/base/types.h>
#include <whisperstreamlib/base/element_mapper.h>
#include <whisperstreamlib/rtp/rtsp/rtsp_media_interface.h>

namespace streaming {
namespace rtsp {

// This interface connects Broadcasters to whispercast elements through
// MediaMapper.
class ElementMapperMediaInterface : public MediaInterface {
private:
  struct BData {
    enum State {
      PLAY,
      PAUSE,
    };
    static const char* StateName(State state) {
      switch(state) {
        CONSIDER(PLAY);
        CONSIDER(PAUSE);
      }
    }
    streaming::Request* req_;
    ProcessingCallback* callback_;
    State state_;
    bool first_play_;

    BData() : req_(NULL), callback_(NULL), state_(PAUSE),
              first_play_(true) {}
    BData(streaming::Request* req, ProcessingCallback* callback, State state)
      : req_(req), callback_(callback), state_(state),
        first_play_(true) {}
    const char* state_name() const {
      return StateName(state_);
    }
  };

public:
  ElementMapperMediaInterface(ElementMapper* element_mapper);
  virtual ~ElementMapperMediaInterface();

private:
  void ProcessTag(rtp::Broadcaster* broadcaster, BData* bdata,
      const Tag* tag);
  void RestartFlow();

  void AddBroadcaster(rtp::Broadcaster* broadcaster, BData* bdata);
  const BData* GetBroadcaster(rtp::Broadcaster* broadcaster) const;
  BData* GetBroadcaster(rtp::Broadcaster* broadcaster);
  void DelBroadcaster(rtp::Broadcaster* broadcaster);

public:
  //////////////////////////////////////////////////////////////////////
  // MediaInterface methods
  virtual bool Describe(const string& media, MediaInfoCallback* callback);
  virtual bool Attach(rtp::Broadcaster* broadcaster, const string& media);
  virtual void Detach(rtp::Broadcaster* broadcaster);
  virtual void Play(rtp::Broadcaster* broadcaster, bool play);

private:
  ElementMapper* element_mapper_;
  typedef map<rtp::Broadcaster*, BData*> BroadcasterMap;
  BroadcasterMap broadcasters_;
};

} // namespace rtsp
} // namespace streaming

#endif // __MEDIA_RTP_RTSP_ELEMENT_MAPPER_MEDIA_INTERFACE_H__
