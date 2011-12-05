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

#ifndef __MEDIA_F4V_ATOMS_MOVIE_MVHD_ATOM_H__
#define __MEDIA_F4V_ATOMS_MOVIE_MVHD_ATOM_H__

#include <string>
#include <whisperlib/common/io/buffer/io_memory_stream.h>
#include <whisperstreamlib/f4v/atoms/versioned_atom.h>

namespace streaming {
namespace f4v {

class MvhdAtom : public VersionedAtom {
 public:
  static const AtomType kType = ATOM_MVHD;
  static const uint32 kVersionedBodySize_0;
  static const uint32 kVersionedBodySize_1;
 public:
  MvhdAtom();
  MvhdAtom(const MvhdAtom& other);
  virtual ~MvhdAtom();

  uint64 creation_time() const;
  uint64 modification_time() const;
  uint32 time_scale() const;
  uint64 duration() const;
  uint32 preferred_rate() const;
  uint16 preferred_volume() const;
  uint32 preview_time() const;
  uint32 preview_duration() const;
  uint32 poster_time() const;
  uint32 selection_time() const;
  uint32 selection_duration() const;
  uint32 current_time() const;
  uint32 next_track_id() const;

  void set_creation_time(uint64 creation_time);
  void set_modification_time(uint64 modification_time);
  void set_time_scale(uint32 time_scale);
  void set_duration(uint64 duration);
  void set_preferred_rate(uint32 preferred_rate);
  void set_preferred_volume(uint16 preferred_volume);
  void set_reserved(const uint8* data, uint32 size/*[10]*/);
  void set_matrix_structure(const uint8* data, uint32 size/*[36]*/);
  void set_preview_time(uint32 preview_time);
  void set_preview_duration(uint32 preview_duration);
  void set_poster_time(uint32 poster_time);
  void set_selection_time(uint32 selection_time);
  void set_selection_duration(uint32 selection_duration);
  void set_current_time(uint32 current_time);
  void set_next_track_id(uint32 next_track_id);

  ///////////////////////////////////////////////////////////////////////////
  // Methods from VersionedAtom
  virtual void GetSubatoms(vector<const BaseAtom*>& subatoms) const;
  virtual BaseAtom* Clone() const;
  virtual TagDecodeStatus DecodeVersionedBody(uint64 size, io::MemoryStream& in,
                                              Decoder& decoder);
  virtual void EncodeVersionedBody(io::MemoryStream& out, Encoder& encoder) const;
  virtual uint64 MeasureVersionedBodySize() const;
  virtual string ToStringVersionedBody(uint32 indent) const;

 private:
  uint64 creation_time_;     // seconds since January 1, 1904 UTC; 32b/v0, 64b/v1
  uint64 modification_time_; // seconds since January 1, 1904 UTC; 32b/v0, 64b/v1
  uint32 time_scale_;
  uint64 duration_;          // duration in time_scale_ units; 32b/v0, 64b/v1
  uint32 preferred_rate_;
  uint16 preferred_volume_;
  uint8 reserved_[10];
  uint8 matrix_structure_[36];
  uint32 preview_time_;
  uint32 preview_duration_;
  uint32 poster_time_;
  uint32 selection_time_;
  uint32 selection_duration_;
  uint32 current_time_;
  uint32 next_track_id_;
};
}
}

#endif // __MEDIA_F4V_ATOMS_MOVIE_MVHD_ATOM_H__
