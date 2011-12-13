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

#ifndef __MEDIA_F4V_F4V_TAG_H__
#define __MEDIA_F4V_F4V_TAG_H__

#include <string>
#include <whisperlib/common/base/types.h>
#include <whisperstreamlib/base/tag.h>
#include <whisperstreamlib/f4v/atoms/base_atom.h>
#include <whisperstreamlib/f4v/frames/frame.h>

namespace streaming {
namespace f4v {

class Tag : public streaming::Tag {
 public:
  static const Type kType;
  Tag(uint32 attributes,
      uint32 flavour_mask,
      BaseAtom* atom)
      : streaming::Tag(kType, attributes, flavour_mask),
        is_atom_(true) {
    un_.atom_ = atom;
  }
  Tag(uint32 attributes,
      uint32 flavour_mask,
      Frame* frame)
      : streaming::Tag(kType, attributes, flavour_mask),
        is_atom_(false) {
    un_.frame_ = frame;
  }
  Tag(const Tag& other)
      : streaming::Tag(other),
        is_atom_(other.is_atom()) {
    if ( other.is_atom() ) {
      un_.atom_ = other.atom()->Clone();
    } else {
      un_.frame_ = other.frame()->Clone();
    }
  }
  virtual ~Tag() {
    if ( is_atom() ) {
      delete un_.atom_;
      un_.atom_ = NULL;
    } else {
      delete un_.frame_;
      un_.frame_ = NULL;
    }
  }

  bool is_atom() const { return is_atom_; }
  bool is_frame() const { return !is_atom(); }

  BaseAtom* atom() { CHECK(is_atom()); return un_.atom_; }
  const BaseAtom* atom() const { CHECK(is_atom()); return un_.atom_; }
  BaseAtom* release_atom() {BaseAtom* a = atom(); un_.atom_ = NULL; return a; }
  const BaseAtom* release_atom () const {
    const BaseAtom* a = atom(); un_.atom_ = NULL; return a; }

  Frame* frame() { CHECK(is_frame()); return un_.frame_; }
  const Frame* frame() const { CHECK(is_frame()); return un_.frame_; }
  Frame* release_frame() { Frame* a = frame(); un_.frame_ = NULL; return a; }
  const Frame* release_frame() const {
    const Frame* a = frame(); un_.frame_ = NULL; return a; }

  int64 timestamp_ms() const {
    return is_atom() ? 0LL : frame()->header().timestamp();
  }

  //////////////////////////////////////////////////////////////////////
  //
  // Methods from Tag
  //
 public:
  virtual int64 duration_ms() const {
    return is_atom() ? 0LL : frame()->header().duration();
  }
  virtual uint32 size() const {
    return is_atom() ? atom()->size() : frame()->header().size();
  }
  virtual streaming::Tag* Clone() const {
    return new Tag(*this);
  }
  virtual string ToStringBody() const {
    return is_atom() ? atom()->ToString() :
                       frame()->ToString();
  }

 private:
  mutable union {
    BaseAtom* atom_;
    Frame* frame_;
  } un_;
  bool is_atom_;
};

}

typedef streaming::f4v::Tag F4vTag;

}

#endif // __MEDIA_F4V_F4V_TAG_H__
