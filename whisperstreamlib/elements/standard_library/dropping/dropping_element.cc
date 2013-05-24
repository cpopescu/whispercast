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

#include "elements/standard_library/dropping/dropping_element.h"

namespace {
class StreamDroppingUtil {
 public:
  StreamDroppingUtil(
      int64 audio_accept_period_ms,
      int64 audio_drop_period_ms,
      int64 video_accept_period_ms,
      int64 video_drop_period_ms,
      int32 video_grace_period_key_frames)
      : audio_accept_period_ms_(audio_accept_period_ms),
        audio_drop_period_ms_(audio_drop_period_ms),
        video_accept_period_ms_(video_accept_period_ms),
        video_drop_period_ms_(video_drop_period_ms),
        video_grace_period_key_frames_(video_grace_period_key_frames),
        video_grace_period_key_frames_sent_(0),
        first_audio_tag_(true),
        first_video_tag_(true),
        video_key_frame_sent_(false),
        is_dropping_video_(false),
        next_time_to_switch_video_(video_accept_period_ms),
        audio_key_frame_sent_(false),
        is_dropping_audio_(false),
        next_time_to_switch_audio_(audio_accept_period_ms) {
  }
  StreamDroppingUtil(const StreamDroppingUtil& s)
      : audio_accept_period_ms_(s.audio_accept_period_ms_),
        audio_drop_period_ms_(s.audio_drop_period_ms_),
        video_accept_period_ms_(s.video_accept_period_ms_),
        video_drop_period_ms_(s.video_drop_period_ms_),
        video_grace_period_key_frames_(s.video_grace_period_key_frames_),
        video_grace_period_key_frames_sent_(0),
        first_audio_tag_(true),
        first_video_tag_(true),
        video_key_frame_sent_(false),
        is_dropping_video_(false),
        next_time_to_switch_video_(s.next_time_to_switch_video_),
        audio_key_frame_sent_(false),
        is_dropping_audio_(false),
        next_time_to_switch_audio_(s.next_time_to_switch_audio_) {
  }

  ~StreamDroppingUtil() {
  }

  // Returns:
  //   true = forward
  //   false = drop
  bool Filter(const streaming::Tag* tag, int64 timestamp_ms);

 private:
  // We have periods of accept and periods of dropping. Each acceptance period
  // begins with a key frame and takes the *_accept_period_ms_. After that,
  // we start dropping for at lease *_drop_period_ms_.
  // If video_grace_period_key_frames is specified we accept all video until
  // we got these many key frames sent.

  // Parameter members:
  const int64 audio_accept_period_ms_;
  const int64 audio_drop_period_ms_;
  const int64 video_accept_period_ms_;
  const int64 video_drop_period_ms_;
  const int32 video_grace_period_key_frames_;

  // State members:
  int32 video_grace_period_key_frames_sent_;
  bool first_audio_tag_;
  bool first_video_tag_;

  bool video_key_frame_sent_;
  bool is_dropping_video_;
  int64 next_time_to_switch_video_;

