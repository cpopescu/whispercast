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
// Authors: Cosmin Tudorache
//
// Transform a flv file into a f4v file. Only the container is changed,
// the binary audio & video data remains unchanged.

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

#include <whisperstreamlib/rtmp/objects/rtmp_objects.h>

#include <whisperstreamlib/flv/flv_coder.h>
#include <whisperstreamlib/flv/flv_tag.h>

#include <whisperstreamlib/f4v/f4v_tag.h>
#include <whisperstreamlib/f4v/f4v_decoder.h>
#include <whisperstreamlib/f4v/f4v_encoder.h>
#include <whisperstreamlib/f4v/atoms/base_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/ftyp_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/moov_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/mvhd_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/trak_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/tkhd_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/mdia_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/mdhd_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/hdlr_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/minf_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/smhd_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/vmhd_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/auto/dinf_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/stbl_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/stsd_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/mp4a_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/esds_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/avc1_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/avcc_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/stts_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/stsc_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/stsz_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/stco_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/stss_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/mdat_atom.h>

//////////////////////////////////////////////////////////////////////

DEFINE_string(in,
              "",
              "The input .flv file.");
DEFINE_string(out,
              "",
              "The output .f4v file. If not specified we use '<in>.f4v'.");

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

class FlvFileReader {
public:
  FlvFileReader()
    : buf_(), file_(), file_size_(0), eof_(false) {
  }
  virtual ~FlvFileReader() {
  }

  bool Open(const string& filename) {
    if ( !file_.Open(filename.c_str(),
                     io::File::GENERIC_READ,
                     io::File::OPEN_EXISTING) ) {
      LOG_ERROR << "Failed to open file: [" << filename << "]"
                   ", error: " << GetLastSystemErrorDescription();
      return false;
    }
    file_size_ = io::GetFileSize(filename);
    if ( file_size_ == -1 ) {
      LOG_ERROR << "Failed to check file size, file: [" << filename << "]"
                   ", error: " << GetLastSystemErrorDescription();
      return false;
    }
    return true;
  }
  streaming::TagReadStatus ReadHeader(
      scoped_ref<streaming::FlvHeader>* out) {
    while ( true ) {
      char* scratch_buffer;
      int32 scratch_size;
      buf_.GetScratchSpace(&scratch_buffer, &scratch_size);
      int32 read = file_.Read(scratch_buffer, scratch_size);
      if ( read < 0 ) {
        LOG_ERROR << "Error reading file: [" << file_.filename() << "]"
                     ", error: " << GetLastSystemErrorDescription();
        buf_.ConfirmScratch(0);
        return streaming::READ_CORRUPTED_FAIL;
      }
      buf_.ConfirmScratch(read);

      int cb = 0;
      streaming::TagReadStatus result =
        streaming::FlvCoder::DecodeHeader(&buf_, out, &cb);
      if ( result == streaming::READ_NO_DATA ) {
        CHECK_NULL(out->get());
        if ( read == 0 ) {
          // file EOS reached
          return streaming::READ_NO_DATA;
        }
        // read more data from file and try DecodeHeader again
        continue;
      }
      if ( result != streaming::READ_OK ) {
        CHECK_NULL(out->get());
        LOG_ERROR << "Failed to decode flv header"
                     ", result: " << streaming::TagReadStatusName(result);
        return result;
      }
      // successfully read FlvHeader
      CHECK_NOT_NULL(out->get());
      return result;
    }
  }
  streaming::TagReadStatus ReadTag(
      scoped_ref<streaming::FlvTag>* out) {
    while ( true ) {
      char* scratch_buffer;
      int32 scratch_size;
      buf_.GetScratchSpace(&scratch_buffer, &scratch_size);
      int32 read = file_.Read(scratch_buffer, scratch_size);
      if ( read < 0 ) {
        LOG_ERROR << "Error reading file: [" << file_.filename() << "]"
                     ", error: " << GetLastSystemErrorDescription();
        buf_.ConfirmScratch(0);
        return streaming::READ_CORRUPTED_FAIL;
      }
      buf_.ConfirmScratch(read);

      int cb = 0;
      streaming::TagReadStatus result =
          streaming::FlvCoder::DecodeTag(&buf_, out, &cb);
      if ( result == streaming::READ_NO_DATA ) {
        CHECK_NULL(out->get());
        if ( read == 0 ) {
          // file EOS reached
          eof_ = true;
          return streaming::READ_NO_DATA;
        }
        // read more data from file and try DecodeTag again
        continue;
      }
      if ( result != streaming::READ_OK ) {
        CHECK_NULL(out->get());
        LOG_ERROR << "Failed to decode flv tag"
                     ", result: " << streaming::TagReadStatusName(result)
                  << ", at file_pos: " << (file_.Position() - buf_.Size());
        return result;
      }
      // successfully read FlvTag
      CHECK_NOT_NULL(out->get());
      return streaming::READ_OK;
    }
  }
  bool IsEos() const {
    return eof_;
  }
  uint64 Position() const {
    CHECK_GE(file_.Position(), buf_.Size());
    return file_.Position() - buf_.Size();
  }
  uint64 Remaining() const {
    const int64 position = Position();
    CHECK(position <= file_size_)
        << " position: " << position
        << " file_size_ : " << file_size_
        << " file_.Position(): " << file_.Position()
        << ", buf_.Size(): " << buf_.Size();
    return file_size_ - position;
  }
  void Rewind() {
    Seek(0);
  }
  void Seek(int64 pos) {
    file_.SetPosition(pos, io::File::FILE_SET);
  }

private:
  io::MemoryStream buf_;
  io::File file_;
  int64 file_size_;
  bool eof_;
};

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
void ComputeSetAtomSize(streaming::f4v::BaseAtom& atom) {
  vector<const streaming::f4v::BaseAtom*> subatoms;
  atom.GetSubatoms(subatoms);
  for ( uint32 i = 0; i < subatoms.size(); i++ ) {
    streaming::f4v::BaseAtom* subatom =
      const_cast<streaming::f4v::BaseAtom*>(subatoms[i]);
    ComputeSetAtomSize(*subatom);
  }
  atom.set_size(MeasureAtom(atom));
}

