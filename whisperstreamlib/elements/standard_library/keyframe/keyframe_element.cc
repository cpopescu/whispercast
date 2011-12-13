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

#include "elements/standard_library/keyframe/keyframe_element.h"

///////////////////////////////////////////////////////////////////////
//
// Helper FilteringCallbackData
//

namespace {

class KeyFrameExtractionState {
 public:
  KeyFrameExtractionState(int64 ms_between_video_frames,
                          bool drop_audio)
      : ms_between_video_frames_(ms_between_video_frames),
        drop_audio_(drop_audio),
        last_extracted_keyframe_time_offset_(-0x7fffffff) {
  }
  KeyFrameExtractionState(const KeyFrameExtractionState& s)
      : ms_between_video_frames_(s.ms_between_video_frames_),
        drop_audio_(s.drop_audio_),
        last_extracted_keyframe_time_offset_(-0x7fffffff) {
  }

  // true = forward
  // false = drop
  bool Filter(const streaming::Tag* tag, int64 timestamp_ms);

 private:
  const int64 ms_between_video_frames_;
  const bool drop_audio_;

  // The timestamp of the last keyframe tag that was extracted.
  int64 last_extracted_keyframe_time_offset_;
};

bool KeyFrameExtractionState::Filter(const streaming::Tag* tag,
                                     int64 timestamp_ms) {
  if ( tag->is_audio_tag() ) {    // maybe drop audio
    return !drop_audio_;
  }
  if ( tag->is_video_tag() ) {
    if ( !tag->can_resync() ) {   // drop interframes
      return false;
    }
    if ( timestamp_ms > last_extracted_keyframe_time_offset_ &&
         timestamp_ms - last_extracted_keyframe_time_offset_
         < ms_between_video_frames_ ) {
      return false;               // drop keyframes outside the time window
    }
    // now is the time to forward current frame -> pass
    last_extracted_keyframe_time_offset_ = timestamp_ms;
    return true;
  }
  // forward everything else
  return true;
}

//////////////////////////////////////////////////////////////////////

class KeyFrameExtractorCallbackData : public streaming::FilteringCallbackData {
 public:
  KeyFrameExtractorCallbackData(int64 ms_between_video_frames,
                                bool drop_audio);
  virtual ~KeyFrameExtractorCallbackData();

  /////////////////////////////////////////////////////////////////////
  //
  // FilteringCallbackData methods
  //
  virtual void FilterTag(const streaming::Tag* tag,
                         int64 timestamp_ms,
                         TagList* out);

  virtual bool Unregister(streaming::Request* req);
 private:
  virtual void RegisterFlavour(int flavour_id) {
    CHECK_NULL(droppers_[flavour_id]);
    droppers_[flavour_id] = new KeyFrameExtractionState(params_);
  }

  KeyFrameExtractionState params_;
  KeyFrameExtractionState* droppers_[streaming::kNumFlavours];

  DISALLOW_EVIL_CONSTRUCTORS(KeyFrameExtractorCallbackData);
};

KeyFrameExtractorCallbackData::KeyFrameExtractorCallbackData(
    int64 ms_between_video_frames,
    bool drop_audio)
  : FilteringCallbackData(),
    params_(ms_between_video_frames,
            drop_audio) {
  for ( int i = 0; i < NUMBEROF(droppers_); ++i ) {
    droppers_[i] = NULL;
  }
}

KeyFrameExtractorCallbackData::~KeyFrameExtractorCallbackData() {
  for ( int i = 0; i < NUMBEROF(droppers_); ++i ) {
    delete droppers_[i];
  }
}

bool KeyFrameExtractorCallbackData::Unregister(streaming::Request* req) {
  if ( FilteringCallbackData::Unregister(req) ) {
    for ( int i = 0; i < NUMBEROF(droppers_); ++i ) {
      delete droppers_[i];
      droppers_[i] = NULL;
    }
    return true;
  }
  return false;
}

void KeyFrameExtractorCallbackData::FilterTag(const streaming::Tag* tag,
                                              int64 timestamp_ms,
                                              TagList* out) {
  if ( tag->type() == streaming::Tag::TYPE_EOS ) {
    out->push_back(FilteredTag(tag, timestamp_ms));
    return; // forward tag
  }

  // Treat tag on all flavours..
  uint32 flavours_to_keep = 0;
  uint32 flavour_mask = tag->flavour_mask();
  while ( flavour_mask ) {
    const int id = streaming::RightmostFlavourId(
        flavour_mask);  // id in range [0, 31]
    if ( droppers_[id] == NULL ) {
      RegisterFlavour(id);
    }
    if ( droppers_[id]->Filter(tag, timestamp_ms) ) {
      flavours_to_keep |= (1 << id);
    }
  }

  if ( !flavours_to_keep ) {
    // drop tag
    return;
  }
  // forward tag
  if ( flavours_to_keep != tag->flavour_mask() ) {
    streaming::Tag* const t = tag->Clone();
    t->set_flavour_mask(flavours_to_keep);
    out->push_back(FilteredTag(t, timestamp_ms));
    return;
  }
  out->push_back(FilteredTag(tag, timestamp_ms));
}
}

