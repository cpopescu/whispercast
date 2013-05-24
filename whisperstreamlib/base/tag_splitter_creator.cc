// (c) 2012, Whispersoft s.r.l.
// All rights reserved.
//
// Author: Cosmin Tudorache
//

#include <whisperstreamlib/base/tag_splitter.h>
#include <whisperstreamlib/aac/aac_tag_splitter.h>
#include <whisperstreamlib/raw/raw_tag_splitter.h>
#include <whisperstreamlib/mp3/mp3_tag_splitter.h>
#include <whisperstreamlib/flv/flv_tag_splitter.h>
#include <whisperstreamlib/f4v/f4v_tag_splitter.h>
#include <whisperstreamlib/mts/mts_decoder.h>
#include <whisperstreamlib/internal/internal_tag_splitter.h>

DEFINE_int32(flv_splitter_max_tag_composition_time_ms,
             0,
             "We compose flv tags in chunks of this size, after they are "
             "extracted by a splitter");
DEFINE_int32(f4v_splitter_max_tag_composition_time_ms,
             0,
             "We compose f4v tags in chunks of this size, after they are "
             "extracted by a splitter");
DEFINE_int32(raw_splitter_bps,
             1 << 20, // 1 mbps
             "Data processing speed for RawTagSplitter.");

namespace streaming {

TagSplitter* CreateSplitter(const string& name, MediaFormat media_format) {
  switch ( media_format ) {
    case streaming::MFORMAT_FLV: {
      streaming::TagSplitter* s = new streaming::FlvTagSplitter(name);
      s->set_max_composition_tag_time_ms(
          FLAGS_flv_splitter_max_tag_composition_time_ms);
      return s;
    }
    case streaming::MFORMAT_F4V: {
      streaming::TagSplitter* s = new streaming::F4vTagSplitter(name);
      s->set_max_composition_tag_time_ms(
          FLAGS_f4v_splitter_max_tag_composition_time_ms);
      return s;
    }
    case streaming::MFORMAT_MP3:
      return new streaming::Mp3TagSplitter(name);
    case streaming::MFORMAT_AAC:
      return new streaming::AacTagSplitter(name);
    case streaming::MFORMAT_MTS:
      return new streaming::MtsTagSplitter(name);
    case streaming::MFORMAT_INTERNAL:
      return new streaming::InternalTagSplitter(name);
    case streaming::MFORMAT_RAW:
      return new streaming::RawTagSplitter(name, FLAGS_raw_splitter_bps);
  }
  LOG_FATAL << "Illegal MediaFormat: " << (int)media_format;
  return NULL;
}

} // namespace streaming
