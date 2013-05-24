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

#ifndef __MEDIA_F4V_F4V_FILE_WRITER_H__
#define __MEDIA_F4V_F4V_FILE_WRITER_H__

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/system.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/base/timer.h>
#include <whisperlib/common/base/date.h>

#include <whisperlib/common/io/file/file.h>
#include <whisperlib/common/io/ioutil.h>
#include <whisperlib/common/io/buffer/memory_stream.h>

#include <whisperstreamlib/f4v/f4v_tag.h>
#include <whisperstreamlib/f4v/f4v_encoder.h>

namespace streaming {
namespace f4v {

class FileWriter {
 public:
  FileWriter();
  virtual ~FileWriter();

  // Open file for write. Overwrites existing files.
  // Returns success status;
  bool Open(const string& filename);
  void Close();

  // Writes the tag to file.
  //NOTES:
  // - MOOV atom is written as is
  // - for MDAT atom only the header is written
  // - frames must come in decoding order
  bool Write(const Tag& tag);
  bool Write(const BaseAtom& atom);
  bool Write(const Frame& frame);
  // Write raw data to file
  bool WriteRaw(const io::MemoryStream& ms);
  // Flush internal buffer to file.
  bool Flush();

  // Current write position in file. = File size.
  uint64 Position() const;

  // return some statistics about the written file
  string StrStats() const;
 private:
  io::MemoryStream buf_;
  io::File file_;
  streaming::f4v::Encoder encoder_;

  uint64 written_tags_;
  uint64 written_bytes_;
};

} // namespace f4v
} // namespace streaming

#endif // __MEDIA_F4V_F4V_FILE_WRITER_H__
