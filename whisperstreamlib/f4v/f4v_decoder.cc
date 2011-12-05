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
#include <whisperlib/common/io/num_streaming.h>
#include <whisperlib/common/base/scoped_ptr.h>
#include "f4v/f4v_decoder.h"
#include "f4v/f4v_tag.h"
#include "f4v/f4v_util.h"
#include "f4v/atoms/base_atom.h"
#include "f4v/atoms/raw_atom.h"
#include "f4v/frames/frame.h"

#include "f4v/atoms/movie/auto/blnk_atom.h"
#include "f4v/atoms/movie/auto/chpl_atom.h"
#include "f4v/atoms/movie/auto/dinf_atom.h"
#include "f4v/atoms/movie/auto/dlay_atom.h"
#include "f4v/atoms/movie/auto/drpo_atom.h"
#include "f4v/atoms/movie/auto/drpt_atom.h"
#include "f4v/atoms/movie/auto/edts_atom.h"
#include "f4v/atoms/movie/auto/elts_atom.h"
#include "f4v/atoms/movie/auto/hclr_atom.h"
#include "f4v/atoms/movie/auto/hlit_atom.h"
#include "f4v/atoms/movie/auto/href_atom.h"
#include "f4v/atoms/movie/auto/ilst_atom.h"
#include "f4v/atoms/movie/auto/krok_atom.h"
#include "f4v/atoms/movie/auto/load_atom.h"
#include "f4v/atoms/movie/auto/meta_atom.h"
#include "f4v/atoms/movie/auto/pdin_atom.h"
#include "f4v/atoms/movie/auto/sdtp_atom.h"
#include "f4v/atoms/movie/auto/styl_atom.h"
#include "f4v/atoms/movie/auto/tbox_atom.h"
#include "f4v/atoms/movie/auto/twrp_atom.h"
#include "f4v/atoms/movie/auto/udta_atom.h"
#include "f4v/atoms/movie/auto/uuid_atom.h"

#include "f4v/atoms/movie/avcc_atom.h"
#include "f4v/atoms/movie/avc1_atom.h"
#include "f4v/atoms/movie/co64_atom.h"
#include "f4v/atoms/movie/ctts_atom.h"
#include "f4v/atoms/movie/esds_atom.h"
#include "f4v/atoms/movie/free_atom.h"
#include "f4v/atoms/movie/ftyp_atom.h"
#include "f4v/atoms/movie/hdlr_atom.h"
#include "f4v/atoms/movie/mdat_atom.h"
#include "f4v/atoms/movie/mdhd_atom.h"
#include "f4v/atoms/movie/mdia_atom.h"
#include "f4v/atoms/movie/minf_atom.h"
#include "f4v/atoms/movie/moov_atom.h"
#include "f4v/atoms/movie/mp4a_atom.h"
#include "f4v/atoms/movie/mvhd_atom.h"
#include "f4v/atoms/movie/null_atom.h"
#include "f4v/atoms/movie/smhd_atom.h"
#include "f4v/atoms/movie/stbl_atom.h"
#include "f4v/atoms/movie/stco_atom.h"
#include "f4v/atoms/movie/stsc_atom.h"
#include "f4v/atoms/movie/stsd_atom.h"
#include "f4v/atoms/movie/stss_atom.h"
#include "f4v/atoms/movie/stsz_atom.h"
#include "f4v/atoms/movie/stts_atom.h"
#include "f4v/atoms/movie/tkhd_atom.h"
#include "f4v/atoms/movie/trak_atom.h"
#include "f4v/atoms/movie/vmhd_atom.h"
#include "f4v/atoms/movie/wave_atom.h"

DEFINE_int32(f4v_log_level, LERROR,
              "Enables F4v debug messages. Set to 4 for all messages.");
DEFINE_int32(f4v_moov_record_print_count, 5,
              "How many records to print from STSC, STCO, STSZ,.. atoms");

