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

#ifndef __MEDIA_BASE_MEDIA_STREAM_RENAMER_SOURCE_H__
#define __MEDIA_BASE_MEDIA_STREAM_RENAMER_SOURCE_H__

#include <string>
#include <whisperlib/common/base/re.h>
#include <whisperstreamlib/base/filtering_element.h>

namespace streaming {
///////////////////////////////////////////////////////////////////////
//
// StreamRenamerElement
//
// An element that modifies the TYPE_SOURCE_STARTED / TYPE_SOURCE_ENDED
// tags and forwards every other tag.
//

class StreamRenamerElement : public FilteringElement {
 public:
   StreamRenamerElement(const char* name,
                        const char* id,
                        ElementMapper* mapper,
                        net::Selector* selector,
                        const string& pattern,
                        const string& replace);
  virtual ~StreamRenamerElement();

  virtual bool Initialize();

  static const char kElementClassName[];

 protected:
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
  const string pattern_;
  const string replace_;
  re::RE re_; // regular expression to match "pattern_"

  DISALLOW_EVIL_CONSTRUCTORS(StreamRenamerElement);
};
}

#endif  // __MEDIA_BASE_MEDIA_STREAM_RENAMER_SOURCE_H__
