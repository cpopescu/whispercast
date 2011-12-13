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

#ifndef __MEDIA_AAC_AAC_TAG_SPLITTER_H__
#define __MEDIA_AAC_AAC_TAG_SPLITTER_H__

// New include:
#include <neaacdec.h>
// If you have something really old, use this instread:
// #include <faad.h>

#include <string>

#include <whisperlib/common/base/types.h>
#include <whisperstreamlib/base/tag.h>
#include <whisperstreamlib/base/tag_splitter.h>

// IMPORTANT : this is not thread safe !!

namespace streaming {

//////////////////////////////////////////////////////////////////////

class AacFrameTag : public streaming::Tag {
 public:
  static const Type kType;
  AacFrameTag(uint32 attributes,
              uint32 flavour_mask,
              int64 timestamp_ms,
              int64 duration_ms,
              int32 samplerate,
              int32 channels,
              int32 num_samples,
              const char* data,
              int data_size)
    : Tag(kType, attributes, flavour_mask),
      timestamp_ms_(timestamp_ms),
      duration_ms_(duration_ms),
      samplerate_(samplerate),
      channels_(channels),
      num_samples_(num_samples) {
    data_.Write(data, data_size);
  }
  AacFrameTag(const AacFrameTag& other)
    : Tag(other),
      timestamp_ms_(other.timestamp_ms_),
      duration_ms_(other.duration_ms_),
      samplerate_(other.samplerate_),
      channels_(other.channels_),
      num_samples_(other.num_samples_) {
    data_.AppendStreamNonDestructive(&other.data_);
  }
  virtual ~AacFrameTag() {
  }
  int32 samplerate() const { return samplerate_; }
  int32 channels() const { return channels_; }
  int32 num_samples() const { return num_samples_; }
  int64 timestamp_ms() const { return timestamp_ms_; }
  const io::MemoryStream& data() const { return data_; }
  virtual int64 duration_ms() const { return duration_ms_; }
  virtual uint32 size() const { return data_.Size(); }
  virtual Tag* Clone() const {
    return new AacFrameTag(*this);
  }

  virtual string ToStringBody() const;

 private:
  const int64 timestamp_ms_;
  const int64 duration_ms_;
  const int32 samplerate_;
  const int32 channels_;
  const int32 num_samples_;
  io::MemoryStream data_;
};

//////////////////////////////////////////////////////////////////////

class AacTagSerializer : public streaming::TagSerializer {
 public:
  AacTagSerializer() : streaming::TagSerializer() { }
  virtual ~AacTagSerializer() {}
  virtual void Initialize(io::MemoryStream* out) {}
  virtual void Finalize(io::MemoryStream* out) {}
 protected:
  virtual bool SerializeInternal(const Tag* tag,
                                 int64 timestamp_ms,
                                 io::MemoryStream* out) {
    if ( tag->type() != Tag::TYPE_AAC ) {
      return false;
    }
    out->AppendStreamNonDestructive(
        &static_cast<const AacFrameTag*>(tag)->data());
    return true;
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(AacTagSerializer);
};

//////////////////////////////////////////////////////////////////////

class AacTagSplitter : public streaming::TagSplitter {
 public:
  static const Type kType;
 public:
  AacTagSplitter(const string& name);
  virtual ~AacTagSplitter();

  static const int kAacHeaderLen;

 protected:
  virtual streaming::TagReadStatus GetNextTagInternal(io::MemoryStream* in,
                                                      scoped_ref<Tag>* tag,
                                                      int64* timestamp_ms,
                                                      bool is_at_eos);
  streaming::TagReadStatus MaybeFinalizeFrame(
      scoped_ref<Tag>* tag, const char* buffer);

 private:
  streaming::TagReadStatus ReadHeader(
    io::MemoryStream* in, scoped_ref<Tag>* tag, int64* timestamp_ms);

  faacDecHandle aac_handle_;
  int32 samplerate_;
  unsigned char channels_;
  bool header_extracted_;
  faacDecFrameInfo frame_info_;
  string last_stream_;

  double stream_offset_ms_;

  DISALLOW_EVIL_CONSTRUCTORS(AacTagSplitter);
};
}

#endif  // __MEDIA_AAC_AAC_TAG_SPLITTER_H__
