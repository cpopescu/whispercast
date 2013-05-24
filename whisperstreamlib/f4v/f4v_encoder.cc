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

#include <whisperlib/common/base/types.h>
#include <whisperstreamlib/base/media_info_util.h>
#include <whisperstreamlib/f4v/f4v_encoder.h>
#include <whisperstreamlib/f4v/f4v_tag.h>
#include <whisperstreamlib/f4v/f4v_util.h>
#include <whisperstreamlib/f4v/atoms/base_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/ftyp_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/mdat_atom.h>
#include <whisperstreamlib/f4v/frames/frame.h>

namespace streaming {
namespace f4v {

void Encoder::WriteTag(io::MemoryStream& out, const Tag& in) {
  if ( in.is_atom() ) {
    WriteAtom(out, *in.atom());
    return;
  }
  CHECK(in.is_frame());
  WriteFrame(out, *in.frame());
}
void Encoder::WriteAtom(io::MemoryStream& out, const BaseAtom& in) {
  in.Encode(out, *this);
}
void Encoder::WriteFrame(io::MemoryStream& out, const Frame& in) {
  in.Encode(out, *this);
}

////////////////////////////////////////////////////////////////////////

Serializer::Serializer()
  : TagSerializer(MFORMAT_F4V),
    encoder_(),
    frames_(),
    frame_index_(0),
    mdat_header_written_(false),
    failed_(false) {
}
Serializer::~Serializer() {
}

void Serializer::Initialize(io::MemoryStream* out) {
}
void Serializer::Finalize(io::MemoryStream* out) {
  frames_.clear();
  frame_index_ = 0;
  mdat_header_written_ = false;
  failed_ = false;
}
bool Serializer::SerializeInternal(const streaming::Tag* tag,
                                   int64 timestamp_ms,
                                   io::MemoryStream* out) {
  CHECK(!failed_) << "Serialization already failed";
  if ( tag->type() == Tag::TYPE_MEDIA_INFO ) {
    const MediaInfoTag* inf = static_cast<const MediaInfoTag*>(tag);
    // write FTYP atom
    f4v::FtypAtom ftyp(FourCC<'m', 'p', '4', '2'>::value, 1);
    ftyp.add_compatible_brand(FourCC<'m', 'p', '4', '2'>::value);
    ftyp.add_compatible_brand(FourCC<'m', 'p', '4', '1'>::value);
    ftyp.UpdateSize();
    encoder_.WriteAtom(*out, ftyp);

    // compose MOOV
    scoped_ref<F4vTag> f4v_tag;
    if ( !streaming::util::ComposeMoov(inf->info(), ftyp.size(), &f4v_tag) ) {
      LOG_ERROR << "Failed to compose MOOV from: " << tag->ToString();
      failed_ = true;
      return false;
    }

    // write MOOV atom
    encoder_.WriteTag(*out, *f4v_tag.get());

    // set the expected frames
    frames_ = inf->info().frames();

    return true;
  }
  // Free Atoms go through here (otherwise we have to recompute frames offset)
  if ( tag->type() == Tag::TYPE_F4V ) {
    const F4vTag* f4v_tag = static_cast<const F4vTag*>(tag);
    if ( f4v_tag->is_atom() ) {
      const BaseAtom* atom = f4v_tag->atom();
      // ignore FTYP (already serialized through Initialize())
      if ( atom->type() == ATOM_FTYP ) {
        return true;
      }
      if ( atom->type() == ATOM_FREE ) {
        LOG_WARNING << "### FREE atom";
      }
      LOG_WARNING << "### Serialize F4V: " << f4v_tag->ToString();
      encoder_.WriteAtom(*out, *atom);
      return true;
    }
  }
  // serialize frames, but first check that they match the previous MOOV
  if ( tag->is_audio_tag() || tag->is_video_tag() ) {
    CHECK_NOT_NULL(tag->Data());
    if ( frames_.empty() ) {
      LOG_ERROR << "Unexpected frame, no previous MOOV";
      failed_ = true;
      return false;
    }
    if ( !mdat_header_written_ ) {
      // write MDAT atom header
      MdatAtom mdat;
      mdat.set_body_size(FramesSize());
      encoder_.WriteAtom(*out, mdat);
      mdat_header_written_ = true;
    }
    string err;
    if ( !IsFrameMatch(tag, timestamp_ms, frames_[frame_index_], &err) ) {
      LOG_ERROR << "Frame mismatch: (err: " << err << ")\n"
                   " - got: " << tag->ToString() << "\n"
                   " - expected: " << frames_[frame_index_].ToString();

      // RAW frames go through here
      LOG_WARNING << "Ignoring the error, passing the mismatched frame through...";
      out->AppendStreamNonDestructive(tag->Data());
      return true;

      //failed_ = true;
      //return false;
    }
    out->AppendStreamNonDestructive(tag->Data());
    frame_index_++;
    return true;
  }
  return true;
}
uint64 Serializer::FramesSize() const {
  uint64 size = 0;
  for ( vector<MediaInfo::Frame>::const_iterator it = frames_.begin();
      it != frames_.end(); ++it ) {
    size += it->size_;
  }
  return size;
}
bool Serializer::IsFrameMatch(const streaming::Tag* tag,
                              int64 tag_ts,
                              const MediaInfo::Frame& frame,
                              string* out_err) {
  if ( tag->is_audio_tag() && !frame.is_audio_ ) {
    *out_err = "Audio vs Video";
    return false;
  }
  if ( tag->is_video_tag() && frame.is_audio_ ) {
    *out_err = "Video vs Audio";
    return false;
  }
  if ( tag->Data() == NULL ) {
    *out_err = "No Data(), not a media frame";
    return false;
  }
  if ( tag->Data()->Size() != frame.size_ ) {
    *out_err = "Different size";
    return false;
  }
  if ( tag_ts != frame.decoding_ts_ ) {
    *out_err = "Different timestamps";
    return false;
  }
  return true;
}
}
}
