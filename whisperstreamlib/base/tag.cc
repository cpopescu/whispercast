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

#include <whisperstreamlib/base/tag.h>
#include <whisperstreamlib/base/consts.h>

namespace streaming {

const int Tag::kNumMutexes = 1024;
const int TagSet::kNumMutexes = 1024;

const Tag::Type Tag::kInvalidType = Tag::Type(-1);
const Tag::Type Tag::kAnyType = Tag::Type(-2);

const Tag::Type CuePointTag::kType     = Tag::TYPE_CUE_POINT;
const Tag::Type FeatureFoundTag::kType = Tag::TYPE_FEATURE_FOUND;
const Tag::Type ComposedTag::kType     = Tag::TYPE_COMPOSED;

template<>
const Tag::Type
TSourceChangedTag<Tag::TYPE_SOURCE_STARTED>::kType = Tag::TYPE_SOURCE_STARTED;

template<>
const Tag::Type
TSourceChangedTag<Tag::TYPE_SOURCE_ENDED>::kType = Tag::TYPE_SOURCE_ENDED;

template<>
const Tag::Type
TSignalTag<Tag::TYPE_BOS>::kType =
    Tag::TYPE_BOS;
template<>
const Tag::Type
TSignalTag<Tag::TYPE_EOS>::kType =
    Tag::TYPE_EOS;
template<>
const Tag::Type
TSignalTag<Tag::TYPE_BOOTSTRAP_BEGIN>::kType =
    Tag::TYPE_BOOTSTRAP_BEGIN;
template<>
const Tag::Type
TSignalTag<Tag::TYPE_BOOTSTRAP_END>::kType =
    Tag::TYPE_BOOTSTRAP_END;
template<>
const Tag::Type
TSignalTag<Tag::TYPE_SEEK_PERFORMED>::kType =
    Tag::TYPE_SEEK_PERFORMED;

template<>
const Tag::Type
TSignalTag<Tag::TYPE_FLUSH>::kType =
    Tag::TYPE_FLUSH;

template<>
const Tag::Type
TSignalTag<Tag::TYPE_SEGMENT_STARTED>::kType =
    Tag::TYPE_SEGMENT_STARTED;

//////////////////////////////////////////////////////////////////////

const char* Tag::TypeName(Tag::Type type) {
  switch(type) {
    CONSIDER(TYPE_FLV);
    CONSIDER(TYPE_FLV_HEADER);
    CONSIDER(TYPE_MP3);
    CONSIDER(TYPE_AAC);
    CONSIDER(TYPE_INTERNAL);
    CONSIDER(TYPE_F4V);
    CONSIDER(TYPE_RAW);
    CONSIDER(TYPE_FEATURE_FOUND);
    CONSIDER(TYPE_CUE_POINT);
    CONSIDER(TYPE_SOURCE_STARTED);
    CONSIDER(TYPE_SOURCE_ENDED);
    CONSIDER(TYPE_SEGMENT_STARTED);
    CONSIDER(TYPE_COMPOSED);
    CONSIDER(TYPE_OSD);
    CONSIDER(TYPE_BOS);
    CONSIDER(TYPE_EOS);
    CONSIDER(TYPE_BOOTSTRAP_BEGIN);
    CONSIDER(TYPE_BOOTSTRAP_END);
    CONSIDER(TYPE_SEEK_PERFORMED);
    CONSIDER(TYPE_FLUSH);
  }
  if ( type == kInvalidType ) {
    return "INVALID";
  }
  if ( type == kAnyType ) {
    return "ANY";
  }
  LOG_FATAL << "Illegal type: " << type;
  return "Unknown";
}
string Tag::AttributesName(uint32 attr) {
  string ret = "";
#define CONSIDER_ATTR(A) \
  if ( (A & attr) != 0 ) {\
    ret += #A[5];\
  } else { \
    ret += '-';\
  }
  CONSIDER_ATTR(ATTR_METADATA);
  CONSIDER_ATTR(ATTR_AUDIO);
  CONSIDER_ATTR(ATTR_VIDEO);
  CONSIDER_ATTR(ATTR_DROPPABLE);
  CONSIDER_ATTR(ATTR_CAN_RESYNC);
#undef CONSIDER_ATTR
  return ret;
}

///////////////////////////////////////////////////////////////////////////

synch::MutexPool Tag::mutex_pool_(Tag::kNumMutexes);
synch::MutexPool TagSet::mutex_pool_(TagSet::kNumMutexes);

///////////////////////////////////////////////////////////////////////////

StreamTimeCalculator::StreamTimeCalculator()
  : last_tag_ts_(0),
    media_time_ms_(0),
    stream_time_ms_(0) {
}
StreamTimeCalculator::~StreamTimeCalculator() {
}

void StreamTimeCalculator::Reset(int64 timestamp_ms) {
  last_tag_ts_ = timestamp_ms;

  media_time_ms_ = 0;
  stream_time_ms_ = 0;
}

void StreamTimeCalculator::ProcessTag(const Tag* tag) {
  // the timestamp of these tags is not relevant
  if ( tag->type() == Tag::TYPE_FLV_HEADER ||
      tag->type() == Tag::TYPE_BOOTSTRAP_BEGIN ||
      tag->type() == Tag::TYPE_BOOTSTRAP_END ||
      tag->type() == Tag::TYPE_EOS ||
      tag->type() == Tag::TYPE_SOURCE_ENDED ) {
    return;
  }

  /*
  if ( tag->type() == Tag::TYPE_SOURCE_STARTED ) {
    LOG_ERROR << last_tag_ts_ << " -> " << media_time_ms_ << "/" << stream_time_ms_;
  }
  if ( tag->type() == Tag::TYPE_SEGMENT_STARTED ) {
    LOG_ERROR << last_tag_ts_ << " -> " << media_time_ms_ << "/" << stream_time_ms_;
  }
  if ( tag->type() == Tag::TYPE_SEEK_PERFORMED ) {
    LOG_ERROR << last_tag_ts_ << " -> " << media_time_ms_ << "/" << stream_time_ms_;
  }
  */

  if ( tag->type() == Tag::TYPE_SOURCE_STARTED ) {
    last_tag_ts_ = tag->timestamp_ms();
  }

  int64 delta = tag->timestamp_ms() - last_tag_ts_;
  if ( tag->type() == Tag::TYPE_SEGMENT_STARTED ) {
    int64 media_time_ms =
        static_cast<const SegmentStartedTag*>(tag)->media_timestamp_ms();
    media_time_ms_ += media_time_ms - media_time_ms_;
  } else {
    media_time_ms_ += delta;
  }

  stream_time_ms_ += delta;
  last_tag_ts_ = tag->timestamp_ms();

  /*
  if ( tag->type() == Tag::TYPE_SOURCE_STARTED ) {
    LOG_ERROR << tag->ToString();
    LOG_ERROR << last_tag_ts_ << " -> " << media_time_ms_ << "/" << stream_time_ms_ << ", delta: " << delta;
  }
  if ( tag->type() == Tag::TYPE_SEGMENT_STARTED ) {
    LOG_ERROR << tag->ToString();
    LOG_ERROR << last_tag_ts_ << " -> " << media_time_ms_ << "/" << stream_time_ms_ << ", delta: " << delta;
  }
  if ( tag->type() == Tag::TYPE_SEEK_PERFORMED ) {
    LOG_ERROR << tag->ToString();
    LOG_ERROR << last_tag_ts_ << " -> " << media_time_ms_ << "/" << stream_time_ms_ << ", delta: " << delta;
  }
  */
}

}
