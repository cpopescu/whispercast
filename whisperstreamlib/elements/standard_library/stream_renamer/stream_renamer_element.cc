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

#include "elements/standard_library/stream_renamer/stream_renamer_element.h"

namespace streaming {
const char StreamRenamerElement::kElementClassName[] = "stream_renamer";

namespace {
class StreamRenamerCallbackData : public streaming::FilteringCallbackData {
 public:
  StreamRenamerCallbackData(re::RE& re, const string& replace)
    : FilteringCallbackData(),
      re_(re),
      replace_(replace){
    private_flags_ |= Flag_DontAppendNameToPath;
  }

  virtual ~StreamRenamerCallbackData() {
  }

  /////////////////////////////////////////////////////////////////////
  //
  // FilteringCallbackData methods
  //
  virtual void FilterTag(const Tag* tag, int64 timestamp_ms,  TagList* out) {
    if ( tag->type() == streaming::Tag::TYPE_SOURCE_STARTED ||
         tag->type() == streaming::Tag::TYPE_SOURCE_ENDED ) {
      const streaming::SourceChangedTag* source_change =
        static_cast<const streaming::SourceChangedTag*>(tag);
      const string old_stream_name = source_change->source_element_name();
      string new_stream_name;
      if ( !re_.Replace(old_stream_name, replace_, new_stream_name) ) {
        LOG_WARNING << "No match for pattern: [" << re_.regex() << "] and"
                       " stream_name: [" << old_stream_name << "]";
        // forward tag
        out->push_back(FilteredTag(tag, timestamp_ms));
        return;
      }

      string new_path, old_path = source_change->path();
      size_t pos = 0;
      while (true) {
        size_t new_pos = old_path.find(old_stream_name, pos);
        if (new_pos != string::npos) {
          new_path = old_path.substr(pos, new_pos-pos)+new_stream_name;
          pos += old_stream_name.length();
        } else {
          new_path += old_path.substr(pos);
          break;
        }
      }
      VLOG(10) << "StreamRenamerElement renamed: \"" << old_stream_name
        << "\" to \"" << new_stream_name << "\", \"" << old_path
        << "\" to \"" << new_path << "\"...";

      // Alter the TYPE_SOURCE_STARTED tag:
      //  change stream name to new_stream_name.
      //

      SourceChangedTag* td = static_cast<SourceChangedTag*>(
          source_change->Clone());
      td->set_source_element_name(new_stream_name);
      td->set_path(new_path);
      // forward tag
      out->push_back(FilteredTag(td, timestamp_ms));
      return;
    }
    // default: forward tag
    out->push_back(FilteredTag(tag, timestamp_ms));
  }
  virtual bool Unregister(streaming::Request* req) {
    return FilteringCallbackData::Unregister(req);
  }

 private:
  re::RE& re_;
  const string replace_;
  DISALLOW_EVIL_CONSTRUCTORS(StreamRenamerCallbackData);
};
}

//////////////////////////////////////////////////////////////////////
//
//  StreamRenamerElement
//
StreamRenamerElement::StreamRenamerElement(
    const string& name,
    ElementMapper* mapper,
    net::Selector* selector,
    const string& pattern,
    const string& replace)
    : FilteringElement(kElementClassName, name, mapper, selector),
      pattern_(pattern),
      replace_(replace),
      re_(pattern_) {
}
StreamRenamerElement::~StreamRenamerElement() {
}

bool StreamRenamerElement::Initialize() {
  return true;
}

FilteringCallbackData* StreamRenamerElement::CreateCallbackData(
    const string& media_name,
    streaming::Request* req) {
  return new StreamRenamerCallbackData(re_, replace_);
}

bool StreamRenamerElement::AddRequest(const string& media, Request* req,
                                      ProcessingCallback* callback) {
  VLOG(10) << "StreamRenamerElement adding callback for name: ["
           << name() << "]";
  if ( !FilteringElement::AddRequest(media, req, callback) ) {
    return false;
  }
  return true;
}

}
