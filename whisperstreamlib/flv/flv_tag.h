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
// Author: Cosmin Tudorache & Catalin Popescu

#ifndef __MEDIA_FLV_FLV_TAG_H__
#define __MEDIA_FLV_FLV_TAG_H__

#include <string>
#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/strutil.h>
#include <whisperlib/common/base/ref_counted.h>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperlib/common/io/num_streaming.h>
#include <whisperstreamlib/base/tag.h>
#include <whisperstreamlib/base/tag_serializer.h>
#include <whisperstreamlib/base/consts.h>
#include <whisperstreamlib/flv/flv_consts.h>
#include <whisperstreamlib/rtmp/objects/rtmp_objects.h>

namespace streaming {

//////////////////////////////////////////////////////////////////////

class FlvHeader : public streaming::Tag {
 public:
  static const Type kType;
  FlvHeader(uint32 attributes,
            uint32 flavour_mask,
            uint32 unknown,
            uint8 version,
            uint8 audio_video_flags,
            uint32 data_offset)
      : Tag(kType, attributes, flavour_mask),
        unknown_(unknown),
        version_(version),
        audio_video_flags_(audio_video_flags),
        data_offset_(data_offset),
        timestamp_ms_(0) {
  }
  FlvHeader(const FlvHeader& other, int64 timestamp_ms)
      : Tag(other),
        unknown_(other.unknown_),
        version_(other.version_),
        audio_video_flags_(other.audio_video_flags_),
        data_offset_(other.data_offset_),
        timestamp_ms_(timestamp_ms) {
  }
  virtual ~FlvHeader() {
  }
  uint32 unknown() const               { return unknown_; }
  uint8  version() const               { return version_; }
  uint8  audio_video_flags() const     { return audio_video_flags_; }
  uint32 data_offset() const           { return data_offset_; }

  // true if the .flv file contains audio data
  bool has_audio() const      {  return (audio_video_flags_ & 0x04) != 0; }
  // true if the .flv file contains video data
  bool has_video() const       { return (audio_video_flags_ & 0x01) != 0; }

  // Write header to memory stream.
  void Write(io::MemoryStream* out) const;

  int64 timestamp_ms() const { return timestamp_ms_; }

  virtual int64 duration_ms() const { return 0; }
  virtual uint32 size() const { return kFlvHeaderSize; }
  virtual int64 composition_offset_ms() const { return 0; }
  virtual const io::MemoryStream* Data() const { return NULL; }
  virtual Tag* Clone() const {
    return new FlvHeader(*this);
  }

  virtual string ToStringBody() const {
    return strutil::StringPrintf("ver:%hhu, type:%s/%s off:%u",
                                 version_,
                                 has_audio() ? "AUDIO" : "-",
                                 has_video() ? "VIDEO" : "-",
                                 data_offset_);
  }
  static FlvHeader* StandardHeader(bool has_video = true,
                                   bool has_audio = true) {
    return new FlvHeader(0, kDefaultFlavourMask,
        0x464c56,
        0x01,
        (has_audio ? 0x04 : 0x00) | (has_video ? 0x01 : 0x00),
        0x09);
  }
  static void AppendStandardHeader(io::MemoryStream* out,
                                   bool has_video = true,
                                   bool has_audio = true);
 protected:
  // Some header of the header :)
  uint32 unknown_;
  // FLV version
  uint8 version_;
  // Defines what's in the file
  uint8 audio_video_flags_;
  // DATA OFFSET reserved for data up to 4,294,967,295
  uint32 data_offset_;
  // Timestamp
  int64 timestamp_ms_;
};

//////////////////////////////////////////////////////////////////////

class FlvTag : public streaming::Tag {
 public:
  class Body : public RefCounted {
   public:
    Body(FlvFrameType t) : type_(t) {}
    Body(const Body& other) : type_(other.type_) {}
    virtual ~Body() {}
    FlvFrameType type() const { return type_; }
    const char* type_name() const { return FlvFrameTypeName(type_); }
    virtual TagReadStatus Decode(io::MemoryStream& in,
                                 uint32 size) = 0;
    virtual void Encode(io::MemoryStream* out) const = 0;
    virtual Body* Clone() const = 0;
    virtual uint32 Size() const = 0;
    virtual string ToString() const = 0;
   private:
    FlvFrameType type_;
  };
  class Audio : public Body {
   public:
    static const FlvFrameType kType;
    Audio()
      : Body(kType),
        data_(),
        type_(FLV_FLAG_SOUND_TYPE_MONO),
        format_(FLV_FLAG_SOUND_FORMAT_RAW),
        rate_(FLV_FLAG_SOUND_RATE_5_5_KHZ),
        size_(FLV_FLAG_SOUND_SIZE_8_BIT),
        is_aac_header_(false) {}
    Audio(const Audio& other)
      : Body(other),
        data_(),
        type_(other.type_),
        format_(other.format_),
        rate_(other.rate_),
        size_(other.size_),
        is_aac_header_(other.is_aac_header_) {
      data_.AppendStreamNonDestructive(&other.data_);
    }
    virtual ~Audio() {}

