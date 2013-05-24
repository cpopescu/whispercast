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
#ifndef __MEDIA_F4V_ATOMS_MOVIE_AVC1_ATOM_H__
#define __MEDIA_F4V_ATOMS_MOVIE_AVC1_ATOM_H__

#include <string>
#include <whisperlib/common/io/buffer/io_memory_stream.h>
#include <whisperstreamlib/f4v/atoms/container_versioned_atom.h>

namespace streaming {
namespace f4v {

class AvccAtom;

class Avc1Atom : public ContainerVersionedAtom {
 public:
  static const AtomType kType = ATOM_AVC1;
  static const uint32 kDataSize = 39+31+4;
 public:
  Avc1Atom();
  Avc1Atom(const Avc1Atom& other);
  virtual ~Avc1Atom();

  uint16 reference_index() const;
  uint16 qt_video_encoding_version() const;
  uint16 qt_video_encoding_revision_level() const;
  uint32 qt_v_encoding_vendor() const;
  uint32 qt_video_temporal_quality() const;
  uint32 qt_video_spatial_quality() const;
  uint32 video_frame_pixel_size() const;
  uint32 horizontal_dpi() const;
  uint32 vertical_dpi() const;
  uint32 qt_video_data_size() const;
  uint16 video_frame_count() const;
  const string& video_encoder_name() const;
  uint16 video_pixel_depth() const;
  uint16 qt_video_color_table_id() const;

  void set_reserved(uint16 reserved);
  void set_reference_index(uint16 reference_index);
  void set_qt_video_encoding_version(uint16 qt_video_encoding_version);
  void set_qt_video_encoding_revision_level(uint16 qt_video_encoding_revision_level);
  void set_qt_v_encoding_vendor(uint32 qt_v_encoding_vendor);
  void set_qt_video_temporal_quality(uint32 qt_video_temporal_quality);
  void set_qt_video_spatial_quality(uint32 qt_video_spatial_quality);
  void set_video_frame_pixel_size(uint32 video_frame_pixel_size);
  void set_horizontal_dpi(uint32 horizontal_dpi);
  void set_vertical_dpi(uint32 vertical_dpi);
  void set_qt_video_data_size(uint32 qt_video_data_size);
  void set_video_frame_count(uint16 video_frame_count);
  void set_video_encoder_name(const string& video_encoder_name);
  void set_video_pixel_depth(uint16 video_pixel_depth);
  void set_qt_video_color_table_id(uint16 qt_video_color_table_id);

  const AvccAtom* avcc() const;

  ///////////////////////////////////////////////////////////////////////////
  // Methods from ContainerVersionedAtom
 protected:
  virtual void SubatomFound(BaseAtom* atom);
  virtual BaseAtom* Clone() const;
  virtual TagDecodeStatus DecodeData(uint64 size, io::MemoryStream& in,
                                     Decoder& decoder);
  virtual void EncodeData(io::MemoryStream& out, Encoder& encoder) const;
  virtual uint64 MeasureDataSize() const;
  virtual bool EqualsData(const ContainerVersionedAtom& other) const;
  virtual string ToStringData(uint32 indent) const;

 private:
  uint16 reserved_;
  uint16 reference_index_;
  uint16 qt_video_encoding_version_;
  uint16 qt_video_encoding_revision_level_;
  uint32 qt_v_encoding_vendor_;
  uint32 qt_video_temporal_quality_;
  uint32 qt_video_spatial_quality_;
  uint32 video_frame_pixel_size_;
  uint32 horizontal_dpi_;
  uint32 vertical_dpi_;
  uint32 qt_video_data_size_;
  uint16 video_frame_count_;
  string video_encoder_name_; // always 31 bytes long, padded with \0
  uint16 video_pixel_depth_;
  uint16 qt_video_color_table_id_;

  AvccAtom* avcc_;
};
}
}

#endif // __MEDIA_F4V_ATOMS_MOVIE_AVC1_ATOM_H__
