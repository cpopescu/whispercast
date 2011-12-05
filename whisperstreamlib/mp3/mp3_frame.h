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
// Author: Catalin Popescu

#ifndef __MEDIA_MP3_MP3_FRAME_H__
#define __MEDIA_MP3_MP3_FRAME_H__

#include <string>
#include <whisperlib/common/base/types.h>
#include <whisperstreamlib/base/consts.h>
#include <whisperstreamlib/base/tag.h>

namespace io {
class MemoryStream;
}

namespace streaming {

class Mp3FrameTag : public streaming::Tag {
 public:
  enum Version {
    MPEG_VERSION_1   = 0,
    MPEG_VERSION_2   = 1,
    MPEG_VERSION_2_5 = 2,
    VERSION_UNKNOWN  = 3,
  };
  static const char* VersionName(Version v);
  enum Layer {
    MPEG_LAYER_I    = 0,
    MPEG_LAYER_II   = 1,
    MPEG_LAYER_III  = 2,
    LAYER_UNKNOWN   = 3,
  };
  static const char* LayerName(Layer l);
  enum ChannelMode {
    STEREO          = 0,
    JOINT_STEREO    = 1,
    DUAL_CHANNEL    = 2,
    SINGLE_CHANNEL  = 3,
    CHANNEL_UNKNOWN = 4,
  };
  static const char* ChannelModeName(ChannelMode c);

  // Length in mytes of an mp3 header
  static const int kMp3HeaderLen;

  static const Type kType;
  Mp3FrameTag(uint32 attributes,
              uint32 flavour_mask,
              int64 timestamp_ms)
    : Tag(kType, attributes, flavour_mask),
      version_(VERSION_UNKNOWN),
      layer_(LAYER_UNKNOWN),
      channel_mode_(CHANNEL_UNKNOWN),
      has_crc_(false),
      bitrate_kbps_(0),
      sampling_rate_hz_(0),
      padding_(false),
      frame_length_bytes_(0),
      data_(),
      timestamp_ms_(timestamp_ms) {
  }
  Mp3FrameTag(const Mp3FrameTag& other)
    : Tag(other),
      version_(other.version_),
      layer_(other.layer_),
      channel_mode_(other.channel_mode_),
      has_crc_(other.has_crc_),
      bitrate_kbps_(other.bitrate_kbps_),
      sampling_rate_hz_(other.sampling_rate_hz_),
      padding_(other.padding_),
      frame_length_bytes_(other.frame_length_bytes_),
      data_(),
      timestamp_ms_(other.timestamp_ms_) {
    data_.AppendStreamNonDestructive(&other.data_);
  }

  // Reads frame data from the given input stream. Call it multiple times
  // until READ_OK is returned. On READ_OK it advances the stream pointer,
  // else will leave it as it has received it.
  // If synchronize is set, we synchronize w/ the first frame
  streaming::TagReadStatus Read(io::MemoryStream* in, bool do_synchronize);

  // Extracts the header data from a kMp3HeaderLen buffer
  streaming::TagReadStatus ExtractHeader(const uint8 buffer[4]);

  // Computes the length (in miliseconds) of this frame.
  int64 TimeLengthInMs() const;

  Version version() const { return version_; }
  Layer layer() const { return layer_; }
  ChannelMode channel_mode() const { return channel_mode_; }
  bool has_crc() const { return has_crc_; }
  int bitrate_kbps() const { return bitrate_kbps_; }
  int sampling_rate_hz() const { return sampling_rate_hz_; }
  bool padding() const { return padding_; }
  int frame_length_bytes() const { return frame_length_bytes_; }
  const io::MemoryStream& data() const { return data_; }
  void set_timestamp_ms(int64 timestamp_ms) {
    timestamp_ms_ = timestamp_ms;
  }

  const char* VersionName() const { return VersionName(version_); }
  const char* LayerName() const { return LayerName(layer_); }
  const char* ChannelModeName() const { return ChannelModeName(channel_mode_); }
  virtual int64 timestamp_ms() const { return timestamp_ms_; }
  virtual int64 duration_ms() const { return TimeLengthInMs(); }
  virtual uint32 size() const { return data_.Size(); }
  virtual Tag* Clone(int64 timestamp_ms) const {
    return new Mp3FrameTag(*this);
  }

  virtual string ToStringBody() const;

 private:
  // Data extracted from the mp3 frame header
  Version     version_;
  Layer       layer_;
  ChannelMode channel_mode_;
  bool        has_crc_;
  int         bitrate_kbps_;
  int         sampling_rate_hz_;
  bool        padding_;
  // Data computed from the above extracted data
  int         frame_length_bytes_;

  // The raw data of the frame
  io::MemoryStream data_;

  // tag timestmap
  int64 timestamp_ms_;
};

//////////////////////////////////////////////////////////////////////

class Mp3TagSerializer : public streaming::TagSerializer {
 public:
  Mp3TagSerializer() : streaming::TagSerializer() { }
  virtual ~Mp3TagSerializer() {}
  virtual void Initialize(io::MemoryStream* out) {}
  virtual void Finalize(io::MemoryStream* out) {}
 protected:
  virtual bool SerializeInternal(const Tag* tag, int64 base_timestamp_ms,
                                 io::MemoryStream* out) {
    if ( tag->type() != Tag::TYPE_MP3 ) {
      return false;
    }
    out->AppendStreamNonDestructive(
        &static_cast<const Mp3FrameTag*>(tag)->data());
    return true;
  }
 private:
  DISALLOW_EVIL_CONSTRUCTORS(Mp3TagSerializer);
};
}

#endif  // __MEDIA_MP3_MP3_FRAME_H__
