// Copyright (c) 2009, Whispersoft s.r.l
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

#include "f4v_to_flv_converter_element.h"

namespace streaming {

F4vToFlvConverterElement::ClientCallbackData::ClientCallbackData(
    const string& creation_path)
  : FilteringCallbackData(),
    creation_path_(creation_path),
    converter_(),
    cue_point_number_(0) {
}
F4vToFlvConverterElement::ClientCallbackData::~ClientCallbackData() {
}

const string&
F4vToFlvConverterElement::ClientCallbackData::creation_path() const {
  return creation_path_;
}

scoped_ref<FlvTag>
F4vToFlvConverterElement::ClientCallbackData::PrepareCuePoint(FlvTag* cue_point,
    uint32 flavour_mask, int64 timestamp_ms) {
  cue_point->set_attributes(Tag::ATTR_METADATA);
  cue_point->set_flavour_mask(flavour_mask);
  cue_point->set_timestamp_ms(timestamp_ms);
  return cue_point;
}

void F4vToFlvConverterElement::ClientCallbackData::FilterTag(
    const streaming::Tag* tag, TagList* out) {
  scoped_ref<FlvTag> cue_point;
  if ( tag->type() == Tag::TYPE_F4V ) {
    const streaming::F4vTag* f4v_tag =
        static_cast<const streaming::F4vTag*>(tag);

    // convert F4V tag into one or more FLV tags
    vector< scoped_ref<FlvTag> > converted_flv;
    converter_.ConvertTag(f4v_tag, &converted_flv);
    for ( uint32 i = 0; i < converted_flv.size(); i++ ) {
      scoped_ref<FlvTag>& flv_tag = converted_flv[i];
      flv_tag->set_attributes(tag->attributes());
      flv_tag->set_flavour_mask(tag->flavour_mask());
      flv_tag->set_timestamp_ms(tag->timestamp_ms());
      out->push_back(flv_tag.get());
      // forward the converted tags
    }
    converted_flv.clear();

    if ( f4v_tag->is_frame() &&
         f4v_tag->frame()->header().is_keyframe() ) {
      // forward cue point
      out->push_front(PrepareCuePoint(converter_.CreateCuePoint(
            f4v_tag->frame()->header(), 0, cue_point_number_),
          tag->flavour_mask(), tag->timestamp_ms()).get());
      ++cue_point_number_;
    }
    return;
  }
  if ( tag->type() == Tag::TYPE_COMPOSED ) {
    const ComposedTag* ctd = static_cast<const ComposedTag*>(tag);
    if ( ctd->sub_tag_type() == Tag::TYPE_F4V ) {
      ComposedTag* new_ctd = new ComposedTag(
          0, streaming::kDefaultFlavourMask, ctd->timestamp_ms());
      new_ctd->set_flavour_mask(tag->flavour_mask());
      vector< scoped_ref<FlvTag> > converted_flv;
      for ( int i = 0; i < ctd->tags().size(); ++i ) {
        const streaming::Tag* crt_tag = ctd->tags().tag(i).get();
        const streaming::F4vTag* f4v_tag =
            static_cast<const streaming::F4vTag*>(crt_tag);
        converted_flv.clear();

        converter_.ConvertTag(f4v_tag, &converted_flv);
        if ( f4v_tag->is_frame() &&
             f4v_tag->frame()->header().is_keyframe() ) {
          // forward cue point
          out->push_front(PrepareCuePoint(converter_.CreateCuePoint(
                f4v_tag->frame()->header(), 0, cue_point_number_),
              tag->flavour_mask(), tag->timestamp_ms()).get());
          ++cue_point_number_;
        }
        // insert converted tags into the new composed tag
        for ( uint32 j = 0; j < converted_flv.size(); j++ ) {
          scoped_ref<FlvTag>& flv_tag = converted_flv[j];
          flv_tag->set_attributes(crt_tag->attributes());
          flv_tag->set_flavour_mask(crt_tag->flavour_mask());
          flv_tag->set_timestamp_ms(crt_tag->timestamp_ms());
          new_ctd->add_prepared_tag(flv_tag.release());
        }
        converted_flv.clear();
      }
      // forward the new composed tag
      out->push_back(new_ctd);
      return;
    }
  }

  // default: forward original tag
  out->push_back(tag);

  return;
}

//////////////////////////////////////////////////////////////////////////////

const char F4vToFlvConverterElement::kElementClassName[] =
  "f4v_to_flv_converter";

F4vToFlvConverterElement::F4vToFlvConverterElement(
    const char* name, const char* id, ElementMapper* mapper,
    net::Selector* selector)
    : FilteringElement(kElementClassName, name, id, mapper, selector),
      default_caps_(streaming::Tag::TYPE_FLV, kDefaultFlavourMask) {
}
F4vToFlvConverterElement::~F4vToFlvConverterElement() {
}

bool F4vToFlvConverterElement::Initialize() {
  return true;
}

bool F4vToFlvConverterElement::HasMedia(const char* media_name,
                                        Capabilities* out) {
  if ( !FilteringElement::HasMedia(media_name, out) ) {
    return true;
  }
  if (out->tag_type_ == streaming::Tag::TYPE_F4V) {
    out->tag_type_ = default_caps_.tag_type_;
  }
  return true;
}

void F4vToFlvConverterElement::ListMedia(const char* media_dir,
                                         ElementDescriptions* media) {
  int initial_size = media->size();
  FilteringElement::ListMedia(media_dir, media);
  for ( int i = initial_size; i < media->size(); ++i ) {
    if ( (*media)[i].second.tag_type_ == streaming::Tag::TYPE_F4V) {
      (*media)[i].second.tag_type_ = default_caps_.tag_type_;
    }
  }
}

bool F4vToFlvConverterElement::AddRequest(const char* media,
    streaming::Request* req, streaming::ProcessingCallback* callback) {
  if ( !req->caps().IsCompatible(default_caps_) ) {
    LOG_ERROR << name() << ": Incompatible caps"
                 ", req: " << req->caps().ToString()
              << ", ours: " << default_caps_.ToString();
    return false;
  }
  Tag::Type original_tag_type = req->caps().tag_type_;
  req->mutable_caps()->tag_type_ = streaming::Tag::TYPE_F4V;
  if ( !FilteringElement::AddRequest(media, req, callback) ) {
    req->mutable_caps()->tag_type_ = original_tag_type;
    return false;
  }
  LOG_INFO << "Modifyined caps tag_type from ["
           << original_tag_type << " to TYPE_F4V on request media: ["
           << req->info().path_ << "]";
  return true;
}

FilteringCallbackData* F4vToFlvConverterElement::CreateCallbackData(
    const char* media_name, streaming::Request* req) {
  return new ClientCallbackData(media_name);
}

void F4vToFlvConverterElement::DeleteCallbackData(FilteringCallbackData* data) {
  ClientCallbackData* client = static_cast<ClientCallbackData*>(data);
  FilteringElement::DeleteCallbackData(client);
}

}