void WriteMemoryStreamToFile(io::MemoryStream& in, io::File& out) {
  const char* buf = NULL;
  int32 cb_read = 0;
  while ( in.ReadNext(&buf, &cb_read) ) {
    DCHECK_GE(cb_read, 0);
    int32 cb_write = out.Write(buf, cb_read);
    CHECK_EQ(cb_read, cb_write);
  }
  CHECK(in.IsEmpty());
}

bool ConvertFlvToF4v(const string& flv_file, const string& f4v_file) {
  FlvFileReader file_reader;
  if ( !file_reader.Open(flv_file.c_str()) ) {
    LOG_ERROR << "Failed to open file for read: [" << flv_file << "]";
    return false;
  }

  streaming::f4v::FtypAtom* ftyp = new streaming::f4v::FtypAtom();
  ftyp->set_minor_version(1);
  ftyp->set_major_brand(1836069938); // backcountry.f4v
  ftyp->mutable_compatible_brands().push_back(1836069938); // backcountry.f4v
  ftyp->mutable_compatible_brands().push_back(1836069937); // backcountry.f4v
  ComputeSetAtomSize(*ftyp);

  ///////////////////////////////////////////////////////////////////////
  // Pass 1: count audio/video frames and build MOOV
  //
  scoped_ref<streaming::FlvHeader> flv_header;
  if ( file_reader.ReadHeader(&flv_header) != streaming::READ_OK ){
    LOG_ERROR << "Failed to decode flv header";
    return false;
  }
  CHECK_NOT_NULL(flv_header.get());

  LOG_INFO << "FlvHeader: " << flv_header->ToString();

  // mark the file position where the flv tags begin
  const int64 flv_body_offset = file_reader.Position();

  streaming::f4v::MoovAtom* moov = NULL;

  // Audio trak information atoms. Will point atoms inside moov.
  streaming::f4v::StszAtom* audio_stsz = NULL;
  streaming::f4v::SttsAtom* audio_stts = NULL;
  streaming::f4v::StscAtom* audio_stsc = NULL;
  streaming::f4v::StcoAtom* audio_stco = NULL;

  // AAC specific. Will point an atom inside moov.
  streaming::f4v::EsdsAtom* audio_esds = NULL;
  bool audio_esds_built = false;

  // Video trak information atoms. Will point atoms inside moov.
  streaming::f4v::StszAtom* video_stsz = NULL;
  streaming::f4v::SttsAtom* video_stts = NULL;
  streaming::f4v::StscAtom* video_stsc = NULL;
  streaming::f4v::StcoAtom* video_stco = NULL;
  streaming::f4v::StssAtom* video_stss = NULL;

  // H264 specific. Will point an atom inside moov.
  streaming::f4v::AvccAtom* video_avcc = NULL;
  bool video_avcc_built = false;

  // convention: first frame starts on offset 0
  int64 frame_offset = 0;

  // count frames
  int64 frame_count = 0;
  int64 audio_frame_count = 0;
  int64 video_frame_count = 0;

  vector<F4vChunk*> audio_chunks;
  vector<F4vChunk*> video_chunks;
  bool last_frame_was_audio = true;

  while ( true ) {
    scoped_ref<streaming::FlvTag> flv_tag;
    streaming::TagReadStatus result = file_reader.ReadTag(&flv_tag);
    if ( result == streaming::READ_NO_DATA ) {
      // file EOS reached
      break;
    }
    if ( result != streaming::READ_OK ) {
      LOG_ERROR << "Failed to decode flv tag"
                   ", result: " << streaming::TagReadStatusName(result)
                << ", at file_pos: " << file_reader.Position();
      return false;
    }

    //successfully read FlvTag
    CHECK_NOT_NULL(flv_tag.get());
    LOG_DEBUG << "Pass1, Flv Tag: " << flv_tag->ToString();


    if ( flv_tag->body().type() == streaming::FLV_FRAMETYPE_METADATA ) {
      do {
        const streaming::FlvTag::Metadata& metadata = flv_tag->metadata_body();
        LOG_INFO << "Metadata name: [" << metadata.name() << "]";

        if ( strutil::StrToLower(metadata.name().value()) != "onmetadata" ) {
          LOG_INFO << "Not 'onMetaData', ignoring..";
          break;
        }

        const rtmp::CMixedMap& values = metadata.values();
        LOG_INFO << "Metadata values:\n" << values.ToString();

        if ( moov != NULL ) {
          LOG_ERROR << "Multiple Metadata, ignoring..";
          break;
        }

        ////////////////////////////////////////////////////////////////
        // Extract every useful bit from metadata
        //
        const double duration_sec = ExtractNumber(values, "duration");
        const uint32 width = static_cast<uint32>(
            ExtractNumber(values, "width"));
        const uint32 height = static_cast<uint32>(
            ExtractNumber(values, "height"));
        const uint32 audiodatarate = static_cast<uint32>(
            ExtractNumber(values, "audiodatarate")); // bitrate in kbps
        const uint32 audiosamplesize = static_cast<uint32>(
            ExtractNumber(values, "audiosamplesize"));
        const string audiocodecid_str = ExtractString(values, "audiocodecid");
        const uint32 videodatarate = static_cast<uint32>(
            ExtractNumber(values, "videodatarate")); // bitrate in kbps
        const uint32 videocodecid_int = static_cast<uint32>(
            ExtractNumber(values, "videocodecid"));
        const string videocodecid_str = ExtractString(values, "videocodecid");

        vector<FlvTrackInfo> trackinfo;
        ExtractTrackInfo(values, &trackinfo);
        LOG_INFO << "Using trackinfo: " << strutil::ToString(trackinfo);
        const FlvTrackInfo* audio_trackinfo = NULL;
        const FlvTrackInfo* video_trackinfo = NULL;
        for ( uint32 i = 0; i < trackinfo.size(); i++ ) {
          const FlvTrackInfo* info = &trackinfo[i];
          if ( info->sampletype_ == "mp4a" ) {
            if ( audio_trackinfo != NULL ) {
              LOG_ERROR << "Multiple audio_trackinfo: " << *info
                        << ", previous found: " << *audio_trackinfo;
            } else {
              audio_trackinfo = info;
            }
          }
          if ( info->sampletype_ == "avc1" ) {
            if ( video_trackinfo != NULL ) {
              LOG_ERROR << "Multiple video_Trackinfo: " << *info
                        << ", previous found: " << *video_trackinfo;
            } else {
              video_trackinfo = info;
            }
          }
        }

        double framerate = ExtractNumber(values, "framerate");
        bool stereo = ExtractBoolean(values, "stereo");
        // TODO(cosmin): use these values, don't ignore them
        UNUSED_ALWAYS(videocodecid_int);
        UNUSED_ALWAYS(audiodatarate);
        UNUSED_ALWAYS(videodatarate);

        CHECK_NULL(moov);
        moov = new streaming::f4v::MoovAtom();

        streaming::f4v::MvhdAtom* mvhd = new streaming::f4v::MvhdAtom();
        mvhd->set_version(0);
        mvhd->set_flags(0, 0, 0);
        mvhd->set_creation_time(MacDateNow());
        mvhd->set_modification_time(MacDateNow());
        mvhd->set_time_scale(kDefaultTimeScale);
        mvhd->set_duration(duration_sec * mvhd->time_scale());
        mvhd->set_preferred_rate(kDefaultPreferredRate);
        mvhd->set_preferred_rate(kDefaultPreferredVolume);
        mvhd->set_preview_time(0);
        mvhd->set_preview_duration(0);
        mvhd->set_poster_time(0);
        mvhd->set_selection_time(0);
        mvhd->set_selection_duration(0);
        mvhd->set_current_time(0);
        mvhd->set_next_track_id(
            (flv_header->has_video() && flv_header->has_audio()) ? 3 :
            (flv_header->has_video() || flv_header->has_audio()) ? 2 : 1);
        moov->AddSubatom(mvhd);

        ///////////////////////////////////////////////////////////////////
        // Create AUDIO Trak
        //
        {
        streaming::f4v::TrakAtom* audio_trak = new streaming::f4v::TrakAtom();
        moov->AddSubatom(audio_trak);

        streaming::f4v::TkhdAtom* audio_tkhd = new streaming::f4v::TkhdAtom();
        audio_tkhd->set_version(0);
        audio_tkhd->set_flags(0, 0, 1);
        audio_tkhd->set_creation_time(MacDateNow());
        audio_tkhd->set_modification_time(MacDateNow());
        audio_tkhd->set_id(1);
        audio_tkhd->set_duration(duration_sec * mvhd->time_scale());
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
        audio_mdhd->set_time_scale(audio_trackinfo != NULL
                                   ? audio_trackinfo->timescale_
                                   : 44100);
        audio_mdhd->set_duration(audio_trackinfo != NULL
                                 ? audio_trackinfo->length_
                                 : duration_sec * audio_mdhd->time_scale());
        audio_mdhd->set_language(streaming::f4v::MacLanguageCode(
            audio_trackinfo != NULL ? audio_trackinfo->language_
                                    : string("eng")));
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
        if ( audiocodecid_str == "mp4a" ) {
          streaming::f4v::Mp4aAtom* audio_mp4a = new streaming::f4v::Mp4aAtom();
          audio_stsd->AddSubatom(audio_mp4a);
          audio_mp4a->set_data_reference_index(1);
          audio_mp4a->set_inner_version(0);
          audio_mp4a->set_revision_level(0);
          audio_mp4a->set_vendor(0);
          audio_mp4a->set_number_of_channels(stereo ? 2 : 1);
          audio_mp4a->set_sample_size_in_bits(audiosamplesize);
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
          audio_esds->set_mp4_dec_config_desc_tag_max_bit_rate(
              audiodatarate != 0 ? audiodatarate * 1000 : 96000);
          audio_esds->set_mp4_dec_config_desc_tag_avg_bit_rate(
              audiodatarate != 0 ? audiodatarate * 1000 : 96000);
          audio_mp4a->AddSubatom(audio_esds);
        }

        CHECK_NULL(audio_stsz);
        audio_stsz = new streaming::f4v::StszAtom();
        audio_stbl->AddSubatom(audio_stsz);
        //TODO(cosmin): build StszAtom

        CHECK_NULL(audio_stts);
        audio_stts = new streaming::f4v::SttsAtom();
        audio_stbl->AddSubatom(audio_stts);
        //TODO(cosmin): build SttsAtom

        CHECK_NULL(audio_stsc);
        audio_stsc = new streaming::f4v::StscAtom();
        audio_stbl->AddSubatom(audio_stsc);
        //TODO(cosmin): build StscAtom

        CHECK_NULL(audio_stco);
        audio_stco = new streaming::f4v::StcoAtom();
        audio_stbl->AddSubatom(audio_stco);
        //TODO(cosmin): build StcoAtom
        }

        ///////////////////////////////////////////////////////////////////
        // TODO(cosmin): Create VIDEO Trak
        //
        {
        streaming::f4v::TrakAtom* video_trak = new streaming::f4v::TrakAtom();
        moov->AddSubatom(video_trak);

        streaming::f4v::TkhdAtom* video_tkhd = new streaming::f4v::TkhdAtom();
        video_tkhd->set_version(0);
        video_tkhd->set_flags(0, 0, 1);
        video_tkhd->set_creation_time(MacDateNow());
        video_tkhd->set_modification_time(MacDateNow());
        video_tkhd->set_id(2);
        video_tkhd->set_duration(duration_sec * mvhd->time_scale());
        video_tkhd->set_layer(0);
        video_tkhd->set_alternate_group(0);
        video_tkhd->set_volume(0);
        video_tkhd->set_width(streaming::f4v::FPU16U16(width));
        video_tkhd->set_height(streaming::f4v::FPU16U16(height));
        video_trak->AddSubatom(video_tkhd);

        //TODO(cosmin): maybe add LoadAtom

        //TODO(cosmin): add EdtsAtom

        streaming::f4v::MdiaAtom* video_mdia = new streaming::f4v::MdiaAtom();
        video_trak->AddSubatom(video_mdia);

        streaming::f4v::MdhdAtom* video_mdhd = new streaming::f4v::MdhdAtom();
        video_mdhd->set_creation_time(MacDateNow());
        video_mdhd->set_modification_time(MacDateNow());
        video_mdhd->set_time_scale(video_trackinfo != NULL
                                   ? video_trackinfo->timescale_
                                   : static_cast<uint32>(framerate * 100));
        video_mdhd->set_duration(video_trackinfo != NULL
                                 ? video_trackinfo->length_
                                 : duration_sec * video_mdhd->time_scale());
        video_mdhd->set_language(streaming::f4v::MacLanguageCode(
            video_trackinfo != NULL ? video_trackinfo->language_
                                    : string("eng")));
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
        if ( videocodecid_str == "avc1" ) {
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

          CHECK_NULL(video_avcc);
          video_avcc = new streaming::f4v::AvccAtom();
          // "video_avcc" is built on the first video tag which contains avc configuration
          video_avc1->AddSubatom(video_avcc);
        }

        CHECK_NULL(video_stsz);
        video_stsz = new streaming::f4v::StszAtom();
        video_stbl->AddSubatom(video_stsz);
        //TODO(cosmin): build StszAtom

        CHECK_NULL(video_stts);
        video_stts = new streaming::f4v::SttsAtom();
        video_stbl->AddSubatom(video_stts);
        //TODO(cosmin): build SttsAtom

        CHECK_NULL(video_stsc);
        video_stsc = new streaming::f4v::StscAtom();
        video_stbl->AddSubatom(video_stsc);
        //TODO(cosmin): build StscAtom

        CHECK_NULL(video_stco);
        video_stco = new streaming::f4v::StcoAtom();
        video_stbl->AddSubatom(video_stco);
        //TODO(cosmin): build StcoAtom

        CHECK_NULL(video_stss);
        video_stss = new streaming::f4v::StssAtom();
        video_stbl->AddSubatom(video_stss);
        //TODO(cosmin): build StssAtom
        }

        LOG_INFO << "Built base MOOV: " << moov->ToString();
      } while ( false );
      continue;
    }

    CHECK(flv_tag->body().type() == streaming::FLV_FRAMETYPE_AUDIO ||
          flv_tag->body().type() == streaming::FLV_FRAMETYPE_VIDEO);

    if ( moov == NULL ) {
      LOG_WARNING << "No moov atom, ignoring flv tag: " << flv_tag->ToString();
      continue;
    }

    io::MemoryStream data;

    if ( flv_tag->body().type() == streaming::FLV_FRAMETYPE_AUDIO ) {
      const streaming::FlvTag::Audio& audio = flv_tag->audio_body();
      data.AppendStreamNonDestructive(&audio.data());
      if ( audio_esds != NULL &&
           data.Size() >= kAudioAacHeaderSize ) {
        uint8 tag_data_header[kAudioAacHeaderSize] = {0,};
        data.Peek(tag_data_header, sizeof(tag_data_header));
        if ( ::memcmp(tag_data_header,
                      kAudioAacInit, kAudioAacHeaderSize) == 0 ) {
          // this frame is AAC init frame
          if ( audio_esds_built ) {
            continue;
          }
          data.Skip(sizeof(tag_data_header));
          // Now flv_tag->data() contains esds_extra_data
          //TODO(cosmin): build esds
          audio_esds->set_extra_data(data);
          audio_esds_built = true;
          continue;
        }
        if (::memcmp(tag_data_header,
                     kAudioAacFrameHeader, sizeof(tag_data_header)) == 0 ) {
          // Skip AAC header, count the rest of the frame
          data.Skip(kAudioAacHeaderSize);
        }
      }
      // build audio_stsz (the AAC header has been already skipped)
      audio_stsz->AddRecord(new streaming::f4v::StszRecord(data.Size()));
      // prepare chunks for stsc stco
      if ( !last_frame_was_audio || audio_chunks.empty() ) {
        audio_chunks.push_back(new F4vChunk(frame_offset, 0));
      }
      audio_chunks[audio_chunks.size() - 1]->sample_count_++;
      last_frame_was_audio = true;

      audio_frame_count++;
    }
    if ( flv_tag->body().type() == streaming::FLV_FRAMETYPE_VIDEO ) {
      const streaming::FlvTag::Video& video = flv_tag->video_body();
      data.AppendStreamNonDestructive(&video.data());
      if ( video_avcc != NULL &&
           data.Size() >= kVideoAvcHeaderSize ) {
        uint8 tag_data_header[kVideoAvcHeaderSize] = {0,};
        data.Peek(tag_data_header, sizeof(tag_data_header));
        if ( ::memcmp(tag_data_header,
                      kVideoAvcInit, sizeof(tag_data_header)) == 0 ) {
          // this frame is AVC init frame
          if ( video_avcc_built ) {
            continue;
          }
          // build avcc atom
          data.Skip(sizeof(tag_data_header));
          // Now flv_tag->data() contains avcc_extra_data
          streaming::f4v::Decoder f4v_decoder;
          streaming::f4v::TagDecodeStatus status = video_avcc->DecodeBody(
              data.Size(), data, f4v_decoder);
          if ( status != streaming::f4v::TAG_DECODE_SUCCESS ) {
            LOG_ERROR << "Failed to decode AVCC";
            continue;
          }
          video_avcc_built = true;
          continue;
        }
        if ( ::memcmp(tag_data_header,
                      kVideoAvcKeyframeHeader,
                      sizeof(tag_data_header)) == 0 ||
             ::memcmp(tag_data_header,
                      kVideoAvcFrameHeader,
                      sizeof(tag_data_header)) == 0 ) {
          // Skip AVC header, count the rest of the frame
          data.Skip(kVideoAvcHeaderSize);
        }
      }
      // build video_stsz (the AVC header has been already skipped)
      video_stsz->AddRecord(new streaming::f4v::StszRecord(
          data.Size()));
      // build video_stss
      if ( video.video_frame_type() ==
           streaming::FLV_FLAG_VIDEO_FRAMETYPE_KEYFRAME ) {
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
    // NOTE: !use data.Size() and NOT flv_tag->Size()
    //       Because we may have skipped some tag data header(aac/avc header).
    frame_offset += data.Size();
  }

  if ( file_reader.Remaining() > 0 ) {
    LOG_ERROR << "Useless bytes at file end: " << file_reader.Remaining()
              << " bytes, at file offset: " << file_reader.Position();
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
  int64 mdat_begin_offset = MeasureAtom(*ftyp) + MeasureAtom(*moov) + 16;

  // update chunk offsets
  for ( uint32 i = 0; i < audio_stco->records().size(); i++ ) {
    audio_stco->records()[i]->chunk_offset_ += mdat_begin_offset;
  }
  for ( uint32 i = 0; i < video_stco->records().size(); i++ ) {
    video_stco->records()[i]->chunk_offset_ += mdat_begin_offset;
  }

  ComputeSetAtomSize(*moov);
  // MOOV atom complete

  streaming::f4v::MdatAtom* mdat = new streaming::f4v::MdatAtom();
  mdat->set_size(frame_offset);
  mdat->set_extended_size(true);

  if ( ftyp == NULL ) {
    LOG_ERROR << "Failed to build FTYP atom at first pass. Aborting..";
    return false;
  }
  if ( moov == NULL ) {
    LOG_ERROR << "Failed to build MOOV atom at first pass. Aborting..";
    return false;
  }
  if ( audio_esds != NULL && !audio_esds_built ) {
    LOG_ERROR << "Failed to build MOOV::ESDS atom at first pass. Aborting..";
    return false;
  }
  if ( video_avcc != NULL && !video_avcc_built ) {
    LOG_ERROR << "Failed to build MOOV::AVCC atom at first pass. Aborting..";
    return false;
  }
  if ( mdat == NULL ) {
    LOG_ERROR << "Failed to build MDAT atom at first pass. Aborting..";
    return false;
  }
  scoped_ptr<streaming::f4v::FtypAtom> auto_del_ftyp(ftyp);
  scoped_ptr<streaming::f4v::MoovAtom> auto_del_moov(moov);
  scoped_ptr<streaming::f4v::MdatAtom> auto_del_mdat(mdat);

  LOG_INFO << "Built complete MOOV: " << moov->ToString();

  // Pass 1 complete
  ////////////////////////////////////////////////////////////////////////////

  // move file pointer to file body begin
  file_reader.Seek(flv_body_offset);

  // prepare the output file
  io::File* out = new io::File();
  if ( !out->Open(f4v_file.c_str(),
                  io::File::GENERIC_READ_WRITE,
                  io::File::CREATE_ALWAYS) ) {
    LOG_ERROR << "Failed to create output file: [" << f4v_file << "]";
    return false;
  }

  ///////////////////////////////////////////////////////////////////////
  // Pass 2: copy FTYP, MOOV and audio/video frames
  //
  streaming::f4v::Encoder f4v_encoder;
  io::MemoryStream out_ms;
  f4v_encoder.WriteAtom(out_ms, *ftyp);
  f4v_encoder.WriteAtom(out_ms, *moov);
  f4v_encoder.WriteAtom(out_ms, *mdat);
  WriteMemoryStreamToFile(out_ms, *out);

  while ( true ) {
    scoped_ref<streaming::FlvTag> flv_tag;
    streaming::TagReadStatus result = file_reader.ReadTag(&flv_tag);
    if ( result == streaming::READ_NO_DATA ) {
      // file EOS reached
      break;
    }
    if ( result != streaming::READ_OK ) {
      LOG_ERROR << "Failed to decode flv tag"
                   ", result: " << streaming::TagReadStatusName(result)
                << ", at file_pos: " << file_reader.Position();
      return false;
    }

    //successfully read FlvTag
    CHECK_NOT_NULL(flv_tag.get());
    LOG_DEBUG << "Pass2, Flv Tag: " << flv_tag->ToString();

    if ( flv_tag->body().type() == streaming::FLV_FRAMETYPE_METADATA ) {
      // in this pass we ignore metadata
      continue;
    }

    if ( flv_tag->body().type() == streaming::FLV_FRAMETYPE_AUDIO ) {
      // copy audio frame
      const streaming::FlvTag::Audio& audio = flv_tag->audio_body();
      io::MemoryStream data;
      data.AppendStreamNonDestructive(&audio.data());

      if ( audio_esds != NULL &&
           data.Size() >= kAudioAacHeaderSize ) {
        uint8 tag_data_header[kAudioAacHeaderSize] = {0,};
        data.Peek(tag_data_header, sizeof(tag_data_header));
        if (::memcmp(tag_data_header,
                     kAudioAacInit, sizeof(tag_data_header)) == 0 ) {
          // Completely skip AAC init frame.
          continue;
        }
        if (::memcmp(tag_data_header,
                     kAudioAacFrameHeader, sizeof(tag_data_header)) == 0 ) {
          // Skip AAC header, copy the rest of the frame
          data.Skip(kAudioAacHeaderSize);
        }
      }
      WriteMemoryStreamToFile(data, *out);
    }
    if ( flv_tag->body().type() == streaming::FLV_FRAMETYPE_VIDEO ) {
      // copy video frame
      const streaming::FlvTag::Video& video = flv_tag->video_body();
      io::MemoryStream data;
      data.AppendStreamNonDestructive(&video.data());

      if ( video_avcc != NULL &&
           data.Size() >= kVideoAvcHeaderSize ) {
        uint8 tag_data_header[kVideoAvcHeaderSize] = {0,};
        data.Peek(tag_data_header, sizeof(tag_data_header));
        if (::memcmp(tag_data_header,
                     kVideoAvcInit, sizeof(tag_data_header)) == 0 ) {
          // Completely skip AVC init frame.
          continue;
        }
        if ( ::memcmp(tag_data_header,
                      kVideoAvcKeyframeHeader,
                      sizeof(tag_data_header)) == 0 ||
             ::memcmp(tag_data_header,
                      kVideoAvcFrameHeader,
                      sizeof(tag_data_header)) == 0 ) {
          // Skip AVC header, copy the rest of the frame
          data.Skip(kVideoAvcHeaderSize);
        }
      }
      WriteMemoryStreamToFile(data, *out);
    }
  }

  LOG_INFO << "Completely written output file";

  // Pass 2 complete
  ////////////////////////////////////////////////////////////////////////////

  return true;
}

int main(int argc, char* argv[]) {
  common::Init(argc, argv);

  if ( !io::IsReadableFile(FLAGS_in) ) {
    LOG_ERROR << "No such input file: [" << FLAGS_in << "]";
    common::Exit(1);
  }
  if ( FLAGS_out == "" ) {
    FLAGS_out = strutil::CutExtension(FLAGS_in) + ".f4v";
    LOG_WARNING << "Using implicit output: [" << FLAGS_out << "]";
  }
  if ( FLAGS_in == FLAGS_out ) {
    LOG_ERROR << "Input and output files cannot be identical";
    common::Exit(1);
  }

  LOG_WARNING << "Analyzing file: [" << FLAGS_in << "]"
                 ", size: " << io::GetFileSize(FLAGS_in) << " bytes.";

  if ( !ConvertFlvToF4v(FLAGS_in, FLAGS_out) ) {
    LOG_ERROR << "Conversion failed";
    common::Exit(1);
  }

  LOG_WARNING << "Success. Output file: [" << FLAGS_out << "]";
  common::Exit(0);
}
