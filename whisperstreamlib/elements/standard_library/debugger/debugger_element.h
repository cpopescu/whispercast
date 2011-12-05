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

#ifndef __MEDIA_BASE_MEDIA_DEBUGGER_ELEMENT_H__
#define __MEDIA_BASE_MEDIA_DEBUGGER_ELEMENT_H__

#include <string>
#include <whisperstreamlib/base/filtering_element.h>


namespace streaming {

class DebuggerElementCallbackData : public FilteringCallbackData {
 public:
  DebuggerElementCallbackData()
      : FilteringCallbackData() {
  }
  virtual void FilterTag(const Tag* tag, TagList* out) {
    LOG_INFO << media_name() << tag->ToString();
    out->push_back(tag);
  }
};


// This guy just prints whatever passes through him
class DebuggerElement : public FilteringElement {
 public:
  DebuggerElement(const char* name,
                  const char* id,
                  ElementMapper* mapper,
                  net::Selector* selector)
      : FilteringElement("debugger", name, id,
                         mapper, selector) {
  }
  virtual bool Initialize() {
    return true;
  }
  virtual FilteringCallbackData* CreateCallbackData(
      const char* media_name,
      Request* req) {
    return new DebuggerElementCallbackData();
  }
  static const char kElementClassName[];

};

}

#endif