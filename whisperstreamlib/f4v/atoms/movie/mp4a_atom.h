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
#ifndef __MEDIA_F4V_ATOMS_MOVIE_MP4A_ATOM_H__
#define __MEDIA_F4V_ATOMS_MOVIE_MP4A_ATOM_H__

#include <string>
#include <whisperlib/common/io/buffer/io_memory_stream.h>
#include <whisperstreamlib/f4v/atoms/container_versioned_atom.h>

namespace streaming {
namespace f4v {

class EsdsAtom;
class WaveAtom;
//class ChanAtom;

class Mp4aAtom : public ContainerVersionedAtom {
 public:
  static const AtomType kType = ATOM_MP4A;
 public:
  Mp4aAtom();
  Mp4aAtom(const Mp4aAtom& other);
  virtual ~Mp4aAtom();

  uint16 data_reference_index() const;
  uint16 inner_version() const;
  uint16 revision_level() const;
  uint32 vendor() const;
  uint16 number_of_channels() const;
  uint16 sample_size_in_bits() const;
  int16 compression_id() const;
  uint16 packet_size() const;
  FPU16U16 sample_rate() const;
  uint32 samples_per_packet() const;
  uint32 bytes_per_packet() const;
  uint32 bytes_per_frame() const;
  uint32 bytes_per_sample() const;

  void set_unknown(uint16 unknown);
  void set_data_reference_index(uint16 data_reference_index);
  void set_inner_version(uint16 inner_version);
  void set_revision_level(uint16 revision_level);
  void set_vendor(uint32 vendor);
  void set_number_of_channels(uint16 number_of_channels);
  void set_sample_size_in_bits(uint16 sample_size_in_bits);
  void set_compression_id(int16 compression_id);
  void set_packet_size(uint16 packet_size);
  void set_sample_rate(FPU16U16 sample_rate);
  void set_samples_per_packet(uint32 samples_per_packet);
  void set_bytes_per_packet(uint32 bytes_per_packet);
  void set_bytes_per_frame(uint32 bytes_per_frame);
  void set_bytes_per_sample(uint32 bytes_per_sample);

  const EsdsAtom* esds() const;
  const WaveAtom* wave() const;

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
  io::MemoryStream raw_data_;

  uint16 unknown_;
  uint16 data_reference_index_;
  uint16 inner_version_;
  uint16 revision_level_;
  uint32 vendor_;
  uint16 number_of_channels_;
  uint16 sample_size_in_bits_;
  int16 compression_id_;
  uint16 packet_size_;
  FPU16U16 sample_rate_; // 32-bit unsigned fixed-point number (16.16).
  uint32 samples_per_packet_;
  uint32 bytes_per_packet_;
  uint32 bytes_per_frame_;
  uint32 bytes_per_sample_;

  EsdsAtom* esds_;
  WaveAtom* wave_;
  //ChanAtom* chan_;
};
}
}

#endif // __MEDIA_F4V_ATOMS_MOVIE_MP4A_ATOM_H__
