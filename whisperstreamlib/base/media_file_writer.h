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

#ifndef __MEDIA_SOURCE_MEDIA_FILE_WRITER_H__
#define __MEDIA_SOURCE_MEDIA_FILE_WRITER_H__

#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperlib/common/io/file/file.h>
#include <whisperlib/net/base/selector.h>
#include <whisperstreamlib/base/consts.h>
#include <whisperstreamlib/base/tag.h>
#include <whisperstreamlib/base/tag_serializer.h>

namespace streaming {

class MediaFileWriter {
 public:
  MediaFileWriter();
  virtual ~MediaFileWriter();

  bool IsOpen() const;

  // Creates/truncates the file and initializes the writing.
  bool Open(const string& filename, MediaFormat format);

  // returns success. If it returns false it was probably feed junk,
  // so the file should be canceled.
  bool Write(const Tag* tag, int64 ts);

  // selector: may be NULL. Used when the completion callback is specified.
  // completion: If NULL, then Finalize blocks until file is done.
  //             If not NULL, then Finalize returns immediately,
  //             the file is asynchronously finalized,
  //             and the completion callback is called
  //             (within the selector context) when done,
  //             indicating the success status.
  // When this process is complete, the file is closed.
  // Use another Open() for a new file.
  bool Finalize(net::Selector* selector, Callback1<bool>* completion);

  // Abandons the currently open file.
  void CancelFile();

 private:
  string PartFilename() const { return strutil::CutExtension(filename_) + ".part"; }
  string TmpFilename() const { return filename_ + ".tmp"; }

  void CloseFile();
  void AsyncFinalize(net::Selector* selector, Callback1<bool>* completion);
  void AsyncFinalizeComleted(Callback1<bool>* completion, bool success);
  bool FinalizeFile();
  void Flush(bool force);

  static const MediaFormat kPartFileFormat = MFORMAT_FLV;
  static const uint32 kFlushSize = 1 << 20; // 1 MB
 private:
  TagSerializer* serializer_;
  io::MemoryStream buf_;
  io::File file_;

  // current output filename & format
  string filename_;
  MediaFormat format_;

  // the info for the current file.
  // Contains current file frames (accumulated here to create the MediaInfo)
  MediaInfo media_info_;

  // runs AsyncFinalize
  thread::Thread* async_finalize_;
};
}

#endif /* __MEDIA_SOURCE_MEDIA_FILE_WRITER_H__ */
