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

#include <whisperlib/common/base/alarm.h>
#include <whisperstreamlib/elements/standard_library/normalizing/normalizing_element.h>

namespace {
class NormalizingStruct {
 public:
  NormalizingStruct(net::Selector* selector,
                    streaming::Request* request,
                    int64 flow_control_write_ahead_ms,
                    int64 flow_control_extra_write_ahead_ms);
  ~NormalizingStruct();

  scoped_ref<const streaming::Tag>
  ProcessTag(const streaming::Tag* tag);
 private:
  void UnpauseFlowControl(int64 element_seq_id);

  net::Selector* const selector_;
  streaming::Request* request_;
  streaming::StreamTimeCalculator stream_time_calculator_;
  const int64 flow_control_write_ahead_ms_;
  const int64 flow_control_extra_write_ahead_ms_;
  int64 element_seq_id_;

  bool is_flow_control_first_tag_;
  int64 flow_control_first_tag_time_;
  util::Alarm unpause_flow_control_alarm_;

  DISALLOW_EVIL_CONSTRUCTORS(NormalizingStruct);
};

NormalizingStruct::NormalizingStruct(
    net::Selector* selector,
    streaming::Request* request,
    int64 flow_control_write_ahead_ms,
    int64 flow_control_extra_write_ahead_ms)
    : selector_(selector),
      request_(request),
      flow_control_write_ahead_ms_(flow_control_write_ahead_ms),
      flow_control_extra_write_ahead_ms_(5000),//flow_control_extra_write_ahead_ms),
      element_seq_id_(0),
      is_flow_control_first_tag_(true),
      flow_control_first_tag_time_(0),
      unpause_flow_control_alarm_(*selector) {
}

NormalizingStruct::~NormalizingStruct() {
}

void NormalizingStruct::UnpauseFlowControl(int64 element_seq_id) {
  if ( element_seq_id != element_seq_id_ ) {
    return;
  }

  streaming::ElementController* controller = request_->controller();
  if ( controller != NULL && controller->SupportsPause() ) {
    controller->Pause(false);
  }
}

scoped_ref<const streaming::Tag>
NormalizingStruct::ProcessTag(const streaming::Tag* tag) {
  return tag;
  if ( flow_control_write_ahead_ms_ > 0 ) {
    const int64 now = selector_->now();

    if ( tag->type() == streaming::Tag::TYPE_EOS ) {
      return tag;
    }

    stream_time_calculator_.ProcessTag(tag);
    const int64 stream_time_ms = stream_time_calculator_.stream_time_ms();

    if ( tag->type() == streaming::Tag::TYPE_SOURCE_ENDED ) {
      element_seq_id_++;
    }

    if ( tag->type() == streaming::Tag::TYPE_SOURCE_STARTED ||
        tag->type() == streaming::Tag::TYPE_SEEK_PERFORMED ) {

      // restart flow control from scratch
      if ( tag->type() == streaming::Tag::TYPE_SEEK_PERFORMED ) {
        flow_control_first_tag_time_ =
            now - stream_time_ms + flow_control_write_ahead_ms_;
      }

      if ( unpause_flow_control_alarm_.IsStarted() ) {
        Closure* closure = unpause_flow_control_alarm_.Release();

        DCHECK(closure != NULL);
        DCHECK(closure->is_permanent());

        closure->Run();
        delete closure;
      }
    }
    if ( tag->type() == streaming::Tag::TYPE_EOS ) {

    }

    if ( is_flow_control_first_tag_ ) {
      flow_control_first_tag_time_ = now;
      is_flow_control_first_tag_ = false;
    } else {
      int64 real_ts = now - flow_control_first_tag_time_;
      const int64 delta =
          stream_time_ms - real_ts - flow_control_write_ahead_ms_;

      /*
      if ( delta > 2 * (flow_control_write_ahead_ms_) ) {
        LOG_ERROR << " Amazingly long delta: " << delta
                  << " flow_control_write_ahead_ms_: "
                  << flow_control_write_ahead_ms_
                  << " stream_time_ms: " << stream_time_ms
                  << " (now - flow_control_first_tag_time_): "
                  << (now - flow_control_first_tag_time_);
      */

      streaming::ElementController* controller = request_->controller();
      if ( controller != NULL &&
           controller->SupportsPause() &&
           !unpause_flow_control_alarm_.IsStarted() &&
           delta > flow_control_extra_write_ahead_ms_ ) {
        controller->Pause(true);
        unpause_flow_control_alarm_.Set(NewPermanentCallback(
            this, &NormalizingStruct::UnpauseFlowControl,
            element_seq_id_), true, delta, false, true);
      }
    }
  }
  return tag;
}


//////////////////////////////////////////////////////////////////////

class NormalizingElementCallbackData
    : public streaming::FilteringCallbackData {
 public:
  NormalizingElementCallbackData(net::Selector* selector,
                                 streaming::Request* request,
                                 int64 flow_control_write_ahead_ms,
                                 int64 flow_control_extra_write_ahead_ms);
  virtual ~NormalizingElementCallbackData();

  /////////////////////////////////////////////////////////////////////

  virtual void FilterTag(const streaming::Tag* tag, TagList* out);

 protected:
  virtual void RegisterFlavour(int flavour_id) {
    CHECK_NULL(normalizers_[flavour_id]);
    normalizers_[flavour_id] =
        new NormalizingStruct(selector_,
                              request_,
                              flow_control_write_ahead_ms_,
                              flow_control_extra_write_ahead_ms_);
  }

 private:
  net::Selector* const selector_;
  streaming::Request* request_;
  const int64 flow_control_write_ahead_ms_;
  const int64 flow_control_extra_write_ahead_ms_;

  NormalizingStruct* normalizers_[streaming::kNumFlavours];

  DISALLOW_EVIL_CONSTRUCTORS(NormalizingElementCallbackData);
};



NormalizingElementCallbackData::NormalizingElementCallbackData(
    net::Selector* selector,
    streaming::Request* request,
    int64 flow_control_write_ahead_ms,
    int64 flow_control_extra_write_ahead_ms)
    : FilteringCallbackData(),
      selector_(selector),
      request_(request),
      flow_control_write_ahead_ms_(flow_control_write_ahead_ms),
      flow_control_extra_write_ahead_ms_(flow_control_extra_write_ahead_ms) {
  for ( int i = 0; i < NUMBEROF(normalizers_); ++i ) {
    normalizers_[i] = NULL;
  }
}

NormalizingElementCallbackData::~NormalizingElementCallbackData() {
  for ( int i = 0; i < NUMBEROF(normalizers_); ++i ) {
    delete normalizers_[i];
  }
}

void NormalizingElementCallbackData::FilterTag(const streaming::Tag* tag,
    TagList* out) {
  // We normalize on all flavours
  uint32 flavour_mask = tag->flavour_mask();
  if ( req_ != NULL ) {
    flavour_mask &= req_->caps().flavour_mask_;
  }

  // while ( flavour_mask )  {
  if ( flavour_mask )  {
    const int id = streaming::RightmostFlavourId(
        flavour_mask);  // id in range [0, 31]
    if ( normalizers_[id] == NULL ) {
      RegisterFlavour(id);
    }
    scoped_ref<const streaming::Tag> tag_to_send =
        normalizers_[id]->ProcessTag(tag);
    if ( tag_to_send.get() != NULL ) {
      // forward tag
      out->push_back(tag_to_send);
    }
  }
  // default: drop tag
}
}

namespace streaming {

const char NormalizingElement::kElementClassName[] = "normalizing";

NormalizingElement::NormalizingElement(const char* name,
                                       const char* id,
                                       ElementMapper* mapper,
                                       net::Selector* selector,
                                       int64 flow_control_write_ahead_ms,
                                       int64 flow_control_extra_write_ahead_ms)
    :  FilteringElement(kElementClassName, name, id, mapper, selector),
       flow_control_write_ahead_ms_(flow_control_write_ahead_ms),
       flow_control_extra_write_ahead_ms_(flow_control_extra_write_ahead_ms) {
}
NormalizingElement::~NormalizingElement() {
}

FilteringCallbackData* NormalizingElement::CreateCallbackData(
    const char* media_name, Request* req) {
  return new NormalizingElementCallbackData(
      selector_,
      req,
      (req->mutable_info()->write_ahead_ms_ > 1) ?
          req->mutable_info()->write_ahead_ms_ : 0,
      (req->mutable_info()->write_ahead_ms_ > 1) ?
          req->mutable_info()->write_ahead_ms_/2 : 0
  );
}
}
