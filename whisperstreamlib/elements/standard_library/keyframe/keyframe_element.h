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

#ifndef __MEDIA_BASE_MEDIA_KEYFRAME_SOURCE_H__
#define __MEDIA_BASE_MEDIA_KEYFRAME_SOURCE_H__

#include <string>
#include <whisperstreamlib/base/filtering_element.h>

namespace streaming {
///////////////////////////////////////////////////////////////////////
//
// KeyFrameExtractorElement
//
// A element that forwards some video keyframes, and drops all video
// interframes.
// You can configure the interval between what keyframes to forward.
// The header, control and audio tags are only yes / no;
//
// You can register callback transparently through us. The problem
// is that once registered, you cannot register on another name until
// all become unregistered.
//
// You should register all callbacks to a single media name,
// so that a single media stream passes through here.
// If your register to multiple streams, the boostrap gets confused.
//
class KeyFrameExtractorElement : public FilteringElement {
 public:
  KeyFrameExtractorElement(const char* name,
                           const char* id,
                           ElementMapper* mapper,
                           net::Selector* selector,
                           const char* media_filtered,
                           int64 ms_between_video_frames,
                           bool drop_audio);
  virtual ~KeyFrameExtractorElement();

  virtual bool Initialize();

  static const char kElementClassName[];
 protected:
  void SetBootstrap(const streaming::Tag* tag);
  void PlayBootstrap(streaming::ProcessingCallback* callback,
                     uint32 flavour_mask);
  void ClearBootstrap();

  // A dead-end callback, looks like a client callback for the element_.
  // We receive & process tags here, in order to update our bootstrap video tag.
  void ProcessTag(const Tag* tag);

  //////////////////////////////////////////////////////////////////

  // FilteringElement methods
  virtual FilteringCallbackData* CreateCallbackData(const char* media_name,
                                                    Request* req);

  //////////////////////////////////////////////////////////////////

  // Element methods
  virtual bool AddRequest(const char* media,
                          streaming::Request* req,
                          streaming::ProcessingCallback* callback);

 private:
  const string media_filtered_;
  const int64 ms_between_video_frames_;
  const bool drop_audio_;

  // permanent callback to ProcessTag
  streaming::Request* internal_req_;
  streaming::ProcessingCallback* process_tag_callback_;

  // The last keyframe tag that was extracted.
  // Quickly provides the first keyframe to a new client.
  scoped_ref<const Tag> last_extracted_keyframe_[kNumFlavours];

  DISALLOW_EVIL_CONSTRUCTORS(KeyFrameExtractorElement);
};
}

#endif  // __MEDIA_BASE_MEDIA_KEYFRAME_SOURCE_H__
