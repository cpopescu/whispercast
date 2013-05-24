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

#ifndef __MEDIA_F4V_ATOMS_MOVIE_AVCC_ATOM_H__
#define __MEDIA_F4V_ATOMS_MOVIE_AVCC_ATOM_H__

#include <string>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperstreamlib/f4v/atoms/base_atom.h>

namespace streaming {
namespace f4v {

struct AvccParameter {
  io::MemoryStream raw_data_;

  AvccParameter();
  AvccParameter(const string& data);
  AvccParameter(const AvccParameter& other);
  ~AvccParameter();

  bool Equals(const AvccParameter& other) const;
  AvccParameter* Clone() const;

  bool Decode(io::MemoryStream& in, uint64 in_size);
  void Encode(io::MemoryStream& out) const;
  uint64 MeasureSize() const;

  string ToString() const;
};

class AvccAtom : public BaseAtom {
 public:
  static const AtomType kType = ATOM_AVCC;
  // body size without seq_parameters_ & pic_parameters_
  static const uint32 kBodyMinSize = 7;
 public:
  AvccAtom();
  AvccAtom(uint8 configuration_version,
           uint8 profile,
           uint8 profile_compatibility,
           uint8 level,
           uint8 nalu_length_size,
           const vector<string>& seq_parameters,
           const vector<string>& pic_parameters);
  AvccAtom(const AvccAtom& other);
  virtual ~AvccAtom();

  uint8 configuration_version() const;
  uint8 profile() const;
  uint8 profile_compatibility() const;
  uint8 level() const;
  uint8 nalu_length_size() const;
  const vector<AvccParameter*>& seq_parameters() const;
  const vector<AvccParameter*>& pic_parameters() const;
  void get_seq_parameters(vector<string>* out) const;
  void get_pic_parameters(vector<string>* out) const;

  //void set_configuration_version(uint8 configuration_version) const;
  //void set_profile(uint8 profile) const;
  //void set_profile_compatibility(uint8 profile_compatiblity) const;
  //void set_level(uint8 level) const;
  //void set_nalu_length_size(uint8 nalu_length_size) const;
  //void set_seq_parameters(const vector<string>& data) const;
  //void set_pic_parameters(const vector<string>& data) const;

  ///////////////////////////////////////////////////////////////////////////

  // Methods from BaseAtom
  virtual bool EqualsBody(const BaseAtom& other) const;
  virtual void GetSubatoms(vector<const BaseAtom*>& subatoms) const;
  virtual BaseAtom* Clone() const;
  virtual TagDecodeStatus DecodeBody(uint64 size,
                                     io::MemoryStream& in,
                                     Decoder& decoder);
  virtual void EncodeBody(io::MemoryStream& out, Encoder& encoder) const;
  virtual uint64 MeasureBodySize() const;
  virtual string ToStringBody(uint32 indent) const;

 private:
  uint8 configuration_version_;
  uint8 profile_;
  uint8 profile_compatibility_;
  uint8 level_;
  uint8 nalu_length_size_;
  vector<AvccParameter*> seq_parameters_;
  vector<AvccParameter*> pic_parameters_;
};
}
}

#endif // __MEDIA_F4V_ATOMS_MOVIE_AVCC_ATOM_H__