  bool audio_key_frame_sent_;
  bool is_dropping_audio_;
  int64 next_time_to_switch_audio_;
};

bool StreamDroppingUtil::Filter(const streaming::Tag* tag, int64 timestamp_ms) {
  if ( tag->type() == streaming::Tag::TYPE_SOURCE_STARTED ) {
    if ( is_dropping_video_ )
      next_time_to_switch_video_ = timestamp_ms;
    if ( is_dropping_audio_ )
      next_time_to_switch_audio_ = timestamp_ms;
  }

  bool to_drop = false;

  if ( tag->type() == streaming::Tag::TYPE_COMPOSED ) {
    to_drop = true;  // we always drop these things
  } else {
    ////////////////////

    // FIRST TAG SETUP

    if ( first_audio_tag_ ) {
      first_audio_tag_ = false;
      is_dropping_audio_ = audio_accept_period_ms_ <= 0;
      next_time_to_switch_audio_ =
          audio_drop_period_ms_ > 0 ?
          timestamp_ms + audio_accept_period_ms_ : kMaxInt64;
    }
    if ( first_video_tag_ ) {
      if ( video_grace_period_key_frames_ <=
             video_grace_period_key_frames_sent_ ) {
        first_video_tag_ = false;
        is_dropping_video_ = video_accept_period_ms_ <= 0;
        next_time_to_switch_video_ =
            video_drop_period_ms_ > 0 ?
            timestamp_ms + video_accept_period_ms_ : kMaxInt64;
      }
    }

    ////////////////////

    // VIDEO

    if ( tag->is_video_tag() ) {
      if ( first_video_tag_ ) {
        if ( tag->can_resync() ) {
          video_grace_period_key_frames_sent_++;
        }
        return true;
      }
      if ( timestamp_ms >= next_time_to_switch_video_ ) {
        if ( is_dropping_video_ && video_accept_period_ms_ > 0 ) {
          if ( tag->can_resync() ) {
              video_key_frame_sent_ = true;
              is_dropping_video_ = false;
              next_time_to_switch_video_ = (timestamp_ms +
                                            video_accept_period_ms_);
          }
        } else if ( !is_dropping_video_ && video_key_frame_sent_ ) {
          is_dropping_video_ = true;
          video_key_frame_sent_ = false;
          next_time_to_switch_video_ = (timestamp_ms +
                                        video_drop_period_ms_);
        }
      }
      video_key_frame_sent_ |= tag->can_resync() && !is_dropping_video_;
      if ( is_dropping_video_ ) {
        to_drop = true;
      }
    } else if ( tag->is_audio_tag() ) {
      ////////////////////

      // AUDIO     (damn I hate code cut&past)..

      if ( timestamp_ms >= next_time_to_switch_audio_ ) {
        if ( is_dropping_audio_ && audio_accept_period_ms_ > 0 ) {
            if ( tag->can_resync() ) {
              audio_key_frame_sent_ = true;
              is_dropping_audio_ = false;
              next_time_to_switch_audio_ = (timestamp_ms +
                                            audio_accept_period_ms_);
            }
        } else if ( !is_dropping_audio_ && audio_key_frame_sent_ ) {
          is_dropping_audio_ = true;
          audio_key_frame_sent_ = false;
          next_time_to_switch_audio_ = (timestamp_ms +
                                        audio_drop_period_ms_);
        }
      }
      audio_key_frame_sent_ |= tag->can_resync() && !is_dropping_audio_;
      if ( is_dropping_audio_ ) {
        to_drop = true;
      }
    }
  }
  return !to_drop;
}

//////////////////////////////////////////////////////////////////////

class DroppingElementCallbackData : public streaming::FilteringCallbackData {
 public:
  DroppingElementCallbackData(
      int64 audio_accept_period_ms,
      int64 audio_drop_period_ms,
      int64 video_accept_period_ms,
      int64 video_drop_period_ms,
      int32 video_grace_period_key_frames);
  virtual ~DroppingElementCallbackData();

  /////////////////////////////////////////////////////////////////////
  // FilteringCallbackData methods
  virtual void FilterTag(const streaming::Tag* tag,
                         int64 timestamp_ms,
                         TagList* out);
  virtual bool Unregister(streaming::Request* req);
 protected:
  virtual void RegisterFlavour(int flavour_id) {
    CHECK_NULL(droppers_[flavour_id]);
    droppers_[flavour_id] = new StreamDroppingUtil(params_);
  }

