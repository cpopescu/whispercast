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

#ifndef __MEDIA_BASE_TAG_H__
#define __MEDIA_BASE_TAG_H__

#include <string>
#include <vector>
#include <algorithm>
#include <utility>

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/callback.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperlib/common/base/ref_counted.h>

namespace streaming {

//////////////////////////////////////////////////////////////////////

class Tag : public RefCounted {
 public:
  enum Type {
    // flv streams: name: 'flv', content-type: 'video/x-flv'
    TYPE_FLV,
    TYPE_FLV_HEADER,
    // mp3 streams: name: 'mp3', content-type: 'audio/mpeg'
    TYPE_MP3,
    // aac streams: name: 'aac', content-type: 'audio/aac' or 'audio/aacp'
    TYPE_AAC,
    // internal formated stream: name: 'internal',
    //                           content-type: 'media/x-whisper-internal'
    TYPE_INTERNAL,
    // f4v streams: name: 'f4v', content-type: 'video/x-f4v'
    TYPE_F4V,
    // internal formated stream: name: 'raw',
    //                           content-type: 'application/octet-stream'
    TYPE_RAW,
    // the first tag you will see on a processing callback
    TYPE_BOS,
    // the last tag you will see on a processing callback
    // ** YOU MUST ** remove the association between the request and callback
    //                at *this* point
    TYPE_EOS,
    // A feature discovered at this time.
    TYPE_FEATURE_FOUND,
    // information about the cue-points in the current media
    TYPE_CUE_POINT,
    // notifies that a source has started producing tags
    TYPE_SOURCE_STARTED,
    // notifies that a source has stopped producing tags
    TYPE_SOURCE_ENDED,
    // a new contiguous sequence of tags will be produced
    TYPE_SEGMENT_STARTED,
    // this is a composed tag (from several tags of the same type)
    TYPE_COMPOSED,
    // denotes that the data_ is an OsdTag
    TYPE_OSD,
    // we just did a seek to another position in file. A new sequence of media
    // tags follow
    TYPE_SEEK_PERFORMED,
    // we need to flush the pipeline
    TYPE_FLUSH,

    // the bootstrap begin/end sequence
    TYPE_BOOTSTRAP_BEGIN,
    TYPE_BOOTSTRAP_END
  };
  // This is not a valid TagType. Use it whenever you need an illegal tag type
  // (e.g. for an uninitialized element).
  static const Type kInvalidType;
  // This is not a valid TagType. Used in Capabilities and in Requests that
  // advertise they accept any incoming tag type.
  static const Type kAnyType;

  static const char* TypeName(Type type);

  // bit flags, you can combine them. e.g. AUDIO_TAG | DROPPABLE_TAG
  enum Attributes {
    ATTR_METADATA     = 0x0001,
    ATTR_AUDIO        = 0x0002,
    ATTR_VIDEO        = 0x0004,
    ATTR_DROPPABLE    = 0x0008,
    ATTR_CAN_RESYNC   = 0x0010,
  };
  static string AttributesName(uint32 attr);

 public:
  Tag(Type type,
      uint32 attributes,
      uint32 flavour_mask)
    : RefCounted(mutex_pool_.GetMutex(this)),
      type_(type),
      attributes_(attributes),
      flavour_mask_(flavour_mask) {
    // sanity check: attribute flags should be in the least significant 2 bytes
    CHECK((attributes_ & 0xFF00) == 0)
        << "Illegal attributes: " << attributes;
    // sanity check: flavour_mask should contain only 1 flavour_id
    CHECK(flavour_mask != 0 && (flavour_mask & (flavour_mask-1)) == 0)
        << "Illegal flavour_mask: " << flavour_mask
        << ", MUST contain just 1 flavour_id";
  }
  Tag(const Tag& other)
    : RefCounted(mutex_pool_.GetMutex(this)),
      type_(other.type_),
      attributes_(other.attributes_),
      flavour_mask_(other.flavour_mask_) {
    CHECK_EQ((attributes_ & 0xFF00), 0);
  }
  virtual ~Tag() {
  }

