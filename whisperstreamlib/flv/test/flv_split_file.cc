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
//
// Simple utility that extracts a time interval from a FLV file.
//

#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/system.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/io/file/file_input_stream.h>
#include <whisperlib/common/io/buffer/memory_stream.h>

#include <whisperstreamlib/base/media_file_reader.h>
#include <whisperstreamlib/flv/flv_file_writer.h>
#include <whisperstreamlib/flv/flv_joiner.h>

//////////////////////////////////////////////////////////////////////

DEFINE_string(in_path,
              "",
              "Input file");
DEFINE_string(out_path,
              "",
              "Output file");
DEFINE_int32(start_sec,
             0,
             "We start from this timestamp");
DEFINE_int32(end_sec,
             kMaxInt32,
             "We end at this timestamp");
DEFINE_int32(cue_ms_time,
             5000,
             "Cue points interval in the output file");

//////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
  common::Init(argc, argv);

  string tmp_path = FLAGS_out_path + ".tmp.flv";

  LOG_INFO << "Splitting..";
  streaming::MediaFileReader reader;
  CHECK(reader.Open(FLAGS_in_path)) << "Cannot open input file: ["
                                    << FLAGS_in_path << "]";

  int64 timestamp_ms;
  scoped_ref<streaming::Tag> first_tag;
  streaming::TagReadStatus result = reader.Read(&first_tag, &timestamp_ms);
  if ( result != streaming::READ_OK ) {
    LOG_ERROR << "Failed to read first tag. Input file is probably corrupt.";
    common::Exit(1);
  }
  if ( first_tag->type() != streaming::Tag::TYPE_FLV_HEADER ) {
    LOG_ERROR << "First tag is not the header. Input file is probably corrupt.";
    common::Exit(1);
  }
  streaming::FlvHeader* flv_header =
      static_cast<streaming::FlvHeader*>(first_tag.get());


  streaming::FlvFileWriter writer(flv_header->has_video(),
                                  flv_header->has_audio());
  CHECK(writer.Open(tmp_path)) << "Cannot create output file: ["
                               << tmp_path << "]";

  while ( true ) {
    int64 timestamp_ms;
    scoped_ref<streaming::Tag> tag;
    streaming::TagReadStatus result = reader.Read(&tag, &timestamp_ms);
    if ( result == streaming::READ_EOF ) {
      break;
    }
    if ( result != streaming::READ_OK ) {
      LOG_ERROR << "Failed to read next tag. Input file is probably corrupt.";
      common::Exit(1);
    }
    if ( tag->type() != streaming::Tag::TYPE_FLV ) {
      continue;
    }
    streaming::FlvTag* flv_tag = static_cast<streaming::FlvTag*>(tag.get());
    if ( flv_tag->body().type() == streaming::FLV_FRAMETYPE_METADATA ) {
      writer.Write(*flv_tag, -1);
      continue;
    }
    if ( flv_tag->body().type() == streaming::FLV_FRAMETYPE_VIDEO &&
         flv_tag->video_body().codec() == streaming::FLV_FLAG_VIDEO_CODEC_AVC &&
         flv_tag->video_body().avc_packet_type() == streaming::AVC_SEQUENCE_HEADER ) {
      writer.Write(*flv_tag, -1);
      continue;
    }

    if ( flv_tag->timestamp_ms() >= FLAGS_start_sec * 1000 &&
         flv_tag->timestamp_ms() <= FLAGS_end_sec * 1000 ) {
      writer.Write(*flv_tag, -1);
      continue;
    }
  }
  writer.Close();

  LOG_INFO << "Re-Joining..";
  vector<string> files;
  files.push_back(tmp_path);
  CHECK(streaming::JoinFilesIntoFlv(files, FLAGS_out_path, FLAGS_cue_ms_time) > 0);
  io::Rm(tmp_path);

  LOG_INFO << "DONE";
}
