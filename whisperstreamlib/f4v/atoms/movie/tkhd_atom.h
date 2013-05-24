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

#ifndef __MEDIA_F4V_ATOMS_MOVIE_TKHD_ATOM_H__
#define __MEDIA_F4V_ATOMS_MOVIE_TKHD_ATOM_H__

#include <string>
#include <whisperlib/common/io/buffer/io_memory_stream.h>
#include <whisperstreamlib/f4v/atoms/versioned_atom.h>

namespace streaming {
namespace f4v {

class TkhdAtom : public VersionedAtom {
public:
 static const AtomType kType = ATOM_TKHD;
 static const uint32 kVersionedBodySize_0;
 static const uint32 kVersionedBodySize_1;
public:
 TkhdAtom();
 TkhdAtom(const TkhdAtom& other);
 virtual ~TkhdAtom();

 uint64 creation_time() const;
 uint64 modification_time() const;
 uint32 id() const;
 uint64 duration() const;
 uint16 layer() const;
 uint16 alternate_group() const;
 uint16 volume() const;
 FPU16U16 width() const;
 FPU16U16 height() const;

 void set_creation_time(uint64 creation_time);
 void set_modification_time(uint64 modification_time);
 void set_id(uint32 id);
 void set_reserved1(const uint8* data, uint32 size/*[4]*/);
 void set_duration(uint64 duration);
 void set_reserved2(const uint8* data, uint32 size/*[8]*/);
 void set_layer(uint16 layer);
 void set_alternate_group(uint16 alternate_group);
 void set_volume(uint16 volume);
 void set_reserved3(const uint8* data, uint32 size/*[2]*/);
 void set_matrix_structure(const uint8* data, uint32 size/*[36]*/);
 void set_width(FPU16U16 width);
 void set_height(FPU16U16 height);

 ///////////////////////////////////////////////////////////////////////////
 // Methods from VersionedAtom
 virtual bool EqualsVersionedBody(const VersionedAtom& other) const;
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
 uint32 id_;
 uint8 reserved1_[4];
 uint64 duration_;          // duration in mvhd.time_scale units; 32b/v0, 64b/v1
 uint8 reserved2_[8];
 uint16 layer_;
 uint16 alternate_group_;
 uint16 volume_;
 uint8 reserved3_[2];
 uint8 matrix_structure_[36];
 FPU16U16 width_; // 32-bit unsigned fixed-point number (16.16).
 FPU16U16 height_; // 32-bit unsigned fixed-point number (16.16).
};
}
}

#endif // __MEDIA_F4V_ATOMS_MOVIE_TKHD_ATOM_H__
