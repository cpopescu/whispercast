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

// A tag that contains just data :) Its time scale is the size of the
// data inside

#ifndef __MEDIA_RAW_RAW_TAG_H__
#define __MEDIA_RAW_RAW_TAG_H__

#include <string>
#include <whisperlib/common/base/types.h>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperlib/common/base/ref_counted.h>
#include <whisperstreamlib/base/tag.h>
#include <whisperstreamlib/base/tag_splitter.h>
#include <whisperstreamlib/base/tag_serializer.h>

namespace streaming {

class RawTag : public Tag {
 public:
  static const Type kType;
  RawTag(uint32 attributes,
         uint32 flavour_mask)
      : Tag(kType, attributes, flavour_mask),
        content_type_() {
  }
  RawTag(const RawTag& other, bool duplicate_data)
      : Tag(other),
        content_type_(other.content_type_) {
    if ( duplicate_data ) {
      data_.AppendStreamNonDestructive(&other.data_);
    }
  }
  virtual ~RawTag() {
  }

  virtual int64 duration_ms() const { return 0; }
  virtual uint32 size() const { return data_.Size(); }
  virtual int64 composition_offset_ms() const { return 0; }
  virtual const io::MemoryStream* Data() const { return NULL; }
  // the new tag shares the internal data buffer with this tag
  virtual Tag* Clone() const {
    return new RawTag(*this, false);
  }
  virtual string ToStringBody() const {
    return strutil::StringPrintf("RawTag(%d - {%s})",
        size(), content_type_.c_str());
  }

  const string& content_type() const {
    return content_type_;
  }
  const io::MemoryStream& data() const {
    return data_;
  }
  io::MemoryStream* mutable_data() {
    return &data_;
  }

 private:
  string content_type_;
  io::MemoryStream data_;

  DISALLOW_EVIL_CONSTRUCTORS(RawTag);
};

class RawTagSplitter : public streaming::TagSplitter {
 public:
  RawTagSplitter(const string& name, int bits_per_second)
      : TagSplitter(MFORMAT_RAW, name),
        bits_per_second_(bits_per_second),
        stream_offset_(0),
        total_size_(0) {
  }
  virtual ~RawTagSplitter() {
  }

  int64 Duration(int64 tag_size) const {
    return tag_size * 8000 / bits_per_second_;
  }
  int64 stream_offset() const {
    return stream_offset_ + Duration(total_size_);
  }
  void set_stream_offset(int64 offset) {
    stream_offset_ = offset;
    total_size_ = 0;
  }

 protected:
  virtual streaming::TagReadStatus GetNextTagInternal(
      io::MemoryStream* in,
      scoped_ref<Tag>* tag,
      int64* timestamp_ms,
      bool is_at_eos);

 private:
  const int bits_per_second_;
  int64 stream_offset_;
  int64 total_size_;
  DISALLOW_EVIL_CONSTRUCTORS(RawTagSplitter);
};

class RawTagSerializer : public TagSerializer {
 public:
  RawTagSerializer() : TagSerializer(MFORMAT_RAW) {
  }
  virtual ~RawTagSerializer() {
  }

  // If any starting things are necessary to be serialized before the
  // actual tags, this is the moment :)
  virtual void Initialize(io::MemoryStream* out) {
  }
  // If any finishing touches things are necessary to be serialized after the
  // actual tags, this is the moment :)
  virtual void Finalize(io::MemoryStream* out) {
  }
 protected:
  virtual bool SerializeInternal(const Tag* tag, int64 timestamp_ms,
                                 io::MemoryStream* out) {
    if ( tag->Data() != NULL ) {
      out->AppendStreamNonDestructive(tag->Data());
    }
    return true;
  }

 private:
  DISALLOW_EVIL_CONSTRUCTORS(RawTagSerializer);
};
}

#endif   // __MEDIA_RAW_RAW_TAG_H__