 private:
  StreamDroppingUtil params_;
  StreamDroppingUtil* droppers_[streaming::kNumFlavours];
 private:
  DISALLOW_EVIL_CONSTRUCTORS(DroppingElementCallbackData);
};

DroppingElementCallbackData::DroppingElementCallbackData(
    int64 audio_accept_period_ms,
    int64 audio_drop_period_ms,
    int64 video_accept_period_ms,
    int64 video_drop_period_ms,
    int32 video_grace_period_key_frames)
    : FilteringCallbackData(),
      params_(audio_accept_period_ms,
              audio_drop_period_ms,
              video_accept_period_ms,
              video_drop_period_ms,
              video_grace_period_key_frames) {
  for ( int i = 0; i < NUMBEROF(droppers_); ++i ) {
    droppers_[i] = NULL;
  }
}

DroppingElementCallbackData::~DroppingElementCallbackData() {
  for ( int i = 0; i < NUMBEROF(droppers_); ++i ) {
    delete droppers_[i];
  }
}


bool DroppingElementCallbackData::Unregister(streaming::Request* req) {
  if ( FilteringCallbackData::Unregister(req) ) {
    for ( int i = 0; i < NUMBEROF(droppers_); ++i ) {
      delete droppers_[i];
      droppers_[i] = NULL;
    }
    return true;
  }
  return false;
}

void DroppingElementCallbackData::FilterTag(const streaming::Tag* tag,
                                            int64 timestamp_ms,
                                            TagList* out) {
  if ( tag->type() == streaming::Tag::TYPE_EOS ) {
    out->push_back(FilteredTag(tag, timestamp_ms));
    return;
  }
  // Treat tag on all flavours..
  uint32 flavours_to_keep = 0;
  uint32 flavour_mask = tag->flavour_mask();
  while ( flavour_mask )  {
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
  // forward the tag
  if ( flavours_to_keep != tag->flavour_mask() ) {
    streaming::Tag* t = tag->Clone();
    t->set_flavour_mask(flavours_to_keep);
    out->push_back(FilteredTag(t, timestamp_ms));
    return;
  }
  out->push_back(FilteredTag(tag, timestamp_ms));
}
}

namespace streaming {

const char DroppingElement::kElementClassName[] = "dropping";

//////////////////////////////////////////////////////////////////////
//
//  DroppingElement
//
DroppingElement::DroppingElement(const string& name,
                                 ElementMapper* mapper,
                                 net::Selector* selector,
                                 const string& media_filtered,
                                 int64 audio_accept_period_ms,
                                 int64 audio_drop_period_ms,
                                 int64 video_accept_period_ms,
                                 int64 video_drop_period_ms,
                                 int32 video_grace_period_key_frames)
    : FilteringElement(kElementClassName, name, mapper, selector),
      media_filtered_(media_filtered),
      audio_accept_period_ms_(audio_accept_period_ms),
      audio_drop_period_ms_(audio_drop_period_ms),
      video_accept_period_ms_(video_accept_period_ms),
      video_drop_period_ms_(video_drop_period_ms),
      video_grace_period_key_frames_(video_grace_period_key_frames),
      internal_req_(new streaming::Request()),
      process_tag_callback_(NULL) {
}

DroppingElement::~DroppingElement() {
  ClearBootstrap();
  if ( process_tag_callback_ ) {
    // internal_req_ gets deleted internally here:
    mapper_->RemoveRequest(internal_req_, process_tag_callback_);
    delete process_tag_callback_;
  }
}
void DroppingElementInitialize(DroppingElement* elem) {
  elem->Initialize();
}
bool DroppingElement::Initialize() {
  CHECK(process_tag_callback_ == NULL);
  if ( !media_filtered_.empty() ) {
    process_tag_callback_ =
        NewPermanentCallback(this, &DroppingElement::ProcessTag);
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

void DroppingElement::PlayBootstrap(streaming::ProcessingCallback* callback,
                                    int64 timestamp_ms,
                                    uint32 flavour_mask) {
  while ( flavour_mask )  {
    const int id = RightmostFlavourId(flavour_mask);
    if ( video_bootstrap_[id].get() != NULL ) {
      DLOG_DEBUG << name() << " Bootstraping: "
                 << video_bootstrap_[id]->ToString();
      callback->Run(video_bootstrap_[id].get(), timestamp_ms);
    }
  }
}

void DroppingElement::ClearBootstrap() {
  for ( int i = 0; i < NUMBEROF(video_bootstrap_); ++i ) {
    video_bootstrap_[i] = NULL;
  }
}
void DroppingElement::ProcessTag(const Tag* tag, int64 timestamp_ms) {
  if ( tag->type() == streaming::Tag::TYPE_EOS ) {
    mapper_->RemoveRequest(internal_req_, process_tag_callback_);
    delete process_tag_callback_;
    process_tag_callback_ = NULL;
    // We need to re-add it ..
    selector_->RunInSelectLoop(
        NewCallback(&DroppingElementInitialize, this));
    return;
  }
  if ( tag->type() == streaming::Tag::TYPE_SOURCE_ENDED ) {
    ClearBootstrap();
  }
  if ( tag->is_video_tag() && tag->can_resync() ) {
    uint32 flavour_mask = tag->flavour_mask();
    while ( flavour_mask )  {
      const int id = RightmostFlavourId(flavour_mask);
      video_bootstrap_[id] = tag;
    }
  }
}

FilteringCallbackData* DroppingElement::CreateCallbackData(
    const string& media_name,
    streaming::Request* req) {
  if ( !media_filtered_.empty() && media_filtered_ != media_name ) {
    LOG_WARNING << name() << ": Dropping element cannot create an element "
                << " for: [" << media_name << "] as it bootstraps from: "
                << media_filtered_;
    return NULL;
  }
  return new DroppingElementCallbackData(audio_accept_period_ms_,
                                         audio_drop_period_ms_,
                                         video_accept_period_ms_,
                                         video_drop_period_ms_,
                                         video_grace_period_key_frames_);
}

bool DroppingElement::AddRequest(const string& media, Request* req,
                                 ProcessingCallback* callback) {
  VLOG(10) << "DroppingElement adding callback for name: [" << name() << "]";
  if ( !FilteringElement::AddRequest(media, req, callback) ) {
    return false;
  }
  // TODO: cpopescu --- Possible crash when if request is removed in between..
  selector_->RunInSelectLoop(NewCallback(this,
                                         &DroppingElement::PlayBootstrap,
                                         callback,
                                         (int64)0,
                                         req->caps().flavour_mask_));
  return true;
}

}
