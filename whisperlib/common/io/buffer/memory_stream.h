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

#ifndef __COMMON_IO_BUFFER_MEMORY_STREAM_H__
#define __COMMON_IO_BUFFER_MEMORY_STREAM_H__

#include <sys/uio.h>

#include <utility>
#include <string>
#include <vector>

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/io/buffer/data_block.h>
#include <whisperlib/common/io/num_streaming.h>

namespace io {

//
// A buffer used for passing data around without copying.
//
// Thread safe notice: this is *not* thread safe in general. However it is
// safe to use const MemoryStream-s (*just* const functions), concurrently in
// different threads (provided that nobody alters the memory stream).
//
class MemoryStream {
 public:
  MemoryStream(BlockSize block_size = DataBlock::kDefaultBufferSize);
  MemoryStream(const MemoryStream& other);
  ~MemoryStream();

  BlockSize block_size() const { return block_size_; }

  void operator=(const MemoryStream& other);

  //////////////////////////////////////////////////////////////////////
  //
  // "FAST" interface - works mostly by actioning on the underneath
  //                    data buffers and moving them around.
  //

  // Appends a raw block to the buffer - creates a new block and
  // calls AppendBlock
  // The block becomes ours and is integrated in a new DataBlock.
  // The data should be deletable (e.g. is OK to do delete [] data)
  // NEVER - AppendRaw the same buffer to two MemoryStreams !! (instead
  // append to one temp buffer and use AppendStream /
  // AppendStreamNonDestructive)
  void AppendRaw(const char* data, int32 size, Closure* disposer = NULL);

  // Appends this block to our internal list (increments the reference
  // count)
  void AppendBlock(DataBlock* data);

  // Returns a piece of data from the buffer and advances the read.
  // If not 0, *size may specify the maximum ammount ot be read..
  // Retruns false on end-of-buffer.
  bool ReadNext(const char** buffer, int32* size);

  // Reads all the internal buffers for a writev operation
  int32 ReadForWritev(struct ::iovec** iov, int* iovcnt, int32 max_size);

  // Returns a piece of buffer already allocated and ready to be written to
  // (it is reserved and considered written). Use ConfirmScratch to confirm
  // how much you used from returned size.
  // IMPORTANT NOTE: after a GetScratchSpace the next operation w/ this
  //                 stream must be ConfirmScratch !!!
  void GetScratchSpace(char** buffer, int32* size);
  void ConfirmScratch(int32 size);

  // Returns true if the buffer is empty.
  bool IsEmpty() const {
    return size_ <= 0;
  }

  // Returns the total amount of data in the buffer
  uint32 Size() const {
    CHECK_GE(size_, 0);
    return (uint32)size_;
  }

  // Appends a portion of the given data block to our list (makes a copy)
  // in a fresh new block..
  void AppendNewBlock(DataBlock* data,
                      BlockSize pos_begin, BlockSize pos_end);

  //////////////////////////////////////////////////////////////////////
  //
  // "MEDIUM" interface functions - these may take a long time, or a small
  //                                time, depending on conditions..

  // Appends a full buffer to this one -
  void AppendStream(MemoryStream* buffer, int32 size = -1);

  // Appends without destroying the buffer.
  // offset - skip this many bytes before reading from 'in'
  // size - append just size bytes (or until the end of in buffer if size == -1)
  void AppendStreamNonDestructive(const MemoryStream* in,
                                  int32 offset = 0,
                                  int32 size = -1);

 private:
  // As above, but appends between the pointers of the same owner
  void AppendStreamNonDestructive(DataBlockPointer* begin,
                                  const DataBlockPointer* end);

 public:
  //////////////////////////////////////////////////////////////////////
  //
  // "SLOWER" inteface functions - these may take longer as they involve
  //                               data copying and buffer allocation

  // Reads "len" bytes of data into the 'out' buffer
  // Returns the number of bytes read. Can be less than "len" if less bytes
  // are available.
  int32 Read(void* out, int32 len) {
    return ReadInternal(out, len, true);
  }

  // Same as read, but w/ strings - this tends to be slower..
  // If len == -1 read to the end of input, else read len bytes.
  // Returns anyway the number of read bytes
  int32 ReadString(string* s, int32 len = -1);
  // Reads without destroying the stream.
  int32 PeekString(string* s, int32 len = -1) const;

  bool Equals(const io::MemoryStream& other) const;

  // These are really slow - do not use them unless for debugging..
  string ToString() {
    string s;
    ReadString(&s);
    return s;
  }
  string DebugString() const {
#ifdef _DEBUG
    char* t = new char[Size()];
    string s;
    const int32 size = Peek(t, Size());
    s.assign(t, size);
    delete [] t;
    return s;
#else
    return "!!! io::MemoryStream::DebugString in Release Mode !!!";
#endif
  }

  // Utility to read from the pointer to a LF. Will leave the pointer
  // after the LF at the end of the buffer. Returns true (and reads)
  // a line if found. On true, s will contain the line *and* the CR/LF
  bool ReadLFLine(string* s) {
    if ( !MaybeInitReadPointer() ) {
      return false;
    }
    if ( read_pointer_.ReadLFLine(s) ) {
      size_ -= s->size();
      return true;
    }
    return false;
  }

