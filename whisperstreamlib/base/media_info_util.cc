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
namespace streaming {
namespace util {

void MemoryStreamToVector(const io::MemoryStream& ms, vector<uint8>* out) {
  out->resize(ms.Size());
  ms.Peek(&(out->at(0)), ms.Size());
}

bool ExtractMediaInfoFromMoov(const streaming::f4v::MoovAtom& moov,
    MediaInfo* out) {
  streaming::f4v::util::MediaInfo info;
  streaming::f4v::util::ExtractMediaInfo(moov, &info);
  LOG_INFO << "ExtractMediaInfoFromMoov using info: " << info.ToString();

  // add stream length
  out->set_duration_ms(info.duration_);

  // audio stream configuration
  do {
    const streaming::f4v::TrakAtom* audio_trak = moov.trak(true);
    if ( audio_trak == NULL ) {
      break;
    }
    if ( info.audio_codec_id_ == "mp4a" ) {
      out->mutable_audio()->format_ = MediaInfo::Audio::FORMAT_AAC;
      const streaming::f4v::EsdsAtom* esds =
          streaming::f4v::util::GetEsdsAtom(moov);
      if ( esds != NULL ) {
        esds->extra_data().Peek(out->mutable_audio()->aac_config_,
            sizeof(out->mutable_audio()->aac_config_));
      }
      out->mutable_audio()->aac_profile_ = 1;
      out->mutable_audio()->aac_level_ = 15;
    }
    if ( !out->has_audio() ) {
      LOG_ERROR << "Unknown audio codec: [" << info.audio_codec_id_ << "]"
                << ", skipping audio stream";
      break;
    }
    out->mutable_audio()->sample_rate_ = info.audio_sample_rate_;
    out->mutable_audio()->channels_ = info.audio_channels_;
  } while (false);

  // video stream configuration
  do {
    const streaming::f4v::TrakAtom* video_trak = moov.trak(false);
    if ( video_trak == NULL ) {
      break;
    }
    if ( info.video_codec_id_ == "avc1" ) {
      out->mutable_video()->format_ = MediaInfo::Video::FORMAT_H264;
      const streaming::f4v::AvccAtom* avcc =
          streaming::f4v::util::GetAvccAtom(moov);
      if ( avcc != NULL &&
           avcc->seq_parameters().size() ) {
        MemoryStreamToVector(avcc->seq_parameters()[0]->raw_data_,
                             &out->mutable_video()->h264_sps_);
      }
      if ( avcc != NULL &&
           avcc->seq_parameters().size() ) {
        MemoryStreamToVector(avcc->pic_parameters()[0]->raw_data_,
                             &out->mutable_video()->h264_pps_);
      }
      out->mutable_video()->h264_profile_ = avcc->profile();
      out->mutable_video()->h264_profile_compatibility_ = avcc->profile_compatibility();
      out->mutable_video()->h264_level_ = avcc->level();
    }
    if ( !out->has_video() ) {
      LOG_ERROR << "Unknown video codec: [" << info.video_codec_id_ << "]"
                << ", skipping video stream";
      break;
    }
    // 90000 = most common clock rate for video
    out->mutable_video()->clock_rate_ = 90000;
  } while (false);

  return true;
}

bool ExtractMediaInfoFromFlv(const FlvTag& metadata, const FlvTag* audio,
    const FlvTag* video, MediaInfo* out) {
  CHECK_EQ(metadata.body().type(), FLV_FRAMETYPE_METADATA);
  CHECK(audio == NULL || audio->body().type() == FLV_FRAMETYPE_AUDIO);
  CHECK(video == NULL || video->body().type() == FLV_FRAMETYPE_VIDEO);

  // add stream length
  const rtmp::CObject* d = metadata.metadata_body().values().Find("duration");
  if ( d != NULL && d->object_type() == rtmp::CObject::CORE_NUMBER ) {
    out->set_duration_ms(static_cast<uint32>(
                         static_cast<const rtmp::CNumber*>(d)->value() * 1000));
  }

  // audio configuration
  do {
    if ( audio == NULL ) {
      break;
    }
    const FlvTag::Audio& a = audio->audio_body();
    switch ( a.audio_format() ) {
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
      LOG_ERROR << "Unknown audio codec: " << a.audio_format()
        << "(" << FlvFlagSoundFormatName(a.audio_format()) << ")"
        << ", skipping audio stream";

      break;
    }
    out->mutable_audio()->sample_rate_ = FlvFlagSoundRateNumber(
        a.audio_rate())/2;
    out->mutable_audio()->channels_ =
        a.audio_type() == FLV_FLAG_SOUND_TYPE_MONO ? 1 : 2;
  } while (false);

  // video configuration
  do {
    if ( video == NULL ) {
      break;
    }
    const FlvTag::Video& v = video->video_body();
    switch ( v.video_codec() ) {
      case streaming::FLV_FLAG_VIDEO_CODEC_H263:
        out->mutable_video()->format_ = MediaInfo::Video::FORMAT_H263;
        break;
      case streaming::FLV_FLAG_VIDEO_CODEC_AVC: {
        out->mutable_video()->format_ = MediaInfo::Video::FORMAT_H264;
        vector<io::MemoryStream*> nalus;
        io::MemoryStream& video_data = const_cast<io::MemoryStream&>(v.data());
        video_data.MarkerSet();
        video_data.Skip(10); // unknown 10 bytes
        while ( !video_data.IsEmpty() )  {
          video_data.Skip(1);
          uint32 nalu_size = io::NumStreamer::ReadUInt16(&video_data,
              common::BIGENDIAN);
          if ( nalu_size > video_data.Size() ) {
            LOG_ERROR << "nalu_size too big: nalu_size: " << nalu_size
                      << " , stream size: " << video_data.Size();
            break;
          }
          io::MemoryStream* p = new io::MemoryStream();
          p->AppendStream(&video_data, nalu_size);
          nalus.push_back(p);
        }
        video_data.MarkerRestore();

        for ( uint32 i = 0; i < nalus.size(); i++ ) {
          io::MemoryStream* nalu = nalus[i];
          scoped_ptr<io::MemoryStream> auto_del_nalu(nalu);
          if ( nalu->IsEmpty() ) {
            continue;
          }
          uint8 nalu_hdr = io::NumStreamer::PeekByte(nalu);
          //uint8 nalu_f = (nalu_hdr & 0x80) >> 7;
          //uint8 nalu_nri = (nalu_hdr & 0x60) >> 5;
          uint8 nalu_type = nalu_hdr & 0x1f;
          if ( nalu_type == 7 ) {
            // nalu contains:
            // <1 byte unknown> <3 bytes profile level id> ...
            uint8 tmp[4] = {0};
            if ( sizeof(tmp) != nalu->Peek(tmp, sizeof(tmp)) ) {
              LOG_ERROR << "Nalu too small: " << nalu->Size()
                        << ", cannot extract profile level id";
              continue;
            }
            out->mutable_video()->h264_flv_container_ = true;
            out->mutable_video()->h264_profile_ = tmp[1];
            out->mutable_video()->h264_profile_compatibility_ = tmp[2];
            out->mutable_video()->h264_level_ = tmp[3];
            MemoryStreamToVector(*nalu, &out->mutable_video()->h264_sps_);
            continue;
          }
          if ( nalu_type == 8 ) {
            MemoryStreamToVector(*nalu, &out->mutable_video()->h264_pps_);
            continue;
          }
          LOG_ERROR << "Ignoring unknown nalu_type: " << nalu_type;
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
      LOG_ERROR << "unknown video codec: " << v.video_codec()
        << "(" << FlvFlagVideoCodecName(v.video_codec())
        << "), skipping video stream";
      break;
    }
    // 90000 = most common clock rate for video
    out->mutable_video()->clock_rate_ = 90000;
  } while (false);
  return true;
}

bool ExtractMediaInfoFromMp3(const Mp3FrameTag& mp3_tag, MediaInfo* out) {
  out->mutable_audio()->format_ = MediaInfo::Audio::FORMAT_MP3;
  LOG_ERROR << "TODO: extract audio_sample_rate";
  out->mutable_audio()->sample_rate_ = 90000;
  LOG_ERROR << "TODO: extract audio_channels";
  out->mutable_audio()->channels_ = 2;
  return true;
}

bool ExtractMediaInfoFromFile(const string& filename, MediaInfo* out) {
  MediaFileReader reader;
  if ( !reader.Open(filename) ) {
    LOG_ERROR << "cannot open file: [" << filename << "]";
    return false;
  }
  scoped_ref<const FlvTag> flv_metadata;
  scoped_ref<const FlvTag> flv_audio;
  scoped_ref<const FlvTag> flv_video;
  while ( true ) {
    int64 timestamp_ms;
    scoped_ref<Tag> tag;
    TagReadStatus status = reader.Read(&tag, &timestamp_ms);
    if ( status != READ_OK ) {
      LOG_ERROR << "failed to extract media info from file: ["
                << filename << "], status: " << TagReadStatusName(status);
      return false;
    }

    if ( reader.splitter()->type() == TagSplitter::TS_F4V ) {
      // extract media info from F4V file
      CHECK_EQ(tag->type(), streaming::Tag::TYPE_F4V);
      const f4v::Tag* f4v_tag = static_cast<const f4v::Tag*>(tag.get());
      if ( f4v_tag->is_atom() && f4v_tag->atom()->type() == f4v::ATOM_MOOV ) {
        const f4v::MoovAtom* moov = static_cast<const f4v::MoovAtom*>(
            f4v_tag->atom());
        if ( !ExtractMediaInfoFromMoov(*moov, out) ) {
          LOG_ERROR << "cannot extract media info from f4v file: ["
                    << filename << "]";
          return false;
        }
        return true;
      }
      continue;
    }
    if ( reader.splitter()->type() == TagSplitter::TS_FLV ) {
      // extract media info from FLV file
      // WARNING: we get 2 types of tags: TYPE_FLV | TYPE_FLV_HEADER
      if ( tag->type() != streaming::Tag::TYPE_FLV ) {
        continue;
      }
      CHECK_EQ(tag->type(), streaming::Tag::TYPE_FLV);
      const FlvTag* flv_tag = static_cast<const FlvTag*>(tag.get());
      if ( flv_metadata.get() == NULL &&
           flv_tag->body().type() == streaming::FLV_FRAMETYPE_METADATA ) {
        flv_metadata = flv_tag;
      }
      if ( flv_audio.get() == NULL &&
           flv_tag->body().type() == streaming::FLV_FRAMETYPE_AUDIO ) {
        flv_audio = flv_tag;
      }
      if ( flv_video.get() == NULL &&
           flv_tag->body().type() == streaming::FLV_FRAMETYPE_VIDEO ) {
        flv_video = flv_tag;
      }
      if ( flv_metadata.get() != NULL &&
           flv_audio.get() != NULL &&
           flv_video.get() != NULL ) {
        if ( !ExtractMediaInfoFromFlv(*flv_metadata,
                                      flv_audio.get(),
                                      flv_video.get(), out) ) {
          LOG_ERROR << "cannot extract media info from flv file: ["
                    << filename << "]";
          return false;
        }
        return true;
      }
      continue;
    }
    if ( reader.splitter()->type() == TagSplitter::TS_MP3 ) {
      // extract media info from MP3 file
      CHECK_EQ(tag->type(), streaming::Tag::TYPE_MP3);
      const Mp3FrameTag* mp3_tag = static_cast<const Mp3FrameTag*>(tag.get());
      if ( !ExtractMediaInfoFromMp3(*mp3_tag, out) ) {
        LOG_ERROR << "cannot extract SDP from mp3 file: [" << filename << "]";
        return false;
      }
      return true;
    }
    LOG_ERROR << "Don't know how to extract media info from file: ["
              << filename << "]";
    return false;
  }
}

} // namespace util
} // namespace streaming
