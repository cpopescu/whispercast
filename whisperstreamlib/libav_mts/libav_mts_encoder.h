// Copyright (c) 2013, Whispersoft s.r.l.
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

#ifndef __MEDIA_LIBAV_MTS_ENCODER_H__
#define __MEDIA_LIBAV_MTS_ENCODER_H__

#include <whisperstreamlib/base/media_info.h>
#include <whisperstreamlib/base/tag.h>
#include <whisperstreamlib/base/tag_serializer.h>

class AVFormatContext;
class AVStream;
class AVBitStreamFilterContext;
class AVPacket;

namespace streaming {
namespace libav_mts {

// globally initialize libav
void GlobalInit();

class Encoder {
 public:
  Encoder();
  virtual ~Encoder();

  bool IsInitialized() const;

  // Write headers to output stream.
  // If the encoder is already initialized, it re-initialized to the new info.
  // The Encoder can be initialized either by calling Initialize(),
  // or by sending a MediaInfoTag to using WriteTag()
  bool Initialize(const MediaInfo& info, io::MemoryStream* out);

  // Write tag to output stream.
  // If the tag is a MediaInfoTag, it will Initialize the encoder.
  bool WriteTag(const streaming::Tag& tag, int64 ts, io::MemoryStream* out);

  // Write trailer to output stream.
  bool Finalize(io::MemoryStream* out);

 private:
  bool AddAudioStream(const MediaInfo::Audio& info);
  bool AddVideoStream(const MediaInfo::Video& info);
  static int IOWrite(void* opaque, uint8_t* buf, int buf_size) {
    Encoder* This = reinterpret_cast<Encoder*>(opaque);
    CHECK_NOT_NULL(This->crt_out_) << " crt_out_ NOT set";
    return This->crt_out_->Write(buf, buf_size);
  }

  // free libav resources
  void Cleanup();

  // utility functions.
  // Returns a description of the libav error code.
  static string av_err2str(int err);
  // Returns libav codec name
  static const char* CodecIDName(uint32 codec_id);
  // Destructs an libav packet
  static void DestructPacket(AVPacket* pkt);

 private:
  AVFormatContext* av_context_;
  AVStream* video_st_;
  AVStream* audio_st_;
  AVBitStreamFilterContext* video_filter_;

  static const uint32 kIOBufSize = 32 * 1024;
  uint8* io_buf_;

  // NOTE: libav bug: calling av_write_trailer() without a previous
  //       avformat_write_header() causes a crash in libav.
  //
  // true = avformat_write_header() completed successfully;
  bool headers_written_;

  // current output stream
  io::MemoryStream* crt_out_;
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
  Encoder encoder_;
};

}

typedef libav_mts::Serializer LibavMtsTagSerializer;
}

#endif // __MEDIA_LIBAV_MTS_ENCODER_H__
