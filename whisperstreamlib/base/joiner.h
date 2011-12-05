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
// Authors: Catalin Popescu

#ifndef __MEDIA_BASE_JOINER_H__
#define __MEDIA_BASE_JOINER_H__

#include <string>
#include <vector>

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperlib/common/io/file/file_output_stream.h>
#include <whisperstreamlib/base/tag.h>

namespace streaming {
class TagSplitter;

class JoinProcessor {
 public:
  explicit JoinProcessor();
  virtual ~JoinProcessor();

  // Call this after constructor to initialize the save state.
  virtual bool InitializeOutput(const string& out_file, // write output here
                                int cue_interval);      // w/ cues at this ms.

  // Call this before starting to read a new file. Used for resetting internal
  // counters, as new tags will probably arrive with timestamps starting from 0
  virtual void MarkNewFile() = 0;

  // After processing everything, call this to finalize the output file
  // with cue-points and stuff..
  // Returns the duration of file in ms.
  virtual int64 FinalizeFile() = 0;

  // Process each tag of the input file(s).
  enum PROCESS_STATUS {
    PROCESS_CONTINUE,  // regular status, continue processing the current file.
    PROCESS_SKIP_FILE, // error in current file, go to next file.
    PROCESS_ABANDON,   // serious error, abandon processing.
  };
  virtual PROCESS_STATUS ProcessTag(const streaming::Tag* media_tag) = 0;

 protected:
  int cue_ms_time_;               // put cues at this interval
  string out_file_;               // we write the output to this guy
 private:
  DISALLOW_EVIL_CONSTRUCTORS(JoinProcessor);
};

// Joins multiple flv files into a single big flv file. The output file will
// contain cuePoints to enable seek.
// return: the duration of the output file in milliseconds,
//         or 0 on failure.
int64 JoinFlvFiles(const vector<string>& in_files,
                   const string& out_file,
                   int cue_ms_time,
                   bool keep_all_metadata);

// Same JoinFiles but with only 1 input file -> 1 output file..
// Q: What's the reason for 1 file -> 1 file transformation?
// A: The output file contains cuePoints thus becoming seekable.
int64 JoinFlvFiles(const string& in_file,
                   const string& out_file,
                   int cue_ms_time,
                   bool keep_all_metadata);

}

#endif   // __MEDIA_BASE_JOINER_H__
