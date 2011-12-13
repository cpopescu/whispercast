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

#include <whisperlib/common/base/log.h>
#include "internal/internal_tag_splitter.h"

namespace streaming {

const TagSplitter::Type InternalTagSplitter::kType = TagSplitter::TS_INTERNAL;

streaming::TagReadStatus InternalTagSplitter::GetNextTagInternal(
    io::MemoryStream* in,
    scoped_ref<Tag>* tag,
    int64* timestamp_ms,
    bool is_at_eos) {
  *tag = NULL;
  if ( crt_tag_ == NULL ) {
    crt_tag_ = new InternalFrameTag(0, kDefaultFlavourMask, mime_type_, "");
  }
  streaming::TagReadStatus err = crt_tag_->Read(in);
  if ( err != streaming::READ_OK ) {
    return err;
  }
  mime_type_ = crt_tag_->mime_type();

  // IMPORTANT  - we do not handle here seeks / limits

  crt_tag_->set_attributes(
      (((crt_tag_->header().flags_ & streaming::AUDIO) != 0) ?
       streaming::Tag::ATTR_AUDIO : 0) |
      (((crt_tag_->header().flags_ & streaming::VIDEO) != 0) ?
       streaming::Tag::ATTR_VIDEO : 0) |
      (((crt_tag_->header().flags_ & streaming::DELTA) == 0) ?
       streaming::Tag::ATTR_CAN_RESYNC : 0) |
      (((crt_tag_->header().flags_ & streaming::HEADER) != 0) ?
       streaming::Tag::ATTR_DROPPABLE : 0));

  *timestamp_ms = crt_tag_->timestamp_ms();
  *tag = crt_tag_;

  crt_tag_ = NULL;
  return streaming::READ_OK;
}
}
