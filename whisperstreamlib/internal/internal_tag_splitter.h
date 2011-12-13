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
// Author: Mihai Ianculescu

#ifndef __MEDIA_INTERNAL_INTERNAL_TAG_SPLITTER_H__
#define __MEDIA_INTERNAL_INTERNAL_TAG_SPLITTER_H__

#include <string>
#include <whisperlib/common/base/types.h>
#include <whisperstreamlib/base/tag.h>
#include <whisperstreamlib/base/tag_splitter.h>
#include <whisperstreamlib/internal/internal_frame.h>

// IMPORTANT : this is not thread safe !!

namespace streaming {

class InternalTagSplitter : public streaming::TagSplitter {
 public:
  static const Type kType;
 public:
  InternalTagSplitter(const string& name)
      : streaming::TagSplitter(kType, name),
        stream_offset_(0.0),
        crt_tag_(NULL) {
  }
  virtual ~InternalTagSplitter() {
    delete crt_tag_;
  }

 protected:
  virtual streaming::TagReadStatus GetNextTagInternal(
      io::MemoryStream* in,
      scoped_ref<Tag>* tag,
      int64* timestamp_ms,
      bool is_at_eos);

 private:
  string mime_type_;

  double stream_offset_;
  InternalFrameTag* crt_tag_;

  DISALLOW_EVIL_CONSTRUCTORS(InternalTagSplitter);
};
}

#endif  // __MEDIA_INTERNAL_INTERNAL_TAG_SPLITTER_H__
