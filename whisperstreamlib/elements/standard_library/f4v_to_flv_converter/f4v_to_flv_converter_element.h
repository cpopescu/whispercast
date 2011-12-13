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

#ifndef __MEDIA_ELEMENTS_F4V_TO_FLV_CONVERTER_ELEMENT_H__
#define __MEDIA_ELEMENTS_F4V_TO_FLV_CONVERTER_ELEMENT_H__

#include <string>
#include <whisperstreamlib/base/filtering_element.h>
#include <whisperstreamlib/flv/flv_tag.h>
#include <whisperstreamlib/f4v/f4v_tag.h>
#include <whisperstreamlib/f4v/f4v_to_flv.h>

namespace streaming {
///////////////////////////////////////////////////////////////////////
//
// F4vToFlvConverterElement
//
// An element that converts incoming f4v tags into flv tags.
// It also injects flv cue_points from time to time.
//
class F4vToFlvConverterElement : public FilteringElement {
 public:
  class ClientCallbackData : public FilteringCallbackData {
   public:
    ClientCallbackData(const string& creation_path);
    virtual ~ClientCallbackData();

    const string& creation_path() const;

    virtual void FilterTag(const Tag* tag, int64 timestamp_ms, TagList* out);
   private:
    static scoped_ref<FlvTag> PrepareCuePoint(FlvTag* cue_point,
        uint32 flavour_mask, int64 timestamp_ms);

    // the stream path
    const string creation_path_;

    // the actual converter
    F4vToFlvConverter converter_;

    // current cue point number
    uint32 cue_point_number_;
  };
 public:
  static const char kElementClassName[];
  F4vToFlvConverterElement(const char* name,
                           const char* id,
                           ElementMapper* mapper,
                           net::Selector* selector);
  virtual ~F4vToFlvConverterElement();

 protected:
  virtual bool Initialize();
  virtual bool HasMedia(const char* media_name, Capabilities* out);
  virtual void ListMedia(const char* media_dir,
                         ElementDescriptions* media);
  virtual bool AddRequest(const char* media,
                          streaming::Request* req,
                          streaming::ProcessingCallback* callback);
  virtual FilteringCallbackData* CreateCallbackData(const char* media_name,
                                                    streaming::Request* req);
  virtual void DeleteCallbackData(FilteringCallbackData* data);

  // Caps we serve (we do flv..);
  const streaming::Capabilities default_caps_;
 private:
  DISALLOW_EVIL_CONSTRUCTORS(F4vToFlvConverterElement);
};
} // namespace streaming

#endif  // __MEDIA_ELEMENTS_F4V_TO_FLV_CONVERTER_ELEMENT_H__
