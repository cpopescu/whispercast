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

#ifndef __MEDIA_BASE_MEDIA_DROPPING_SOURCE_H__
#define __MEDIA_BASE_MEDIA_DROPPING_SOURCE_H__

#include <string>
#include <whisperstreamlib/base/filtering_element.h>

namespace streaming {

///////////////////////////////////////////////////////////////////////
//
// DroppingElement
//
// A element that drops as you wish :)
//
// We have periods of accepting tags and of dropping tags for audio and
// video tags. Once we decide to start accepting, we accept begining w/
// can resync tags..
// The header and control tags are only yes / no;
//
// You can register callback transparently through us. The problem
// is that once registered, you cannot register on another name until
// all become unregistered.
//
class DroppingElement : public FilteringElement {
 public:
  // Constructs a SwitchingElement - we don NOT own empty callback !
  DroppingElement(const string& name,
                  ElementMapper* mapper,
                  net::Selector* selector,
                  const string& media_filtered,
                  int64 audio_accept_period_ms,
                  int64 audio_drop_period_ms,
                  int64 video_accept_period_ms,
                  int64 video_drop_period_ms,
                  int32 video_grace_period_key_frames);
  virtual ~DroppingElement();

  // We register the ProcessTag with the element_ on path: media_name_ .
  // Returns success status.
  // This function should be called right after constructor.
  virtual bool Initialize();

  static const char kElementClassName[];
 protected:
  void PlayBootstrap(streaming::ProcessingCallback* callback,
                     int64 timestamp_ms,
                     uint32 flavour_mask);
  void ClearBootstrap();

  // A dead-end callback, looks like a client callback for the element_.
  // We receive & process tags here, in order to update our bootstrap video tag.
  void ProcessTag(const Tag* tag, int64 timestamp_ms);

  //////////////////////////////////////////////////////////////////

  // FilteringElement methods

  virtual FilteringCallbackData* CreateCallbackData(const string& media_name,
                                                    Request* req);

  //////////////////////////////////////////////////////////////////

  // Element methods
  virtual bool AddRequest(const string& media, Request* req,
                          ProcessingCallback* callback);

 private:
  const string media_filtered_;
  const int64 audio_accept_period_ms_;
  const int64 audio_drop_period_ms_;
  const int64 video_accept_period_ms_;
  const int64 video_drop_period_ms_;
  const int32 video_grace_period_key_frames_;

  // permanent callback to ProcessTag
  streaming::Request* internal_req_;
  streaming::ProcessingCallback* process_tag_callback_;

  // quickly provides the first keyframe to a new client
  scoped_ref<const Tag> video_bootstrap_[kNumFlavours];

  DISALLOW_EVIL_CONSTRUCTORS(DroppingElement);
};
}

#endif  // __MEDIA_BASE_MEDIA_DROPPING_SOURCE_H__
