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

#ifndef __MEDIA_RTP_RTP_PACKETIZATION_H__
#define __MEDIA_RTP_RTP_PACKETIZATION_H__

#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperstreamlib/rtp/rtp_consts.h>

namespace streaming {
namespace rtp {

// Classes for splitting audio/video data into packets of kRtpMtu max size.

class Packetizer {
public:
  enum Type {
    H263,
    H264,
    MP4A,
    MP3,
    SPLIT,
  };
  static const char* TypeName(Type t);

public:
  Packetizer(Type t);
  virtual ~Packetizer();

  Type type() const;
  const char* type_name() const;

  // returns: true - OK
  //          false - invalid stream data
  virtual bool Packetize(const io::MemoryStream& in,
                         vector<io::MemoryStream*>* out) = 0;
  // returns packetizer description
  string ToString();
private:
  Type type_;
};

class H263Packetizer : public Packetizer {
public:
  static const Type kType = H263;
  H263Packetizer() : Packetizer(kType) {};
  virtual ~H263Packetizer() {};
  ///////////////////////////////////////////////////////////////////////////
  // Packetizer methods
  virtual bool Packetize(const io::MemoryStream& in,
                         vector<io::MemoryStream*>* out);
};

class H264Packetizer : public Packetizer {
public:
  static const Type kType = H264;
  H264Packetizer(bool flv_format)
    : Packetizer(kType), is_flv_format_(flv_format) {};
  virtual ~H264Packetizer() {};
  ///////////////////////////////////////////////////////////////////////////
  // Packetizer methods
  virtual bool Packetize(const io::MemoryStream& in,
                         vector<io::MemoryStream*>* out);
private:
  static bool SplitNalu(io::MemoryStream& in,
      uint8 nalu_size_bytes,
      vector<io::MemoryStream*>* out);
private:
  // H264 is encoded into FLV like this:
  // <7 bytes unknown> <2 bytes NALU size> <size bytes NALU body>
  const bool is_flv_format_;

  // H264 is encoded into MP4 like this:
  // <4 bytes NALU size> <size bytes NALU body>
};

class Mp4aPacketizer : public Packetizer {
public:
  static const Type kType = MP4A;
  Mp4aPacketizer(bool flv_format)
    : Packetizer(kType), is_flv_format_(flv_format) {};
  virtual ~Mp4aPacketizer() {};
  ///////////////////////////////////////////////////////////////////////////
  // Packetizer methods
  virtual bool Packetize(const io::MemoryStream& in,
                         vector<io::MemoryStream*>* out);
private:
  // AAC is encoded into FLV like this: <2 byte unknown> <aac frame>
  const bool is_flv_format_;
};

class Mp3Packetizer : public Packetizer {
public:
  static const Type kType = MP3;
  Mp3Packetizer(bool flv_format)
    : Packetizer(kType), is_flv_format_(flv_format) {};
  virtual ~Mp3Packetizer() {};
  ///////////////////////////////////////////////////////////////////////////
  // Packetizer methods
  virtual bool Packetize(const io::MemoryStream& in,
                         vector<io::MemoryStream*>* out);
private:
  // MP3 is encoded into FLV like this:
  // <1 byte unknown: 0x2e usually> <4 bytes mp3 header> <mp3 data>
  const bool is_flv_format_;
};

// simply split in kRtpMtu size buffers
class SplitPacketizer : public Packetizer {
public:
  static const Type kType = SPLIT;
  SplitPacketizer() : Packetizer(kType) {};
  virtual ~SplitPacketizer() {};
  ///////////////////////////////////////////////////////////////////////////
  // Packetizer methods
  virtual bool Packetize(const io::MemoryStream& in,
                         vector<io::MemoryStream*>* out);
};

} // namespace rtp
} // namespace streaming

#endif // __MEDIA_RTP_RTP_PACKETIZATION_H__
