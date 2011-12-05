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

#ifndef __MEDIA_F4V_F4V_SPLITTER_H__
#define __MEDIA_F4V_F4V_SPLITTER_H__

#include <vector>
#include <list>
#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperstreamlib/base/tag.h>
#include <whisperstreamlib/base/tag_splitter.h>
#include <whisperstreamlib/f4v/f4v_decoder.h>

namespace streaming {

//////////////////////////////////////////////////////////////////////

class F4vTagSplitter : public streaming::TagSplitter {
 public:
  static const Type kType;
 public:
  F4vTagSplitter(const string& name);
  virtual ~F4vTagSplitter();

  ///////////////////////////////////////////////////////////////////
  //
  // Methods from TagSplitter
  //
 protected:
  // This is how we detect Seek
  /*
  virtual void set_limits(int64 media_origin_pos_ms,
                          int64 seek_pos_ms,
                          int64 limit_ms);
  */

  // Read data from 'in', assemble and return a new Tag in '*tag'
  virtual streaming::TagReadStatus GetNextTagInternal(
      io::MemoryStream* in, scoped_ref<Tag>* tag, bool is_at_eos);

 private:
  f4v::Decoder f4v_decoder_;
  bool generate_cue_point_table_now_;
  DISALLOW_EVIL_CONSTRUCTORS(F4vTagSplitter);
};
}

#endif  // __MEDIA_F4V_F4V_SPLITTER_H__
