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
  typedef Callback1<Saver*> StopCallback;
  Saver(const string& name,
        ElementMapper* mapper,          // mapps media for us - can be null
        Tag::Type media_type,           // what to expect (if any..)
        const string& media_name,       // what to save exactly
        const string& media_dir,        // we save exactly in this dir
        int64 start_time,               // holder for 'start time' of saver
        bool started_on_command,        // was started on user command
        StopCallback* stop_callback);   // we call this when element closes
  virtual ~Saver();

  bool StartSaving();
  void StopSaving();
  bool IsSaving() const { return current_file_.is_open(); }

  const string& name() const          { return name_; }
  const string& media_dir() const     { return media_dir_; }

  bool started_on_command() const     { return started_on_command_; }
  int64 start_time() const            { return start_time_; }
  const Request* request() const      { return req_; }
  Request* mutable_request1() const    { return req_; }

  // Processes a tag - writes it to the internal buffer and may be
  // flushes the buffer if necessary
  void ProcessTag(const Tag* tag);

  bool CreateSignalingFile(const string& name, const string& content);
 private:
  // Writes the buffer to the disk
  void Flush();
  // Looks in the media_dir and determines the name of the next file to save
  // (finds out the sequence number).
  string GetNextFileForSaving(bool new_chunk) const;
  // Creates the next file to save to..
  bool OpenNextFile(bool new_chunk);
  void CloseFile();

 public:
  void SaveBootstrap() {
    vector< scoped_ref<const Tag> > bootstrap;
    bootstrapper_.GetBootstrapTags(&bootstrap);
    for ( int i = 0; i < bootstrap.size(); ++i ) {
      serializer_->Serialize(bootstrap[i].get(), &buffer_);
    }
  }


 private:
  const string name_;
  ElementMapper* const mapper_;
  const Tag::Type media_type_;
  const string media_name_;
  const string media_dir_;
  const int64  start_time_;
  const bool started_on_command_;   // vs. started on schedule
  streaming::Request* req_;
  StopCallback* stop_callback_;

  // current file being written
  io::File current_file_;
  // buffer serialized tags, so that we don't write to file on every tag
  io::MemoryStream buffer_;
  // write buffer_ to file, when buffer_.Size() becomes GT this value
  static const int kDumpBufferSize = 1 << 17;  // 128K

  streaming::ProcessingCallback* processing_callback_;
                                          // registered w/ element

  Bootstrapper bootstrapper_;             // Stream boot. We save for every file.

  TagSerializer* serializer_;
  char* tmp_buf[2 * kDumpBufferSize];

  // A SOURCE_STARTED was encountered
  bool source_started_;

  DISALLOW_EVIL_CONSTRUCTORS(Saver);
};

//
// Here is the problem that this function resolves.
// You have a saver, that save its files, and has whisperproc gets them and
// puts them in a standard <prefix>_<start_date>_<end_date>.<ext> format.
//
// This function identifies the first file to play from those files
// to play for the givven time.
//
// play_time - identifies the first file to play that starts from this
//             particular time
//
// home_dir - we look for files in this dir
// file_prefix - files have this prefix
// root_media_path - for the file that we find we prepend this to get the media
//                   path
//
// crt_media -- what was play before - to give us hints (from end_date above)
// begin_file_timestamp -- we start from this timestamp in the file (i.e. seek
//       in file)
// playref_time -- the real time of the first tag in the file we play
//
// Returns the delay in which to start playing crt_media if you want to
//        synchronize with given play_time (from now - ie to map the
//        play_time with the first frame)
//        if crt_media is empty - nothing to play ..
//
int64 GetNextStreamMediaFile(const timer::Date& play_time,
                             const string& home_dir,
                             const string& file_prefix,
                             const string& root_media_path,
			                 vector<string>* files,
                             int* last_selected,
                             string* crt_media,
                             int64* begin_file_timestamp,
                             int64* playref_time,
                             int64* duration);

static const char kWhisperProcFileTermination[] =
    "_[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]-"
    "[0-9][0-9][0-9][0-9][0-9][0-9]-[0-9][0-9][0-9]"
    "_"
    "[0-9][0-9][0-9][0-9][0-9][0-9][0-9][0-9]-"
    "[0-9][0-9][0-9][0-9][0-9][0-9]-[0-9][0-9][0-9]"
    ".*";

}

#endif  // __MEDIA_BASE_MEDIA_SAVER_H__
