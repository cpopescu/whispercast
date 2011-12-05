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

#ifndef __MEDIA_MP3_MP3_TAG_SPLITTER_H__
#define __MEDIA_MP3_MP3_TAG_SPLITTER_H__

#include <whisperlib/common/base/types.h>
#include <whisperstreamlib/base/tag.h>
#include <whisperstreamlib/base/tag_splitter.h>
#include <whisperstreamlib/mp3/mp3_frame.h>

// IMPORTANT : this is not thread safe !!

namespace streaming {
class Mp3TagSplitter : public streaming::TagSplitter {
 public:
  static const Type kType;
 public:
  Mp3TagSplitter(const string& name, bool resynchronize_always = true)
      : streaming::TagSplitter(kType, name),
        stream_offset_ms_(0),
        crt_tag_(NULL),
        do_synchronize_(true),
        resynchronize_always_(resynchronize_always) {
  }
  virtual ~Mp3TagSplitter() {
    delete crt_tag_;
  }

 protected:
  streaming::TagReadStatus MaybeFinalizeFrame(
    streaming::Tag** tag, const char* buffer);

  virtual streaming::TagReadStatus GetNextTagInternal(
      io::MemoryStream* in, scoped_ref<Tag>* tag, bool is_at_eos);

 private:
  int64 stream_offset_ms_;
  Mp3FrameTag* crt_tag_;
  bool do_synchronize_;
  const bool resynchronize_always_;

  DISALLOW_EVIL_CONSTRUCTORS(Mp3TagSplitter);
};
}

#endif  // __MEDIA_MP3_MP3_TAG_SPLITTER_H__