namespace streaming {
const char KeyFrameExtractorElement::kElementClassName[] = "keyframe";

//////////////////////////////////////////////////////////////////////
//
//  KeyFrameExtractorElement
//
KeyFrameExtractorElement::KeyFrameExtractorElement(
    const char* name,
    const char* id,
    ElementMapper* mapper,
    net::Selector* selector,
    const char* media_filtered,
    int64 ms_between_video_frames,
    bool drop_audio)
    : FilteringElement(kElementClassName, name, id, mapper, selector),
      media_filtered_(media_filtered != NULL ? media_filtered : ""),
      ms_between_video_frames_(ms_between_video_frames),
      drop_audio_(drop_audio),
      internal_req_(new streaming::Request()),
      process_tag_callback_(NULL) {
  internal_req_->mutable_info()->internal_id_ = id;
}
KeyFrameExtractorElement::~KeyFrameExtractorElement() {
  ClearBootstrap();
  if ( process_tag_callback_ ) {
    mapper_->RemoveRequest(internal_req_, process_tag_callback_);
    delete process_tag_callback_;
  }
}

bool KeyFrameExtractorElement::Initialize() {
  if ( !media_filtered_.empty() ) {
    process_tag_callback_ =
        NewPermanentCallback(this, &KeyFrameExtractorElement::ProcessTag);
    if ( !mapper_->AddRequest(media_filtered_.c_str(),
                              internal_req_,
                              process_tag_callback_) ) {
      LOG_ERROR << name() << ":  Dropping source cannot register to underneat "
                << "filtered media: " << media_filtered_;
      delete process_tag_callback_;
      process_tag_callback_ = NULL;
      return false;
    }
  }
  return true;
}

void KeyFrameExtractorElement::SetBootstrap(const streaming::Tag* tag) {
  uint32 flavour_mask = tag->flavour_mask();
  while ( flavour_mask )  {
    const int id = streaming::RightmostFlavourId(flavour_mask);
    last_extracted_keyframe_[id] = tag;
  }
}

void KeyFrameExtractorElement::PlayBootstrap(
    streaming::ProcessingCallback* callback,
    int64 timestamp_ms,
    uint32 flavour_mask) {
  while ( flavour_mask )  {
    const int id = RightmostFlavourId(flavour_mask);
    if ( last_extracted_keyframe_[id].get() != NULL ) {
      DLOG_DEBUG << name() << " Bootstraping: "
                 << last_extracted_keyframe_[id]->ToString();
      callback->Run(last_extracted_keyframe_[id].get(), timestamp_ms);
    }
  }
}
void KeyFrameExtractorElement::ClearBootstrap() {
  for ( int i = 0; i < NUMBEROF(last_extracted_keyframe_); ++i ) {
    last_extracted_keyframe_[i] = NULL;
  }
}

void KeyFrameExtractorElement::ProcessTag(const Tag* tag, int64 timestamp_ms) {
  if ( tag->type() == streaming::Tag::TYPE_EOS ) {
    mapper_->RemoveRequest(internal_req_, process_tag_callback_);
    delete process_tag_callback_;
    process_tag_callback_ = NULL;
    return;
  }
  if ( tag->type() == streaming::Tag::TYPE_SOURCE_ENDED ) {
    ClearBootstrap();
  }
  if ( tag->is_video_tag() && tag->can_resync() ) {
    SetBootstrap(tag);
  }
}

FilteringCallbackData* KeyFrameExtractorElement::CreateCallbackData(
    const char* media_name,
    streaming::Request* req) {
  if ( !media_filtered_.empty() && media_filtered_ != media_name ) {
    LOG_DEBUG << name() << ": Key frame extractor element cannot create "
              << " an element for: " << media_name << " as it bootstraps from: "
              << media_filtered_;
    return NULL;
  }
  return new KeyFrameExtractorCallbackData(ms_between_video_frames_,
                                           drop_audio_);
}

bool KeyFrameExtractorElement::AddRequest(const char* media,
                                          streaming::Request* req,
                                          streaming::ProcessingCallback* callback) {
  VLOG(10) << "KeyFrameExtractorElement adding callback for name: ["
           << name() << "]";
  if ( !FilteringElement::AddRequest(media, req, callback) ) {
    return false;
  }
  // TODO: cpopescu --- Possible crash when if request is removed in between..
  selector_->RunInSelectLoop(NewCallback(
                                 this,
                                 &KeyFrameExtractorElement::PlayBootstrap,
                                 callback,
                                 (int64)0,
                                 req->caps().flavour_mask_));
  return true;
}

}
