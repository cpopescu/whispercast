// Copyright (c) 2012, Whispersoft s.r.l.
// All rights reserved.
// Author: Cosmin Tudorache

// Prints the tags from a media file.

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/base/errno.h>

#include <whisperstreamlib/base/media_file_reader.h>
#include <whisperstreamlib/base/media_file_writer.h>

//////////////////////////////////////////////////////////////////////

DEFINE_string(in,
              "",
              "The media file to read.");
DEFINE_string(out,
              "",
              "The media file to write.");
DEFINE_string(output_format,
              "",
              "The format of the output file. Can be: flv, f4v.");

//////////////////////////////////////////////////////////////////////

int main(int argc, char* argv[]) {
  common::Init(argc, argv);
  if ( FLAGS_in.empty() ) {
    LOG_ERROR << "Please specify input file !";
    common::Exit(1);
  }
  if ( FLAGS_out.empty() ) {
    LOG_ERROR << "Please specify output file !";
    common::Exit(1);
  }
  if ( FLAGS_output_format.empty() ) {
    LOG_ERROR << "Please specify output format !";
    common::Exit(1);
  }

  streaming::MediaFileReader reader;
  if ( !reader.Open(FLAGS_in) ) {
    LOG_ERROR << "Cannot open input file: [" << FLAGS_in << "]";
    common::Exit(1);
  }

  streaming::MediaFormat out_format;
  if ( !streaming::MediaFormatFromSmallType(FLAGS_output_format, &out_format) ) {
    LOG_ERROR << "Invalid output format: " << FLAGS_output_format;
    common::Exit(1);
  }

  streaming::MediaFileWriter writer;
  if ( !writer.Open(FLAGS_out, out_format) ) {
    LOG_ERROR << "Cannot open output file: [" << FLAGS_out << "]"
                 ", error: " << GetLastSystemErrorDescription();
    common::Exit(1);
  }

  while ( true ) {
    int64 timestamp_ms = 0;
    scoped_ref<streaming::Tag> tag;
    streaming::TagReadStatus result = reader.Read(&tag, &timestamp_ms);
    if ( result == streaming::READ_EOF ) {
      break;
    }
    if ( result == streaming::READ_SKIP ) {
      continue;
    }
    if ( result != streaming::READ_OK ) {
      LOG_ERROR << "Failed to read next tag, result: "
                << streaming::TagReadStatusName(result);
      common::Exit(1);
    }
    if ( !writer.Write(tag.get(), timestamp_ms) ) {
      LOG_ERROR << "Failed to write tag: " << tag->ToString();
      common::Exit(1);
    }
  }
  if ( !writer.Finalize(NULL, NULL) ) {
    LOG_ERROR << "Failed to finalize output file";
    common::Exit(1);
  }
  reader.Close();

  LOG_INFO << "Success.";
}
