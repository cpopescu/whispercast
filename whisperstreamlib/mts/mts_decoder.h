// Copyright (c) 2012, Whispersoft s.r.l.
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

#ifndef __MEDIA_MTS_MTS_DECODER_H__
#define __MEDIA_MTS_MTS_DECODER_H__

#include <whisperstreamlib/base/media_info.h>
#include <whisperstreamlib/base/tag.h>
#include <whisperstreamlib/base/tag_splitter.h>
#include <whisperstreamlib/mts/mts_types.h>

namespace streaming {
namespace mts {

class TSDecoder {
 public:
  TSDecoder() {}
  virtual ~TSDecoder() {}
  DecodeStatus Read(io::MemoryStream& in, scoped_ref<TSPacket>* out);
};

class PESDecoder {
 public:
  PESDecoder() {}
  virtual ~PESDecoder() {}
  DecodeStatus Read(io::MemoryStream& in, scoped_ref<PESPacket>* out);
};

class Splitter : public streaming::TagSplitter {
 public:
  Splitter(const string& name);
  virtual ~Splitter();

 private:
  DecodeStatus FetchTSPacket(io::MemoryStream* in);
  DecodeStatus ReadPESPacket(scoped_ref<PESPacket>* out);

  ///////////////////////////////////////////////////////////////////
  // Methods from TagSplitter
 protected:
  // Read data from 'in', assemble and return a new Tag in '*tag'
  virtual streaming::TagReadStatus GetNextTagInternal(
      io::MemoryStream* in,
      scoped_ref<Tag>* tag,
      int64* timestamp_ms,
      bool is_at_eos);

 private:
  TSDecoder ts_decoder_;
  io::MemoryStream pes_buffer_;
  PESDecoder pes_decoder_;
  DISALLOW_EVIL_CONSTRUCTORS(Splitter);
};
}

typedef mts::Splitter MtsTagSplitter;
}

#endif // __MEDIA_MTS_MTS_DECODER_H__
