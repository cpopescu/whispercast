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

#include <whisperlib/common/base/gflags.h>
#include "raw/raw_tag_splitter.h"

DEFINE_int32(raw_tag_max_size,
             16384 * 3,
             "We create raw tags with maximum this size - higher - "
             "better performance, lower - failure for flow control");

namespace streaming {

const Tag::Type RawTag::kType = Tag::TYPE_RAW;

streaming::TagReadStatus RawTagSplitter::GetNextTagInternal(
    io::MemoryStream* in,
    scoped_ref<Tag>* tag,
    int64* timestamp_ms,
    bool is_at_eos) {
  *tag = NULL;
  if ( in->IsEmpty() ) {
    return READ_NO_DATA;
  }
  const uint32 size = min(in->Size(), (uint32)FLAGS_raw_tag_max_size);
  RawTag* const raw_tag = new RawTag(0, kDefaultFlavourMask);
  raw_tag->mutable_data()->AppendStream(in, size);

  *timestamp_ms = 0;
  *tag = raw_tag;

  total_size_ += size;
  return READ_OK;
}
}
