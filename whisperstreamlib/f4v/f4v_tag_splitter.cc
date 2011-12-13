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

#include <whisperlib/common/base/log.h>
#include <whisperlib/common/io/buffer/io_memory_stream.h>
#include <whisperstreamlib/base/media_info_util.h>
#include <whisperstreamlib/f4v/f4v_tag_splitter.h>
#include <whisperstreamlib/f4v/f4v_tag.h>
#include <whisperstreamlib/f4v/atoms/movie/moov_atom.h>

namespace streaming {

const TagSplitter::Type F4vTagSplitter::kType = TagSplitter::TS_F4V;

F4vTagSplitter::F4vTagSplitter(const string& name)
    : streaming::TagSplitter(kType, name),
      f4v_decoder_(),
      generate_cue_point_table_now_(false) {
}
F4vTagSplitter::~F4vTagSplitter() {
}
/* @@@
void F4vTagSplitter::set_limits(int64 media_origin_pos_ms,
                                int64 seek_pos_ms,
                                int64 limit_ms) {
  F4V_LOG_INFO << "set_limits media_origin_pos_ms: " << media_origin_pos_ms
               << ", seek_pos_ms: " << seek_pos_ms
               << ", limit_ms: " << limit_ms
               << ", going to SeekToTime " << seek_pos_ms;
  streaming::TagSplitter::set_limits(media_origin_pos_ms, seek_pos_ms, limit_ms);
  uint32 seek_frame = 0;
  int64 seek_file_offset = 0;
  f4v_decoder_.SeekToTime(seek_pos_ms, true, seek_frame, seek_file_offset);
}
*/
streaming::TagReadStatus F4vTagSplitter::GetNextTagInternal(
    io::MemoryStream* in,
    scoped_ref<Tag>* tag,
    int64* timestamp_ms,
    bool is_at_eos) {
  *tag = NULL;

  if ( generate_cue_point_table_now_ ) {
    CuePointTag* cue_point_tag = f4v_decoder_.GenerateCuePointTableTag();
    CHECK_NOT_NULL(cue_point_tag);
    cue_point_tag->set_attributes(streaming::Tag::ATTR_METADATA);
    *tag = cue_point_tag;
    generate_cue_point_table_now_ = false;
    return streaming::READ_OK;
  }

  scoped_ref<f4v::Tag> f4v_tag;
  f4v::TagDecodeStatus status = f4v_decoder_.ReadTag(*in, &f4v_tag);

  if ( status != f4v::TAG_DECODE_SUCCESS ) {
    CHECK_NULL(f4v_tag.get());
    switch (status) {
      case f4v::TAG_DECODE_SUCCESS:
        break;
      case f4v::TAG_DECODE_NO_DATA:
        return READ_NO_DATA;
      case f4v::TAG_DECODE_ERROR:
        return READ_CORRUPTED_FAIL;
    }
    LOG_FATAL << "unknown f4v::TagDecodeStatus: " << status;
    return READ_CORRUPTED_FAIL;
  }
  CHECK_NOT_NULL(f4v_tag.get());

  // Update media_info
  if ( f4v_tag->is_atom() && f4v_tag->atom()->type() == f4v::ATOM_MOOV ) {
    util::ExtractMediaInfoFromMoov(static_cast<const f4v::MoovAtom&>(
        *f4v_tag->atom()), &media_info_);
  }

  if ( f4v_tag->is_frame() &&
       f4v_tag->frame()->header().timestamp() == 0 &&
       f4v_tag->frame()->header().type() ==
         streaming::f4v::FrameHeader::VIDEO_FRAME ) {
    // The next call to ProcessTagInternal will generate the CuePointTag
    generate_cue_point_table_now_ = true;
  }

  if ( f4v_tag->is_atom() ) {
    f4v_tag->add_attributes(streaming::Tag::ATTR_METADATA);
  } else {
    const f4v::FrameHeader& f4v_frame_header = f4v_tag->frame()->header();
    const f4v::FrameHeader::Type f4v_type = f4v_frame_header.type();
    if ( f4v_type == f4v::FrameHeader::AUDIO_FRAME ) {
      f4v_tag->add_attributes(streaming::Tag::ATTR_AUDIO);
    }
    if ( f4v_type == f4v::FrameHeader::VIDEO_FRAME ) {
      f4v_tag->add_attributes(streaming::Tag::ATTR_VIDEO);
    }
    if ( f4v_frame_header.is_keyframe() ||
         f4v_frame_header.type() == f4v::FrameHeader::AUDIO_FRAME ) {
      f4v_tag->add_attributes(streaming::Tag::ATTR_CAN_RESYNC);
    }
  }

  *timestamp_ms = f4v_tag->timestamp_ms();
  *tag = f4v_tag.get();
  return streaming::READ_OK;
}
}
