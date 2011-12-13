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

#ifndef __MEDIA_SOURCE_MEDIA_FILE_READER_H__
#define __MEDIA_SOURCE_MEDIA_FILE_READER_H__

#include <string>
#include <list>
#include <vector>

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/io/file/file.h>
#include <whisperlib/common/io/ioutil.h>
#include <whisperlib/common/io/buffer/memory_stream.h>

#include <whisperstreamlib/base/tag.h>
#include <whisperstreamlib/base/tag_splitter.h>

namespace streaming {

class MediaFileReader {
public:
  MediaFileReader();
  virtual ~MediaFileReader();

  TagSplitter* splitter();

  bool Open(const string& filename,
            TagSplitter::Type ts_type = (TagSplitter::Type)-1);
  // read next tag from file.
  TagReadStatus Read(scoped_ref<Tag>* out, int64* timestamp_ms);
  // similar to the Read() above, but also returns raw data for the read tag.
  TagReadStatus Read(scoped_ref<Tag>* out, io::MemoryStream* out_data);

  // Test for end of file reached.
  bool IsEof() const;

  // Current read position in file.
  uint64 Position() const;

  // Bytes between current Position() and file end.
  uint64 Remaining() const;

private:
  io::MemoryStream buf_;
  io::File file_;
  int64 file_size_;
  bool file_eof_reached_;

  TagSplitter* splitter_;
};

}

#endif // __MEDIA_SOURCE_MEDIA_FILE_READER_H__
