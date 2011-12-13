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

// @@@ NEEDS REFACTORING - CLOSE/TAG DISTRIBUTION

#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperstreamlib/base/tag_distributor.h>
#include <whisperstreamlib/base/consts.h>
#include <whisperstreamlib/elements/standard_library/splitting/splitting_element.h>
#include <whisperstreamlib/raw/raw_tag_splitter.h>

namespace {

class SplittingElementCallbackData
    : public streaming::FilteringCallbackData {
 public:
  SplittingElementCallbackData(net::Selector* selector,
                               streaming::Capabilities caps,
                               streaming::TagSplitter* splitter,
                               int32 max_tag_size)
      : streaming::FilteringCallbackData(),
        selector_(selector),
        caps_(caps),
        max_tag_size_(max_tag_size),
        splitter_(splitter),
        distributor_(caps_.flavour_mask_, "") {
  }
  virtual ~SplittingElementCallbackData() {
    distributor_.CloseAllCallbacks(true);
    delete splitter_;
  }

  /////////////////////////////////////////////////////////////////////

  virtual bool Register(streaming::Request* req) {
    if ( !caps_.IsCompatible(req->caps()) ) {
      LOG_WARNING << " Incompatible request caps: "
                  << req->caps().ToString()
                  << " vs. " << caps_.ToString();
      return false;
    }
    if ( (req->caps().flavour_mask_ &
          (req->caps().flavour_mask_ - 1)) != 0 ) {
      LOG_WARNING << " Can resolve just one flavour in a Splitting element.";
      return false;
    }
    const streaming::Tag::Type saved_tag_type = req->caps().tag_type_;
    req->mutable_caps()->tag_type_ = streaming::Tag::TYPE_RAW;
    if ( FilteringCallbackData::Register(req) ) {
      req->mutable_caps()->tag_type_ = saved_tag_type;
      req->mutable_caps()->IntersectCaps(caps_);
      return true;
    }
    return false;
  }

  virtual void FilterTag(const streaming::Tag* tag,
                         int64 timestamp_ms,
                         TagList* out);

 private:
  void ProcessFilteredTag(const streaming::Tag* tag,
                          int64 timestamp_ms,
                          TagList* tags);

  net::Selector* const selector_;
  streaming::Capabilities caps_;
  const int32 max_tag_size_;
  streaming::TagSplitter* const splitter_;
  streaming::TagDistributor distributor_;
  io::MemoryStream data_;

  DISALLOW_EVIL_CONSTRUCTORS(SplittingElementCallbackData);
};

void SplittingElementCallbackData::FilterTag(const streaming::Tag* tag,
                                             int64 timestamp_ms,
                                             TagList* out) {
  if ( tag->type() == streaming::Tag::TYPE_RAW ||
       tag->type() == streaming::Tag::TYPE_EOS ) {
    // forward 0 or more tags
    ProcessFilteredTag(tag, timestamp_ms, out);
    return;
  }
  // default: forward tag
  out->push_back(FilteredTag(tag, timestamp_ms));
}

void SplittingElementCallbackData::ProcessFilteredTag(
    const streaming::Tag* tag,
    int64 timestamp_ms,
    TagList* tags) {
  // forward EOS
  if ( tag->type() == streaming::Tag::TYPE_EOS ) {
    tags->push_back(FilteredTag(tag, timestamp_ms));
    return;
  }
  // extract data from RAW tags
  if ( tag->type() == streaming::Tag::TYPE_RAW ) {
    const streaming::RawTag* raw_tag =
        static_cast<const streaming::RawTag*>(tag);
    data_.AppendStreamNonDestructive(raw_tag->data());
  }
  // decode tags from raw data
  while ( true ) {
    int64 timestamp_ms;
    scoped_ref<streaming::Tag> new_tag;
    streaming::TagReadStatus status = splitter_->GetNextTag(
        &data_, &new_tag, &timestamp_ms, false);
    if ( status == streaming::READ_SKIP ||
         status == streaming::READ_CORRUPTED_CONT ) {
      continue;
    }
    if ( status == streaming::READ_NO_DATA ) {
      if ( data_.Size() > max_tag_size_ ) {
        LOG_ERROR << " Media Error on oversized buffer"
                  << " for element: " << media_name()
                  << " size: " << data_.Size();
        if ( tag->type() != streaming::Tag::TYPE_EOS ) {
          tags->push_back(FilteredTag(new streaming::EosTag(
              0, distributor_.flavour_mask(), false), last_tag_ts_));
        }
      }
      break;
    }
    if ( status != streaming::READ_OK ) {
      // error cases
      LOG_ERROR << "Error reading next tag, on element: " << media_name()
                << ", status: " << streaming::TagReadStatusName(status);
      tags->push_back(FilteredTag(new streaming::EosTag(
          0, distributor_.flavour_mask(), false), last_tag_ts_));
      return;
    }
    CHECK_EQ(status, streaming::READ_OK);
    CHECK_NOT_NULL(new_tag.get());
    tags->push_back(FilteredTag(new_tag.get(), timestamp_ms));
    last_tag_ts_ = timestamp_ms;
  }
}
}

namespace streaming {
const char SplittingElement::kElementClassName[] = "splitting";

SplittingElement::SplittingElement(
    const char* name,
    const char* id,
    ElementMapper* mapper,
    net::Selector* selector,
    streaming::SplitterCreator* splitter_creator,
    streaming::Tag::Type splitter_tag_type,
    int32 max_tag_size)
    :  FilteringElement(kElementClassName, name, id, mapper, selector),
       splitter_creator_(splitter_creator),
       caps_(splitter_tag_type, streaming::kAllFlavours),
       max_tag_size_(max_tag_size) {
}

SplittingElement::~SplittingElement() {
  delete splitter_creator_;
}

FilteringCallbackData* SplittingElement::CreateCallbackData(
    const char* media_name, Request* req) {
  streaming::TagSplitter* splitter =
      splitter_creator_->CreateSplitter(name_, caps_.tag_type_);
  if ( splitter == NULL ) {
    return NULL;
  }
  return new SplittingElementCallbackData(selector_,
                                          caps_,
                                          splitter,
                                          max_tag_size_);
}

}