  Type type() const { return type_; }
  const char* type_name() const { return TypeName(type()); }

  // Every tag is responsible with implementing these.
  // If a tag does not care for some of these it should return 0.
  virtual int64 timestamp_ms() const = 0;
  virtual int64 duration_ms() const = 0;
  virtual uint32 size() const = 0;

  uint32 attributes() const { return attributes_; }
  string attributes_name() const { return AttributesName(attributes_); }

  void set_attributes(uint32 attributes) { attributes_ = attributes; }
  void add_attributes(uint32 attr) { attributes_ |= attr; }

  uint32 flavour_mask() const { return flavour_mask_; }
  void set_flavour_mask(uint32 flavour_mask) {
    // sanity check: flavour_mask should contain only 1 flavour_id
    CHECK(flavour_mask != 0 && (flavour_mask & (flavour_mask-1)) == 0)
        << "Illegal flavour_mask: " << flavour_mask
        << ", MUST contain just 1 flavour_id";
    flavour_mask_ = flavour_mask;
  }

  // Attribute accessors
  bool is_droppable() const {
    return (attributes_ & ATTR_DROPPABLE) == ATTR_DROPPABLE;
  }
  bool can_resync() const {
    return (attributes_ & ATTR_CAN_RESYNC) == ATTR_CAN_RESYNC;
  }
  bool is_video_tag() const {
    return (attributes_ & ATTR_VIDEO) == ATTR_VIDEO;
  }
  bool is_audio_tag() const {
    return (attributes_ & ATTR_AUDIO) == ATTR_AUDIO;
  }
  bool is_metadata_tag() const {
    return (attributes_ & ATTR_METADATA) == ATTR_METADATA;
  }

  virtual Tag& operator=(const Tag& other) {
    CHECK_EQ(type(), other.type());
    attributes_ = other.attributes_;
    flavour_mask_ = other.flavour_mask_;
    return *this;
  }

  // if timestamp_ms == -1, the result copies the parent timestamp
  // else, the result has the given timestamp_ms
  virtual Tag* Clone(int64 timestamp_ms)  const = 0;

  virtual string ToStringBody() const {
    return "{}";
  }

  const string ToString() const {
    return strutil::StringPrintf(
        "@%8"PRId64" - %s [%s] fl:%x -> %s",
        (timestamp_ms()),
        type_name(),
        attributes_name().c_str(),
        flavour_mask_,
        ToStringBody().c_str());
  }

 protected:
  static const int kNumMutexes;
  static synch::MutexPool mutex_pool_;

 private:
  const Type type_;

  // A combination of 0, 1 or more Attributes
  uint32 attributes_;
  // The sub-stream (stream flavour) to which this tag belongs
  uint32 flavour_mask_;
};

//////////////////////////////////////////////////////////////////////
//
// Several common tag data types
//
//////////////////////////////////////////////////////////////////////

class CuePointTag : public Tag {
 public:
  static const Type kType;
  CuePointTag(uint32 attributes,
              uint32 flavour_mask,
              int64 timestamp_ms)
    : Tag(kType, attributes, flavour_mask),
      timestamp_ms_(timestamp_ms) {
  }
  CuePointTag(const CuePointTag& other, int64 timestamp_ms)
    : Tag(other),
      timestamp_ms_(timestamp_ms != -1 ? timestamp_ms : other.timestamp_ms_) {
    cue_points_.resize(other.cue_points_.size());
    copy(other.cue_points_.begin(), other.cue_points_.end(),
         cue_points_.begin());
  }

 public:
  virtual ~CuePointTag() {
  }

  virtual int64 timestamp_ms() const { return timestamp_ms_; }
  virtual int64 duration_ms() const { return 0; }
  virtual uint32 size() const { return 0; }

