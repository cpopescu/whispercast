// (c) 2012, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Cosmin Tudorache
//

#include <whisperlib/common/base/gflags.h>
#include <whisperstreamlib/base/tag_serializer.h>
#include <whisperstreamlib/aac/aac_tag_splitter.h>
#include <whisperstreamlib/raw/raw_tag_splitter.h>
#include <whisperstreamlib/mp3/mp3_frame.h>
#include <whisperstreamlib/flv/flv_tag.h>
#include <whisperstreamlib/internal/internal_frame.h>
#include <whisperstreamlib/f4v/f4v_encoder.h>
#include <whisperstreamlib/mts/mts_encoder.h>
#include <whisperstreamlib/libav_mts/libav_mts_encoder.h>

DEFINE_bool(enable_libav_mts,
            true,
            "Use the LibAV Mpeg Transport Stream implementation, "
            "instead of the internal TS implementation.");

namespace streaming {

TagSerializer* CreateSerializer(MediaFormat media_format) {
  switch ( media_format ) {
    case MFORMAT_MP3:
      return new streaming::Mp3TagSerializer();
    case MFORMAT_FLV:
      return new streaming::FlvTagSerializer(true, true, true);
    case MFORMAT_F4V:
      return new streaming::F4vTagSerializer();
    case MFORMAT_AAC:
      return new streaming::AacTagSerializer();
    case MFORMAT_MTS:
      return FLAGS_enable_libav_mts ?
          (TagSerializer*)new LibavMtsTagSerializer() :
          (TagSerializer*)new MtsTagSerializer();
    case MFORMAT_INTERNAL:
      return new streaming::InternalTagSerializer();
    case MFORMAT_RAW:
      return new streaming::RawTagSerializer();
  }
  LOG_FATAL << "Illegal media_format_: " << (int)media_format;
  return NULL;
}

} // namespace streaming