    const io::MemoryStream& data() const { return data_; }
    FlvFlagSoundType type() const { return type_; }
    FlvFlagSoundFormat format() const { return format_; }
    FlvFlagSoundRate rate() const { return rate_; }
    FlvFlagSoundSize size() const { return size_; }
    bool is_aac_header() const { return is_aac_header_; }

    virtual TagReadStatus Decode(io::MemoryStream& in, uint32 size);
    virtual void Encode(io::MemoryStream* out) const;
    virtual Body* Clone() const { return new Audio(*this); }
    virtual uint32 Size() const { return data_.Size(); }
    virtual string ToString() const;

   private:
    io::MemoryStream data_;

    // Audio flags, extracted from 'data_', without consuming any data.
    // Therefore these flags are read-only.
    FlvFlagSoundType type_;
    FlvFlagSoundFormat format_;
    FlvFlagSoundRate rate_;
    FlvFlagSoundSize size_;
    bool is_aac_header_;
  };
  class Video : public Body {
   public:
    static const FlvFrameType kType;
    Video()
      : Body(kType),
        data_(),
        codec_(FLV_FLAG_VIDEO_CODEC_H263),
        frame_type_(FLV_FLAG_VIDEO_FRAMETYPE_KEYFRAME),
        avc_packet_type_(AVC_SEQUENCE_HEADER),
        avc_composition_offset_ms_(0),
        avc_moov_() {}
    Video(const Video& other)
      : Body(other),
        data_(),
        codec_(other.codec_),
        frame_type_(other.frame_type_),
        avc_packet_type_(other.avc_packet_type_),
        avc_composition_offset_ms_(other.avc_composition_offset_ms_),
        avc_moov_(other.avc_moov_) {
      data_.AppendStreamNonDestructive(&other.data_);
    }

    virtual ~Video() {}
    const io::MemoryStream& data() const { return data_; }
    FlvFlagVideoCodec codec() const { return codec_; }
    FlvFlagVideoFrameType frame_type() const { return frame_type_; }
    AvcPacketType avc_packet_type() const { return avc_packet_type_; }
    int32 avc_composition_offset_ms() const { return avc_composition_offset_ms_; }
    const scoped_ref<const FlvTag>& avc_moov() const { return avc_moov_; }

    void set_avc_moov(const FlvTag* moov) { avc_moov_ = moov; }

    virtual TagReadStatus Decode(io::MemoryStream& in, uint32 size);
    virtual void Encode(io::MemoryStream* out) const;
    virtual Body* Clone() const { return new Video(*this); }
    virtual uint32 Size() const { return data_.Size(); }
    virtual string ToString() const;

