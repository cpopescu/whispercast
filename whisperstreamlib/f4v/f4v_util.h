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

#ifndef __MEDIA_F4V_F4V_TOOLS_H__
#define __MEDIA_F4V_F4V_TOOLS_H__

#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/log.h>
#include <whisperlib/common/base/system.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/base/errno.h>
#include <whisperlib/common/base/timer.h>
#include <whisperlib/common/base/date.h>

#include <whisperlib/common/io/file/file.h>
#include <whisperlib/common/io/ioutil.h>
#include <whisperlib/common/io/buffer/memory_stream.h>

#include <whisperstreamlib/base/tag_splitter.h>
#include <whisperstreamlib/f4v/f4v_tag.h>
#include <whisperstreamlib/f4v/f4v_decoder.h>
#include <whisperstreamlib/f4v/atoms/movie/moov_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/esds_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/avcc_atom.h>
#include <whisperstreamlib/f4v/atoms/movie/stco_atom.h>

namespace streaming {
namespace f4v {
namespace util {

bool CompareFramesByTimestamp(FrameHeader* a, FrameHeader* b) ;
bool CompareFramesByOffset(FrameHeader* a, FrameHeader* b) ;

// Go through MOOV atom and gather audio/video frame headers.
void ExtractFrames(const MoovAtom& moov, bool audio,
    vector<FrameHeader*>* out);

// Update the Size of all subatoms
void UpdateSize(BaseAtom* atom);

struct MediaInfo {
 public:
  struct Track {
    Track()
      : length_(0),
        timescale_(0),
        language_() {
    }
    uint32 length_;
    uint32 timescale_;
    string language_;
    string ToString() const {
      return strutil::StringPrintf("{\n"
                                   "\t\t length_: %u, \n"
                                   "\t\t timescale_: %u, \n"
                                   "\t\t language_: %s}",
                                   length_, timescale_, language_.c_str());
    }
  };
  MediaInfo()
    : aacaot_(0),
      audio_channels_(0),
      audio_codec_id_(),
      audio_sample_rate_(0),
      audio_esds_extra_(),
      avc_level_(0),
      avc_profile_(0),
      video_codec_id_(),
      video_frame_rate_(0.0f),
      video_avcc_extra_(),
      width_(0),
      height_(0),
      duration_(0),
      moov_position_(0),
      audio_(),
      video_() {
  }
  uint32 aacaot_;
  uint32 audio_channels_;
  string audio_codec_id_;
  uint32 audio_sample_rate_;
  io::MemoryStream audio_esds_extra_; // esds extra data
  uint32 avc_level_;
  uint32 avc_profile_;
  string video_codec_id_;
  float video_frame_rate_;
  io::MemoryStream video_avcc_extra_; // avcc atom, without type & size
  uint32 width_;
  uint32 height_;
  uint64 duration_; // milliseconds
  uint64 moov_position_;
  Track audio_;
  Track video_;
  uint32 frame_count_;

  string ToString() const {
    return strutil::StringPrintf("{\n"
                                 "\t aacaot_: %u, \n"
                                 "\t audio_channels_: %u, \n"
                                 "\t audio_codec_id_: %s, \n"
                                 "\t audio_sample_rate_: %u, \n"
                                 "\t audio_esds_extra_: %s, \n"
                                 "\t avc_level_: %u, \n"
                                 "\t avc_profile_: %u, \n"
                                 "\t video_codec_id_: %s, \n"
                                 "\t video_frame_rate_: %f, \n"
                                 "\t video_avcc_extra_: %s,\n"
                                 "\t width_: %u, \n"
                                 "\t height_: %u, \n"
                                 "\t duration_: %"PRIu64", \n"
                                 "\t moov_position_: %"PRIu64", \n"
                                 "\t audio_: %s, \n"
                                 "\t video_: %s}",
                                 aacaot_,
                                 audio_channels_,
                                 audio_codec_id_.c_str(),
                                 audio_sample_rate_,
                                 audio_esds_extra_.DumpContentInline().c_str(),
                                 avc_level_,
                                 avc_profile_,
                                 video_codec_id_.c_str(),
                                 video_frame_rate_,
                                 video_avcc_extra_.DumpContentInline().c_str(),
                                 width_,
                                 height_,
                                 duration_,
                                 moov_position_,
                                 audio_.ToString().c_str(),
                                 video_.ToString().c_str());
  }
};

// Extract all sort of information from 'moov' atom, output into MediaInfo.
bool ExtractMediaInfo(const MoovAtom& moov, MediaInfo* out);
// Extracts only audio or video parameters.
bool ExtractMediaInfo(const MoovAtom& moov, bool audio, MediaInfo* out);

// returns a pointer to the EsdsAtom inside 'moov'.
// Don't delete the pointer! Returns NULL if not found.
const EsdsAtom* GetEsdsAtom(const MoovAtom& moov);
// returns a pointer to the AvccAtom inside 'moov'.
// Don't delete the pointer! Returns NULL if not found.
const AvccAtom* GetAvccAtom(const MoovAtom& moov);
// returns a pointer to the StcoAtom inside 'moov'.
// Don't delete the pointer! Returns NULL if not found.
StcoAtom* GetStcoAtom(MoovAtom& atom, bool audio);

// returns a string containing 'count' spaces.
const char* Spaces(uint32 count);
// returns a blank string with size proportional with 'indent'.
const char* AtomIndent(uint32 indent);

enum FixResult {
  FIX_ALREADY,  // original file is already good
  FIX_DONE,     // file fixed
  FIX_ERROR,    // error
};
// Moves the MoovAtom from the end of file to the beginning.
// - If the file structure is already good and always_fix==false then
//   no copying is performed, 'filename_out' is not created and it returns
//   FIX_ALREADY.
// - If the file can be fixed or always_fix==true, the output 'filename_out'
//   is created and it returns FIX_DONE
// - If an error occurs, it returns FIX_ERROR.
FixResult FixFileStructure(const string& filename_in,
                           const string& filename_out,
                           bool always_fix);
// returns: success.
bool FixFileStructure(const string& filename);


} // namespace util
} // namespace f4v
} // namespace streaming

#endif // __MEDIA_F4V_F4V_TOOLS_H__
