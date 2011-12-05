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

#ifndef __MEDIA_F4V_F4V_REORDER_H__
#define __MEDIA_F4V_F4V_REORDER_H__

#include <vector>
#include <list>
#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>

namespace streaming {
namespace f4v {

class Tag;
class MoovAtom;
class FrameHeader;

//////////////////////////////////////////////////////////////////////

class TagReorder {
 public:
  // order_by_timestamp: true means ordering frames by timestamp
  //                     false means ordering frames by offset
  TagReorder(bool order_by_timestamp);
  virtual ~TagReorder();

  // push tags in file order
  void Push(const Tag* tag);
  // pop tags in timestamp order
  const Tag* Pop();

 private:
  // pop frames from cache_ into output_. As many as we can.
  void PopCache();

  void Clear();
  void ClearOutput();

 private:
  const bool order_by_timestamp_;

  // current moov atom
  const MoovAtom* moov_;

  vector<const FrameHeader*> ordered_frames_;
  // index in ordered_frames_. The next frame to pop.
  uint32 ordered_next_;

  // out of order frames we received and we didn't return yet.
  // Mapped by file offset.
  typedef map<int64, const Tag*> FrameMap;
  FrameMap cache_;

  // the tags to be popped
  typedef list<const Tag*> TagList;
  TagList output_;

  DISALLOW_EVIL_CONSTRUCTORS(TagReorder);
};
} // namespace f4v
} // namespace streaming

#endif  // __MEDIA_F4V_F4V_SPLITTER_H__
