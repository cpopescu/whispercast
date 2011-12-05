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

#ifndef __MEDIA_F4V_TO_FLV_H__
#define __MEDIA_F4V_TO_FLV_H__

#include <whisperstreamlib/f4v/f4v_tag.h>
#include <whisperstreamlib/flv/flv_tag.h>
#include <whisperstreamlib/f4v/f4v_encoder.h>

#include <whisperstreamlib/f4v/atoms/movie/moov_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/esds_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/avcc_atom.h>

namespace streaming {
class F4vToFlvConverter {
 public:
  F4vToFlvConverter();
  ~F4vToFlvConverter();

  // outputs the F4vTag 'tag' equivalent as 0 or more FlvTags
  // into 'out_flv_tags'.
  void ConvertTag(const F4vTag* tag,
                  vector< scoped_ref<FlvTag> >* out_flv_tags);

  FlvTag* CreateCuePoint(const streaming::f4v::FrameHeader& frame_header,
                         int64 timestamp,
                         uint32 cue_point_number);
 private:

  FlvTag* CreateAudioTag(int64 timestamp,
                         bool is_aac_header,
                         const io::MemoryStream& data);
  FlvTag* CreateVideoTag(int64 timestamp,
                         bool is_avc_header,
                         bool is_keyframe,
                         const io::MemoryStream& data);
  void ConvertAtom(int64 timestamp,
                   const f4v::BaseAtom* atom,
                   vector< scoped_ref<FlvTag> >* flv_tags);

  void AppendTag(FlvTag* flv_tag,
                 vector< scoped_ref<FlvTag> >* flv_tags);
  void ConvertMoov(int64 timestamp,
                   const f4v::MoovAtom* moov,
                   vector< scoped_ref<FlvTag> >* flv_tags);
  FlvTag* GetEsds(int64 timestamp,
                  const f4v::EsdsAtom* esds);
  FlvTag* GetAvcc(int64 timestamp,
                  const f4v::AvccAtom* avcc);
  FlvTag* GetMetadata(int64 timestamp,
                      const f4v::MoovAtom* moov);

  int32 previous_tag_size_;
  f4v::Encoder f4v_encoder_;

  DISALLOW_EVIL_CONSTRUCTORS(F4vToFlvConverter);
};

////////////////////////////////////////////////////////////////////////////
// converts a F4V file into a FLV file. Only the container is changed,
// audio&video format stays unchanged.
bool ConvertF4vToFlv(const string& in_f4v_file, const string& out_flv_file);

} // namespace streaming

#endif  // __MEDIA_F4V_TO_FLV_H__
