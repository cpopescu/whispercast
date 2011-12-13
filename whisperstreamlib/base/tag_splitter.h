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

#ifndef __MEDIA_SOURCE_TAG_SPLITTER_H__
#define __MEDIA_SOURCE_TAG_SPLITTER_H__

#include <whisperstreamlib/base/tag.h>
#include <whisperstreamlib/base/consts.h>
#include <whisperstreamlib/base/media_info.h>

namespace streaming {

//////////////////////////////////////////////////////////////////////
//
// These are stats for a tag splitter ..
//
struct  ElementSplitterStats {
  int64 num_decoded_tags_;
  int64 total_tag_size_;
  int64 max_tag_size_;

  int64 num_video_tags_;
  int64 num_video_bytes_;

  int64 num_audio_tags_;
  int64 num_audio_bytes_;

  ElementSplitterStats()
      : num_decoded_tags_(0),
        total_tag_size_(0),
        max_tag_size_(0),
        num_video_tags_(0),
        num_video_bytes_(0),
        num_audio_tags_(0),
        num_audio_bytes_(0) {
  }
  int64 num_total_tags() const {
    return num_audio_tags_ + num_video_tags_;
  }
  void Clear() {
    num_decoded_tags_ = 0;
    total_tag_size_ = 0;
    max_tag_size_ = 0;
    num_video_tags_ = 0;
    num_video_bytes_ = 0;
    num_audio_tags_ = 0;
    num_audio_bytes_ = 0;
  }
  string ToString() const {
    return strutil::StringPrintf(
        "total_tags =%"PRId64" total_bytes =%"PRId64" maximum_tag_size=%"PRId64"\n"
        "video_tags =%"PRId64" video_bytes =%"PRId64" \n"
        "audio_tags =%"PRId64" audio_bytes =%"PRId64"",
        num_decoded_tags_,
        total_tag_size_,
        max_tag_size_,
        num_video_tags_,
        num_video_bytes_,
        num_audio_tags_,
        num_audio_bytes_);
  }
};

//////////////////////////////////////////////////////////////////////
//
// Given a stream of data this splits it in tags - and upon splitting
// it calls a set of registered callbacks
//
class TagSplitter {
 public:
  enum Type {
    TS_F4V,
    TS_FLV,
    TS_RAW,
    TS_MP3,
    TS_AAC,
    TS_INTERNAL,
  };
  static const char* TypeName(Type type);
 public:
  TagSplitter(Type type, const string& name)
      : type_(type),
        name_(name),
        next_tag_to_send_(),
        max_composition_tag_time_ms_(0),
        crt_composed_tag_() {
  }
  virtual ~TagSplitter() {
  }

  Type type() const {
    return type_;
  }
  const char* type_name() const {
    return TypeName(type());
  }

  int32 max_composition_tag_time_ms() const {
    return  max_composition_tag_time_ms_;
  }
  void set_max_composition_tag_time_ms(int32 max_composition_tag_time_ms) {
    max_composition_tag_time_ms_ = max_composition_tag_time_ms;
  }

  const string& name() const {
    return name_;
  }
  const ElementSplitterStats& stats() const {
    return stats_;
  }

  const MediaInfo& media_info() const {
    return media_info_;
  }

  // is_at_eos: set to true if there is no more data to be appended to 'in'.
  //            When true, instead of READ_NO_DATA you get READ_EOF.
  // Read from "in" and return the next tag into 'tag'.
  TagReadStatus GetNextTag(io::MemoryStream* in,
                           scoped_ref<Tag>* tag,
                           int64* timestamp_ms,
                           bool is_at_eos);

 protected:
  // You need to override this in order to extract one tag from 'in' into 'tag'.
  virtual TagReadStatus GetNextTagInternal(io::MemoryStream* in,
                                           scoped_ref<Tag>* tag,
                                           int64* timestamp_ms,
                                           bool is_at_eos) = 0;
 private:
  // Returns true if the given tag can be introduced in a composed tag
  static bool IsComposable(const Tag* tag) {
    return tag->is_audio_tag() || (tag->is_video_tag() && !tag->can_resync());
  }

 protected:
  // splitter type
  const Type type_;
  // the name of this element
  const string name_;

  // May be we have a next tag to send, from a previous GetNextTag
  scoped_ref<Tag> next_tag_to_send_;
  // If this is over 0 we compose tags..
  int32 max_composition_tag_time_ms_;
  // If we compose this is the currently composing tag
  scoped_ref<ComposedTag> crt_composed_tag_;

  // Statistics:
  ElementSplitterStats stats_;

  // Info for the media we're splitting. Every subclass should update this.
  MediaInfo media_info_;

  DISALLOW_EVIL_CONSTRUCTORS(TagSplitter);
};

//////////////////////////////////////////////////////////////////////

// Abstract factory for splitters
class SplitterCreator {
 public:
  SplitterCreator() {}
  virtual ~SplitterCreator() {}

  virtual TagSplitter* CreateSplitter(const string& name,
                                      Tag::Type tag_type) = 0;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(SplitterCreator);
};
}

#endif  //  __MEDIA_SOURCE_TAG_SPLITTER_H__
