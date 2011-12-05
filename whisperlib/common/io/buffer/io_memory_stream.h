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

#ifndef __COMMON_IO_BUFFER_IO_MEMORY_STREAM_H__
#define __COMMON_IO_BUFFER_IO_MEMORY_STREAM_H__

#include <string>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperlib/common/io/input_stream.h>
#include <whisperlib/common/io/output_stream.h>

//
// This exposes the MemoryStream using the InputStream and OutputStream
// interfaces..
//

namespace io {

class IOMemoryStream : public InputStream, public OutputStream {
 public:
  IOMemoryStream(BlockSize block_size = io::DataBlock::kDefaultBufferSize);
  explicit IOMemoryStream(io::MemoryStream* ms_to_wrap);
  ~IOMemoryStream();

  inline MemoryStream& ms() { return *ms_; }
  inline const MemoryStream& ms() const { return *ms_; }

  // Returns the total amount of data bytes in the buffer.
  int32 Size() const {
    return ms_->Size();
  }

  // Tests if the stream is empty.
  bool IsEmpty() const {
    return ms_->IsEmpty();
  }

  // Remove all data in stream.
  //  After this call the stream becomes empty:
  //     Readable() == 0, IsEos() == true.
  void Clear() {
    ms_->Clear();
  }

  // Copy all readable data from 'in' (between crtReadHead .. streamEnd)
  // to this stream. The 'in' stream is not modified.
  // This stream will contain:
  //   <previous data> <data copied from in>
  //                  ^                     ^
  //              read head            write head
  // returns: the number of bytes copied.
  int32 Append(const IOMemoryStream& in, int32 size = kMaxInt32) {
    #ifdef _DEBUG
    const int32 original_size = in.Size();
    #endif
    ms_->AppendStreamNonDestructive(in.ms_, size);
    #ifdef _DEBUG
    CHECK_EQ(original_size, in.Size());   // "in" should not be modified
    #endif
    return size;
  }
  // Same as Append
  const IOMemoryStream& operator=(const IOMemoryStream& in) {
    Append(in);
    return *this;
  }

  // Returns a complete description of the internal state including
  // internal data. Used for logging.
  //
  // !! NOTE !!
  //   -- This is amazingly expensive - use it only in debug mode !!
  string ToString() const;

  //////////////////////////////////////////////////////////////////////
  //
  //              InputStream interface methods
  //
  int32 Read(void* buffer, int32 len) {
    return ms_->Read(buffer, len);
  }
  int32 Peek(void* buffer, int32 len) {
    return ms_->Peek(buffer, len);
  }
  int64 Skip(int64 len) {
    return ms_->Skip(len);
  }
  int64 Readable() const {
    return Size();
  }
  bool IsEos() const {
    return IsEmpty();
  }

  // Sets the read marker in the current stream possition.
  void MarkerSet() {
    ms_->MarkerSet();
  }

  // Restores the read position of the associated stream to the position
  // previously marked by set.
  void MarkerRestore() {
    ms_->MarkerRestore();
  }

  // Removes the last read mark for the underlying stream
  void MarkerClear() {
    ms_->MarkerClear();
  }

  //////////////////////////////////////////////////////////////////////
  //
  //              OutputStream interface methods
  //
  int32 Write(const void* buffer, int32 len) {
    return ms_->Write(buffer, len);
  }
  int32 Write(io::InputStream& in, int32 len = kMaxInt32) {
    return io::OutputStream::Write(in, len);
  }
  int32 Write(const char* text) {
    return ms_->Write(text);
  }
  int64 Writable() const {
    return kMaxInt64;
  }

 protected:
  io::MemoryStream* ms_;
  bool localms_;
};

inline ostream& operator<<(ostream& os,
                           const io::IOMemoryStream& ms) {
  return os << ms.ToString();
}
}
#endif  // __COMMON_IO_BUFFER_IO_MEMORY_STREAM_H__
