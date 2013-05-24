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

#include <whisperlib/common/base/log.h>
#include <whisperstreamlib/mp3/mp3_tag_splitter.h>
#include <whisperstreamlib/base/media_info_util.h>
namespace streaming {

streaming::TagReadStatus Mp3TagSplitter::GetNextTagInternal(
    io::MemoryStream* in,
    scoped_ref<Tag>* tag,
    int64* timestamp_ms,
    bool is_at_eos) {
  if ( crt_tag_.get() == NULL ) {
    crt_tag_ = new Mp3FrameTag(
        Tag::ATTR_AUDIO | Tag::ATTR_CAN_RESYNC | Tag::ATTR_DROPPABLE,
        kDefaultFlavourMask, 0);
  }
  streaming::TagReadStatus err = crt_tag_->Read(in, do_synchronize_);
  if ( err != streaming::READ_OK ) {
    return err;
  }
  if ( !resynchronize_always_ ) {
    do_synchronize_ = false;
  }

  // Update & send media_info
  if ( !media_info_extracted_ ) {
    util::ExtractMediaInfoFromMp3(*crt_tag_.get(), &media_info_);
    next_tag_to_send_ = crt_tag_.get();
    next_tag_to_send_ts_ = crt_tag_->timestamp_ms();
    crt_tag_ = NULL;
    *tag = new MediaInfoTag(0, kDefaultFlavourMask, media_info_);
    *timestamp_ms = 0;
    media_info_extracted_ = true;
    return READ_OK;
  }

  crt_tag_->set_timestamp_ms(stream_offset_ms_);
  stream_offset_ms_ += crt_tag_->duration_ms();

  *tag = crt_tag_.get();
  *timestamp_ms = crt_tag_->timestamp_ms();

  crt_tag_ = NULL;
  return streaming::READ_OK;
}
}
