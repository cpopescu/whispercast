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

//
// Here we have streaming::Saver
//  -- You can attach one of this objects to an element and it starts
//  saving whatever tags it receives from the element.
//  -- It saves into "media_dir" under a name like
//             "files_prefix"#number#"files_suffix"
//  -- Upon receiving a null tag calls the provided stop_callback
//  -- It does not start or stop saving automatically - you need to call
//     the provided StartSaving / StopSaving
//

#ifndef __MEDIA_BASE_SAVER_H__
#define __MEDIA_BASE_SAVER_H__

#include <string>

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/callback.h>
#include <whisperlib/common/base/date.h>

#include <whisperlib/common/io/file/file_output_stream.h>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperstreamlib/base/element.h>
#include <whisperstreamlib/base/bootstrapper.h>

namespace streaming {

class Saver {
 public:
  // Saved filenames have this prefix.
  static const string kFilePrefix;
  // Saved filenames have this suffix.
  static const string kFileSuffix;
  // Saved filenames are identified by this Regular Expression.
  static const string kFileRE;
  // Temporary filenames end with this suffix.
  static const string kTempFileSuffix;
  // Temporary filenames are identified by this Regular Expression.
  static const string kTempFileRE;
  // the name of the file storing informations about the saved media
  static const string kMediaInfoFile;

  // Composes the base filename for saving. e.g.: "part_001335962291100C.part"
  // That number is ts = milliseconds from Epoch.
  // 'C' = continue chunk, 'N' = new chunk.
  static string MakeFilename(int64 ts, bool is_new_chunk);
  // Composes the base filename for saving, using the current system time.
  static string MakeFilenameNow(bool is_new_chunk);
  // Extracts the filename from the temporary filename.
  // Useful when renaming the temporary file to the final file.
  static string MakeFilename(const string& temp_filename);
  // Composes a temporary filename.
  static string MakeTempFilename(int64 ts, bool is_new_chunk);
  // Composes a temporary filename using the current system time.
  static string MakeTempFilenameNow(bool is_new_chunk);
  // The reverse of MakeFilename(). Decomposes a saved filename into:
  // timestamp + is_new_chunk.
  // out_is_new_chunk: may be NULL, if you're not interested about it
  static bool ParseFilename(const string& filename, int64* out_ts,
      bool* out_is_new_chunk);

 public:
  typedef Callback1<Saver*> StopCallback;
  Saver(const string& name,
        ElementMapper* mapper,          // mapps media for us - can be null
        MediaFormat media_format,       // saved media format
        const string& media_name,       // what to save exactly
        const string& media_dir,        // we save exactly in this dir
        StopCallback* stop_callback);   // we call this when element closes
  virtual ~Saver();

  // duration_sec: the saver automatically stops after these many seconds.
  //               Use 0 if you wish to stop it manually.
  bool StartSaving(uint32 duration_sec);
  // Stop now, regardless of 'duration_sec' specified on StartSaving().
  void StopSaving();
  bool IsSaving() const { return current_file_.is_open(); }

  const string& name() const          { return name_; }
  const string& media_dir() const     { return media_dir_; }

  const Request* request() const      { return req_; }

  // Processes a tag - writes it to the internal buffer and may be
  // flushes the buffer if necessary
  void ProcessTag(const Tag* tag, int64 timestamp_ms);

 private:
  // Writes the buffer to the disk
  void Flush();
  // Creates the next file to save to..
  bool OpenNextFile(bool new_chunk);
  void CloseFile();

  // rename all temp files, that were probably left behind by a previous crash
  void RecoverTempFiles();

 public:
  void SaveBootstrap() {
    vector<scoped_ref<const Tag> > bootstrap;
    bootstrapper_.GetBootstrapTags(&bootstrap);
    for ( int i = 0; i < bootstrap.size(); ++i ) {
      serializer_->Serialize(bootstrap[i].get(), 0, &buffer_);
    }
  }


 private:
  const string name_;
  ElementMapper* const mapper_; // may be NULL
  const MediaFormat media_format_;
  const string media_name_;
  const string media_dir_;
  streaming::Request* req_;
  ::util::Alarm* stop_alarm_; // stops the saver
  StopCallback* stop_callback_; // external callback, called the saver stops.

  // current file being written
  io::File current_file_;
  // buffer serialized tags, so that we don't write to file on every tag
  io::MemoryStream buffer_;
  // write buffer_ to file, when buffer_.Size() becomes GT this value
  static const int kDumpBufferSize = 1 << 17;  // 128K

  // registered w/ element
  streaming::ProcessingCallback* processing_callback_;

  // Stream boot. We save for every file.
  Bootstrapper bootstrapper_;

  TagSerializer* serializer_;

  DISALLOW_EVIL_CONSTRUCTORS(Saver);
};
}

#endif  // __MEDIA_BASE_MEDIA_SAVER_H__