  // Reads a line until CR/LF.
  // Leaves the stream pointer after LF, but does not return the CR/LF.
  bool ReadLine(string* s) {
    if ( !ReadLFLine(s) ) {
      return false;
    }
    strutil::StrTrimCRLFInPlace(*s);
    return true;
  }

  // Read the next token - which can be:
  //  - a separator char by itself
  //  - a succession of non blank, non separator charactes
  //  - a full string beginning w/ ' or "
  //    - treats " and ' as begin of string / end of string
  //  - passes over blanks
  TokenReadError ReadNextAsciiToken(string* s) {
    if ( !MaybeInitReadPointer() ) {
      return TOKEN_NO_DATA;
    }
    int len;
    TokenReadError ret =  read_pointer_.ReadNextAsciiToken(s, &len);
    size_ -= len;
    return ret;
  }

  // Not actually reading len bytes into 'out' buffer :)
  int32 Peek(void* out, int32 len) const {
    DataBlockPointer reader(GetReadPointer());
    if ( reader.IsNull() ) {
      return 0;
    }
    return static_cast<int32>(
        reader.ReadData(reinterpret_cast<char*>(out), len));
  }

  // Passes over "len" bytes. (Advances the read head by the "len" number of
  // bytes). Returns the number of bytes skipped. Can be less than "len"
  // if less bytes are available.
  int32 Skip(int32 len);

  // Clears the buffer
  void Clear();

  // Append "len" bytes of data from the given "buffer" to the stream
  // Returns the number of bytes written. Can be less than len if the stream
  // has not enough space, or negative on error.
  int32 Write(const void* buffer, int32 len);

  // Convenience function for writing a NULL terminated string.
  int32 Write(const char* text) {
    return Write(text, strlen(text));
  }
  // Convenience function for writing a string
  int32 Write(const string& s) {
    return Write(s.data(), s.size());
  }

  //////////////////////////////////////////////////////////////////////
  //
  // MARKER interface (reasonaby fast, w/ exception of Restore()
  //

  // Sets the marker in the current stream position
  void MarkerSet() {
    if ( markers_.empty() ) {
      invalid_markers_size_ = false;
    }
    markers_.push_back(make_pair(size_, new DataBlockPointer(read_pointer_)));
  }
  // Test if a marker is set
  bool MarkerIsSet() const {
    return !markers_.empty();
  }
  // Restores the position of the associated stream to the position
  // previously marked by set.
  void MarkerRestore() {
    DCHECK(!markers_.empty());
    read_pointer_ = *markers_.back().second;
    if ( invalid_markers_size_ ) {
      // unfortunately no smarter way for doing this, so we basically invalidate
      // it..
      MaybeInitReadPointer();
      size_ = read_pointer_.Distance(write_pointer_);
    } else {
      size_ = markers_.back().first;
    }
    delete markers_.back().second;
    markers_.pop_back();
  }
  // Removes the last mark for the underlying stream
  void MarkerClear() {
    delete markers_.back().second;
    markers_.pop_back();
    MaybeDisposeBlocks();
  }

  //////////////////////////////////////////////////////////////////////
  //
  // DEBUG functions (terribly slow)

  // Utility debug function
  string DumpContent(uint32 max_size = kMaxUInt32) const;
  string DumpContentHex(uint32 max_size = kMaxUInt32) const;
  string DumpContentInline(uint32 max_size = kMaxUInt32) const;

  string DetailedContent() const;

  DataBlockPointer GetReadPointer() const {
    if ( read_pointer_.IsNull() && !write_pointer_.IsNull() ) {
      return DataBlockPointer(read_pointer_.owner(),
                              blocks_.begin_id(),
                              read_pointer_.pos());
    } else {
      return read_pointer_;
    }
  }

 protected:
  // Does the actual reading
  int32 ReadInternal(void* buffer, int32 len, bool dispose);

  // when the pointer advances, we may dispose some of the underlying blocks
  void MaybeDisposeBlocks();

  bool MaybeInitReadPointer() {
    if ( read_pointer_.IsNull() ) {
      if ( write_pointer_.IsNull() )
        return false;
      CHECK(read_pointer_.owner() != NULL);
      read_pointer_.set_block_id(blocks_.begin_id());
    }
    return true;
  }

  // We allocate buffer of this size by default
  const BlockSize block_size_;

  BlockDqueue blocks_;
  DataBlockPointer read_pointer_;
  DataBlockPointer write_pointer_;
  DataBlockPointer scratch_pointer_;

  // These are for marking positions in the stream
  typedef vector < pair<int, DataBlockPointer*> > MarkersList;
  MarkersList markers_;
  bool invalid_markers_size_;
  // We keep track of our internal size *all* the time - this is of
  // utmost importance in maintaining a fast response.
  // (At all times size_ == read_pointer_.Distance(write_pointer_);
  int32 size_;
};

typedef BaseNumStreamer<MemoryStream, MemoryStream> NumStreamer;
}
#endif  // __COMMON_IO_BUFFER_MEMORY_STREAM_H__