  const vector< pair<int64, int64> >& cue_points() const {
    return cue_points_;
  }
  vector< pair<int64, int64> >& mutable_cue_points() {
    return cue_points_;
  }
  int GetCueForTime(int64 t) const {
    if ( cue_points_.empty() ) return -1;
    if ( t < 0 ) return 0;
    int l = 0;
    int r = cue_points_.size();
    while ( l + 1 < r ) {
      const int mid = (r + l) / 2;
      if ( cue_points_[mid].first <= t ) {
        l = mid;
      } else {
        r = mid;
      }
    }
    return l;
  }
  virtual Tag* Clone(int64 timestamp_ms) const {
    return new CuePointTag(*this, timestamp_ms);
  }
  virtual string ToStringBody() const {
    string s = "{\n";
    for ( int i = 0; i < cue_points_.size(); ++i ) {
      s += strutil::StringPrintf(
          "%4d - @%"PRId64" ms / %"PRId64" B\n",
          i,
          (cue_points_[i].first),
          (cue_points_[i].second));
    }
    s+= "\n}";
    return s;
  }

 private:
  const int64 timestamp_ms_;
  // Pair of timestamp_ms_ / file_pos_
  vector< pair<int64, int64> > cue_points_;
};

//////////////////////////////////////////////////////////////////////

class FeatureFoundTag : public Tag {
 public:
  static const Type kType;
  FeatureFoundTag(uint32 attributes,
                  uint32 flavour_mask,
                  const string& name,
                  int64 timestamp_ms,
                  int64 length_ms)
      : Tag(kType, attributes, flavour_mask),
        name_(name),
        timestamp_ms_(timestamp_ms),
        length_ms_(length_ms) {
  }
  FeatureFoundTag(const FeatureFoundTag& other, int64 timestamp_ms)
    : Tag(other),
      name_(other.name_),
      timestamp_ms_(timestamp_ms != -1 ? timestamp_ms : other.timestamp_ms_),
      length_ms_(other.length_ms_) {
  }

 public:
  virtual ~FeatureFoundTag() {
  }

  virtual int64 timestamp_ms() const { return timestamp_ms_; }
  virtual int64 duration_ms() const { return 0; }
  virtual uint32 size() const { return 0; }

  virtual Tag* Clone(int64 timestamp_ms) const {
    return new FeatureFoundTag(*this, timestamp_ms);
  }
  virtual string ToStringBody() const {
    return strutil::StringPrintf(
        "{ name_: %s, length_ms_: %"PRId64" }",
        name_.c_str(), length_ms_);
  }

  const string& name() const { return name_; }
  int64 length_ms() const { return length_ms_; }

 private:
  const string name_;
  int64 timestamp_ms_;
  const int64 length_ms_;
};

//////////////////////////////////////////////////////////////////////

class SourceChangedTag : public Tag {
 public:
  SourceChangedTag(Type type,
                   uint32 attributes,
                   uint32 flavour_mask,
                   int64 timestamp_ms,
                   const string& source_element_name,
                   const string& path,
                   bool is_final = false)
    : Tag(type, attributes, flavour_mask),
      timestamp_ms_(timestamp_ms),
      source_element_name_(source_element_name),
      path_(path),
      is_final_(is_final) {
  }
  SourceChangedTag(const SourceChangedTag& other, int64 timestamp_ms)
    : Tag(other),
      timestamp_ms_(timestamp_ms != -1 ? timestamp_ms : other.timestamp_ms_),
      source_element_name_(other.source_element_name_),
      path_(other.path_),
      is_final_(other.is_final_) {
  }

 public:
  virtual ~SourceChangedTag() {
  }

  const string& source_element_name() const { return source_element_name_; }
  const string& path() const { return path_; }
  bool is_final() const { return is_final_; }

  void set_source_element_name(const string& source_element_name) {
    source_element_name_ = source_element_name;
  }
  void set_path(const string& path) {
    path_ = path;
  }
  void set_is_final() {
    is_final_ = true;
  }

