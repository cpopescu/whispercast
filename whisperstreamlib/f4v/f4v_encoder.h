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

#ifndef __MEDIA_F4V_F4V_ENCODER_H__
#define __MEDIA_F4V_F4V_ENCODER_H__

#include <list>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperstreamlib/base/media_info.h>
#include <whisperstreamlib/base/tag.h>
#include <whisperstreamlib/base/tag_serializer.h>

namespace streaming {
namespace f4v {

class Tag;
class BaseAtom;
class Frame;

class Encoder {
public:
  Encoder() {
  }
  virtual ~Encoder() {
  }

  // Write f4v tag 'in' in output stream 'out'.
  void WriteTag(io::MemoryStream& out, const Tag& in);

  // Specializations for WriteTag.
  void WriteAtom(io::MemoryStream& out, const BaseAtom& in);
  void WriteFrame(io::MemoryStream& out, const Frame& in);
};

// A simple wrapper of Encoder to implement streaming::TagSerializer
class Serializer : public streaming::TagSerializer {
 public:
  Serializer();
  virtual ~Serializer();

  ////////////////////////////////////////////////////////////////////////
  // methods from streaming::TagSerializer
  //
  virtual void Initialize(io::MemoryStream* out);
  virtual void Finalize(io::MemoryStream* out);
 protected:
  virtual bool SerializeInternal(const streaming::Tag* tag,
                                 int64 timestamp_ms,
                                 io::MemoryStream* out);
 private:
  uint64 FramesSize() const;
  static bool IsFrameMatch(const streaming::Tag* tag,
                           int64 tag_ts,
                           const MediaInfo::Frame& frame,
                           string* out_err);
 private:
  Encoder encoder_;
  // expected frames.
  // When MOOV is serialized, this array is filled.
  // The next incoming tags must coincide with these expected frames, so that
  // the output file is correct. When a mismatch is found, SerializeInternal()
  // returns false and the serialization should be aborted.
  vector<MediaInfo::Frame> frames_;
  uint32 frame_index_;
  // we have to write the Mdat header just once, before the frames
  bool mdat_header_written_;
  // bug trap. Set true on the first failed SerializeInternal().
  bool failed_;
};

}

typedef f4v::Serializer F4vTagSerializer;

}

#endif // __MEDIA_F4V_F4V_ENCODER_H__
