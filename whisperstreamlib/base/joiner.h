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

#include <whisperlib/common/base/types.h>
#include <whisperstreamlib/base/tag.h>
#include <whisperstreamlib/base/tag_splitter.h>

namespace streaming {

class JoinProcessor {
 public:
  JoinProcessor();
  virtual ~JoinProcessor();

  // Call this after constructor to initialize the save state.
  // out_file: write output here
  virtual bool Initialize(const string& out_file) = 0;

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
  virtual PROCESS_STATUS ProcessTag(const streaming::Tag* media_tag,
      int64 timestamp_ms) = 0;

 private:
  DISALLOW_EVIL_CONSTRUCTORS(JoinProcessor);
};

}

#endif   // __MEDIA_BASE_JOINER_H__