  //////////////////////////////////////////////////////////////////////
  // Methods from Tag
  //
  virtual int64 timestamp_ms() const { return timestamp_ms_; }
  virtual int64 duration_ms() const { return 0; }
  virtual uint32 size() const { return 0; }

 private:
  const int64 timestamp_ms_;
  // The very source element name (e.g. "aio_file/spiderman.flv")
  // This name doesn't change while the tag circulates through
  // filtering elements.
  string source_element_name_;
  // The path to the element that generated this tag.
  // This path will add up all element names while circulating through
  // filtering elements.
  string path_;
  // Tags marked as final must not be changed by downstream elements.
  bool is_final_;
};

template <Tag::Type TYPE>
class TSourceChangedTag : public SourceChangedTag {
 public:
  static const Type kType;
  TSourceChangedTag(uint32 attributes,
                    uint32 flavour_mask,
                    int64 timestamp_ms,
                    const string& source_element_name,
                    const string& path,
                    bool is_final = false)
    : SourceChangedTag(kType, attributes, flavour_mask, timestamp_ms,
        source_element_name, path, is_final) {
  }
  TSourceChangedTag(const TSourceChangedTag<TYPE>& other,
                    int64 timestamp_ms)
    : SourceChangedTag(other, timestamp_ms) {
  }

 public:
  virtual ~TSourceChangedTag() {
  }

public:
  virtual Tag* Clone(int64 timestamp_ms) const {
    return new TSourceChangedTag<TYPE>(*this, timestamp_ms);
  }
  virtual string ToStringBody() const {
    return strutil::StringPrintf(
        "{ name_: %s, timestamp_ms_: %"PRId64", source_element_name_: %s, "
        "path_: %s, is_final_: %s }",
        type_name(),
        timestamp_ms(),
        source_element_name().c_str(),
        path().c_str(),
        strutil::BoolToString(is_final()).c_str());
  }
};

typedef TSourceChangedTag<Tag::TYPE_SOURCE_STARTED> SourceStartedTag;
typedef TSourceChangedTag<Tag::TYPE_SOURCE_ENDED> SourceEndedTag;

//////////////////////////////////////////////////////////////////////
//
// An interface for controlling the way in which an element produce media
// data

class ElementController {
 public:
  ElementController() {
  }
  virtual ~ElementController() {
  }

  // Return true if an element controller supports pausing / unpausing
  // of the element
  virtual bool SupportsPause() const { return false; }
  // A controller may support the pause operation, but it can be disabled
  virtual bool DisabledPause() const { return false; }

  // Return true if an element supports seeking in the content
  virtual bool SupportsSeek() const { return false; }
  // A controller may support the seek operation, but it can be disabled
  virtual bool DisabledSeek() const { return false; }

  // Not only seeks are disabled, by we also hide duration
  virtual bool DisabledDuration() const { return false; }

  // Return true if an element supports preprocessing of the data (in order to
  // do a better tag splitting in wire packets)
  virtual bool SupportsPreprocessing() const { return false; }

  // Pause the element (pause true - > stop, false -> restart)
  virtual bool Pause(bool pause) {
    return false;
  }
  // Seek in the element
  virtual bool Seek(int64 position) {
    return false;
  }

 private:
  DISALLOW_EVIL_CONSTRUCTORS(ElementController);
};

//////////////////////////////////////////////////////////////////////

class TagSet : public RefCounted {
 public:
  static const int kMaxSize = 16;
  TagSet()
      : RefCounted(mutex_pool_.GetMutex(this)),
        size_(0) {
  }
  virtual ~TagSet() {
    for ( int i = 0; i < size_; ++i ) {
      tags_[i] = NULL;
    }
  }
  int size() const {
    return size_;
  }
  bool is_full() const {
    return size_ >= kMaxSize;
  }
  const scoped_ref<const Tag>& tag(size_t k) const {
    DCHECK_LT(k, size_);
    return tags_[k];
  }
  void add_tag(const scoped_ref<const Tag>& t) {
    DCHECK(!is_full());
    tags_[size_++] = t;
  }

