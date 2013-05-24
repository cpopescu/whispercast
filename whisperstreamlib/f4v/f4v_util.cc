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


#include <algorithm> // for min and max
#include <whisperlib/common/base/scoped_ptr.h>
#include <whisperlib/common/io/num_streaming.h>
#include <whisperstreamlib/f4v/f4v_decoder.h>
#include <whisperstreamlib/f4v/f4v_tag.h>
#include <whisperstreamlib/f4v/f4v_util.h>
#include <whisperstreamlib/f4v/f4v_file_reader.h>
#include <whisperstreamlib/f4v/f4v_file_writer.h>
#include <whisperstreamlib/f4v/frames/frame.h>
#include <whisperstreamlib/f4v/atoms/base_atom.h>
#include <whisperstreamlib/f4v/atoms/raw_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/avcc_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/avc1_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/co64_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/ctts_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/esds_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/free_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/ftyp_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/hdlr_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/mdat_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/mdhd_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/mdia_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/minf_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/moov_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/mp4a_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/mvhd_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/smhd_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/stbl_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/stco_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/stsc_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/stsd_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/stss_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/stsz_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/stts_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/tkhd_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/trak_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/vmhd_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/wave_atom.h>


namespace streaming {
namespace f4v {
namespace util {

// static
void ExtractFrames(const MoovAtom& moov, bool audio,
    vector<FrameHeader*>* out) {
  // function header, used on logging
  const string fhead = audio ? "BuildFrames audio: " : "BuildFrames video: ";

  vector<FrameHeader*> frames;
  F4V_LOG_INFO << fhead << " ============= Start ============== ";
  const TrakAtom* trak = moov.trak(audio);
  if ( trak == NULL ) {
    F4V_LOG_ERROR << fhead << "no TrakAtom found";
    return;
  }
  const MdiaAtom* mdia = trak->mdia();
  if ( mdia == NULL ) {
    F4V_LOG_ERROR << fhead << "no MdiaAtom found";
    return;
  }
  const MdhdAtom* mdhd = mdia->mdhd();
  if ( mdhd == NULL ) {
    F4V_LOG_ERROR << fhead << "no MdhdAtom found";
    return;
  }
  const MinfAtom* minf = mdia->minf();
  if ( minf == NULL ) {
    F4V_LOG_ERROR << fhead << "no MinfAtom found";
    return;
  }
  const StblAtom* stbl = minf->stbl();
  if ( stbl == NULL ) {
    F4V_LOG_ERROR << fhead << "no StblAtom found";
    return;
  }

  const StszAtom* stsz = stbl->stsz();
  if ( stsz == NULL ) {
    F4V_LOG_ERROR << fhead << "no StszAtom found";
    return;
  }
  const StcoAtom* stco = stbl->stco();
  if ( stco == NULL ) {
    F4V_LOG_ERROR << fhead << "no StcoAtom found";
    return;
  }
  const StscAtom* stsc = stbl->stsc();
  if ( stsc == NULL ) {
    F4V_LOG_ERROR << fhead << "no StscAtom found";
    return;
  }
  const SttsAtom* stts = stbl->stts();
  if ( stts == NULL ) {
    F4V_LOG_ERROR << fhead << "no SttsAtom found";
    return;
  }
  // CTTS is optional and can be missing.
  // If missing, the offset between decoding time and composition time is 0.
  const CttsAtom* ctts = stbl->ctts();
  if ( ctts == NULL ) {
    F4V_LOG_INFO << fhead << "no CttsAtom found";
  }

  // stss Exists only for video track.
  const StssAtom* stss = stbl->stss();
  set<uint32> keyframes;
  if ( stss != NULL ) {
    for ( StssAtom::RecordVector::const_iterator it = stss->records().begin();
          it != stss->records().end(); ++it ) {
      const StssRecord& rec = **it;
      keyframes.insert(rec.sample_number_);
    }
    F4V_LOG_INFO << fhead << keyframes.size() << " keyframes";
  }

  F4V_LOG_INFO << fhead << stsz->records().size() << " frames (STSZ size)";
  F4V_LOG_INFO << fhead << stco->records().size() << " chunks (STCO size)";

  F4V_LOG_INFO << fhead << stsc->records().size()
               << " sample to chunk records (STSC size)";
  // STSC table is compact (STCO is regular table)
  if ( stsc->records().size() > stco->records().size() ||
       stsc->records().empty() ) {
    F4V_LOG_ERROR << fhead << "Wrong number of STSC records: "
                  << stsc->records().size()
                  << " while the number of STCO records is: "
                  << stco->records().size();
    return;
  }

  const SttsAtom::RecordVector& stts_records = stts->records();
  F4V_LOG_INFO << fhead << stts_records.size() << " STTS records";

  if ( ctts != NULL ) {
    F4V_LOG_INFO << fhead << ctts->records().size() << " CTTS records";
  }

  uint32 timescale = mdhd->time_scale();
  F4V_LOG_INFO << fhead << "timescale: " << timescale << " clocks per second";
  if ( timescale == 0 ) {
    F4V_LOG_ERROR << "Invalid timescale: " << timescale;
    return;
  }

  uint32 duration = mdhd->duration();
  F4V_LOG_INFO << fhead << "duration: " << duration << " clocks"
                           " / " << (duration / timescale) << " seconds";

  ///////////////////////////////////////////////////////////////////////
  // Build frames
  uint32 frame_index = 0;
  int64 decoding_time = 0; // in clocks
  // index of current STTS record
  int32 stts_index = -1;
  // current STTS record.
  // The SttsRecord.sample_count_ is decremented with every frame.
  SttsRecord stts_record;
  // index of current CTTS record
  int32 ctts_index = -1;
  // current CTTS record.
  // The CttsRecord.sample_count_ is decremented with every frame.
  CttsRecord ctts_record;

  // index of current STSC record
  int32 stsc_index = 0;

  // the chunk numbering starts with 1
  for ( uint32 chunk_index = 1; chunk_index <= stco->records().size();
        chunk_index++ ) {
    const StcoRecord& stco_record = *stco->records()[chunk_index - 1];

    // maybe go to next STSC record
    // If a STSC record is missing, then he is identical with the one before.
    //
    // The table is compactly coded. Each entry gives the index of the first
    // chunk of a run of chunks with the same characteristics.
    // By subtracting one entry here from the previous one, you can compute
    // how many chunks are in this run.

    if ( stsc_index < stsc->records().size() - 1 ) {
      const StscRecord& stsc_record = *stsc->records()[stsc_index];
      const StscRecord& next_stsc_record = *stsc->records()[stsc_index + 1];
      if ( stsc_record.first_chunk_ > chunk_index ||
           next_stsc_record.first_chunk_ < chunk_index ||
           next_stsc_record.first_chunk_ <= stsc_record.first_chunk_ ) {
        F4V_LOG_ERROR << "Illegal STSC records: "
                      << "chunk_index: " << chunk_index
                      << ", stsc_index: " << stsc_index
                      << ", stsc_record: " << stsc_record.ToString()
                      << ", next_stsc_record: " << next_stsc_record.ToString();
        return;
      }
      if ( next_stsc_record.first_chunk_ == chunk_index ) {
        stsc_index++;
      }
    }
    const StscRecord& stsc_record = *stsc->records()[stsc_index];

    int64 offset = stco_record.chunk_offset_;
    uint32 chunk_sample_index = 0;
    for ( ; chunk_sample_index < stsc_record.samples_per_chunk_ &&
          frame_index < stsz->records().size();
          chunk_sample_index++ ) {

      // compute STTS uncompressed [frame_index]
      uint64 decoding_delta = 0; // 125;
      while ( stts_record.sample_count_ == 0 &&
              stts_index < (int32)stts_records.size() - 1 ) {
        // go to next STTS record
        stts_index++;
        stts_record = *stts_records[stts_index];
      }
      if ( stts_record.sample_count_ > 0 ) {
        stts_record.sample_count_--;
        decoding_delta = stts_record.sample_delta_;
      } else {
        F4V_LOG_ERROR << "No more records in STTS: stts_index: " << stts_index
                      << ", stts_records.size(): " << stts_records.size()
                      << ". Using decoding_delta=0";
      }

      // compute CTTS uncompressed [frame_index]
      int64 composition_offset = 0;
      if ( ctts != NULL ) {
        const CttsAtom::RecordVector& ctts_records = ctts->records();
        while ( ctts_record.sample_count_ == 0 &&
                ctts_index < (int32)ctts_records.size() - 1 ) {
          // go to next CTTS record
          ctts_index++;
          ctts_record = *ctts_records[ctts_index];
        }
        if ( ctts_record.sample_count_ > 0 ) {
          ctts_record.sample_count_--;
          composition_offset = ctts_record.sample_offset_;
        } else {
          F4V_LOG_ERROR << "No more records in CTTS."
                           " Using composition_offset = 0";
        }
      }

      uint32 size = stsz->records()[frame_index]->entry_size_;
      FrameHeader* frame = new FrameHeader(offset, size,
                                           decoding_time * 1000 / timescale,
                                           composition_offset * 1000 / timescale,
                                           decoding_delta * 1000 / timescale,
                                           decoding_time + composition_offset,
                                           audio ? FrameHeader::AUDIO_FRAME :
                                                   FrameHeader::VIDEO_FRAME,
                                           false); // keyframes are set below
      frames.push_back(frame);

      offset += size;
      decoding_time += decoding_delta;
      frame_index++;
    }
    if ( chunk_sample_index < stsc_record.samples_per_chunk_ ) {
      CHECK_EQ(frame_index, stsz->records().size());
      F4V_LOG_ERROR << "Too many frames in chunk " << chunk_index
        << ", ignoring last "
        << (stsc_record.samples_per_chunk_ - chunk_sample_index) << " frames";
    }
  }

  // set keyframes (Applicable only to video)
  if ( stss != NULL ) {
    F4V_LOG_INFO << fhead << stss->records().size() << " keyframes";
    for ( StssAtom::RecordVector::const_iterator it = stss->records().begin();
          it != stss->records().end(); ++it ) {
      const StssRecord& stss_record = **it;
      // sample numbering starts on 1
      uint32 keyframe = stss_record.sample_number_;
      if ( keyframe < 1 || keyframe > frames.size() ) {
        LOG_ERROR << "Illegal keyframe: " << keyframe
                  << ", frame count: " << frames.size();
        continue;
      }
      frames[keyframe - 1]->set_keyframe(true);
    }
  }
  //for ( uint32 i = 0; i < 5 && i < frames.size(); ++i ) {
  //  F4V_LOG_INFO << fhead << frames[i];
  //}
  F4V_LOG_INFO << fhead << "........... built " << frames.size() << " frames";
  ::copy(frames.begin(), frames.end(), ::back_inserter(*out));
}

struct F4vChunk {
  uint64 start_offset_;
  uint32 sample_count_;
  F4vChunk() : start_offset_(0), sample_count_(0) {}
  F4vChunk(const F4vChunk& other)
    : start_offset_(other.start_offset_), sample_count_(other.sample_count_) {}
  F4vChunk(uint64 start_offset, uint32 sample_count)
    : start_offset_(start_offset), sample_count_(sample_count) {}
};

bool ExtractMediaInfo(const MoovAtom& moov, bool audio, MediaInfo* out) {
  const string fhead = string("ExtractMediaInfo(") +
                       (audio ? "audio" : "video") + "): ";
  const MvhdAtom* mvhd = moov.mvhd();
  if ( mvhd == NULL ) {
    F4V_LOG_ERROR << fhead << "no MvhdAtom found";
    return false;
  }
  const TrakAtom* trak = moov.trak(audio);
  if ( trak == NULL ) {
    F4V_LOG_ERROR << fhead << "no TrakAtom found";
    return false;
  }
  const TkhdAtom* tkhd = trak->tkhd();
  if ( tkhd == NULL ) {
    F4V_LOG_ERROR << fhead << "no TkhdAtom found";
    return false;
  }
  const MdiaAtom* mdia = trak->mdia();
  if ( mdia == NULL ) {
    F4V_LOG_ERROR << fhead << "no MdiaAtom found";
    return false;
  }
  const MinfAtom* minf = mdia->minf();
  if ( minf == NULL ) {
    F4V_LOG_ERROR << fhead << "no MinfAtom found";
    return false;
  }
  const MdhdAtom* mdhd = mdia->mdhd();
  if ( mdhd == NULL ) {
    F4V_LOG_ERROR << fhead << "no MdhdAtom found";
    return false;
  }
  const StblAtom* stbl = minf->stbl();
  if ( stbl == NULL ) {
    F4V_LOG_ERROR << fhead << "no StblAtom found";
    return false;
  }
  const StszAtom* stsz = stbl->stsz();
  if ( stsz == NULL ) {
    F4V_LOG_ERROR << fhead << "no StszAtom found";
    return false;
  }
  const StsdAtom* stsd = stbl->stsd();
  if ( stsd == NULL ) {
    F4V_LOG_ERROR << fhead << "no StsdAtom found";
    return false;
  }
  const StscAtom* stsc = stbl->stsc();
  if ( stsc == NULL ) {
    F4V_LOG_ERROR << fhead << "no StscAtom found";
    return false;
  }
  //MediaInfo::Track* out_trak = (audio ? &out->audio_ : &out->video_);

  uint32 presentation_timescale = mvhd->time_scale();
  if ( presentation_timescale == 0 ) {
    F4V_LOG_ERROR << fhead << "Invalid presentation_timescale: "
                  << presentation_timescale;
    return false;
  }
  uint32 presentation_duration = mvhd->duration();
  out->set_duration_ms(1000LL * presentation_duration / presentation_timescale);

  uint32 media_timescale = mdhd->time_scale();
  if ( media_timescale == 0 ) {
    F4V_LOG_ERROR << fhead << "Invalid media_timescale: " << media_timescale;
    return false;
  }
  // track timescale = media_timescale
  F4V_LOG_WARNING << "Don't know what to make of track_timescale: " << media_timescale;

  uint32 media_duration = mdhd->duration();
  float media_duration_s = 1.0f * media_duration / media_timescale; // seconds
  // track duration = media_duration_s
  F4V_LOG_WARNING << "Don't know what to make of track_duration: " << media_duration_s;

  //TODO(cosmin):
  // - the moovPosition does not correspond to MOOV atom position.
  //   The +8 hack seems to work.
  //out->moov_position_ = moov.position() + 8;

  if ( audio ) {
    ////////////////////////////////////////////////////////////////////
    // extract audio specific parameters
    MediaInfo::Audio* out_audio = out->mutable_audio();

    out_audio->mp4_language_ = mdhd->language_name();

    const Mp4aAtom* mp4a = stsd->mp4a();
    if ( mp4a != NULL ) {
      out_audio->format_ = MediaInfo::Audio::FORMAT_AAC;
      out_audio->channels_ = mp4a->number_of_channels();
      out_audio->sample_rate_ = mp4a->sample_rate().integer_;
      out_audio->sample_size_ = mp4a->sample_size_in_bits();
      if ( mp4a->esds() ) {
        mp4a->esds()->extra_data().Peek(out_audio->aac_config_,
            sizeof(out_audio->aac_config_));
      }
      out_audio->aac_profile_ = 1;
      out_audio->aac_level_ = 15;
      return true;
    }

    F4V_LOG_ERROR << fhead << "unrecognized audio specific atom."
        " Cannot extract audio info from: " << moov.ToString();
    return false;
  }

  ////////////////////////////////////////////////////////////////////
  // extract video specific parameters
  MediaInfo::Video* out_video = out->mutable_video();

  // 90000 = most common clock rate for video
  out_video->clock_rate_ = 90000;
  out_video->timescale_ = media_timescale;
  out_video->width_ = tkhd->width().integer_; // 16.16 format
  out_video->height_ = tkhd->height().integer_; // 16.16 format

  // Use Sample Size Atom for sample count. If all samples have same size
  // then STSZ contains 0 records, and we need to compute sample count otherwise.
  uint32 sample_count = stsz->records().size();
  if ( sample_count == 0 ) {
    // find sample count by counting STSC records.
    for ( uint32 i = 0; i < stsc->records().size(); i++ ) {
      const StscRecord* stsc_record = stsc->records()[i];
      sample_count += stsc_record->samples_per_chunk_;
    }
  }
  out_video->frame_rate_ = (media_duration_s == 0
                            ? 0
                            : sample_count / media_duration_s);

  const Avc1Atom* avc1 = stsd->avc1();
  if ( avc1 != NULL ) {
    out_video->format_ = MediaInfo::Video::FORMAT_H264;

    const AvccAtom* avcc = avc1->avcc();
    if ( avcc == NULL ) {
      F4V_LOG_ERROR << fhead << "no AvccAtom found";
      return false;
    }
    avcc->get_seq_parameters(&out_video->h264_sps_);
    avcc->get_pic_parameters(&out_video->h264_pps_);
    out_video->h264_configuration_version_ = avcc->configuration_version();
    out_video->h264_nalu_length_size_ = avcc->nalu_length_size();
    out_video->h264_profile_ = avcc->profile();
    out_video->h264_profile_compatibility_ = avcc->profile_compatibility();
    out_video->h264_level_ = avcc->level();

    io::MemoryStream ms;
    f4v::Encoder().WriteAtom(ms, *avcc);
    ms.Skip(8); // skip type and length (4 + 4 bytes)
    ms.ReadString(&out->mutable_video()->h264_avcc_);

    return true;
  }

  F4V_LOG_ERROR << fhead << "unrecognized video specific atom."
      " Cannot extract video info from: " << moov.ToString();
  return false;
}
bool ExtractMediaInfo(const MoovAtom& moov, MediaInfo* out) {
  return ExtractMediaInfo(moov, true, out) &&
         ExtractMediaInfo(moov, false, out);
}

const EsdsAtom* GetEsdsAtom(const MoovAtom& moov) {
  const TrakAtom* audio_trak = moov.trak(true);
  if ( audio_trak &&
       audio_trak->mdia() &&
       audio_trak->mdia()->minf() &&
       audio_trak->mdia()->minf()->stbl() &&
       audio_trak->mdia()->minf()->stbl()->stsd() &&
       audio_trak->mdia()->minf()->stbl()->stsd()->mp4a() ) {
    const Mp4aAtom* mp4a = audio_trak->mdia()->minf()->stbl()->stsd()->mp4a();
    if ( mp4a->esds() ) {
      return mp4a->esds();
    }
    if ( mp4a->wave() ) {
      return mp4a->wave()->esds();
    }
    return NULL;
  }
  return NULL;
}
const AvccAtom* GetAvccAtom(const MoovAtom& moov) {
  const TrakAtom* video_trak = moov.trak(false);
  if ( video_trak &&
       video_trak->mdia() &&
       video_trak->mdia()->minf() &&
       video_trak->mdia()->minf()->stbl() &&
       video_trak->mdia()->minf()->stbl()->stsd() &&
       video_trak->mdia()->minf()->stbl()->stsd()->avc1() ) {
    return video_trak->mdia()->minf()->stbl()->stsd()->avc1()->avcc();
  }
  return NULL;
}
StcoAtom* GetStcoAtom(MoovAtom& moov, bool audio) {
  TrakAtom* trak = moov.trak(audio);
  if ( trak &&
       trak->mdia() &&
       trak->mdia()->minf() &&
       trak->mdia()->minf()->stbl() ) {
    return trak->mdia()->minf()->stbl()->stco();
  }
  return NULL;
}
StszAtom* GetStszAtom(MoovAtom& moov, bool audio) {
  TrakAtom* trak = moov.trak(audio);
  if ( trak &&
       trak->mdia() &&
       trak->mdia()->minf() &&
       trak->mdia()->minf()->stbl() ) {
    return trak->mdia()->minf()->stbl()->stsz();
  }
  return NULL;
}
const StszAtom* GetStszAtom(const MoovAtom& moov, bool audio) {
  const TrakAtom* trak = moov.trak(audio);
  if ( trak &&
       trak->mdia() &&
       trak->mdia()->minf() &&
       trak->mdia()->minf()->stbl() ) {
    return trak->mdia()->minf()->stbl()->stsz();
  }
  return NULL;
}


const char* Spaces(uint32 count) {
  static const char* spaces = "                                              "
    "                                                                        "
    "                                                                        "
    "                                                                        "
    "                                                                        ";
  static const uint32 spaces_size = ::strlen(spaces);
  if ( count >= spaces_size ) {
    return spaces;
  }
  return spaces + spaces_size - count;
}
const char* AtomIndent(uint32 indent) {
  return Spaces(indent * 2);
}

FixResult FixFileStructure(const string& in_filename,
    const string& out_filename, bool always_fix) {
  if ( in_filename == out_filename ) {
    F4V_LOG_ERROR << "Input and output files cannot be identical";
    return FIX_ERROR;
  }
  FileReader reader;
  if ( !reader.Open(in_filename) ) {
    F4V_LOG_ERROR << "Cannot open file: [" << in_filename << "]";
    return FIX_ERROR;
  }
  reader.decoder().set_split_raw_frames(true);

  // Step1 - First Pass: extract MOOV
  //
  F4V_LOG_INFO << "Step1: first pass, extract MOOV, check for MDAT and FTYP";
  scoped_ref<F4vTag> moov_tag;
  uint64 mdat_offset = 0;
  bool ftyp_met = false;
  while ( !reader.IsEof() ) {
    scoped_ref<F4vTag> tag;
    TagDecodeStatus result = reader.Read(&tag);
    if ( result == TAG_DECODE_ERROR ) {
      F4V_LOG_ERROR << "Failed to decode tag";
      return FIX_ERROR;
    }
    if ( result == TAG_DECODE_NO_DATA ) {
      F4V_LOG_INFO << "EOF. Useless bytes at file end: " << reader.Remaining();
      break;
    }
    CHECK_EQ(result, TAG_DECODE_SUCCESS);
    CHECK_NOT_NULL(tag.get());

    if ( tag->is_atom() ) {
      if ( tag->atom()->type() == ATOM_FTYP ) {
        if ( ftyp_met ) {
          F4V_LOG_ERROR << "Multiple FTYP atoms. Corrupted file.";
          return FIX_ERROR;
        }
        ftyp_met = true;
      }
      if ( tag->atom()->type() == ATOM_MOOV ) {
        if ( moov_tag.get() != NULL ) {
          F4V_LOG_ERROR << "Multiple MOOV atoms. Corrupted file.";
          return FIX_ERROR;
        }
        // maybe the file structure is already correct
        if ( ftyp_met && mdat_offset == 0 && !always_fix ) {
          F4V_LOG_INFO << "MOOV is already between FTYP and MDAT."
                          " Nothing to do";
          return FIX_ALREADY;
        }
        moov_tag = tag;
        continue;
      }
      if ( tag->atom()->type() == ATOM_MDAT ) {
        if ( mdat_offset != 0 ) {
          F4V_LOG_ERROR << "Multiple MDAT atoms. Corrupted file.";
          return FIX_ERROR;
        }
        mdat_offset = tag->atom()->position();
      }
    }
  }
  // check that we've got the FTYP, MOOV and MDAT atoms
  if ( !ftyp_met ) {
    F4V_LOG_ERROR << "No FTYP atom found. Corrupted file.";
    return FIX_ERROR;
  }
  if ( moov_tag.get() == NULL ) {
    F4V_LOG_ERROR << "No MOOV atom found. Corrupted file.";
    return FIX_ERROR;
  }
  if ( mdat_offset == 0 ) {
    F4V_LOG_ERROR << "No MDAT atom found. Corrupted file.";
    return FIX_ERROR;
  }

  // Step2: rebuild frame offset table (STCO atom)
  //
  // we're going to move the MOOV in front of MDAT, so every frame in MDAT
  // will be moved ahead by MOOV's size.
  MoovAtom* moov = static_cast<MoovAtom*>(moov_tag->atom());
  const uint32 delta_offset = moov->size();
  StcoAtom* audio_stco = GetStcoAtom(*moov, true);
  if ( audio_stco != NULL ) {
    for ( uint32 i = 0; i < audio_stco->records().size(); i++ ) {
      audio_stco->records()[i]->chunk_offset_ += delta_offset;
    }
  }
  F4V_LOG_INFO << "#" << audio_stco->records().size()
               << " audio chunks updated";
  StcoAtom* video_stco = GetStcoAtom(*moov, false);
  if ( video_stco != NULL ) {
    for ( uint32 i = 0; i < video_stco->records().size(); i++ ) {
      video_stco->records()[i]->chunk_offset_ += delta_offset;
    }
  }
  F4V_LOG_INFO << "#" << video_stco->records().size()
               << " video chunks updated";

  // Step3 - Second Pass: Output FTYP, MOOV, ..., MDAT.
  //
  F4V_LOG_INFO << "Step2: second pass, write output file in correct order";
  reader.Rewind();

  FileWriter writer;
  if ( !writer.Open(out_filename) ) {
    F4V_LOG_ERROR << "Failed to create output file: [" << out_filename << "]";
    return FIX_ERROR;
  }

  while ( !reader.IsEof() ) {
    scoped_ref<F4vTag> tag;
    TagDecodeStatus result = reader.Read(&tag);
    if ( result == TAG_DECODE_ERROR ) {
      F4V_LOG_ERROR << "Failed to decode tag";
      return FIX_ERROR;
    }
    if ( result == TAG_DECODE_NO_DATA ) {
      F4V_LOG_INFO << "EOF. Useless bytes at file end: " << reader.Remaining();
      break;
    }
    CHECK_EQ(result, TAG_DECODE_SUCCESS);
    CHECK_NOT_NULL(tag.get());

    // skip MOOV in the original place
    if ( tag->is_atom() && tag->atom()->type() == ATOM_MOOV ) {
      F4V_LOG_INFO << "Skipping MOOV atom in the original position";
      continue;
    }

    writer.Write(*tag);

    // add MOOV after FTYP
    if ( tag->is_atom() && tag->atom()->type() == ATOM_FTYP ) {
      F4V_LOG_INFO << "Insert MOOV atom after FTYP";
      writer.Write(*moov_tag);
    }
  }
  F4V_LOG_INFO << "Fix done. Output stats: " << writer.StrStats();
  return FIX_DONE;
}

bool FixFileStructure(const string& filename) {
  const string tmp_outfile = filename + ".tmp";
  FixResult result = FixFileStructure(filename, tmp_outfile, false);
  if ( result == FIX_ERROR ) {
    io::Rm(tmp_outfile);
    return false;
  }
  if ( result == FIX_ALREADY ) {
    return true;
  }
  CHECK_EQ(result, FIX_DONE);
  if ( !io::Rename(tmp_outfile, filename, true) ) {
    F4V_LOG_ERROR << "Failed to rename temporary file: [" << tmp_outfile
                  << "] to original name: [" << filename << "]";
    io::Rm(tmp_outfile);
    return false;
  }
  return true;
}

} // namespace util
} // namespace f4v
} // namespace streaming
