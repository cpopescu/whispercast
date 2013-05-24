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
// Author: Cosmin Tudorache

#include <whisperstreamlib/base/media_info_util.h>
#include <whisperlib/common/base/scoped_ptr.h>
#include <whisperlib/net/util/base64.h>
#include <whisperstreamlib/base/media_file_reader.h>
#include <whisperstreamlib/rtp/rtp_consts.h>
#include <whisperstreamlib/f4v/f4v_util.h>
#include <whisperstreamlib/f4v/atoms/movie/stsz_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/stts_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/stsc_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/stss_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/mvhd_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/trak_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/tkhd_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/mdia_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/mdhd_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/hdlr_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/minf_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/smhd_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/stbl_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/stsd_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/mp4a_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/avc1_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/avcc_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/vmhd_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/ftyp_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/mdat_atom.h>

namespace streaming {
namespace util {

namespace {

static const uint32 kDefaultTimeScale = 600;
static const uint32 kDefaultPreferredRate = 65536;
static const uint16 kDefaultPreferredVolume = 256;
static const uint32 kHandlerTypeSound = 1936684398;
static const string kHandlerNameSound = "Apple Sound Media Handler";
static const uint32 kHandlerTypeVideo = 1986618469;
static const string kHandlerNameVideo = "Apple Video Media Handler";

static const uint32 kAudioAacHeaderSize = 2;
static const uint8 kAudioAacInit[] = {0xaf, 0};
static const uint8 kAudioAacFrameHeader[] = {0xaf, 1};

static const uint32 kVideoAvcHeaderSize = 5;
static const uint8 kVideoAvcInit[] = {0x17, 0, 0, 0, 0};
static const uint8 kVideoAvcKeyframeHeader[] = {0x17, 1, 0, 0, 0};
static const uint8 kVideoAvcFrameHeader[] = {0x27, 1, 0, 0, 0};

//////////////////////////////////////////////////////////////////////

struct F4vChunk {
  uint64 start_offset_;
  uint32 sample_count_;
  F4vChunk() : start_offset_(0), sample_count_(0) {}
  F4vChunk(uint64 start_offset, uint32 sample_count)
    : start_offset_(start_offset), sample_count_(sample_count) {}
};
struct FlvTrackInfo {
  uint64 length_;
  uint32 timescale_;
  string language_;
  string sampletype_;
  FlvTrackInfo() : length_(0), timescale_(0), language_(), sampletype_() {}
  string ToString() const {
    return strutil::StringPrintf("{length_: %"PRIu64""
                                 ", timescale_: %u"
                                 ", language_: %s"
                                 ", sampletype_: %s}",
                                 (length_),
                                 timescale_,
                                 language_.c_str(),
                                 sampletype_.c_str());
  }
};
ostream& operator<<(ostream& os, const FlvTrackInfo& inf) {
  return os << inf.ToString();
}

// returns MAC OS Time: seconds from 1904 until now
uint64 MacDateNow() {
  return timer::Date::Now()/1000 + streaming::f4v::kSecondsFrom1904to1970;
}
double ExtractNumber(const rtmp::CMixedMap& values, const string& value_name) {
  rtmp::CMixedMap::Map::const_iterator value_it =
    values.data().find(value_name);
  if ( value_it != values.data().end() ) {
    const rtmp::CObject* value_object = value_it->second;
    if ( value_object->object_type() == rtmp::CObject::CORE_NUMBER ) {
      const rtmp::CNumber* value_number =
        static_cast<const rtmp::CNumber*>(value_object);
      return value_number->value();
    }
  }
  return 0.0;
}
bool ExtractBoolean(const rtmp::CMixedMap& values, const string& value_name) {
  rtmp::CMixedMap::Map::const_iterator value_it =
    values.data().find(value_name);
  if ( value_it != values.data().end() ) {
    const rtmp::CObject* value_object = value_it->second;
    if ( value_object->object_type() == rtmp::CObject::CORE_BOOLEAN ) {
      const rtmp::CBoolean* value_boolean =
        static_cast<const rtmp::CBoolean*>(value_object);
      return value_boolean->value();
    }
  }
  return false;
}
string ExtractString(const rtmp::CMixedMap& values, const string& value_name) {
  rtmp::CMixedMap::Map::const_iterator value_it =
    values.data().find(value_name);
  if ( value_it != values.data().end() ) {
    const rtmp::CObject* value_object = value_it->second;
    if ( value_object->object_type() == rtmp::CObject::CORE_STRING ) {
      const rtmp::CString* value_string =
        static_cast<const rtmp::CString*>(value_object);
      return value_string->value();
    }
  }
  return "";
}
double ExtractNumber(const rtmp::CStringMap& values, const string& value_name) {
  rtmp::CStringMap::Map::const_iterator value_it =
    values.data().find(value_name);
  if ( value_it != values.data().end() ) {
    const rtmp::CObject* value_object = value_it->second;
    if ( value_object->object_type() == rtmp::CObject::CORE_NUMBER ) {
      const rtmp::CNumber* value_number =
        static_cast<const rtmp::CNumber*>(value_object);
      return value_number->value();
    }
  }
  return 0.0;
}
string ExtractString(const rtmp::CStringMap& values, const string& value_name) {
  rtmp::CStringMap::Map::const_iterator value_it =
    values.data().find(value_name);
  if ( value_it != values.data().end() ) {
    const rtmp::CObject* value_object = value_it->second;
    if ( value_object->object_type() == rtmp::CObject::CORE_STRING ) {
      const rtmp::CString* value_string =
        static_cast<const rtmp::CString*>(value_object);
      return value_string->value();
    }
  }
  return "";
}
void ExtractTrackInfo(const rtmp::CMixedMap& values,
                      vector<FlvTrackInfo>* out) {
  rtmp::CMixedMap::Map::const_iterator trackinfo_it =
    values.data().find("trackinfo");
  if ( trackinfo_it == values.data().end() ) {
    return;
  }
  const rtmp::CObject* trackinfo_object = trackinfo_it->second;
  if ( trackinfo_object->object_type() != rtmp::CObject::CORE_ARRAY ) {
    return;
  }
  const rtmp::CArray* trackinfo_array =
    static_cast<const rtmp::CArray*>(trackinfo_object);
  for ( uint32 i = 0; i < trackinfo_array->data().size(); i++ ) {
    const rtmp::CObject* track_object = trackinfo_array->data()[i];
    if ( track_object->object_type() != rtmp::CObject::CORE_STRING_MAP ) {
      continue;
    }
    const rtmp::CStringMap* track_map =
      static_cast<const rtmp::CStringMap*>(track_object);
    FlvTrackInfo trackinfo;
    trackinfo.length_ = static_cast<uint64>(
        ExtractNumber(*track_map, "length"));
    trackinfo.timescale_ = static_cast<uint32>(
        ExtractNumber(*track_map, "timescale"));
    trackinfo.language_ = ExtractString(*track_map, "language");

    rtmp::CStringMap::Map::const_iterator sampledescription_it =
        track_map->data().find("sampledescription");
    if ( sampledescription_it == track_map->data().end() ) {
      LOG_ERROR << "missing 'sampledescription' from values map: "
                << values.ToString();
      continue;
    }
    const rtmp::CObject* sampledesc_object = sampledescription_it->second;
    if ( sampledesc_object->object_type() != rtmp::CObject::CORE_ARRAY ) {
      LOG_ERROR << "strange 'sampledescription', should be array, found: "
                << sampledesc_object->ToString() << ", in values map: "
                << values.ToString();
      continue;
    }
    const rtmp::CArray* sampledescription_array =
      static_cast<const rtmp::CArray*>(sampledesc_object);
    if ( sampledescription_array->data().size() != 1 ) {
      LOG_ERROR << "strange 'sampledescription', should contain just 1 item"
                   ", found: " << sampledescription_array->ToString()
                << ", in values map: " << values.ToString();
      continue;
    }
    rtmp::CObject* sd_object = sampledescription_array->data()[0];
    if ( sd_object->object_type() != rtmp::CObject::CORE_STRING_MAP ) {
      LOG_ERROR << "strange item in 'sampledescription' array"
                   ", should be a string map, found: " << sd_object->ToString()
                << ", in values map: " << values.ToString();
      continue;
    }
    const rtmp::CStringMap* sd_map =
      static_cast<const rtmp::CStringMap*>(sd_object);
    rtmp::CStringMap::Map::const_iterator sampletype_it =
      sd_map->data().find("sampletype");
    if ( sampletype_it == sd_map->data().end() ) {
      LOG_ERROR << "missing 'sampletype', in values map: " << values.ToString();
      continue;
    }
    rtmp::CObject* sampletype_object = sampletype_it->second;
    if ( sampletype_object->object_type() != rtmp::CObject::CORE_STRING ) {
      LOG_ERROR << "strange 'sampletype', should be a string, found: "
                << sampletype_object->object_type() << ", in values map: "
                << values.ToString();
      continue;
    }
    const rtmp::CString* sampletype =
      static_cast<const rtmp::CString*>(sampletype_object);
    trackinfo.sampletype_ = sampletype->value();

    out->push_back(trackinfo);
  }
}
int64 MeasureAtom(const streaming::f4v::BaseAtom& atom) {
  io::MemoryStream ms;
  streaming::f4v::Encoder encoder;
  encoder.WriteAtom(ms, atom);
  return ms.Size();
}
void UpdateAtomSize(streaming::f4v::BaseAtom& atom) {
  vector<const streaming::f4v::BaseAtom*> subatoms;
  atom.GetSubatoms(subatoms);
  for ( uint32 i = 0; i < subatoms.size(); i++ ) {
    streaming::f4v::BaseAtom* subatom =
      const_cast<streaming::f4v::BaseAtom*>(subatoms[i]);
    UpdateAtomSize(*subatom);
  }
  atom.set_body_size(MeasureAtom(atom) - 8);
  CHECK(!atom.is_extended_size());
}

uint64 FramesSize(const streaming::f4v::MoovAtom& moov, bool audio) {
  const streaming::f4v::StszAtom* stsz = streaming::f4v::util::GetStszAtom(moov, audio);
  const string fhead = audio ? "FramesSize audio " : "FramesSize video ";
  if ( stsz == NULL ) {
    F4V_LOG_ERROR << fhead << "no StszAtom found";
    return 0;
  }
  if ( stsz->uniform_sample_size() > 0 ) {
    F4V_LOG_ERROR << fhead << "dont't know how to handle uniform_sample_size";
    return 0;
  }
  uint64 size = 0;
  for ( uint32 i = 0; i < stsz->records().size(); i++ ) {
    size += stsz->records()[i]->entry_size_;
  }
  return size;
}
uint64 FramesSize(const streaming::f4v::MoovAtom& moov) {
  return FramesSize(moov, false) +
         FramesSize(moov, true);
}
void UpdateFramesOffset(streaming::f4v::MoovAtom* moov, bool audio, int32 offset) {
  streaming::f4v::StcoAtom* stco = streaming::f4v::util::GetStcoAtom(*moov, audio);
  const string fhead = audio ? "UpdateFrameOffset audio " :
                               "UpdateFrameOffset video ";
  if ( stco == NULL ) {
    F4V_LOG_ERROR << fhead << "no StszAtom found";
    return;
  }
  // NOTE !! offset can be negative !!
  for ( uint32 i = 0; i < stco->records().size(); i++ ) {
    stco->records()[i]->chunk_offset_ =
        int32(stco->records()[i]->chunk_offset_) + offset;
  }
}
void UpdateFramesOffset(streaming::f4v::MoovAtom* moov, int32 offset) {
  UpdateFramesOffset(moov, false, offset);
  UpdateFramesOffset(moov, true , offset);
}
}

FlvFlagVideoCodec VideoFormatToFlv(MediaInfo::Video::Format video_format) {
  switch ( video_format ) {
    case MediaInfo::Video::FORMAT_H263: return FLV_FLAG_VIDEO_CODEC_H263;
    case MediaInfo::Video::FORMAT_H264: return FLV_FLAG_VIDEO_CODEC_AVC;
    case MediaInfo::Video::FORMAT_ON_VP6: return FLV_FLAG_VIDEO_CODEC_VP6;
  }
  LOG_FATAL << "Illegal video_format: " << (int)video_format;
  return FLV_FLAG_VIDEO_CODEC_JPEG;
}
FlvFlagSoundFormat AudioFormatToFlv(MediaInfo::Audio::Format audio_format) {
  switch ( audio_format ) {
    case MediaInfo::Audio::FORMAT_AAC: return FLV_FLAG_SOUND_FORMAT_AAC;
    case MediaInfo::Audio::FORMAT_MP3: return FLV_FLAG_SOUND_FORMAT_MP3;
  }
  LOG_FATAL << "Illegal audio_format: " << (int)audio_format;
  return FLV_FLAG_SOUND_FORMAT_MP3;
}

bool ExtractMediaInfoFromMoov(const streaming::f4v::MoovAtom& moov,
    const vector<f4v::FrameHeader*>& frames, MediaInfo* out) {
  // extract audio&video info
  f4v::util::ExtractMediaInfo(moov, out);

  // frames
  out->set_frames(frames);

  // the whole moov
  out->set_mp4_moov(moov);
  // with frames starting on offset 0
  UpdateFramesOffset(out->mutable_mp4_moov(), -frames[0]->offset());

  return true;
}

bool ExtractMediaInfoFromFlv(const FlvTag& metadata, const FlvTag* audio,
    const FlvTag* video, MediaInfo* out) {
  CHECK_EQ(metadata.body().type(), FLV_FRAMETYPE_METADATA);
  CHECK(audio == NULL || audio->body().type() == FLV_FRAMETYPE_AUDIO);
  CHECK(video == NULL || video->body().type() == FLV_FRAMETYPE_VIDEO);
  const FlvTag::Metadata& meta = metadata.metadata_body();

  // audio configuration
  do {
    if ( audio == NULL ) {
      break;
    }
    const FlvTag::Audio& a = audio->audio_body();
    switch ( a.format() ) {
      case streaming::FLV_FLAG_SOUND_FORMAT_MP3:
        out->mutable_audio()->format_ = MediaInfo::Audio::FORMAT_MP3;
        out->mutable_audio()->mp3_flv_container_ = true;
        break;
      case streaming::FLV_FLAG_SOUND_FORMAT_AAC: {
        out->mutable_audio()->format_ = MediaInfo::Audio::FORMAT_AAC;
        out->mutable_audio()->aac_flv_container_ = true;
        uint8 audio_data[4] = {0,}; // 2 bytes ??? + 2 bytes AAC header
        if ( a.data().Peek(&audio_data, sizeof(audio_data))
               < sizeof(audio_data) ) {
          LOG_ERROR << "Not enough data in audio tag to extract AAC header"
              << ", required: " << sizeof(audio_data)
              << ", available: " << a.data().Size()
              << ", audio_tag: " << audio << ": " << audio->ToString();
          break;
        }
        out->mutable_audio()->aac_config_[0] = audio_data[2];
        out->mutable_audio()->aac_config_[1] = audio_data[3];
        out->mutable_audio()->aac_profile_ = 1;
        out->mutable_audio()->aac_level_ = 15;
        break;
      }
      default:
        break;
    }
    if ( !out->has_audio() ) {
      LOG_ERROR << "Unknown audio codec: " << a.format()
        << "(" << FlvFlagSoundFormatName(a.format()) << ")"
        << ", skipping audio stream";

      break;
    }
    out->mutable_audio()->sample_size_ = static_cast<uint32>(
        ExtractNumber(meta.values(), "audiosamplesize"));
    out->mutable_audio()->bps_ = static_cast<uint32>(
        ExtractNumber(meta.values(), "audiodatarate")) * 1000;
    out->mutable_audio()->sample_rate_ = FlvFlagSoundRateNumber(a.rate())/2;
    out->mutable_audio()->channels_ =
        a.type() == FLV_FLAG_SOUND_TYPE_MONO ? 1 : 2;
  } while (false);

  // video configuration
  do {
    if ( video == NULL ) {
      break;
    }
    const FlvTag::Video& v = video->video_body();
    switch ( v.codec() ) {
      case streaming::FLV_FLAG_VIDEO_CODEC_H263:
        out->mutable_video()->format_ = MediaInfo::Video::FORMAT_H263;
        break;
      case streaming::FLV_FLAG_VIDEO_CODEC_AVC: {
        out->mutable_video()->format_ = MediaInfo::Video::FORMAT_H264;
        if ( v.avc_packet_type() == AVC_SEQUENCE_HEADER ) {
          io::MemoryStream& video_data = const_cast<io::MemoryStream&>(v.data());
          video_data.MarkerSet();
          // 1 byte: frame type + codec id
          // 1 byte: AVC packet type
          // 3 bytes: composition offset
          // Total 5 bytes.
          video_data.Skip(5);
          video_data.MarkerSet();
          video_data.ReadString(&out->mutable_video()->h264_avcc_);
          video_data.MarkerRestore();
          f4v::AvccAtom avcc;
          f4v::Decoder f4v_decoder;
          f4v::TagDecodeStatus status = avcc.DecodeBody(
              video_data.Size(), video_data, f4v_decoder);
          video_data.MarkerRestore();
          if ( status != streaming::f4v::TAG_DECODE_SUCCESS ) {
            LOG_ERROR << "Failed to decode AVCC";
            break;
          }
          out->mutable_video()->h264_configuration_version_ = avcc.configuration_version();
          out->mutable_video()->h264_profile_ = avcc.profile();
          out->mutable_video()->h264_profile_compatibility_ = avcc.profile_compatibility();
          out->mutable_video()->h264_level_ = avcc.level();
          avcc.get_seq_parameters(&out->mutable_video()->h264_sps_);
          avcc.get_pic_parameters(&out->mutable_video()->h264_pps_);
          out->mutable_video()->h264_flv_container_ = true;
        }
        break;
      }
      case streaming::FLV_FLAG_VIDEO_CODEC_VP6:
        out->mutable_video()->format_ = MediaInfo::Video::FORMAT_ON_VP6;
        break;
      default:
        break;
    }
    if ( !out->has_video() ) {
      LOG_ERROR << "unknown video codec: " << v.codec()
        << "(" << FlvFlagVideoCodecName(v.codec())
        << "), skipping video stream";
      break;
    }
    out->mutable_video()->bps_ = static_cast<uint32>(
        ExtractNumber(meta.values(), "videodatarate")) * 1000;
    out->mutable_video()->frame_rate_ = static_cast<float>(
        ExtractNumber(meta.values(), "framerate"));
    out->mutable_video()->width_ = static_cast<uint32>(
        ExtractNumber(meta.values(), "width"));
    out->mutable_video()->height_ = static_cast<uint32>(
        ExtractNumber(meta.values(), "height"));

    // 90000 = most common clock rate for video
    out->mutable_video()->clock_rate_ = 90000;
  } while (false);

  out->set_duration_ms(static_cast<uint32>(
          ExtractNumber(meta.values(), "duration")) * 1000);
  out->set_file_size(static_cast<uint32>(
          ExtractNumber(meta.values(), "filesize")));
  out->set_seekable(!ExtractBoolean(meta.values(), "unseekable"));
  out->set_pausable(!ExtractBoolean(meta.values(), "unpausable"));

  // set extra metadata value (other values, besides the extracted ones)
  out->mutable_flv_extra_metadata()->SetAll(meta.values());
  out->mutable_flv_extra_metadata()->Erase("duration");
  out->mutable_flv_extra_metadata()->Erase("filesize");
  out->mutable_flv_extra_metadata()->Erase("unseekable");
  out->mutable_flv_extra_metadata()->Erase("unpausable");
  out->mutable_flv_extra_metadata()->Erase("audiocodecid");
  out->mutable_flv_extra_metadata()->Erase("audiosamplesize");
  out->mutable_flv_extra_metadata()->Erase("audiodatarate");
  out->mutable_flv_extra_metadata()->Erase("videocodecid");
  out->mutable_flv_extra_metadata()->Erase("videodatarate");
  out->mutable_flv_extra_metadata()->Erase("cuePoints");
  out->mutable_flv_extra_metadata()->Erase("width");
  out->mutable_flv_extra_metadata()->Erase("height");
  out->mutable_flv_extra_metadata()->Erase("framerate");
  out->mutable_flv_extra_metadata()->Erase("stereo");

  return true;
}

bool ExtractMediaInfoFromMp3(const Mp3FrameTag& mp3_tag, MediaInfo* out) {
  out->mutable_audio()->format_ = MediaInfo::Audio::FORMAT_MP3;
  out->mutable_audio()->sample_rate_ = mp3_tag.sampling_rate_hz();
  out->mutable_audio()->sample_size_ = mp3_tag.frame_length_bytes();
  out->mutable_audio()->channels_ = mp3_tag.channels();
  return true;
}

bool ExtractMediaInfoFromFile(const string& filename, MediaInfo* out) {
  MediaFileReader reader;
  if ( !reader.Open(filename) ) {
    LOG_ERROR << "cannot open file: [" << filename << "]";
    return false;
  }
  while ( true ) {
    scoped_ref<Tag> tag;
    int64 tag_ts = 0;
    TagReadStatus status = reader.Read(&tag, &tag_ts);
    if ( status == READ_SKIP ) {
      continue;
    }
    if ( status == READ_OK ) {
      CHECK(tag->type() == Tag::TYPE_MEDIA_INFO)
          << "First tag should be MediaInfoTag, instead got: "
          << tag->ToString();
      *out = static_cast<const MediaInfoTag*>(tag.get())->info();
      return true;
    }
    LOG_ERROR << "Failed to read the next tag, status: "
              << TagReadStatusName(status);
    return false;
  }
}

bool ComposeMoov(const MediaInfo& info, uint32 moov_offset,
    scoped_ref<F4vTag>* out) {
  // if we have an already stored MOOV (on files), simply return it
  if ( info.mp4_moov() != NULL ) {
    LOG_WARNING << "Using previously stored moov..";
    f4v::MoovAtom* moov = static_cast<f4v::MoovAtom*>(info.mp4_moov()->Clone());
    UpdateFramesOffset(moov,
                       moov_offset +
                       // MOOV whole atom size
                       info.mp4_moov()->size() +
                       // MDAT header size
                       streaming::f4v::BaseAtom::HeaderSize(
                           streaming::f4v::BaseAtom::IsExtendedSize(
                               FramesSize(*moov))));
    *out = new F4vTag(Tag::ATTR_METADATA, kDefaultFlavourMask,
        moov);
    return true;
  }

  // if there's no info on the frames, the MOOV cannot be built
  if ( info.frames().empty() ) {
    LOG_ERROR << "No frames, cannot build MOOV";
    return false;
  }
  // if there's no audio nor video, the MOOV cannot be built
  if ( !info.has_audio() && !info.has_video() ) {
    LOG_ERROR << "No audio and no video, cannot build MOOV";
    return false;
  }

  // compose MOOV using known frame info

  ///////////////////////////////////////////////////////////////////////
  // Pass 1: count audio/video frames and build MOOV
  //
  streaming::f4v::MoovAtom* moov = NULL;

  // Audio trak information atoms. Will point to atoms inside moov.
  streaming::f4v::StszAtom* audio_stsz = NULL;
  streaming::f4v::SttsAtom* audio_stts = NULL;
  streaming::f4v::StscAtom* audio_stsc = NULL;
  streaming::f4v::StcoAtom* audio_stco = NULL;

  // AAC specific. Will point to an atom inside moov.
  streaming::f4v::EsdsAtom* audio_esds = NULL;

  // Video trak information atoms. Will point to atoms inside moov.
  streaming::f4v::StszAtom* video_stsz = NULL;
  streaming::f4v::SttsAtom* video_stts = NULL;
  streaming::f4v::StscAtom* video_stsc = NULL;
  streaming::f4v::StcoAtom* video_stco = NULL;
  streaming::f4v::StssAtom* video_stss = NULL;

  // convention: first frame starts on offset 0
  int64 frame_offset = 0;

  // count frames
  int64 frame_count = 0;
  int64 audio_frame_count = 0;
  int64 video_frame_count = 0;

  vector<F4vChunk*> audio_chunks;
  vector<F4vChunk*> video_chunks;
  bool last_frame_was_audio = true;

  ////////////////////////////////////////////////////////////////
  // Extract every useful bit from metadata
  //

  CHECK_NULL(moov);
  moov = new streaming::f4v::MoovAtom();

  streaming::f4v::MvhdAtom* mvhd = new streaming::f4v::MvhdAtom();
  mvhd->set_version(0);
  mvhd->set_flags(0, 0, 0);
  mvhd->set_creation_time(MacDateNow());
  mvhd->set_modification_time(MacDateNow());
  mvhd->set_time_scale(kDefaultTimeScale);
  mvhd->set_duration(info.duration_ms() * mvhd->time_scale() / 1000);
  mvhd->set_preferred_rate(kDefaultPreferredRate);
  mvhd->set_preferred_volume(kDefaultPreferredVolume);
  mvhd->set_preview_time(0);
  mvhd->set_preview_duration(0);
  mvhd->set_poster_time(0);
  mvhd->set_selection_time(0);
  mvhd->set_selection_duration(0);
  mvhd->set_current_time(0);
  mvhd->set_next_track_id((info.has_video() && info.has_audio()) ? 3 :
                          (info.has_video() || info.has_audio()) ? 2 : 1);
  moov->AddSubatom(mvhd);

  ///////////////////////////////////////////////////////////////////
  // Create AUDIO Trak
  //
  if ( info.has_audio() ) {
    streaming::f4v::TrakAtom* audio_trak = new streaming::f4v::TrakAtom();
    moov->AddSubatom(audio_trak);

    streaming::f4v::TkhdAtom* audio_tkhd = new streaming::f4v::TkhdAtom();
    audio_tkhd->set_version(0);
    audio_tkhd->set_flags(0, 0, 1);
    audio_tkhd->set_creation_time(MacDateNow());
    audio_tkhd->set_modification_time(MacDateNow());
    audio_tkhd->set_id(1);
    audio_tkhd->set_duration(info.duration_ms() * mvhd->time_scale() / 1000);
    audio_tkhd->set_layer(0);
    audio_tkhd->set_alternate_group(0);
    audio_tkhd->set_volume(256);
    audio_tkhd->set_width(0);
    audio_tkhd->set_height(0);
    audio_trak->AddSubatom(audio_tkhd);

    //TODO(cosmin): add EdtsAtom

    streaming::f4v::MdiaAtom* audio_mdia = new streaming::f4v::MdiaAtom();
    audio_trak->AddSubatom(audio_mdia);

    streaming::f4v::MdhdAtom* audio_mdhd = new streaming::f4v::MdhdAtom();
    audio_mdhd->set_creation_time(MacDateNow());
    audio_mdhd->set_modification_time(MacDateNow());
    audio_mdhd->set_time_scale(info.audio().sample_rate_);
    audio_mdhd->set_duration(info.duration_ms() * audio_mdhd->time_scale() / 1000 );
    audio_mdhd->set_language(streaming::f4v::MacLanguageCode(
        info.audio().mp4_language_));
    audio_mdia->AddSubatom(audio_mdhd);

    streaming::f4v::HdlrAtom* audio_hdlr = new streaming::f4v::HdlrAtom();
    audio_hdlr->set_version(0);
    audio_hdlr->set_flags(0, 0, 0);
    audio_hdlr->set_pre_defined(0);
    audio_hdlr->set_handler_type(kHandlerTypeSound);
    audio_hdlr->set_name(kHandlerNameSound);
    audio_mdia->AddSubatom(audio_hdlr);

    streaming::f4v::MinfAtom* audio_minf = new streaming::f4v::MinfAtom();
    audio_mdia->AddSubatom(audio_minf);

    streaming::f4v::SmhdAtom* audio_smhd = new streaming::f4v::SmhdAtom();
    audio_smhd->set_version(0);
    audio_smhd->set_flags(0, 0, 0);
    audio_minf->AddSubatom(audio_smhd);

    //TODO(cosmin): add DinfAtom

    streaming::f4v::StblAtom* audio_stbl = new streaming::f4v::StblAtom();
    audio_minf->AddSubatom(audio_stbl);
    streaming::f4v::StsdAtom* audio_stsd = new streaming::f4v::StsdAtom();
    audio_stbl->AddSubatom(audio_stsd);
    if ( info.audio().format_ == MediaInfo::Audio::FORMAT_AAC ) {
      streaming::f4v::Mp4aAtom* audio_mp4a = new streaming::f4v::Mp4aAtom();
      audio_stsd->AddSubatom(audio_mp4a);
      audio_mp4a->set_data_reference_index(1);
      audio_mp4a->set_inner_version(0);
      audio_mp4a->set_revision_level(0);
      audio_mp4a->set_vendor(0);
      audio_mp4a->set_number_of_channels(info.audio().channels_);
      audio_mp4a->set_sample_size_in_bits(info.audio().sample_size_);
      audio_mp4a->set_compression_id(0);
      audio_mp4a->set_packet_size(0);
      audio_mp4a->set_sample_rate(streaming::f4v::FPU16U16(
          audio_mdhd->time_scale()));
      audio_mp4a->set_samples_per_packet(0);
      audio_mp4a->set_bytes_per_packet(0);
      audio_mp4a->set_bytes_per_frame(0);
      audio_mp4a->set_bytes_per_sample(0);

      CHECK_NULL(audio_esds);
      audio_esds = new streaming::f4v::EsdsAtom();
      // "audio_esds" is partially built here. The rest is filled
      // on the first audio tag which contains aac configuration
      audio_esds->set_mp4_es_desc_tag_id(0);
      audio_esds->set_mp4_es_desc_tag_priority(0);
      audio_esds->set_mp4_dec_config_desc_tag_object_type_id(64);
      audio_esds->set_mp4_dec_config_desc_tag_stream_type(21);
      audio_esds->set_mp4_dec_config_desc_tag_buffer_size_db(6144);
      audio_esds->set_mp4_dec_config_desc_tag_max_bit_rate(info.audio().bps_);
      audio_esds->set_mp4_dec_config_desc_tag_avg_bit_rate(info.audio().bps_);
      audio_esds->set_extra_data(info.audio().aac_config_,
                                 sizeof(info.audio().aac_config_));
      audio_mp4a->AddSubatom(audio_esds);
    } else {
      LOG_ERROR << "Don't know how to handle audio format: "
                << MediaInfo::Audio::FormatName(info.audio().format_);
      return false;
    }

    CHECK_NULL(audio_stsz);
    audio_stsz = new streaming::f4v::StszAtom();
    audio_stbl->AddSubatom(audio_stsz);
    // audio_stsz is filled later (down below)

    CHECK_NULL(audio_stts);
    audio_stts = new streaming::f4v::SttsAtom();
    audio_stbl->AddSubatom(audio_stts);
    // audio_stts is filled later (down below)

    CHECK_NULL(audio_stsc);
    audio_stsc = new streaming::f4v::StscAtom();
    audio_stbl->AddSubatom(audio_stsc);
    // audio_stsc is filled later (down below)

    CHECK_NULL(audio_stco);
    audio_stco = new streaming::f4v::StcoAtom();
    audio_stbl->AddSubatom(audio_stco);
    // audio_stco is filled later (down below)
  }

  ///////////////////////////////////////////////////////////////////
  // Create VIDEO Trak
  //
  if ( info.has_video() ) {
    streaming::f4v::TrakAtom* video_trak = new streaming::f4v::TrakAtom();
    moov->AddSubatom(video_trak);

    streaming::f4v::TkhdAtom* video_tkhd = new streaming::f4v::TkhdAtom();
    video_tkhd->set_version(0);
    video_tkhd->set_flags(0, 0, 1);
    video_tkhd->set_creation_time(MacDateNow());
    video_tkhd->set_modification_time(MacDateNow());
    video_tkhd->set_id(2);
    video_tkhd->set_duration(info.duration_ms() * mvhd->time_scale() / 1000);
    video_tkhd->set_layer(0);
    video_tkhd->set_alternate_group(0);
    video_tkhd->set_volume(0);
    video_tkhd->set_width(streaming::f4v::FPU16U16(info.video().width_));
    video_tkhd->set_height(streaming::f4v::FPU16U16(info.video().height_));
    video_trak->AddSubatom(video_tkhd);

    //TODO(cosmin): maybe add LoadAtom

    //TODO(cosmin): add EdtsAtom

    streaming::f4v::MdiaAtom* video_mdia = new streaming::f4v::MdiaAtom();
    video_trak->AddSubatom(video_mdia);

    streaming::f4v::MdhdAtom* video_mdhd = new streaming::f4v::MdhdAtom();
    video_mdhd->set_creation_time(MacDateNow());
    video_mdhd->set_modification_time(MacDateNow());
    video_mdhd->set_time_scale(static_cast<uint32>(info.video().frame_rate_ * 100));
    video_mdhd->set_duration(info.duration_ms() * video_mdhd->time_scale() / 1000);
    video_mdhd->set_language(streaming::f4v::MacLanguageCode("eng"));
    video_mdia->AddSubatom(video_mdhd);

    streaming::f4v::HdlrAtom* video_hdlr = new streaming::f4v::HdlrAtom();
    video_hdlr->set_version(0);
    video_hdlr->set_flags(0, 0, 0);
    video_hdlr->set_pre_defined(0);
    video_hdlr->set_handler_type(kHandlerTypeVideo);
    video_hdlr->set_name(kHandlerNameVideo);
    video_mdia->AddSubatom(video_hdlr);

    streaming::f4v::MinfAtom* video_minf = new streaming::f4v::MinfAtom();
    video_mdia->AddSubatom(video_minf);

    streaming::f4v::VmhdAtom* video_vmhd = new streaming::f4v::VmhdAtom();
    video_vmhd->set_version(0);
    video_vmhd->set_flags(0, 0, 0);
    video_vmhd->set_graphics_mode(0);
    video_vmhd->set_opcolor(0, 0, 0);
    video_minf->AddSubatom(video_vmhd);

    //TODO(cosmin): add DinfAtom

    streaming::f4v::StblAtom* video_stbl = new streaming::f4v::StblAtom();
    video_minf->AddSubatom(video_stbl);
    streaming::f4v::StsdAtom* video_stsd = new streaming::f4v::StsdAtom();
    video_stbl->AddSubatom(video_stsd);
    if ( info.video().format_ == MediaInfo::Video::FORMAT_H264 ) {
      streaming::f4v::Avc1Atom* video_avc1 = new streaming::f4v::Avc1Atom();
      video_stsd->AddSubatom(video_avc1);
      video_avc1->set_version(0);
      video_avc1->set_flags(0, 0, 0);
      video_avc1->set_reference_index(1);
      video_avc1->set_qt_video_encoding_version(0);
      video_avc1->set_qt_video_encoding_revision_level(0);
      video_avc1->set_qt_v_encoding_vendor(0);
      video_avc1->set_qt_video_temporal_quality(0);
      video_avc1->set_qt_video_spatial_quality(0);
      video_avc1->set_video_frame_pixel_size(83886800); // in backcountry.f4v
      video_avc1->set_horizontal_dpi(4718592); // = 48.0 in 16.16 format
      video_avc1->set_vertical_dpi(4718592);   // = 48.0 in 16.16 format
      video_avc1->set_qt_video_data_size(0);
      video_avc1->set_video_frame_count(1);
      video_avc1->set_video_encoder_name("");
      video_avc1->set_video_pixel_depth(24);
      video_avc1->set_qt_video_color_table_id(65535);
      video_avc1->AddSubatom(new f4v::AvccAtom(
          info.video().h264_configuration_version_,
          info.video().h264_profile_,
          info.video().h264_profile_compatibility_,
          info.video().h264_level_,
          info.video().h264_nalu_length_size_,
          info.video().h264_sps_,
          info.video().h264_pps_));
    } else {
      LOG_ERROR << "Don't know how to handle video format: "
                << MediaInfo::Video::FormatName(info.video().format_);
      return false;
    }

    CHECK_NULL(video_stsz);
    video_stsz = new streaming::f4v::StszAtom();
    video_stbl->AddSubatom(video_stsz);
    // video_stsz is filled later (down below)

    CHECK_NULL(video_stts);
    video_stts = new streaming::f4v::SttsAtom();
    video_stbl->AddSubatom(video_stts);
    // video_stts is filled later (down below)

    CHECK_NULL(video_stsc);
    video_stsc = new streaming::f4v::StscAtom();
    video_stbl->AddSubatom(video_stsc);
    // video_stsc is filled later (down below)

    CHECK_NULL(video_stco);
    video_stco = new streaming::f4v::StcoAtom();
    video_stbl->AddSubatom(video_stco);
    // video_stco is filled later (down below)

    CHECK_NULL(video_stss);
    video_stss = new streaming::f4v::StssAtom();
    video_stbl->AddSubatom(video_stss);
    // video_stss is filled later (down below)
  }

  LOG_INFO << "Built base MOOV: " << moov->ToString();
  ////////////////////////////////////////////////////////////////////////////////////////////////////////

  for ( uint32 i = 0; i < info.frames().size(); i++ ) {
    const MediaInfo::Frame& frame = info.frames()[i];

    if ( frame.is_audio_ ) {
      if ( !info.has_audio() ) {
        continue; // encountered an audio frame while the info says no audio
      }
      //////////////////////////////////
      // Add Audio frame

      // build audio_stsz (the AAC header has been already skipped)
      audio_stsz->AddRecord(new streaming::f4v::StszRecord(frame.size_));
      // prepare chunks for stsc stco
      if ( !last_frame_was_audio || audio_chunks.empty() ) {
        audio_chunks.push_back(new F4vChunk(frame_offset, 0));
      }
      audio_chunks[audio_chunks.size() - 1]->sample_count_++;
      last_frame_was_audio = true;

      audio_frame_count++;
    } else  {
      if ( !info.has_video() ) {
        continue; // encountered a video frame while the info says no audio
      }
      //////////////////////////////////
      // Add Video frame

      // build video_stsz (the AVC header has been already skipped)
      video_stsz->AddRecord(new streaming::f4v::StszRecord(frame.size_));
      // build video_stss
      if ( frame.is_keyframe_ ) {
        video_stss->AddRecord(new streaming::f4v::StssRecord(
            video_frame_count + 1)); // f4v frame count starts on 1
      }
      // prepare chunks for stsc stco
      if ( last_frame_was_audio || video_chunks.empty() ) {
        video_chunks.push_back(new F4vChunk(frame_offset, 0));
      }
      video_chunks[video_chunks.size() - 1]->sample_count_++;
      last_frame_was_audio = false;

      video_frame_count++;
    }
    frame_count++;
    frame_offset += frame.size_;
  }

  // build audio_stts and video_stts. All audio samples have the same
  // duration (measured in time-scale units). Same for video samples.
  uint32 audio_sample_count = audio_frame_count;
  uint32 audio_sample_delta = moov->trak(true)->mdia()->mdhd()->duration() / audio_sample_count;
  audio_stts->AddRecord(new streaming::f4v::SttsRecord(audio_sample_count, audio_sample_delta));
  uint32 video_sample_count = video_frame_count;
  uint32 video_sample_delta = moov->trak(false)->mdia()->mdhd()->duration() / video_sample_count;
  video_stts->AddRecord(new streaming::f4v::SttsRecord(
      video_sample_count, video_sample_delta));

  // build audio_stsc, audio_stco, video_stsc, video_stco.
  // Now the chunk offset starts on 0. We add all the chunks in STCO, STSC
  // measure the MOOV atom, then update the STCO, STSC chunks.
  for ( uint32 i = 0; i < audio_chunks.size(); i++ ) {
    const F4vChunk& chunk = *audio_chunks[i];
    audio_stsc->AddRecord(new streaming::f4v::StscRecord(i+1, chunk.sample_count_, 1));
    audio_stco->AddRecord(new streaming::f4v::StcoRecord(chunk.start_offset_));
  }
  for ( uint32 i = 0; i < video_chunks.size(); i++ ) {
    const F4vChunk& chunk = *video_chunks[i];
    video_stsc->AddRecord(new streaming::f4v::StscRecord(i+1, chunk.sample_count_, 1));
    video_stco->AddRecord(new streaming::f4v::StcoRecord(chunk.start_offset_));
  }

  // measure where the MDAT body starts
  int64 mdat_begin_offset = MeasureAtom(f4v::FtypAtom()) +
                            MeasureAtom(*moov) + 16;

  // update chunk offsets
  for ( uint32 i = 0; i < audio_stco->records().size(); i++ ) {
    audio_stco->records()[i]->chunk_offset_ += mdat_begin_offset;
  }
  for ( uint32 i = 0; i < video_stco->records().size(); i++ ) {
    video_stco->records()[i]->chunk_offset_ += mdat_begin_offset;
  }

  UpdateAtomSize(*moov);
  // MOOV atom complete

  streaming::f4v::MdatAtom* mdat = new streaming::f4v::MdatAtom();
  mdat->set_body_size(frame_offset);

  //scoped_ptr<streaming::f4v::MoovAtom> auto_del_moov(moov);
  //scoped_ptr<streaming::f4v::MdatAtom> auto_del_mdat(mdat);

  LOG_INFO << "Built complete MOOV: " << moov->ToString();

  // Pass 1 complete
  ////////////////////////////////////////////////////////////////////////////

  *out = new F4vTag(Tag::ATTR_METADATA, kDefaultFlavourMask, moov);
  // TODO(cosmin): return MdatAtom too
  return true;
}

bool ComposeFlv(const MediaInfo& info, scoped_ref<FlvTag>* out) {
  *out = new FlvTag(Tag::ATTR_METADATA, kDefaultFlavourMask,
      0, new FlvTag::Metadata(kOnMetaData, rtmp::CMixedMap()));
  FlvTag::Metadata& metadata = (*out)->mutable_metadata_body();
  metadata.mutable_values()->SetAll(info.flv_extra_metadata());
  if ( info.duration_ms() > 0 ) {
    metadata.mutable_values()->Set("duration",
        new rtmp::CNumber(info.duration_ms()/1000.0));
  }
  if ( info.has_video() ) {
    if ( info.video().width_ > 0 ) {
      metadata.mutable_values()->Set("width",
          new rtmp::CNumber(info.video().width_));
    }
    if ( info.video().height_ > 0 ) {
      metadata.mutable_values()->Set("height",
          new rtmp::CNumber(info.video().height_));
    }
    metadata.mutable_values()->Set("videocodecid",
        new rtmp::CNumber(VideoFormatToFlv(info.video().format_)));
    if ( info.video().frame_rate_ > 0.0f ) {
      metadata.mutable_values()->Set("framerate",
          new rtmp::CNumber(info.video().frame_rate_));
    }
  }
  if ( info.has_audio() ) {
    metadata.mutable_values()->Set("stereo",
        new rtmp::CBoolean(info.audio().channels_ == 2));
    metadata.mutable_values()->Set("audiocodecid",
        new rtmp::CNumber(AudioFormatToFlv(info.audio().format_)));
    if ( info.audio().sample_rate_ > 0 ) {
      metadata.mutable_values()->Set("audiosamplerate",
          new rtmp::CNumber(info.audio().sample_rate_));
    }
    if ( info.audio().sample_size_ > 0 ) {
      metadata.mutable_values()->Set("audiosamplesize",
          new rtmp::CNumber(info.audio().sample_size_));
    }
  }
  if ( info.file_size() > 0 ) {
    metadata.mutable_values()->Set("filesize",
        new rtmp::CNumber(info.file_size()));
  }
  metadata.mutable_values()->Set("unseekable",
      new rtmp::CBoolean(!info.seekable()));
  metadata.mutable_values()->Set("unpausable",
      new rtmp::CBoolean(!info.pausable()));
  return true;
}

} // namespace util
} // namespace streaming