 private:
  static const int kNumMutexes;
  static synch::MutexPool mutex_pool_;

  size_t size_;
  scoped_ref<const Tag> tags_[kMaxSize];

  DISALLOW_EVIL_CONSTRUCTORS(TagSet);
};

class ComposedTag : public Tag {
 public:
  static const Type kType;
  ComposedTag(uint32 attributes,
              uint32 flavour_mask,
              int64 timestamp_ms,
              Type sub_tag_type = kAnyType,
              TagSet* tags = NULL)
      : Tag(kType, attributes, flavour_mask),
        timestamp_ms_(timestamp_ms),
        sub_tag_type_(sub_tag_type),
        tags_(tags == NULL ? new TagSet() : tags) {
  }
  ComposedTag(const ComposedTag& other, int64 timestamp_ms)
      : Tag(other),
        timestamp_ms_(timestamp_ms != -1 ? timestamp_ms : other.timestamp_ms_),
        sub_tag_type_(other.sub_tag_type_),
        tags_(other.tags_) {
  }

 public:
  virtual ~ComposedTag() {
  }

  // Adds a clone of the given tag to our list
  // IMPORTANT NOTE: we don't 'consume' the tag - is your job to take care
  //                 of the given argument from now on
  int64 add_tag(const Tag* tag) {
    DCHECK(!tags_->is_full());
    if ( sub_tag_type_ == kAnyType ) {
      timestamp_ms_ = tag->timestamp_ms();
      sub_tag_type_ = tag->type();
    }
    CHECK_EQ(sub_tag_type_, tag->type());
    add_attributes(tag->attributes());
    tags_->add_tag(tag->Clone(tag->timestamp_ms() - timestamp_ms_));
    return duration_ms();
  }
  // Adds the given tag to our list.
  // IMPORTANT NOTE: we 'consume' the tag - becomes ours ..
  //                 The tag's timestamp MUST be correct (relative to this
  //                 ComposedTag's timestamp_ms_)
  void add_prepared_tag(Tag* tag) {
    DCHECK(!tags_->is_full());
    if ( sub_tag_type_ == kAnyType ) {
      timestamp_ms_ = tag->timestamp_ms();
      sub_tag_type_ = tag->type();
    }
    CHECK_EQ(sub_tag_type_, tag->type());
    add_attributes(tag->attributes());
    tags_->add_tag(tag);
  }
  virtual int64 timestamp_ms() const {
    return timestamp_ms_;
  }
  virtual int64 duration_ms() const {
    int64 d = 0;
    for ( uint32 i = 0; i < tags_->size(); i++ ) {
      d += tags_->tag(i)->duration_ms();
    }
    return d;
  }
  virtual uint32 size() const {
    uint32 s = 0;
    for ( uint32 i = 0; i < tags_->size(); i++ ) {
      s += tags_->tag(i)->size();
    }
    return s;
  }
  virtual Tag* Clone(int64 timestamp_ms) const {
    return new ComposedTag(*this, timestamp_ms);
  }
  virtual string ToStringBody() const {
    string s(strutil::StringPrintf(
        "{ duration_ms_: %"PRId64", tags_:\n",
        duration_ms()));
    for ( size_t i = 0; i < tags_->size(); ++i ) {
      s += tags_->tag(i)->ToString();
      s += "\n";
    }
    s += "\n}";
    return s;
  }

  const TagSet& tags() const     { return *tags_; }
  Type sub_tag_type() const      { return sub_tag_type_; }

