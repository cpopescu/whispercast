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
#include <whisperlib/common/base/scoped_ptr.h>
#include <whisperlib/common/base/timer.h>
#include <whisperlib/common/base/date.h>
#include <whisperlib/common/io/ioutil.h>
#include <whisperlib/common/io/file/file_input_stream.h>
#include <whisperlib/common/io/buffer/memory_stream.h>

#include <whisperstreamlib/f4v/f4v_to_flv.h>
#include <whisperstreamlib/f4v/f4v_util.h>
#include <whisperstreamlib/f4v/f4v_file_reader.h>
#include <whisperstreamlib/f4v/atoms/movie/trak_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/mdia_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/minf_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/stbl_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/stsd_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/avc1_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/mp4a_atom.h>

#include <whisperstreamlib/rtmp/objects/rtmp_objects.h>
#include <whisperstreamlib/rtmp/objects/amf/amf_util.h>
#include <whisperstreamlib/flv/flv_coder.h>
#include <whisperstreamlib/flv/flv_tag.h>
#include <whisperstreamlib/flv/flv_file_writer.h>


namespace streaming {

F4vToFlvConverter::F4vToFlvConverter()
    : previous_tag_size_(0) {
}
F4vToFlvConverter::~F4vToFlvConverter() {
}

void F4vToFlvConverter::ConvertTag(const F4vTag* tag,
                                   vector< scoped_ref<FlvTag> >* flv_tags) {
  if ( tag->is_atom() ) {
    const f4v::BaseAtom* atom = tag->atom();
    ConvertAtom(tag->timestamp_ms(), atom, flv_tags);
  } else {
    CHECK(tag->is_frame());
    const f4v::Frame* frame = tag->frame();
    FlvTag* flv_tag = NULL;
    if ( frame->header().type() == f4v::FrameHeader::AUDIO_FRAME ) {
      flv_tag = CreateAudioTag(tag->timestamp_ms(), false, frame->data());
    } else if ( frame->header().type() == f4v::FrameHeader::VIDEO_FRAME ) {
      flv_tag = CreateVideoTag(tag->timestamp_ms(), false,
                               frame->header().is_keyframe(), frame->data());
    }
    CHECK_NOT_NULL(flv_tag);
    CHECK_EQ(tag->timestamp_ms(), flv_tag->timestamp_ms());
    AppendTag(flv_tag, flv_tags);
  }
}
FlvTag* F4vToFlvConverter::CreateCuePoint(
    const streaming::f4v::FrameHeader& frame_header,
    int64 timestamp,
    uint32 cue_point_number) {
  rtmp::CMixedMap* parameters = new rtmp::CMixedMap();
  parameters->Set("pos", new rtmp::CNumber(frame_header.offset()));

  FlvTag::Metadata* metadata = new FlvTag::Metadata();
  metadata->mutable_name()->set_value(kOnCuePoint);
  metadata->mutable_values()->Set("name",
      new rtmp::CString(strutil::StringPrintf("point_%06d", cue_point_number)));
  metadata->mutable_values()->Set("parameters", parameters);
  metadata->mutable_values()->Set("type", new rtmp::CString("navigation"));
  metadata->mutable_values()->Set("time",
      new rtmp::CNumber(frame_header.timestamp()/1000));

  return new FlvTag(Tag::ATTR_METADATA, kDefaultFlavourMask,
      timestamp, metadata);
}

//////////////////////////////////////////////////////////////////////

void F4vToFlvConverter::ConvertAtom(int64 timestamp,
                                    const f4v::BaseAtom* atom,
                                    vector< scoped_ref<FlvTag> >* flv_tags) {
  if ( atom->type() == f4v::ATOM_MOOV ) {
    const f4v::MoovAtom* moov =
        static_cast<const f4v::MoovAtom*>(atom);
    ConvertMoov(timestamp, moov, flv_tags);
  }
}

void F4vToFlvConverter::AppendTag(FlvTag* flv_tag,
                                  vector< scoped_ref<FlvTag> >* flv_tags) {
  flv_tag->set_previous_tag_size(previous_tag_size_);
  previous_tag_size_ = flv_tag->body().Size();
  flv_tags->push_back(scoped_ref<FlvTag>(flv_tag));
}
void F4vToFlvConverter::ConvertMoov(int64 timestamp,
                                    const f4v::MoovAtom* moov,
                                    vector< scoped_ref<FlvTag> >* flv_tags) {
  AppendTag(GetMetadata(timestamp, moov), flv_tags);
  const f4v::AvccAtom* avcc = f4v::util::GetAvccAtom(*moov);
  if ( avcc != NULL ) {
    AppendTag(GetAvcc(timestamp, avcc), flv_tags);
  }
  const f4v::EsdsAtom* esds = f4v::util::GetEsdsAtom(*moov);
  if ( esds != NULL ) {
    AppendTag(GetEsds(timestamp, esds), flv_tags);
  }
}

FlvTag* F4vToFlvConverter::CreateAudioTag(int64 timestamp,
                                          bool is_aac_header,
                                          const io::MemoryStream& data) {
  static const char audio_aac_header[] = {0xaf, 0};
  static const char audio_header[] = {0xaf, 1};
  io::MemoryStream buf;
  if ( is_aac_header ) {
    buf.Write(audio_aac_header, sizeof(audio_aac_header));
  } else {
    buf.Write(audio_header, sizeof(audio_aac_header));
  }
  buf.AppendStreamNonDestructive(&data);

  FlvTag* flv_tag = new FlvTag(Tag::ATTR_AUDIO, kDefaultFlavourMask,
      timestamp, FLV_FRAMETYPE_AUDIO);
  TagReadStatus result = flv_tag->mutable_body().Decode(buf, buf.Size());
  CHECK_EQ(result, READ_OK);
  return flv_tag;
}

FlvTag* F4vToFlvConverter::CreateVideoTag(int64 timestamp,
                                          bool is_avc_header,
                                          bool is_keyframe,
                                          const io::MemoryStream& data) {
  static const char video_avc_header[] = {0x17, 0, 0, 0, 0};
  static const char video_keyframe_header[] = {0x17, 1, 0, 0, 0};
  static const char video_frame_header[] = {0x27, 1, 0, 0, 0};
  io::MemoryStream buf;
  if ( is_avc_header ) {
    buf.Write(video_avc_header, sizeof(video_avc_header));
  } else {
    if ( is_keyframe ) {
      buf.Write(video_keyframe_header, sizeof(video_keyframe_header));
    } else {
      buf.Write(video_frame_header, sizeof(video_frame_header));
    }
  }
  buf.AppendStreamNonDestructive(&data);

  FlvTag* flv_tag = new FlvTag(Tag::ATTR_VIDEO, kDefaultFlavourMask,
      timestamp, FLV_FRAMETYPE_VIDEO);
  TagReadStatus result = flv_tag->mutable_body().Decode(buf, buf.Size());
  CHECK_EQ(result, READ_OK);
  return flv_tag;
}

FlvTag* F4vToFlvConverter::GetEsds(int64 timestamp,
                                const f4v::EsdsAtom* esds) {
  return CreateAudioTag(timestamp, true, esds->extra_data());
}

FlvTag* F4vToFlvConverter::GetAvcc(int64 timestamp,
                                const f4v::AvccAtom* avcc) {
  // Extra data is the whole AVCC body, without type & size (8 bytes).
  io::MemoryStream ms;
  f4v_encoder_.WriteAtom(ms, *avcc);
  ms.Skip(8);
  return CreateVideoTag(timestamp, true, true, ms);
}

FlvTag* F4vToFlvConverter::GetMetadata(int64 timestamp,
                                       const f4v::MoovAtom* moov) {
  f4v::util::MediaInfo media_info;
  f4v::util::ExtractMediaInfo(*moov, &media_info);

  // Create metadata with stream info
  //
  FlvTag::Metadata* metadata = new FlvTag::Metadata();
  metadata->mutable_name()->set_value(kOnMetaData);
  metadata->mutable_values()->Set("aacaot",
      new rtmp::CNumber(media_info.aacaot_));
  metadata->mutable_values()->Set("audiochannels",
      new rtmp::CNumber(media_info.audio_channels_));
  metadata->mutable_values()->Set("audiocodecid",
      new rtmp::CString(media_info.audio_codec_id_));
  metadata->mutable_values()->Set("avclevel",
      new rtmp::CNumber(media_info.avc_level_));
  metadata->mutable_values()->Set("avcprofile",
      new rtmp::CNumber(media_info.avc_profile_));
  metadata->mutable_values()->Set("duration",
      new rtmp::CNumber(media_info.duration_ / 1000.0f));
  metadata->mutable_values()->Set("height",
      new rtmp::CNumber(media_info.height_));
  metadata->mutable_values()->Set("videocodecid",
      new rtmp::CString(media_info.video_codec_id_));
  metadata->mutable_values()->Set("videoframerate",
      new rtmp::CNumber(media_info.video_frame_rate_));
  metadata->mutable_values()->Set("width",
      new rtmp::CNumber(media_info.width_));

  rtmp::CStringMap* video_track = new rtmp::CStringMap();
  video_track->Set("length",
      new rtmp::CNumber(media_info.video_.length_));
  video_track->Set("timescale",
      new rtmp::CNumber(media_info.video_.timescale_));
  video_track->Set("language",
      new rtmp::CString(media_info.video_.language_));
  rtmp::CStringMap* video_track_sampledescription_0 = new rtmp::CStringMap();
  video_track_sampledescription_0->Set("sampletype",
      new rtmp::CString(media_info.video_codec_id_));
  rtmp::CArray* video_track_sampledescription = new rtmp::CArray(1);
  video_track_sampledescription->mutable_data()[0] =
      video_track_sampledescription_0;
  video_track->Set("sampledescription", video_track_sampledescription);

  rtmp::CStringMap* audio_track = new rtmp::CStringMap();
  audio_track->Set("length",
      new rtmp::CNumber(media_info.audio_.length_));
  audio_track->Set("timescale",
      new rtmp::CNumber(media_info.audio_.timescale_));
  audio_track->Set("language",
      new rtmp::CString(media_info.audio_.language_));
  rtmp::CStringMap* audio_track_sampledescription_0 = new rtmp::CStringMap();
  audio_track_sampledescription_0->Set("sampletype",
      new rtmp::CString(media_info.audio_codec_id_));
  rtmp::CArray* audio_track_sampledescription = new rtmp::CArray(1);
  audio_track_sampledescription->mutable_data()[0] =
      audio_track_sampledescription_0;
  audio_track->Set("sampledescription", audio_track_sampledescription);

  rtmp::CArray* trackinfo = new rtmp::CArray(2);
  trackinfo->mutable_data()[0] = video_track;
  trackinfo->mutable_data()[1] = audio_track;
  metadata->mutable_values()->Set("trackinfo", trackinfo);

  return new FlvTag(Tag::ATTR_METADATA, kDefaultFlavourMask,
                    timestamp, metadata);
}

//////////////////////////////////////////////////////////////////////////////
// ConvertF4vToFlv

bool ConvertF4vToFlv(const string& in_f4v_file, const string& out_flv_file) {
  // prepare input file
  streaming::f4v::FileReader file_reader;
  if ( !file_reader.Open(in_f4v_file) ) {
    LOG_ERROR << "Failed to read file: [" << in_f4v_file << "]";
    return false;
  }

  // prepare the output file
  FlvFileWriter file_writer(true, true);
  if ( !file_writer.Open(out_flv_file) ) {
    LOG_ERROR << "Failed to write file: [" << out_flv_file << "]";
    return false;
  }

  streaming::F4vToFlvConverter converter;

  uint32 in_tag_count = 0;
  uint32 out_tag_count = 0;
  while ( true ) {
    // 1. read f4v_tag
    scoped_ref<streaming::F4vTag> f4v_tag;
    streaming::f4v::TagDecodeStatus result = file_reader.Read(&f4v_tag);
    if ( result == streaming::f4v::TAG_DECODE_NO_DATA ) {
      // EOS reached complete
      break;
    }
    if ( result == streaming::f4v::TAG_DECODE_ERROR ) {
      LOG_ERROR << "Error reading f4v tag";
      return false;
    }
    CHECK_EQ(result, streaming::f4v::TAG_DECODE_SUCCESS);
    CHECK_NOT_NULL(f4v_tag.get());
    in_tag_count++;
    LOG_DEBUG << "Read F4vTag: " << f4v_tag->ToString();

    // 2. maybe print media info (from MOOV atom)
    if ( f4v_tag->is_atom() &&
         f4v_tag->atom()->type() == streaming::f4v::ATOM_MOOV ) {
      const streaming::f4v::MoovAtom& moov =
        static_cast<const streaming::f4v::MoovAtom&>(*f4v_tag->atom());
      streaming::f4v::util::MediaInfo media_info;
      streaming::f4v::util::ExtractMediaInfo(moov, &media_info);
      LOG_INFO << "F4V MediaInfo: " << media_info.ToString();
    }

    // 3. convert f4v_tag to flv_tag s
    //    Ftyp Atom is converted to 0 flv tags.
    //    Moov Atom is converted to 3 flv tags (metadata + aac init + avc init).
    //    A frame is converted to 1 flv tag.
    vector< scoped_ref<streaming::FlvTag> > flv_tags;
    converter.ConvertTag(f4v_tag.get(), &flv_tags);

    // 4. write flv_tags
    for ( uint32 i = 0; i < flv_tags.size(); i++ ) {
      scoped_ref<streaming::FlvTag>& flv_tag = flv_tags[i];
      file_writer.Write(*flv_tag, -1);
      out_tag_count++;
      LOG_DEBUG << "Write FlvTag: " << flv_tag->ToString();
    }
    flv_tags.clear();
  }

  if ( file_reader.Remaining() > 0 ) {
    LOG_ERROR << "Useless bytes at file end: " << file_reader.Remaining()
              << " bytes, at file offset: " << file_reader.Position()
              << ", in file: [" << in_f4v_file << "]";
  }

  LOG_INFO << "Conversion succeeded." << endl
           << "INPUT: " << endl
           << " - " << in_tag_count << " f4v tags read" << endl
           << "OUTPUT: " << endl
           << " - " << out_tag_count << " flv tags written";

  return true;
}

}
