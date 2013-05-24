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

#ifndef __MEDIA_FLV_FLV_JOINER_H__
#define __MEDIA_FLV_FLV_JOINER_H__

#include <string>
#include <vector>
#include <whisperlib/common/base/types.h>
#include <whisperstreamlib/base/joiner.h>
#include <whisperstreamlib/flv/flv_tag.h>
#include <whisperstreamlib/flv/flv_file_writer.h>

namespace streaming {

//////////////////////////////////////////////////////////////////////

// Use this to join flv files:

// Object that holds the state and logic for join proceessing.
class FlvJoinProcessor : public streaming::JoinProcessor {
 private:
  struct CuePoint {
    uint32 timestamp_;
    uint32 position_;
    CuePoint(uint32 timestamp, uint32 position)
      : timestamp_(timestamp), position_(position) {}
  };
 public:
  // cue_ms_time: milliseconds between consecutive cue points
  FlvJoinProcessor(uint32 cue_ms);
  virtual ~FlvJoinProcessor();

  // Call this after constructor to initialize the save state.
  virtual bool Initialize(const string& out_file);
  // Call this before starting to read a new file
  virtual void MarkNewFile();

  // After processing everything, call this to finalize the output file
  // with cue-points and stuff..
  // returns: >0 : file duration in milliseconds
  //          -1 : error
  virtual int64 FinalizeFile();

  // Main tag processor
  virtual PROCESS_STATUS ProcessTag(const streaming::Tag* media_tag,
      int64 timestamp_ms);
 private:
  // Helper - writes a cue point to writer_
  void WriteCuePoint(int crt_cue, double position, int64 timestamp_ms);
  // Helper to process a metadata tag.
  // Checks if this metadata is compatible with the first one.
  // Params:
  //  skip_file: true -> incompatibility found, abort processing current file
  //             false -> continue processing current file
  // Returns: true -> write metadata in output file
  //          false -> skip this metadata
  bool ProcessMetaData(const streaming::FlvTag::Metadata& medata,
                       bool* skip_file);

  // Utilities for cue-point preparation
  static void PrepareCuePoint(int index, double position, int64 timestamp_ms,
                              rtmp::CMixedMap* out);
  static void PrepareCuePoints(const vector<CuePoint>& cues,
                               rtmp::CMixedMap* out);
  static void PrepareMetadataValues(double file_duration,
                                    int64 file_size,
                                    const vector<CuePoint>& cues,
                                    rtmp::CMixedMap* out);

 private:
  // we use this as temporarily output
  string out_file_tmp_;
  // final output file
  string out_file_;

  // simple writer
  FlvFileWriter writer_;

  // milliseconds between consecutive cuepoints
  const uint32 cue_ms_;

  // the metadata saved from the 1st file
  rtmp::CMixedMap metadata_;

  // These flags are needed for the output file header (the header & metadata
  // specify if the file contains audio/video).
  bool has_video_;
  bool has_audio_;

  // count audio/video tag.
  uint64 video_tag_count_;
  uint64 audio_tag_count_;

  // monotonic timestamps
  // for incoming tags (from various input file)
  int64 last_in_timestamp_ms_;
  // for outgoing tags (to output file)
  int64 last_out_timestamp_ms_;

  int64 next_cue_timestamp_ms_;
  vector<CuePoint> cues_;

  DISALLOW_EVIL_CONSTRUCTORS(FlvJoinProcessor);
};

// Joins multiple files into a single big flv file.
// The input file format The output file will
// contain cuePoints to enable seek.
// return: the duration of the output file in milliseconds,
//         or -1 on failure.
//         It return 0 is the files contain only metadata.
int64 JoinFilesIntoFlv(const vector<string>& in_files,
                       const string& out_file,
                       uint32 cue_ms_time);

// Same JoinFiles but with only 1 input file -> 1 output file..
// The reason for 1 file -> 1 file transformation is that
// the output file contains cuePoints thus becoming seekable.
int64 JoinFilesIntoFlv(const string& in_file,
                       const string& out_file,
                       uint32 cue_ms_time);

}

#endif  // __MEDIA_FLV_FLV_JOINER_H__