 private:
  int64 timestamp_ms_;
  Type sub_tag_type_;
  scoped_ref<TagSet> tags_;
};

template <Tag::Type TYPE>
class TSignalTag : public Tag {
 public:
  static const Type kType;
  TSignalTag(uint32 attributes,
            uint32 flavour_mask,
            int64 timestamp_ms)
      : Tag(kType, attributes, flavour_mask),
        timestamp_ms_(timestamp_ms) {
  }
  TSignalTag(const TSignalTag<TYPE>& other, int64 timestamp_ms)
      : Tag(other),
        timestamp_ms_(timestamp_ms != -1 ? timestamp_ms : other.timestamp_ms_) {
  }

 public:
  virtual ~TSignalTag() {
  }

  //////////////////////////////////////////////////////////////////////////
  //
  // Tag interface methods
  //
  virtual int64 timestamp_ms() const { return timestamp_ms_; }
  virtual int64 duration_ms() const { return 0; }
  virtual uint32 size() const { return 0; }

  virtual Tag* Clone(int64 timestamp_ms) const {
    return new TSignalTag<TYPE>(*this, timestamp_ms);
  }

  virtual string ToStringBody() const {
    return strutil::StringPrintf("{ name_: %s }", type_name());
  }

 private:
  const int64 timestamp_ms_;
};

typedef TSignalTag<Tag::TYPE_BOS> BosTag;
class EosTag : public TSignalTag<Tag::TYPE_EOS> {
 public:
  EosTag(uint32 attributes,
                uint32 flavour_mask,
                int64 timestamp_ms,
                bool forced = false)
      : TSignalTag<Tag::TYPE_EOS>(
          attributes, flavour_mask, timestamp_ms),
          forced_(forced) {
  }
  EosTag(const EosTag& other, int64 timestamp_ms)
      : TSignalTag<Tag::TYPE_EOS>(other,
          timestamp_ms != -1 ? timestamp_ms : other.timestamp_ms()),
        forced_(other.forced_) {
  }

 public:
  virtual ~EosTag() {
  }

  bool forced() const {
    return forced_;
  }

  //////////////////////////////////////////////////////////////////////////
  //
  // Tag interface methods
  //
  virtual Tag* Clone(int64 timestamp_ms) const {
    return new EosTag(*this, timestamp_ms);
  }

  virtual string ToStringBody() const {
    return strutil::StringPrintf("{ name_: %s, forced_: %s }",
        type_name(), strutil::BoolToString(forced()).c_str());
  }

private:
  bool forced_;
};

typedef TSignalTag<Tag::TYPE_BOOTSTRAP_BEGIN> BootstrapBeginTag;
typedef TSignalTag<Tag::TYPE_BOOTSTRAP_END> BootstrapEndTag;

typedef TSignalTag<Tag::TYPE_SEEK_PERFORMED> SeekPerformedTag;

typedef TSignalTag<Tag::TYPE_FLUSH> FlushTag;

class SegmentStartedTag : public TSignalTag<Tag::TYPE_SEGMENT_STARTED> {
 public:
  SegmentStartedTag(uint32 attributes,
                   uint32 flavour_mask,
                   int64 timestamp_ms,
                   int64 media_timestamp_ms)
    : TSignalTag<Tag::TYPE_SEGMENT_STARTED>(
          attributes,
          flavour_mask,
          timestamp_ms),
          media_timestamp_ms_(media_timestamp_ms) {
  }
  SegmentStartedTag(const SegmentStartedTag& other)
    : TSignalTag<Tag::TYPE_SEGMENT_STARTED>(
          other.attributes(),
          other.flavour_mask(),
          other.timestamp_ms()),
          media_timestamp_ms_(other.media_timestamp_ms_) {
  }

 public:
  virtual ~SegmentStartedTag() {
  }

  int64 media_timestamp_ms() const {
    return media_timestamp_ms_;
  }

  //////////////////////////////////////////////////////////////////////////
  //
  // Tag interface methods
  //
  virtual Tag* Clone(int64 timestamp_ms) const {
    CHECK(timestamp_ms == -1)
        << "SegmentStartedTag cannot be cloned w/ a different timestamp";
    return new SegmentStartedTag(*this);
  }

