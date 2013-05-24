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

#include <whisperstreamlib/base/media_info.h>
#include <whisperstreamlib/base/media_info_util.h>

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
  : audio_format_(MediaInfo::Audio::FORMAT_AAC),
    video_format_(MediaInfo::Video::FORMAT_H264) {
}
F4vToFlvConverter::~F4vToFlvConverter() {
}

void F4vToFlvConverter::ConvertTag(const F4vTag* tag,
                                   vector< scoped_ref<FlvTag> >* flv_tags) {
  if ( tag->is_atom() ) {
    const f4v::BaseAtom* atom = tag->atom();
    ConvertAtom(tag->timestamp_ms(), atom, flv_tags);
    return;
  }
  CHECK(tag->is_frame());
  const f4v::Frame* frame = tag->frame();
  FlvTag* flv_tag = NULL;
  switch ( frame->header().type() ) {
    case f4v::FrameHeader::AUDIO_FRAME:
      flv_tag = CreateAudioTag(tag->timestamp_ms(), false, frame->data());
      break;
    case f4v::FrameHeader::VIDEO_FRAME:
      flv_tag = CreateVideoTag(tag->timestamp_ms(),
                              tag->composition_offset_ms(),
                              false, frame->header().is_keyframe(),
                              frame->data());
      break;
    case f4v::FrameHeader::RAW_FRAME:
      return;
  }
  CHECK_NOT_NULL(flv_tag);
  CHECK_EQ(tag->timestamp_ms(), flv_tag->timestamp_ms());
  AppendTag(flv_tag, flv_tags);
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
  flv_tags->push_back(scoped_ref<FlvTag>(flv_tag));
}
void F4vToFlvConverter::ConvertMoov(int64 timestamp,
                                    const f4v::MoovAtom* moov,
                                    vector< scoped_ref<FlvTag> >* flv_tags) {
  vector<f4v::FrameHeader*> empty_frames;
  MediaInfo info;
  if ( !util::ExtractMediaInfoFromMoov(*moov, empty_frames, &info) ) {
    LOG_ERROR << "ExtractMediaInfoFromMoov() failed for moov: "
              << moov->ToString();
    return;
  }
  if ( info.has_audio() ) { audio_format_ = info.audio().format_; }
  if ( info.has_video() ) { video_format_ = info.video().format_; }

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
  // Extract from adobe-video_file_format_spec_v10.pdf
  // Audio frame format:
  //  4 bits: Sound format (2 = MP3, 10 = AAC)
  //  2 bits: Sound rate (0 - 5.5kHz, 1 - 11, 2 - 22, 3 - 44;  always 3 on AAC)
  //  1 bit : Sound size (0 - 8 bit, 1 - 16 bit)
  //  1 bit : Sound type (0 - mono, 1 - stereo)
  //  8 bits: Packet type (0 - AAC header, 1 - AAC raw data)
  //  ....... data
  uint8 audio_first_byte = 0x0f; // assume: 44kHz, 16 bit, stereo
  switch ( audio_format_ ) {
    case MediaInfo::Audio::FORMAT_AAC: audio_first_byte |= 0xa0; break;
    case MediaInfo::Audio::FORMAT_MP3: audio_first_byte |= 0x20; break;
  }
  uint8 audio_second_byte = is_aac_header ? 1 : 0;

  io::MemoryStream buf;
  io::NumStreamer::WriteByte(&buf, audio_first_byte);
  io::NumStreamer::WriteByte(&buf, audio_second_byte);
  buf.AppendStreamNonDestructive(&data);

  FlvTag* flv_tag = new FlvTag(Tag::ATTR_AUDIO, kDefaultFlavourMask,
      timestamp, FLV_FRAMETYPE_AUDIO);
  TagReadStatus result = flv_tag->mutable_body().Decode(buf, buf.Size());
  CHECK_EQ(result, READ_OK);
  return flv_tag;
}

FlvTag* F4vToFlvConverter::CreateVideoTag(int64 timestamp,
                                          int64 composition_offset_ms,
                                          bool is_avc_header,
                                          bool is_keyframe,
                                          const io::MemoryStream& data) {
  // Extract from adobe-video_file_format_spec_v10.pdf
  // Video frame format:
  //  4 bits: Frame type (1 keyframe, 2 interframe, 3 disposable H263 interframe)
  //  4 bits: Codec ID (1 = JPEG, 2 = Sorenson H263, 3 = Screen Video, 4 = VP6,
  //                    5 = VP6 with Alpha, 6 = Screen Video v2, 7 = AVC)
  //
  // If codecID==7: the AVCVideoPacket follows:
  //  8 bits: Packet type (0 = sequence header, 1 = NALU, 2 = end of sequence)
  // 24 bits: Composition time offset (only if Packet Type == 1, otherwise 0)
  // .......: data (if Packet Type == 0: decoder config
  //                if Packet Type == 1: 1 or more NALUs
  //                if Packet Type == 2: empty)
  uint8 video_first_byte = (is_avc_header || is_keyframe) ? 0x10 : 0x20;
  switch ( video_format_ ) {
    case MediaInfo::Video::FORMAT_H263:   video_first_byte |= 0x02; break;
    case MediaInfo::Video::FORMAT_H264:   video_first_byte |= 0x07; break;
    case MediaInfo::Video::FORMAT_ON_VP6: video_first_byte |= 0x04; break;
  }

  io::MemoryStream buf;
  io::NumStreamer::WriteByte(&buf, video_first_byte);
  if ( is_avc_header ) {
    io::NumStreamer::WriteByte(&buf, 0x00); // 0 = sequence header
  } else {
    io::NumStreamer::WriteByte(&buf, 0x01); // 1 = NALUs
  }
  io::NumStreamer::WriteUInt24(&buf, composition_offset_ms, common::BIGENDIAN);
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
  f4v::Encoder().WriteAtom(ms, *avcc);
  ms.Skip(8);
  return CreateVideoTag(timestamp, 0, true, true, ms);
}