   private:
    io::MemoryStream data_;

    // Video flags, extracted from 'data_', without consuming any data.
    // Therefore these flags are read-only.
    FlvFlagVideoCodec codec_;
    FlvFlagVideoFrameType frame_type_;
    // Only for codec_ == FLV_FLAG_VIDEO_CODEC_AVC
    AvcPacketType avc_packet_type_;
    int32 avc_composition_offset_ms_;
    // The moov tag extracted from parent. Useful to bootstrapper
    // (the alternative would be: the bootstrapper detects and extracts this
    //  moov; however the more bootstrappers => the more extra processing)
    scoped_ref<const FlvTag> avc_moov_;
  };
  class Metadata : public Body {
   public:
    static const FlvFrameType kType;
    Metadata() : Body(kType) {}
    Metadata(const string& name, const rtmp::CMixedMap& values)
      : Body(kType), name_(name), values_(values) {}
    Metadata(const Metadata& other)
      : Body(other), name_(other.name_), values_(other.values_) {}
    virtual ~Metadata() {}
    const rtmp::CString& name() const { return name_; }
    const rtmp::CMixedMap& values() const { return values_; }
    rtmp::CString* mutable_name() { return &name_; }
    rtmp::CMixedMap* mutable_values() { return &values_; }
    virtual TagReadStatus Decode(io::MemoryStream& in, uint32 size);
    virtual void Encode(io::MemoryStream* out) const;
    virtual Body* Clone() const { return new Metadata(*this); }
    virtual uint32 Size() const;
    virtual string ToString() const;
   private:
    // e.g.: "onMetadata", "onCuePoint"
    rtmp::CString name_;
    // Pair of key/value
    rtmp::CMixedMap values_;
  };

 public:
  static const Type kType;
  FlvTag(uint32 attributes,
         uint32 flavour_mask,
         int64 timestamp_ms,
         Body* body)
      : Tag(kType, attributes, flavour_mask),
        timestamp_ms_(timestamp_ms),
        stream_id_(0),
        body_(body) {
    CHECK_NOT_NULL(body_.get()) << " Null FlvTag::Body ??";
  }
  FlvTag(uint32 attributes,
         uint32 flavour_mask,
         int64 timestamp_ms,
         FlvFrameType frame_type)
      : Tag(kType, attributes, flavour_mask),
        timestamp_ms_(timestamp_ms),
        stream_id_(0),
        body_(frame_type == FLV_FRAMETYPE_AUDIO ? (Body*)new Audio() :
              frame_type == FLV_FRAMETYPE_VIDEO ? (Body*)new Video() :
              frame_type == FLV_FRAMETYPE_METADATA ? (Body*)new Metadata() :
              (Body*)NULL) {
    CHECK_NOT_NULL(body_.get()) << " Illegal frame_type: " << frame_type;
  }
  FlvTag(const FlvTag& other, int64 timestamp_ms, bool duplicate_body)
      : Tag(other),
        timestamp_ms_(timestamp_ms != -1 ? timestamp_ms : other.timestamp_ms_),
        stream_id_(other.stream_id_),
        body_(duplicate_body ? other.body_->Clone() : other.body_) {
  }

  virtual ~FlvTag() {
  }

  // Updates attributes_, based on body_ properties.
  void LearnAttributes();

  int64 timestamp_ms() const { return timestamp_ms_; }

  virtual int64 duration_ms() const { return body_->Size(); }
  virtual uint32 size() const { return body_->Size(); }
  virtual int64 composition_offset_ms() const {
    return is_video_tag() ? video_body().avc_composition_offset_ms() : 0;
  }
  virtual const io::MemoryStream* Data() const {
    switch ( body().type() ) {
      case FLV_FRAMETYPE_AUDIO: return &audio_body().data();
      case FLV_FRAMETYPE_VIDEO: return &video_body().data();
      case FLV_FRAMETYPE_METADATA: return NULL;
    }
    LOG_FATAL << "Illegal body type: " << body().type();
    return NULL;
  }

