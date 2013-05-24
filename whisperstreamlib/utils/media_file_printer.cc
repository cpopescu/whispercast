// Copyright (c) 2012, Whispersoft s.r.l.
// All rights reserved.
// Author: Cosmin Tudorache

// Prints the tags from a media file.

#include <stdio.h>

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/base/errno.h>

#include <whisperstreamlib/base/media_file_reader.h>
#include <whisperstreamlib/flv/flv_tag.h>
#include <whisperstreamlib/f4v/f4v_tag.h>

//////////////////////////////////////////////////////////////////////

DEFINE_string(in,
              "",
              "The media file to read.");
DEFINE_bool(just_info,
            false,
            "If true, it stop right after media info, metadata or MOOV");
DEFINE_bool(generic_tags,
            false,
            "If true, it prints media info and generic tags. "
            "If false, it prints specific tags "
            "(like flv, f4v, ..., depending on the media file)");

//////////////////////////////////////////////////////////////////////

void PrintTag(const streaming::Tag* tag, int64 ts, uint64 file_pos) {
  std::cout << "# pos: " << file_pos << ", ts: " << ts
           << ", tag: " << tag->ToString() << std::endl;
}

int main(int argc, char* argv[]) {
  common::Init(argc, argv);
  CHECK(!FLAGS_in.empty()) << " Please specify input file !";
  streaming::MediaFileReader reader;
  if ( !reader.Open(FLAGS_in) ) {
    LOG_ERROR << "Cannot open input file: [" << FLAGS_in << "]";
    common::Exit(1);
  }
  reader.splitter()->set_generic_tags(FLAGS_generic_tags);

  while ( true ) {
    int64 timestamp_ms;
    scoped_ref<streaming::Tag> tag;
    streaming::TagReadStatus result = reader.Read(&tag, &timestamp_ms);
    if ( result == streaming::READ_EOF ) {
      LOG_INFO << "EOF @offset: " << reader.Position()
               << ", remaining bytes: " << reader.Remaining();
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

    PrintTag(tag.get(), timestamp_ms, reader.Position());

    if ( FLAGS_just_info ) {
      if ( tag->type() == streaming::Tag::TYPE_MEDIA_INFO ||
           (tag->type() == streaming::Tag::TYPE_FLV &&
            static_cast<streaming::FlvTag*>(tag.get())->body().type() == streaming::FLV_FRAMETYPE_METADATA &&
            static_cast<streaming::FlvTag*>(tag.get())->metadata_body().name().value() == streaming::kOnMetaData) ||
           (tag->type() == streaming::Tag::TYPE_F4V &&
            static_cast<streaming::F4vTag*>(tag.get())->is_atom() &&
            static_cast<streaming::F4vTag*>(tag.get())->atom()->type() == streaming::f4v::ATOM_MOOV)) {
        common::Exit(0);
      }
    }
  }

  LOG(INFO) << "STATISTICS:\n" << reader.splitter()->stats().ToString();
}
