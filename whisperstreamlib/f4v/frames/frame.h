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

#ifndef __MEDIA_F4V_F4V_FRAME_H__
#define __MEDIA_F4V_F4V_FRAME_H__

#include <whisperlib/common/io/input_stream.h>
#include <whisperlib/common/io/buffer/io_memory_stream.h>
#include <whisperstreamlib/f4v/frames/header.h>
#include <whisperstreamlib/f4v/f4v_types.h>

namespace streaming {
namespace f4v {

class Decoder;
class Encoder;

class Frame {
 public:
  Frame(const FrameHeader& header);
  Frame(const Frame& other);
  virtual ~Frame();

  const FrameHeader& header() const;
  const io::MemoryStream& data() const;
  io::MemoryStream& mutable_data();

  TagDecodeStatus Decode(io::MemoryStream& in, Decoder& decoder);
  void Encode(io::MemoryStream& out, Encoder& encoder) const;

  // Returns a new copy of this Frame.
  // Delete it when you're done.
  Frame* Clone() const;

  // returns a Frame description, useful for logging
  string ToString() const;

 private:
  FrameHeader header_;
  io::MemoryStream data_;
};

} // namespace f4v
} // namespace streaming

#endif // __MEDIA_F4V_F4V_FRAME_H__
