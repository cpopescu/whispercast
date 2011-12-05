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

// @@@ NEEDS REFACTORING - CLOSE/TAG DISTRIBUTION

#ifndef __MEDIA_ELEMENTS_STANDARD_LIBRARY_SPLITTING_SPLITTING_H__
#define __MEDIA_ELEMENTS_STANDARD_LIBRARY_SPLITTING_SPLITTING_H__

#include <whisperstreamlib/base/filtering_element.h>
#include <whisperstreamlib/base/tag_splitter.h>

namespace streaming {

// A splitter that takes raw data and splits it into tags using an
// incorporated splitter.

class SplittingElement : public FilteringElement {
 public:
  SplittingElement(const char* name,
                   const char* id,
                   ElementMapper* mapper,
                   net::Selector* selector,
                   SplitterCreator* splitter_creator,
                   Tag::Type splitter_tag_type,
                   int32 max_tag_size);
  virtual ~SplittingElement();

  virtual bool Initialize() {
    return true;
  }
  static const char kElementClassName[];

 protected:
  //////////////////////////////////////////////////////////////////

  // FilteringElement methods

  virtual FilteringCallbackData* CreateCallbackData(
      const char* media_name, Request* req);

 private:
  void ProcessTag(const Tag* tag);

  SplitterCreator* splitter_creator_;
  const Capabilities caps_;
  const int32 max_tag_size_;

  DISALLOW_EVIL_CONSTRUCTORS(SplittingElement);
};
}

#endif   // __MEDIA_ELEMENTS_STANDARD_LIBRARY_SPLITTING_SPLITTING_H__
