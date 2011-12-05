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

#ifndef __MEDIA_FLV_FLV_SPLITTER_H__
#define __MEDIA_FLV_FLV_SPLITTER_H__

#include <vector>
#include <list>
#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperstreamlib/flv/flv_tag.h>
#include <whisperstreamlib/base/tag.h>
#include <whisperstreamlib/base/tag_splitter.h>

// IMPORTANT : this is not thread safe !!

namespace streaming {

//////////////////////////////////////////////////////////////////////

// This is a function that decodes flv tags from a stream.
// Upon decoding, it calls a bunch of registered callbacks to process them.
class FlvTagSplitter : public streaming::TagSplitter {
 public:
  static const Type kType;
 public:
  // name: just for logging
  FlvTagSplitter(const string& name);
  virtual ~FlvTagSplitter();

 protected:
  virtual streaming::TagReadStatus GetNextTagInternal(
      io::MemoryStream* in, scoped_ref<Tag>* tag, bool is_at_eos);

 private:
  // Maybe extracts cue data from the metadata 'flv_tag'.
  scoped_ref<CuePointTag> RetrieveCuePoints(const FlvTag::Metadata& metadata,
      int64 timestamp);

  // Terminates bootstrapping
  void EndBootstrapping();
  // This function adjust: duration, size, offset, unseekable, according
  // to limits/seek/controller.
  void UpdateMetadata(FlvTag::Metadata& metadata);

  int64 first_tag_timestamp_ms_;
  int64 tag_timestamp_ms_;

  // The tags to send next (we may put extra tags in this deque)
  list< scoped_ref<Tag> > tags_to_send_next_;

  // incoming stream properties, learned from flv header.
  bool has_audio_;
  bool has_video_;

  // wait at most this many tags to extract media info.
  static const uint32 kMediaInfoMaxWait = 100;

  // we need the first audio + video + metadata tag to extract media info
  scoped_ref<FlvTag> first_audio_;
  scoped_ref<FlvTag> first_video_;
  scoped_ref<FlvTag> first_metadata_;

  bool media_info_extracted_;

  bool bootstrapping_;

  DISALLOW_EVIL_CONSTRUCTORS(FlvTagSplitter);
};
}

#endif  // __MEDIA_FLV_FLV_SPLITTER_H__