  virtual string ToStringBody() const {
    return strutil::StringPrintf(
        "{ timestamp_ms_: %"PRId64", media_timestamp_ms_: %"PRId64", }",
        timestamp_ms(),
        media_timestamp_ms());
  }

private:
  int64 media_timestamp_ms_;
};

//////////////////////////////////////////////////////////////////////
//
// General interface for saving a tag to some place..
//

class TagSerializer {
 public:
  TagSerializer() { }
  virtual ~TagSerializer() { }

  // If any starting things are necessary to be serialized before the
  // actual tags, this is the moment :)
  virtual void Initialize(io::MemoryStream* out) = 0;
  // The main interface function - puts "tag" into "out".
  // It mainly calls serialize internal, but breaking composed tags
  bool Serialize(const Tag* tag, io::MemoryStream* out,
                 int64 timestamp_ms = -1) {
    if ( tag->type() != Tag::TYPE_COMPOSED ) {
      if (timestamp_ms < 0)
        return SerializeInternal(tag, 0, out);
      return SerializeInternal(tag, timestamp_ms-tag->timestamp_ms(), out);
    } else {
      const ComposedTag* ctd = static_cast<const ComposedTag*>(tag);
      bool is_ok = true;
      if ( timestamp_ms < 0 )
        timestamp_ms = tag->timestamp_ms();
      else
        timestamp_ms = timestamp_ms - tag->timestamp_ms();
      for ( int i = 0; is_ok && i < ctd->tags().size(); ++i ) {
        is_ok |= SerializeInternal(ctd->tags().tag(i).get(),
                                   timestamp_ms,
                                   out);
      }
      return is_ok;
    }
  }
  // If any finishing touches things are necessary to be serialized after the
  // actual tags, this is the moment :)
  virtual void Finalize(io::MemoryStream* out) = 0;

 protected:
  // Override this to do your serialization
  virtual bool SerializeInternal(const Tag* tag, int64 base_timestamp_ms,
                                 io::MemoryStream* out) = 0;
 private:
  DISALLOW_EVIL_CONSTRUCTORS(TagSerializer);
};

//////////////////////////////////////////////////////////////////////
//
// Utility for calculating stream time. Tags circulate in a broken stream;
// what this mean is that an element sees sth like:
//   SourceStartedTag timestamp = 123  // stream a       | => stream time 0
//   Tag timestamp = 124                                 | => stream time 1
//   Tag timestamp = 125                                 | => stream time 2
//   ...
//   Tag timestamp = 345                                 | => stream time 221
//   SeekPerformedTag timestamp = 781  // stream b       | => stream time 221
//   Tag timestamp = 782                                 | => stream time 222
//   Tag timestamp = 783                                 | => stream time 223
//   ...
//   Tag timestamp = 933                      | => stream time 374
//   SourceStartedTag timestamp = 7    // stream c       | => stream time 374
//   Tag timestamp = 8                                   | => stream time 375
//   Tag timestamp = 9                                   | => stream time 376
//   ...
//
// This calculator computes a monotonously increasing stream time.
//
class StreamTimeCalculator {
 public:
  StreamTimeCalculator();
  virtual ~StreamTimeCalculator();

  // give me all stream tags...
  void ProcessTag(const Tag* tag);

  // and I'll tell you the timestamp of the last tag
  int64 last_tag_ts() const { return last_tag_ts_; }

  // and also the timestamp inside current media segment
  int64 media_time_ms() const { return media_time_ms_; }

  // and also how long I've been running
  int64 stream_time_ms() const { return stream_time_ms_; }

 private:
  // timestamps of the last processed segment started tag
  int64 last_segment_tag_ts_;
  int64 last_segment_media_ts_;
  // timestamp of the last processed tag
  int64 last_tag_ts_;
  // time inside current media segment
  int64 media_time_ms_;
  // total stream time (0.. infinite), uniformly increasing.
  int64 stream_time_ms_;
};
}

#endif  //  __MEDIA_BASE_TAG_H__
