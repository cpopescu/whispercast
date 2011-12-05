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

#ifndef __MEDIA_F4V_F4V_FILE_READER_H__
#define __MEDIA_F4V_F4V_FILE_READER_H__

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

#include <whisperstreamlib/base/tag_splitter.h>
#include <whisperstreamlib/f4v/f4v_tag.h>
#include <whisperstreamlib/f4v/f4v_decoder.h>

namespace streaming {
namespace f4v {

class FileReader {
public:
  FileReader();
  virtual ~FileReader();

  const ElementSplitterStats& stats() const;
  streaming::f4v::Decoder& decoder();

  // Open file for read.
  // Returns success status;
  bool Open(const string& filename);
  void Close();

  // Reads the next tag from file, allocates the tag and puts it in 'out'
  // Returns decoding status:
  //  TAG_DECODE_SUCCESS: read ok, the tag is in *out
  //  TAG_DECODE_NO_DATA: EOF reached. Use Remaining() to check for useless
  //                      bytes at file end.
  //  TAG_DECODE_ERROR: corrupted data in file
  TagDecodeStatus Read(scoped_ref<Tag>* out);
  // Reads the next tag in 'out_tag' and also returns the tag raw data
  // in 'out_data'.
  TagDecodeStatus Read(scoped_ref<Tag>* out_tag, io::MemoryStream* out_data);

  // Test for end of file reached.
  bool IsEof() const;

  // Current read position in file.
  uint64 Position() const;

  // Bytes between current Position() and file end.
  uint64 Remaining() const;

  // Go to beginning of file. Resets the statistics.
  void Rewind();

private:
  io::MemoryStream buf_;
  io::File file_;
  int64 file_size_;
  bool eof_;
  streaming::f4v::Decoder decoder_;

  // some stats about audio/video/bytes
  // Updated while reading the file. So these are useful only after you have
  // read the file completely.
  ElementSplitterStats stats_;
};

} // namespace f4v
} // namespace streaming

#endif // __MEDIA_F4V_F4V_FILE_READER_H__