FlvTag* F4vToFlvConverter::GetMetadata(int64 timestamp,
                                       const f4v::MoovAtom* moov) {
  MediaInfo info;
  f4v::util::ExtractMediaInfo(*moov, &info);

  string audio_codec_id; // specific FLV 'audiocodecid' constants
  switch ( info.audio().format_ ) {
    case MediaInfo::Audio::FORMAT_AAC: audio_codec_id = "mp4a"; break;
    case MediaInfo::Audio::FORMAT_MP3: audio_codec_id = "2"; break;
  }

  string video_codec_id; // specific FLV 'videocodecid' constants
  switch ( info.video().format_ ) {
    case MediaInfo::Video::FORMAT_H263: video_codec_id = "2"; break;
    case MediaInfo::Video::FORMAT_H264: video_codec_id = "avc1"; break;
    case MediaInfo::Video::FORMAT_ON_VP6: video_codec_id = "4"; break;
  }

  // Create metadata with stream info
  //
  FlvTag::Metadata* metadata = new FlvTag::Metadata();
  rtmp::CStringMap* audio_track = NULL;
  rtmp::CStringMap* video_track = NULL;
  metadata->mutable_name()->set_value(kOnMetaData);
  if ( info.has_audio() ) {
    metadata->mutable_values()->Set("audiocodecid",
        new rtmp::CString(audio_codec_id));
    metadata->mutable_values()->Set("audiochannels",
        new rtmp::CNumber(info.audio().channels_));
    if ( info.audio().format_ == MediaInfo::Audio::FORMAT_AAC ) {
      //TODO(cosmin): Compute 'aacaot' from somewhere.
      //   According to Adobe AS3 Class FLVMetaData:
      //   aacaot = The AAC audio object type; 0, 1, or 2 are supported.
      metadata->mutable_values()->Set("aacaot",
          new rtmp::CNumber(2)); // harcoded
    }

    audio_track = new rtmp::CStringMap();
    audio_track->Set("timescale",
        new rtmp::CNumber(info.audio().sample_rate_));
    audio_track->Set("length",
        new rtmp::CNumber(info.duration_ms() / 1000.0 *
                          info.audio().sample_rate_));
    audio_track->Set("language",
        new rtmp::CString(info.audio().mp4_language_));
    rtmp::CStringMap* audio_track_sampledescription_0 = new rtmp::CStringMap();
    audio_track_sampledescription_0->Set("sampletype",
        new rtmp::CString(audio_codec_id));
    rtmp::CArray* audio_track_sampledescription = new rtmp::CArray(1);
    audio_track_sampledescription->mutable_data()[0] =
        audio_track_sampledescription_0;
    audio_track->Set("sampledescription", audio_track_sampledescription);
  }
  if ( info.has_video() ) {
    metadata->mutable_values()->Set("videocodecid",
        new rtmp::CString(video_codec_id));
    metadata->mutable_values()->Set("width",
        new rtmp::CNumber(info.video().width_));
    metadata->mutable_values()->Set("height",
        new rtmp::CNumber(info.video().height_));
    metadata->mutable_values()->Set("videoframerate",
        new rtmp::CNumber(info.video().frame_rate_));
    if ( info.video().format_ == MediaInfo::Video::FORMAT_H264 ) {
      metadata->mutable_values()->Set("avclevel",
          new rtmp::CNumber(info.video().h264_level_));
      metadata->mutable_values()->Set("avcprofile",
          new rtmp::CNumber(info.video().h264_profile_));
    }

    video_track = new rtmp::CStringMap();
    video_track->Set("timescale",
        new rtmp::CNumber(info.video().timescale_));
    video_track->Set("length",
        new rtmp::CNumber(info.duration_ms() / 1000.0 *
                          info.video().timescale_));
    video_track->Set("language",
        new rtmp::CString(""));
    rtmp::CStringMap* video_track_sampledescription_0 = new rtmp::CStringMap();
    video_track_sampledescription_0->Set("sampletype",
        new rtmp::CString(video_codec_id));
    rtmp::CArray* video_track_sampledescription = new rtmp::CArray(1);
    video_track_sampledescription->mutable_data()[0] =
        video_track_sampledescription_0;
    video_track->Set("sampledescription", video_track_sampledescription);
  }
  metadata->mutable_values()->Set("duration",
      new rtmp::CNumber(info.duration_ms() / 1000.0));

  rtmp::CArray* trackinfo = new rtmp::CArray();
  if ( video_track != NULL ) {
    trackinfo->mutable_data().push_back(video_track);
  }
  if ( audio_track != NULL ) {
    trackinfo->mutable_data().push_back(audio_track);
  }
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
      streaming::MediaInfo media_info;
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
