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

#ifndef __MEDIA_F4V_ATOMS_CONTAINER_VERSIONED_ATOM_H__
#define __MEDIA_F4V_ATOMS_CONTAINER_VERSIONED_ATOM_H__

#include <string>
#include <whisperstreamlib/f4v/f4v_encoder.h>
#include <whisperstreamlib/f4v/f4v_decoder.h>
#include <whisperstreamlib/f4v/atoms/versioned_atom.h>

// A versioned atom containing some data and multiple subatoms.
// General format:
//  <size><type><version><some data fields><atom1><atom2><atom3>...
// Examples of ContainerVersionedAtom: stsd, avc1

namespace streaming {
namespace f4v {

class ContainerVersionedAtom : public VersionedAtom {
 public:
  ContainerVersionedAtom(AtomType type);
  ContainerVersionedAtom(const ContainerVersionedAtom& other);
  virtual ~ContainerVersionedAtom();

  const vector<BaseAtom*>& subatoms() const;

  // Add to subatoms_ vector
  void AddSubatom(BaseAtom* subatom);

 protected:
  // Called by DecodeBody, with every subatom found.
  // This is the way of notifying the upper class about the subatoms.
  // The 'atom' is already stored in subatoms_.
  virtual void SubatomFound(BaseAtom* atom) = 0;

  // Read data between version - subatoms.
  virtual TagDecodeStatus DecodeData(uint64 size,
                                     io::MemoryStream& in,
                                     Decoder& decoder) = 0;
  // Write data between version - subatoms.
  virtual void EncodeData(io::MemoryStream& out, Encoder& encoder) const = 0;
  // Returns the size of the data between version - subatoms.
  virtual uint64 MeasureDataSize() const = 0;
  // test only the data between version - subatoms
  virtual bool EqualsData(const ContainerVersionedAtom& other) const = 0;
  // returns a description of the atom data between version - subatoms.
  virtual string ToStringData(uint32 indent) const = 0;

  // Called by upper class to receive all subatoms.
  // Useful when constructing a ContainerVersionedAtom by copy constructor.
  // This method calls SubatomFound for every item in subatoms_.
  void DeliverSubatomsToUpperClass();

  /////////////////////////////////////////////////////////////
  // VersionedAtom methods
  //
 public:
  virtual void GetSubatoms(vector<const BaseAtom*>& subatoms) const;
  virtual BaseAtom* Clone() const = 0;
 protected:
  virtual bool EqualsVersionedBody(const VersionedAtom& other) const;
  virtual TagDecodeStatus DecodeVersionedBody(uint64 size,
                                              io::MemoryStream& in,
                                              Decoder& decoder);
  virtual void EncodeVersionedBody(io::MemoryStream& out,
                                   Encoder& encoder) const;
  virtual uint64 MeasureVersionedBodySize() const;
  virtual string ToStringVersionedBody(uint32 indent) const;

 private:
  vector<BaseAtom*> subatoms_;
  // Used as bug trap.
  // When using the copy constructor the upper class must call
  // DeliverSubatomsToUpperClass(..).
  bool debug_subatoms_delivered_;
};
}
}

#endif // __MEDIA_F4V_ATOMS_CONTAINER_VERSIONED_ATOM_H__