namespace streaming {
namespace f4v {

Decoder::Decoder()
  : stream_position_(0),
    is_topmost_atom_(true),
    stream_size_on_topmost_atom_(0),
    moov_atom_(NULL),
    mdat_atom_(NULL),
    mdat_begin_(0),
    mdat_end_(0),
    mdat_offset_(0),
    mdat_frames_(),
    mdat_next_(0),
    mdat_prev_frame_(NULL),
    mdat_order_frames_(),
    mdat_order_next_(0),
    mdat_frame_cache_(),
    mdat_seek_order_to_offset_index_(),
    mdat_order_frames_by_timestamp_(true),
    mdat_split_raw_frames_(false) {
}
Decoder::~Decoder() {
  Clear();
}

void Decoder::set_order_frames_by_timestamp(bool order_frames_by_timestamp) {
  mdat_order_frames_by_timestamp_ = order_frames_by_timestamp;
}
void Decoder::set_split_raw_frames(bool split_raw_frames) {
  mdat_split_raw_frames_ = split_raw_frames;
}
uint64 Decoder::timescale(bool audio) const {
  CHECK_NOT_NULL(moov_atom_) << "Attempting to read timescale when no MOOV found yet";
  const string fhead = (audio ? "Audio timescale: " : "Video timescale: ");
  const TrakAtom* trak = moov_atom_->trak(audio);
  if ( trak == NULL ) {
    F4V_LOG_ERROR << fhead << "no TrakAtom found";
    return 1;
  }
  const MdiaAtom* mdia = trak->mdia();
  if ( mdia == NULL ) {
    F4V_LOG_ERROR << fhead << "no MdiaAtom found";
    return 1;
  }
  const MdhdAtom* mdhd = mdia->mdhd();
  if ( mdhd == NULL ) {
    F4V_LOG_ERROR << fhead << "no MdhdAtom found";
    return 1;
  }
  return mdhd->time_scale();
}
TagDecodeStatus Decoder::ReadTag(io::MemoryStream& in, scoped_ref<Tag>* out) {
  *out = NULL;

  if ( mdat_atom_ != NULL ) {
    // read frame
    //
    Frame* frame = NULL;
    TagDecodeStatus result = ReadFrame(in, &frame);
    if ( frame != NULL ) {
      CHECK_EQ(result, TAG_DECODE_SUCCESS);
      *out = new Tag(0, kDefaultFlavourMask, frame);
      return TAG_DECODE_SUCCESS;
    }
    CHECK_NULL(frame);
    if ( result == TAG_DECODE_ERROR ||
         result == TAG_DECODE_NO_DATA ) {
      return result;
    }
    CHECK_EQ(result, TAG_DECODE_SUCCESS);
    // if  result == TAG_DECODE_SUCCESS and
    //     frame == NULL
    // it means there are no more frames to read.
    // Fall through to 'read atom'.
    F4V_LOG_INFO << "Finished reading frames, going to ReadAtom..";
  }

  CHECK_NULL(mdat_atom_);

  // read atom
  //
  BaseAtom* atom = NULL;
  TagDecodeStatus result = ReadAtom(in, &atom);
  if ( result != TAG_DECODE_SUCCESS ) {
    CHECK_NULL(atom);
    return result;
  }
  CHECK_NOT_NULL(atom) << "ReadAtom returned a NULL atom";
  *out = new Tag(0, kDefaultFlavourMask, atom);
  return TAG_DECODE_SUCCESS;
}
TagDecodeStatus Decoder::TryReadAtom(io::MemoryStream& in, BaseAtom** out) {
  *out = NULL;
  if ( mdat_atom_ != NULL ) {
    F4V_LOG_ERROR << "Cannot read atom, current MDAT atom has more frames.";
    return TAG_DECODE_ERROR;
  }
  return ReadAtom(in, out);
}
bool Decoder::SeekToFrame(uint32 desired_frame, bool seek_to_keyframe,
                          uint32& out_frame, int64& out_file_offset) {
  if ( mdat_atom_ == NULL ) {
    F4V_LOG_ERROR << "SeekToFrame: failed, not in frame reading mode.";
    return false;
  }
  if ( desired_frame >= mdat_order_frames_.size() ) {
    F4V_LOG_ERROR << "SeekToFrame: failed, frame: " << desired_frame
                  << " out of range: [0," << mdat_order_frames_.size() << ").";
    return false;
  }
  uint32 frame = desired_frame;
  if ( seek_to_keyframe ) {
    // TODO(cosmin): maybe optimize keyframe selection (instead of iterating)
    while ( frame > 0 &&
            !mdat_order_frames_[frame]->is_keyframe() ) {
      frame--;
    }
  }

  // internal seek
  mdat_order_next_ = frame;
  mdat_next_ = mdat_seek_order_to_offset_index_[frame];
  mdat_offset_ = mdat_frames_[mdat_next_]->offset();

  F4V_LOG_INFO << "SeekToFrame: desired frame: "
               << mdat_order_frames_[desired_frame]->ToString()
               << ", go to keyframe: " << std::boolalpha << seek_to_keyframe
               << ", actual frame: " << mdat_order_frames_[frame]->ToString()
               << ", mdat_offset_: " << mdat_offset_
               << ", mdat_next_: " << mdat_frames_[mdat_next_]->ToString();

  // clear mdat_frame_cache_
  for ( FrameMap::iterator it = mdat_frame_cache_.begin();
        it != mdat_frame_cache_.end(); ++it ) {
    Frame* f = it->second;
    delete f;
  }
  mdat_frame_cache_.clear();

  // clear prev frame
  delete mdat_prev_frame_;
  mdat_prev_frame_ = NULL;

  out_frame = frame;
  out_file_offset = mdat_offset_;
  return true;
}
bool Decoder::SeekToTime(int64 time, bool seek_to_keyframe,
                         uint32& out_frame, int64& out_file_offset) {
  if ( mdat_order_frames_.empty() ) {
    if ( time == 0 ) {
      // we are at the beginning of stream already
      return true;
    }
    F4V_LOG_ERROR << "SeekToTime: failed, no frames, not in frame reading mode";
    return false;
  }
  // find the first frame with a timestamp > time
  uint32 foi = 0; // frame_order_index
  for (; foi < mdat_order_frames_.size() &&
         mdat_order_frames_[foi]->timestamp() <= time; foi++) {
  }
  // go back 1 frame (just before 'time', or exactly on 'time')
  if ( foi > 0 ) {
    foi--;
  }
  F4V_LOG_INFO << "SeekToTime time: " << time
               << ", go to keyframe: " << seek_to_keyframe
               << ", found frame: " << mdat_order_frames_[foi]->ToString();
  return SeekToFrame(foi, seek_to_keyframe, out_frame, out_file_offset);
}

CuePointTag* Decoder::GenerateCuePointTableTag() const {
  if ( moov_atom_ == NULL ) {
    F4V_LOG_ERROR << "GenerateCuePointTag: failed, no MOOV atom.";
    return NULL;
  }

  CuePointTag* cue_point_tag = new CuePointTag(0, kDefaultFlavourMask, 0);
  // build a map of [timestamp_ms -> file offset]
  vector<pair<int64, int64> >& cue_points = cue_point_tag->mutable_cue_points();
  for ( uint32 i = 0; i < mdat_order_frames_.size(); i++ ) {
    FrameHeader* frame = mdat_order_frames_[i];
    if ( !frame->is_keyframe() ) {
      // we map only keyframes
      continue;
    }
    uint32 seek_frame_index = mdat_seek_order_to_offset_index_[i];
    CHECK_LT(seek_frame_index, mdat_frames_.size());
    FrameHeader* seek_frame = mdat_frames_[seek_frame_index];
    cue_points.push_back(pair<int64,int64>(frame->timestamp(),
                                           seek_frame->offset()));
  }
  F4V_LOG_INFO << "GenerateCuePointTag: " << strutil::ToString(cue_points);
  return cue_point_tag;
}
void Decoder::Clear() {
  ClearFrames();
  delete moov_atom_;
  moov_atom_ = NULL;
  mdat_seek_order_to_offset_index_.clear();
  stream_position_ = 0;
  is_topmost_atom_ = true;
  stream_size_on_topmost_atom_ = 0;

  CHECK_EQ(stream_position_, 0);
  CHECK_EQ(is_topmost_atom_, true);
  CHECK_EQ(stream_size_on_topmost_atom_, 0);
  CHECK_NULL(moov_atom_);
  CHECK_NULL(mdat_atom_);
  CHECK_EQ(mdat_begin_, 0);
  CHECK_EQ(mdat_end_, 0);
  CHECK_EQ(mdat_offset_, 0);
  CHECK(mdat_frames_.empty());
  CHECK_EQ(mdat_next_, 0);
  CHECK_NULL(mdat_prev_frame_);
  CHECK(mdat_order_frames_.empty());
  CHECK_EQ(mdat_order_next_, 0);
  CHECK(mdat_frame_cache_.empty());
  CHECK(mdat_seek_order_to_offset_index_.empty());
  //Nothing to CHECK on mdat_order_frames_by_timestamp_;
}

TagDecodeStatus Decoder::ReadAtom(io::MemoryStream& in, BaseAtom** out) {
  *out = NULL;
  CHECK_NULL(mdat_atom_) << "Cannot read atom; current MDAT atom"
                            " has more frames.";
  if ( in.Size() < 8 ) {
    F4V_LOG_DEBUG << "Stream size: " << in.Size() << " too small."
                     " Cannot read next atom.";
    return TAG_DECODE_NO_DATA;
  }
  const int64 stream_size_on_this_atom = in.Size();
  if ( is_topmost_atom_ ) {
    stream_size_on_topmost_atom_ = stream_size_on_this_atom;
  }
  uint64 atom_position = is_topmost_atom_ ? stream_position_ :
                         (stream_position_ + (stream_size_on_topmost_atom_ -
                                              stream_size_on_this_atom));

  in.MarkerSet();
  uint64 size = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  uint32 type = io::NumStreamer::ReadUInt32(&in, common::BIGENDIAN);
  uint32 header_size = 8; // 4 bytes size + 4 bytes type
  if ( size == 1 ) {
    size = io::NumStreamer::ReadUInt64(&in, common::BIGENDIAN);
    F4V_LOG_INFO << "extended size atom: " << size
                 << " found at stream position: " << atom_position;
    header_size = 16;
  }
  if ( size == 0 ) {
    // the remaining data until stream end
    F4V_LOG_WARNING << "Atom with size==0, assigning the whole stream size"
                       " to this atom: " << in.Size() << " bytes";
    size = header_size + in.Size();
  }
  if ( size < header_size ) {
    F4V_LOG_ERROR << "Illegal small atom size: " << size
                  << " , atom type: " << strutil::StringPrintf("0x%08x", type)
                  << "(" << Printable4(type) << ")";
    in.MarkerRestore();
    return TAG_DECODE_ERROR;
  }
  // performance boost: check if the stream contains the full atom
  //                    before trying to decode it.
  //                    Exception for MDAT atom: we never fully read it;
  //                    instead we read frames (one at a time) from MDAT body.
  if ( type != ATOM_MDAT &&
       in.Size() < size - header_size ) {
    in.MarkerRestore();
    F4V_LOG_DEBUG << "Not enough data in stream: " << in.Size()
                  << " is less than expected: " << (size - header_size)
                  << " , atom type: " << strutil::StringPrintf("0x%08x", type)
                  << "(" << Printable4(type) << ")";
    return TAG_DECODE_NO_DATA;
  }

  f4v::BaseAtom * atom = NULL;
  switch ( type ) {
    case ATOM_AVC1: atom = new Avc1Atom(); break;
    case ATOM_AVCC: atom = new AvccAtom(); break;
    case ATOM_BLNK: atom = new BlnkAtom(); break;
    case ATOM_CHPL: atom = new ChplAtom(); break;
    case ATOM_CO64: atom = new Co64Atom(); break;
    case ATOM_CTTS: atom = new CttsAtom(); break;
    case ATOM_DINF: atom = new DinfAtom(); break;
    case ATOM_DLAY: atom = new DlayAtom(); break;
    case ATOM_DRPO: atom = new DrpoAtom(); break;
    case ATOM_DRPT: atom = new DrptAtom(); break;
    case ATOM_EDTS: atom = new EdtsAtom(); break;
    case ATOM_ELTS: atom = new EltsAtom(); break;
    case ATOM_ESDS: atom = new EsdsAtom(); break;
    case ATOM_FREE: atom = new FreeAtom(); break;
    case ATOM_FTYP: atom = new FtypAtom(); break;
    case ATOM_HCLR: atom = new HclrAtom(); break;
    case ATOM_HDLR: atom = new HdlrAtom(); break;
    case ATOM_HLIT: atom = new HlitAtom(); break;
    case ATOM_HREF: atom = new HrefAtom(); break;
    case ATOM_ILST: atom = new IlstAtom(); break;
    case ATOM_KROK: atom = new KrokAtom(); break;
    case ATOM_LOAD: atom = new LoadAtom(); break;
    case ATOM_MDAT: atom = new MdatAtom(); break;
    case ATOM_MDHD: atom = new MdhdAtom(); break;
    case ATOM_MDIA: atom = new MdiaAtom(); break;
    case ATOM_META: atom = new MetaAtom(); break;
    case ATOM_MINF: atom = new MinfAtom(); break;
    case ATOM_MOOV: atom = new MoovAtom(); break;
    case ATOM_MP4A: atom = new Mp4aAtom(); break;
    case ATOM_MVHD: atom = new MvhdAtom(); break;
    case ATOM_NULL: atom = new NullAtom(); break;
    case ATOM_PDIN: atom = new PdinAtom(); break;
    case ATOM_SDTP: atom = new SdtpAtom(); break;
    case ATOM_SMHD: atom = new SmhdAtom(); break;
    case ATOM_STBL: atom = new StblAtom(); break;
    case ATOM_STCO: atom = new StcoAtom(); break;
    case ATOM_STSC: atom = new StscAtom(); break;
    case ATOM_STSD: atom = new StsdAtom(); break;
    case ATOM_STSS: atom = new StssAtom(); break;
    case ATOM_STSZ: atom = new StszAtom(); break;
    case ATOM_STTS: atom = new SttsAtom(); break;
    case ATOM_STYL: atom = new StylAtom(); break;
    case ATOM_TBOX: atom = new TboxAtom(); break;
    case ATOM_TKHD: atom = new TkhdAtom(); break;
    case ATOM_TRAK: atom = new TrakAtom(); break;
    case ATOM_TWRP: atom = new TwrpAtom(); break;
    case ATOM_UDTA: atom = new UdtaAtom(); break;
    case ATOM_UUID: atom = new UuidAtom(); break;
    case ATOM_VMHD: atom = new VmhdAtom(); break;
    case ATOM_WAVE: atom = new WaveAtom(); break;
  }
  if ( atom == NULL ) {
    F4V_LOG_ERROR << "unknown atom type: "
                  << strutil::StringPrintf("0x%08x", type) << "("
                  << Printable4(type) << "), using RawAtom..";
    atom = new RawAtom(type);
  }
  CHECK_NOT_NULL(atom);

  F4V_LOG_DEBUG << "Found " << (is_topmost_atom_ ? "Atom" : "Subatom")
                << " Header: " << atom->type_name() << " @" << atom_position
                << " with size: " << size << ", stream size: " << in.Size();

  // backup is_topmost_atom_
  bool this_is_topmost_atom = is_topmost_atom_;
  // we're going to decode an atom, recursive calls will find subatoms
  is_topmost_atom_ = false;
  const int32 size_before_decode = in.Size();
  TagDecodeStatus status = atom->Decode(atom_position, size,
                                         header_size == 16, in, *this);
  const int32 size_after_decode = in.Size();
  CHECK_GE(size_before_decode, size_after_decode);
  const int32 size_decoded = size_before_decode - size_after_decode;
  // restore is_topmost_atom_
  is_topmost_atom_ = this_is_topmost_atom;

  if ( status != TAG_DECODE_SUCCESS ) {
    F4V_LOG_ERROR << "Failed to decode atom: " << atom->type_name()
                  << " , error: " << TagDecodeStatusName(status)
                  << " , at stream position: " << atom_position;
    in.MarkerRestore();
    delete atom;
    atom = NULL;
    return status;
  }

  // MDAT atom is partially decoded.
  // The rest of the atoms should be fully decoded.
  if ( atom->type() != ATOM_MDAT ) {
    CHECK(size == header_size + size_decoded)
      << ", with size=" << size
      << " header_size=" << header_size
      << " size_decoded=" << size_decoded
      << " atom->type()=" << atom->type_name();
  }

  in.MarkerClear();
  if ( is_topmost_atom_ ) {
    // update stream position only for topmost atoms
    stream_position_ += header_size + size_decoded;
  }

  if ( atom->type() == ATOM_MOOV ) {
    // this is the last MOOV encountered, make a copy of it
    delete moov_atom_;
    moov_atom_ = static_cast<MoovAtom*>(atom->Clone());
    // extract and print MediaInfo .. just for debug
    util::MediaInfo media_info;
    util::ExtractMediaInfo(*moov_atom_, &media_info);
    F4V_LOG_INFO << "MediaInfo: " << media_info.ToString();
  }
  if ( atom->type() == ATOM_MDAT ) {
    if ( !mdat_split_raw_frames_ && moov_atom_ == NULL ) {
      F4V_LOG_ERROR << "Found MDAT with no previous MOOV.";
      delete atom;
      atom = NULL;
      return TAG_DECODE_ERROR;
    }
    CHECK_NULL(mdat_atom_) << "why are we reading atoms when there's"
                              " a current MDAT atom set?";
    mdat_atom_ = static_cast<MdatAtom*>(atom->Clone());
    mdat_begin_ = mdat_atom_->position() + header_size;
    mdat_end_ = mdat_atom_->position() + mdat_atom_->size();
    mdat_offset_ = mdat_begin_;
    F4V_LOG_DEBUG << "Before BuildFrames"
                     ", mdat_begin_: " << mdat_begin_
                  << ", mdat_end_: " << mdat_end_
                  << ", mdat_offset_: " << mdat_offset_
                  << ", peek data: " << in.DumpContentHex(10);
    BuildFrames();
  }

  *out = atom;
  return TAG_DECODE_SUCCESS;
}
TagDecodeStatus Decoder::ReadFrame(io::MemoryStream& in, Frame** out) {
  *out = NULL;

  if ( mdat_split_raw_frames_ ) {
    // just split MDAT into raw frames, no decoding
    int64 raw_frame_size = min(mdat_end_ - mdat_offset_, (int64)10000LL);
    if ( raw_frame_size == 0 ) {
      F4V_LOG_INFO << "No more raw frames.";
      ClearFrames();
      return TAG_DECODE_SUCCESS;
    }
    if ( in.Size() < raw_frame_size ) {
      return TAG_DECODE_NO_DATA;
    }
    FrameHeader header(mdat_offset_ - mdat_begin_, raw_frame_size, 0,
                0, 0, 0, FrameHeader::RAW_FRAME, false);
    *out = new Frame(header);
    (*out)->mutable_data().AppendStream(&in, raw_frame_size);
    mdat_offset_ += raw_frame_size;
    stream_position_ += raw_frame_size;
    return TAG_DECODE_SUCCESS;
  }
  if ( mdat_order_frames_by_timestamp_ == false ) {
    // direct decoding, stream order
    TagDecodeStatus status = IOReadFrame(in, out);
    if ( status == TAG_DECODE_SUCCESS && *out == NULL ) {
      F4V_LOG_INFO << "No more frames.";
      ClearFrames();
    }
    return status;
  }

  // decode frames in custom order
  //

  if ( mdat_order_next_ >= mdat_order_frames_.size() ) {
    // no more frames to decode
    CHECK_EQ(mdat_order_next_, mdat_order_frames_.size());
    F4V_LOG_INFO << "No more frames.";
    ClearFrames();
    return TAG_DECODE_SUCCESS;
  }
  FrameHeader& lookout_frame = *mdat_order_frames_[mdat_order_next_];

  // look in frame cache
  //
  FrameMap::iterator it = mdat_frame_cache_.find(lookout_frame.offset());
  if ( it != mdat_frame_cache_.end() ) {
    F4V_LOG_DEBUG << "ReadFrame: Frame found in cache, offset: "
                  << lookout_frame.offset();
    Frame* frame = it->second;
    mdat_frame_cache_.erase(it);
    *out = frame;
    mdat_order_next_++;
    return TAG_DECODE_SUCCESS;
  }

  // IO read frames until we find it
  //
  while ( true ) {
    Frame* frame = NULL;
    // 1. read a new frame from "in" stream
    TagDecodeStatus status = IOReadFrame(in, &frame);
    if ( status != TAG_DECODE_SUCCESS ) {
      if ( status == TAG_DECODE_NO_DATA ) {
        F4V_LOG_DEBUG << "IOReadFrame failed. result: "
                      << TagDecodeStatusName(status);
      } else {
        F4V_LOG_ERROR << "IOReadFrame failed. result: "
                      << TagDecodeStatusName(status);
      }
      return status;
    }
    // 2. is it the frame we're looking for?
    if ( frame->header().offset() == lookout_frame.offset() ) {
      F4V_LOG_DEBUG << "ReadFrame: Frame found by IO read, offset: "
                    << frame->header().offset();
      *out = frame;
      mdat_order_next_++;
      return TAG_DECODE_SUCCESS;
    }
    // 3. is it a raw frame? We don't expect raw frames so there's no reason
    //    caching them.
    if ( frame->header().type() == FrameHeader::RAW_FRAME ) {
      F4V_LOG_DEBUG << "ReadFrame: RawFrame found by IO read, offset: "
                    << frame->header().offset();
      *out = frame;
      // don't advance mdat_order_next_
      return TAG_DECODE_SUCCESS;
    }
    // 4. cache other frames
    // IOReadFrame should read frames in offset order, and we cannot
    // skip/lose frames";
    CHECK_LT(frame->header().offset(), lookout_frame.offset());
    if ( mdat_frame_cache_.size() > kMaxFrameCacheSize ) {
      F4V_LOG_ERROR << "ReadFrame: frame cache max size exceeded"
                       ", current size: " << mdat_frame_cache_.size();
      return TAG_DECODE_ERROR;
    }
    pair<FrameMap::iterator, bool> result = mdat_frame_cache_.insert(
        make_pair(frame->header().offset(), frame));
    if ( !result.second ) {
      Frame* old = result.first->second;
      F4V_LOG_ERROR << "Duplicate frame offset!"
                       " old: " << old->header().ToString()
                    << ", new: " << frame->header().ToString()
                    << " ignoring new frame.";
      delete frame;
      frame = NULL;
    }
    F4V_LOG_DEBUG << "ReadFrame: caching frame, offset: "
                  << frame->header().offset()
                  << " (looking for offset: "
                  << lookout_frame.offset() << ")"
                     ", cache size: " << mdat_frame_cache_.size();
  }
}
TagDecodeStatus Decoder::IOReadFrame(io::MemoryStream& in, Frame** out) {
  *out = NULL;

  // BUGTRAP: integrity checks
  CHECK_NOT_NULL(mdat_atom_) << "Cannot read frame; no current MDAT atom";
  CHECK_GE(mdat_offset_, mdat_begin_);
  CHECK_LE(mdat_offset_, mdat_end_);

  // BUGTRAP: stream offset should be just after the previous frame end.
  if ( mdat_prev_frame_ != NULL ) {
    int64 prev_frame_end = mdat_prev_frame_->offset() +
                           mdat_prev_frame_->size();
    CHECK_EQ(prev_frame_end, mdat_offset_);
  }

  // The frame_offset is computed by adding up frame_size s starting from
  // chunk start offset. Inside the chunk the frame offset cannot be wrong.
  // But the chunk offset may be wrong. So theoretically,
  // 3 BAD cases are possible:
  //  - frame overlaps MDAT begin (i.e. frame offset is before MDAT begin)
  //  - frame overlaps one another (i.e. frame offset is before prev frame end)
  //  - frame overlaps MDAT end (i.e. frame end is after MDAT end)
  //

  Frame* frame = NULL;
  bool advance_to_next_frame = false;
  while (true) {
    // Did we finish reading frames?
    if ( mdat_next_ >= mdat_frames_.size() ) {
      // no more frames to decode
      CHECK_EQ(mdat_next_, mdat_frames_.size());
      if ( mdat_offset_ >= mdat_end_ ) {
        // No more data between previous frame and MDAT end.
        CHECK_EQ(mdat_offset_, mdat_end_);
        F4V_LOG_INFO << "IOReadFrame: No more frames to read.";
        return TAG_DECODE_SUCCESS;
      }
      // there's some RAW data remaining between stream offset and MDAT atom end
      int64 raw_size = mdat_end_ - mdat_offset_;
      int64 raw_ts = mdat_prev_frame_ ? mdat_prev_frame_->timestamp() : 0;
      frame = new Frame(FrameHeader(mdat_offset_, raw_size,
                                    raw_ts, raw_ts, 0, 0, FrameHeader::RAW_FRAME,
                                    false));
      advance_to_next_frame = false;
      F4V_LOG_WARNING << "IOReadFrame: Raw frame before MDAT end: "
                      << frame->header().ToString();
      break;
    }

    CHECK_LT(mdat_next_, mdat_frames_.size());
    const FrameHeader& frame_header = *mdat_frames_[mdat_next_];

    // Skip frames overlapping MDAT begin
    if ( frame_header.offset() < mdat_begin_ ) {
      F4V_LOG_ERROR << "IOReadFrame: Frame is overlapping MDAT begin!"
                       " mdat_offset_: " << mdat_offset_
                    << ", mdat_begin_: " << mdat_begin_
                    << ", frame: " << frame_header.ToString();
      mdat_next_++;
      continue;
    }

    // Skip frames beyond MDAT end
    if ( frame_header.offset() >= mdat_end_ ) {
      F4V_LOG_ERROR << "IOReadFrame: Frame is beyond MDAT end!"
                       " mdat_offset_: " << mdat_offset_
                    << ", mdat_end_: " << mdat_end_
                    << ", frame: " << frame_header.ToString();
      mdat_next_++;
      continue;
    }

    // Skip frames in the past (having start offset < current stream offset)
    if ( frame_header.offset() < mdat_offset_ ) {
      // we must read a previous frame, otherwise the mdat_offset_
      // could not have advanced.
      CHECK_NOT_NULL(mdat_prev_frame_);
      F4V_LOG_ERROR << "IOReadFrame: Overlapping frames detected!"
                       " Previous read frame: " << mdat_prev_frame_->ToString()
                    << ", current frame: " << frame_header.ToString()
                    << ", mdat_offset_: " << mdat_offset_;
      mdat_next_++;
      continue;
    }

    // We're left with present or future frame
    CHECK(mdat_offset_ <= frame_header.offset())
        << " mdat_offset_: " << mdat_offset_
        << "frame in the past: " << frame_header.ToString();

    // For frames in the future, insert a RAW frame to include the data
    // between stream offset and the future frame. Don't advance to next frame.
    if ( mdat_offset_ < frame_header.offset() ) {
      // Current 'frame_header' is actually in the future.
      // Create a RAW frame, containing binary data between
      // stream offset and next frame start.
      int64 raw_end = std::min(frame_header.offset(), mdat_end_);
      int64 raw_size = raw_end - mdat_offset_;
      int64 raw_ts = mdat_prev_frame_ ? mdat_prev_frame_->timestamp() : 0;
      frame = new Frame(FrameHeader(mdat_offset_, raw_size,
                                    raw_ts, raw_ts, 0, 0, FrameHeader::RAW_FRAME,
                                    false));
      advance_to_next_frame = false;
      if ( mdat_prev_frame_ != NULL ) {
        F4V_LOG_WARNING << "IOReadFrame: Raw frame between frames"
                           ", prev: " << mdat_prev_frame_->ToString()
                        << ", raw: " << frame->header().ToString()
                        << ", next: " << frame_header.ToString()
                        << ", raw data: "
                        << in.DumpContentHex(frame->header().size());
      } else {
        F4V_LOG_WARNING << "IOReadFrame: Raw frame on MDAT begin"
                           ", mdat_begin_: " << mdat_begin_
                        << ", raw: " << frame->header().ToString()
                        << ", next: " << frame_header.ToString()
                        << ", raw data: "
                        << frame->header().size() << " bytes";
                        //<< in.DumpContentHex(frame->header().size());
      }
      break;
    }

    // Good, present: the frame offset is exactly on current stream offset.
    // But is the frame end before MDAT end?
    CHECK_EQ(mdat_offset_, frame_header.offset());
    int64 frame_end = frame_header.offset() + frame_header.size();
    if ( frame_end > mdat_end_ ) {
      // frame is overlapping MDAT end
      F4V_LOG_ERROR << "IOReadFrame: Frame is overlapping MDAT end!"
                       " mdat_offset_: " << mdat_offset_
                    << ", mdat_begin_: " << mdat_begin_
                    << ", mdat_end_: " << mdat_end_
                    << ", frame: " << frame_header.ToString();
      // create a RAW frame from the first part of this frame.
      // We need to advance to next frame.
      frame = new Frame(FrameHeader(mdat_offset_, mdat_end_ - mdat_offset_,
                                    frame_header.decoding_timestamp(),
                                    frame_header.composition_timestamp(),
                                    frame_header.duration(),
                                    frame_header.sample_index(),
                                    FrameHeader::RAW_FRAME,
                                    false));
      advance_to_next_frame = true;
      F4V_LOG_WARNING << "IOReadFrame: Raw frame created from the first part"
                         " of an overlapping frame on MDAT end: "
                      << frame->header().ToString();
      break;
    }
    // The GOOD regular case.
    // The frame offset is good.
    CHECK_GE(frame_header.offset(), mdat_begin_);
    CHECK_EQ(frame_header.offset(), mdat_offset_);
    // The frame end is good
    CHECK_LE(frame_end, mdat_end_);
    frame = new Frame(frame_header);
    advance_to_next_frame = true;
    F4V_LOG_DEBUG << "IOReadFrame: Regular frame: "
                  << frame->header().ToString();
    break;
  }

  CHECK_NOT_NULL(frame);

  // Performance: early check for enough data in stream.
  // Instead of: marker set, try decode, marker restore, return failure.
  if ( in.Size() < frame->header().size() ) {
    F4V_LOG_DEBUG << "IOReadFrame: Not enough data in stream: " << in.Size()
                  << ", is less than frame size: " << frame->header().size();
    return TAG_DECODE_NO_DATA;
  }

  in.MarkerSet();
  const int64 stream_size_before = in.Size();
  TagDecodeStatus result = frame->Decode(in, *this);
  const int64 stream_size_after = in.Size();
  if ( result != TAG_DECODE_SUCCESS ) {
    F4V_LOG_ERROR << "IOReadFrame: Failed to read frame."
                     " result: " << TagDecodeStatusName(result);
    in.MarkerRestore();
    delete frame;
    frame = NULL;
    return result;
  }
  in.MarkerClear();

  CHECK_GE(stream_size_before, stream_size_after);
  const int64 read = stream_size_before - stream_size_after;
  CHECK_EQ(read, frame->header().size());

  mdat_offset_ += read;
  stream_position_ += read;
  delete mdat_prev_frame_;
  mdat_prev_frame_ = new FrameHeader(frame->header());
  // RAW frames do not advance mdat_next_.
  if ( advance_to_next_frame ) {
    mdat_next_++;
  }
  *out = frame;
  return TAG_DECODE_SUCCESS;
}
void Decoder::BuildFrames() {
  if ( mdat_split_raw_frames_ ) {
    F4V_LOG_WARNING << "BuildFrames: going for raw frames. Nothing to build.";
    return;
  }
  // check prerequisites
  if ( moov_atom_ == NULL ) {
    F4V_LOG_ERROR << "BuildFrames fatal: no moov_atom_ found";
    return;
  }

  // check clear
  CHECK(mdat_frames_.empty());
  CHECK(mdat_order_frames_.empty());
  CHECK(mdat_frame_cache_.empty());

  // BuildFrames into temporary containers
  vector<FrameHeader*> mdat_audio_frames;
  util::ExtractFrames(*moov_atom_, true, &mdat_audio_frames);
  vector<FrameHeader*> mdat_video_frames;
  util::ExtractFrames(*moov_atom_, false, &mdat_video_frames);

  // remember audio & video frame count, to print later
  uint32 stream_audio_frames = mdat_audio_frames.size();
  uint32 stream_video_frames = mdat_video_frames.size();

  // First: merge audio & video frames into one single vector, ordered by frame
  // offset. Used by IOReadFrame to read from frames from file.
  vector<FrameHeader*>::iterator audio_it = mdat_audio_frames.begin();
  vector<FrameHeader*>::iterator video_it = mdat_video_frames.begin();
  for ( ; audio_it != mdat_audio_frames.end() ||
          video_it != mdat_video_frames.end(); ) {
    bool is_audio = video_it == mdat_video_frames.end() ? true :
                    audio_it == mdat_audio_frames.end() ? false:
                    (*audio_it)->offset() < (*video_it)->offset();
    vector<FrameHeader*>::iterator& it = is_audio ? audio_it : video_it;
    FrameHeader* frame = *it;
    ++it;
    mdat_frames_.push_back(frame);
  }
  // Second: make a frame vector in read order (by timestamp or by offset).
  // Begin with making a copy of the frames in offset order, then sort it.
  ::copy(mdat_frames_.begin(), mdat_frames_.end(),
         inserter(mdat_order_frames_, mdat_order_frames_.end()));
  ::stable_sort(mdat_order_frames_.begin(), mdat_order_frames_.end(),
                mdat_order_frames_by_timestamp_ ? util::CompareFramesByTimestamp
                                                : util::CompareFramesByOffset);
  mdat_audio_frames.clear();
  mdat_video_frames.clear();

  // Build mdat_seek_frame_
  //
  CHECK(mdat_seek_order_to_offset_index_.empty());
  {
    uint32 offset_index = 0;
    set<FrameHeader*> visited;
    for ( uint32 order_index = 0; order_index < mdat_order_frames_.size();
          order_index++ ) {
      FrameHeader* order_frame = mdat_order_frames_[order_index];
      FrameHeader* offset_frame = mdat_frames_[offset_index];

      while ( visited.find(offset_frame) != visited.end() ) {
        visited.erase(offset_frame);
        offset_index++;
        offset_frame = mdat_frames_[offset_index];
      }

      //Make: mdat_seek_order_to_offset_index_[order_index] = offset_index;
      CHECK_EQ(mdat_seek_order_to_offset_index_.size(), order_index);
      mdat_seek_order_to_offset_index_.push_back(offset_index);

      visited.insert(order_frame);
    }
  }

  //for ( uint32 i = 0; i < 1000 && i < mdat_order_frames_.size(); ++i ) {
  //  F4V_LOG_INFO << "#" << i << " a&v ordered frame: " << *mdat_order_frames_[i];
  //}
  F4V_LOG_INFO << "audio frames: " << stream_audio_frames << " in stream";
  F4V_LOG_INFO << "video frames: " << stream_video_frames << " in stream";
  F4V_LOG_INFO << "total frames: " << mdat_frames_.size();
  F4V_LOG_INFO << "mdat_begin_: " << mdat_begin_;
  F4V_LOG_INFO << "mdat_end_: " << mdat_end_;
  if ( !mdat_order_frames_.empty() ) {
    F4V_LOG_INFO << "first ordered frame: "
                 << mdat_order_frames_[0]->ToString();
    F4V_LOG_INFO << "last ordered frame: "
                 << mdat_order_frames_[mdat_order_frames_.size() - 1]->ToString();
  }
  F4V_LOG_INFO << "frame order: " << (mdat_order_frames_by_timestamp_ ?
                                     "by timestamp" : "by offset");
}
void Decoder::ClearFrames() {
  for ( vector<FrameHeader*>::iterator it = mdat_frames_.begin();
        it != mdat_frames_.end(); ++it ) {
    FrameHeader* frame_header = *it;
    delete frame_header;
  }
  mdat_frames_.clear();
  mdat_order_frames_.clear();
  mdat_next_ = 0;
  mdat_order_next_ = 0;
  delete mdat_prev_frame_;
  mdat_prev_frame_ = NULL;
  for ( FrameMap::iterator it = mdat_frame_cache_.begin();
        it != mdat_frame_cache_.end(); it++ ) {
    Frame* frame = it->second;
    delete frame;
  }
  mdat_frame_cache_.clear();
  mdat_seek_order_to_offset_index_.clear();

  delete mdat_atom_;
  mdat_atom_ = NULL;
  mdat_begin_ = 0;
  mdat_end_ = 0;
  mdat_offset_ = 0;

  CHECK(mdat_frames_.empty());
  CHECK(mdat_order_frames_.empty());
  CHECK(mdat_frame_cache_.empty());
  CHECK_NULL(mdat_prev_frame_);
  CHECK_NULL(mdat_atom_);
}

string Decoder::ToString() const {
  return strutil::StringPrintf(
      "{stream_position_: %"PRIu64""
      ", mdat_begin_: %"PRId64""
      ", mdat_end_: %"PRId64""
      ", mdat_offset_: %"PRId64""
      ", mdat_frames_: %zu frames"
      ", mdat_next_: %u"
      ", mdat_prev_frame_: %s"
      ", mdat_order_frames_: %zu frames"
      ", mdat_order_next_: %u"
      ", mdat_frame_cache_: %zu frames"
      ", mdat_seek_order_to_offset_index_: %zu entries"
      ", mdat_order_frames_by_timestamp_: %s}",
      (stream_position_),
      (mdat_begin_),
      (mdat_end_),
      (mdat_offset_),
      mdat_frames_.size(),
      mdat_next_,
      mdat_prev_frame_ == NULL ? "NULL" : mdat_prev_frame_->ToString().c_str(),
      mdat_order_frames_.size(),
      mdat_order_next_,
      mdat_frame_cache_.size(),
      mdat_seek_order_to_offset_index_.size(),
      strutil::BoolToString(mdat_order_frames_by_timestamp_).c_str());
}

}
}
