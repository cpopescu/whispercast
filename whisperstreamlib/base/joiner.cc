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

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/base/scoped_ptr.h>
#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/io/file/file.h>
#include <whisperlib/common/io/file/file_input_stream.h>
#include <whisperstreamlib/base/joiner.h>
#include <whisperstreamlib/base/media_file_reader.h>
#include <whisperstreamlib/flv/flv_joiner.h>

// defined in flv_joiner.cc
DECLARE_bool(flv_joiner_normalize_tags);

namespace streaming {

JoinProcessor::JoinProcessor()
  : cue_ms_time_(0) {
}

JoinProcessor::~JoinProcessor() {
}

bool JoinProcessor::InitializeOutput(const string& out_file,
                                     int cue_ms_time) {
  out_file_ = out_file;
  cue_ms_time_ = cue_ms_time;
  return true;
}

//////////////////////////////////////////////////////////////////////

static const int kDefaultBufferSize = 16384;

// The main join function - we basically use the processor to
// do our job..
int64 JoinFlvFiles(const vector<string>& in_files,
                   const string& out_file,
                   int cue_ms_time,
                   bool keep_all_metadata) {
  FlvJoinProcessor processor(keep_all_metadata);
  if ( !processor.InitializeOutput(out_file, cue_ms_time) ) {
    return 0;
  }

  for ( int i = 0; i < in_files.size(); ++i ) {
    const string& filename = in_files[i];
    MediaFileReader reader;
    if ( !reader.Open(filename, TagSplitter::TS_FLV) ) {
      LOG_ERROR << "Cannot open file: [" << filename << "] skipping..";
      continue;
    }
    if ( reader.splitter()->type() != TagSplitter::TS_FLV ) {
      LOG_ERROR << "Not a FLV file: [" << filename << "] skipping..";
      continue;
    }
    LOG_INFO << " Processing file: [" << filename << "] for joining";

    processor.MarkNewFile();

    io::MemoryStream ms;
    while ( true ) {
      scoped_ref<Tag> tag;
      streaming::TagReadStatus err = reader.Read(&tag);

      if ( err == streaming::READ_EOF ) {
        LOG_INFO << "Joiner: [" << filename << "] EOF";
        break;
      }
      if ( err == streaming::READ_SKIP ) {
        LOG_DEBUG << "Joiner: [" << filename << "] READ_SKIP";
        continue;
      }
      if ( err != streaming::READ_OK ) {
        LOG_ERROR << "Joiner: Cannot read next tag, filename: [" << filename
                  << "], result: " << TagReadStatusName(err)
                  << " going to next file.";
        break;
      }

      JoinProcessor::PROCESS_STATUS status =
          processor.ProcessTag(tag.get());
      if ( status == JoinProcessor::PROCESS_SKIP_FILE ) {
        LOG_ERROR << "Skipping the rest of current file: [" << filename << "]";
        break;
      }
      if ( status == JoinProcessor::PROCESS_ABANDON ) {
        LOG_ERROR << "Abandoning job on current file: [" << filename << "]";
        return 0;
      }
      CHECK(status == JoinProcessor::PROCESS_CONTINUE)
        << " Untreated PROCESS_STATUS case";
    }
  }
  return processor.FinalizeFile();
}

int64 JoinFlvFiles(const string& in_file,
                   const string& out_file,
                   int cue_ms_time,
                   bool keep_all_metadata) {
  vector<string> in_files(1);
  in_files.push_back(in_file);
  return JoinFlvFiles(in_files, out_file, cue_ms_time, keep_all_metadata);
}


}