  // the new tag shares the internal data buffer with this tag
  virtual Tag* Clone() const {
    return new FlvTag(*this, -1, false);
  }

  uint32 stream_id() const {
    return stream_id_;
  }
  void set_timestamp_ms(int64 timestamp_ms) {
    timestamp_ms_ = timestamp_ms;
  }
  void set_stream_id(uint32 stream_id) {
    stream_id_ = stream_id;
  }
  const Body& body() const {
    return *body_.get();
  }
  Body& mutable_body() {
    CHECK(body_->ref_count() == 1) << " Attempting to modify tag shared body";
    return *body_.get();
  }

  // T can be: Audio, Video, Metadata.
  template <typename T>
  const T& tbody() const {
    CHECK_EQ(body_->type(), T::kType);
    return *static_cast<const T*>(body_.get());
  }
  const Audio& audio_body() const { return tbody<Audio>(); }
  const Video& video_body() const { return tbody<Video>(); }
  const Metadata& metadata_body() const { return tbody<Metadata>(); }

  template <typename T>
  T& mutable_tbody() {
    CHECK_EQ(body_->type(), T::kType);
    return *static_cast<T*>(body_.get());
  }
  Audio& mutable_audio_body() { return mutable_tbody<Audio>(); }
  Video& mutable_video_body() { return mutable_tbody<Video>(); }
  Metadata& mutable_metadata_body() { return mutable_tbody<Metadata>(); }

  virtual string ToStringBody() const {
    return strutil::StringPrintf("timestamp_ms_: %"PRId64""
        ", composition_offset_ms_: %"PRId64""
        ", stream_id_: %u, body_: %s",
        timestamp_ms_, composition_offset_ms(), stream_id_,
        body_->ToString().c_str());
  }

 protected:
  // Decoding Timestamp. 3 bytes BIGENDIAN + 1 byte extension(hi)
  int64 timestamp_ms_;

  // Stream ID. 3 bytes: "\0x00\0x00\0x00"
  // According to 'Video File Format Specification, v.10': not used, always 0.
  uint32 stream_id_;

  // Flv tag body. Cannot be NULL.
  scoped_ref<Body> body_;

  DISALLOW_EVIL_CONSTRUCTORS(FlvTag);
};

//////////////////////////////////////////////////////////////////////

//inline ostream& operator<<(ostream& os, const FlvTag& obj) {
//  return os << obj.ToString();
//}

class FlvTagSerializer : public streaming::TagSerializer {
 public:
  FlvTagSerializer(bool write_header = true,
                   bool has_video = true, bool has_audio = true);
  virtual ~FlvTagSerializer();

  void set_has_video(bool has_video) { has_video_ = has_video; }
  void set_has_audio(bool has_audio) { has_audio_ = has_audio; }

  ////////////////////////////////////////////////////////////////////////
  // Methods from TagSerializer:

  // If any starting things are necessary to be serialized before the
  // actual tags, this is the moment :)
  virtual void Initialize(io::MemoryStream* out);
private:
  // The main interface function - puts "tag" into "out"
  virtual bool SerializeInternal(const Tag* tag,
                                 int64 timestamp_ms,
                                 io::MemoryStream* out);
public:
  // If any finishing touches things are necessary to be serialized after the
  // actual tags, this is the moment :)
  virtual void Finalize(io::MemoryStream* out);

  // Returns the serialized tag size.
  static uint32 EncodingSize(const FlvTag* tag);

 private:
  bool write_header_;
  bool has_video_;
  bool has_audio_;
  int32 previous_tag_size_;
  uint32 last_timestamp_ms_;

  DISALLOW_EVIL_CONSTRUCTORS(FlvTagSerializer);
};
}

#endif  // __NET_RTMP_FLV_FLV_TAG_H__
