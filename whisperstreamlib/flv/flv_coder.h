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

#ifndef __MEDIA_FLV_FLV_CODER_H__
#define __MEDIA_FLV_FLV_CODER_H__

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperstreamlib/flv/flv_consts.h>
#include <whisperstreamlib/flv/flv_tag.h>
#include <whisperstreamlib/base/consts.h>

// The static class FlvCoder knows how to wire FlvTags

namespace streaming {
class FlvCoder {
 public:
  static streaming::TagReadStatus DecodeHeader(io::MemoryStream* in,
                                               scoped_ref<FlvHeader>* header,
                                               int* cb);

  // Decodes one tag from the input stream. If READ_OK is returned, *tag
  // contains the newly allocated tag, and cb the bytes read from in.
  static streaming::TagReadStatus DecodeTag(io::MemoryStream* in,
                                            scoped_ref<FlvTag>* tag,
                                            int* cb);

  // Extracts video flags from the given frame.
  // The frame is the the body of a Flv Video Tag.
  static TagReadStatus DecodeVideoFlags(const io::MemoryStream& frame,
      FlvFlagVideoCodec* video_codec,
      FlvFlagVideoFrameType* video_frame_type,
      AvcPacketType* video_avc_packet_type,
      int32* video_avc_composition_offset_ms);

  // Extracts audio flags from the given frame.
  // The frame is the the body of a Flv Audio Tag.
  static TagReadStatus DecodeAudioFlags(const io::MemoryStream& frame,
      FlvFlagSoundType* audio_type,
      FlvFlagSoundFormat* audio_format_,
      FlvFlagSoundRate* audio_rate,
      FlvFlagSoundSize* audio_size,
      bool* audio_is_aac_header);

  // Looks in the data included in tag and figures out if there is a moov
  // box inside, and if it it, extracts it in a separate tag.
  static FlvTag* DecodeAuxiliaryMoovTag(const FlvTag* tag);

  // Extracts raw frame from FLV tag, skips specific FLV header.
  // Returns success.
  static bool ExtractVideoCodecData(const FlvTag* tag,
                                    io::MemoryStream* out);
  // Transforms NALU from packet format (<size><nalu><size><nalu>...)
  // to byte stream format (<start_code><nalu><start_code><nalu>...)
  // Returns success.
  // For details, see: http://en.wikipedia.org/wiki/Network_Abstraction_Layer
  static bool TransformNaluFromPacketToStream(io::MemoryStream& in,
                                              AvcPacketType avc_packet_type,
                                              io::MemoryStream* out);

  // metadata: the metadata to be updated
  // min_time_ms: if the time inside metadata is below this value, ignore meta.
  // crt_time_ms: set this values inside metadata
  // is_relative: specifies that crt_time_ms is relative.
  //              The time set in metadata is always absolute.
  static bool UpdateTimeInCuePoint(FlvTag::Metadata* metadata,
                                   int64 min_time_ms,
                                   int64 crt_time_ms,
                                   bool is_relative);
};
}

#endif  // __MEDIA_FLV_FLV_CODER_H__
