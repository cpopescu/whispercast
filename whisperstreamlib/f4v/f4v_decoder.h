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
#ifndef __MEDIA_F4V_F4V_DECODER_H__
#define __MEDIA_F4V_F4V_DECODER_H__

#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperlib/common/base/gflags.h>
#include <whisperstreamlib/base/tag.h>
#include <whisperstreamlib/f4v/f4v_types.h>
#include <whisperstreamlib/f4v/frames/header.h>    // for FrameHeader

// Defined in f4v_decoder.cc
DECLARE_int32(f4v_log_level);
#define F4V_LOG_DEBUG if ( FLAGS_f4v_log_level < LDEBUG ) {} else LOG_DEBUG << "[f4v] "
#define F4V_LOG_INFO if ( FLAGS_f4v_log_level < LINFO ) {} else LOG_INFO << "[f4v] "
#define F4V_LOG_WARNING if ( FLAGS_f4v_log_level < LWARNING ) {} else LOG_WARNING << "[f4v] "
#define F4V_LOG_ERROR if ( FLAGS_f4v_log_level < LERROR ) {} else LOG_ERROR << "[f4v] "
#define F4V_LOG_FATAL LOG_FATAL << "[f4v] "

namespace streaming {
namespace f4v {

class Tag;
class BaseAtom;
class Frame;
class MoovAtom;
class MdatAtom;

class Decoder {
 public:
  static const uint32 kMaxFrameCacheSize = 64;
 public:
  Decoder();
  virtual ~Decoder();

  void set_split_raw_frames(bool split_raw_frames);

  // samplerate according to current MOOV
  uint64 timescale(bool audio) const;
  // the frames from MDAT
  const vector<FrameHeader*> frames() const;

  // Read next f4v tag from stream 'in' and store it in 'out'.
  // Returns read success.
  TagDecodeStatus ReadTag(io::MemoryStream& in, scoped_ref<Tag>* out);

  // Read next f4v atom from stream 'in' and store it in 'out'.
  // If we are currently inside a MDAT frame reading, this function
  // returns TAG_DECODE_ERROR.
  // NOTE: Used by ContainerAtom to read subatoms.
  TagDecodeStatus TryReadAtom(io::MemoryStream& in, BaseAtom** out);

  // Internal seek to given frame number.
  //  frame: frame number (0 based)
  //  seek_to_keyframe: if true we seek to closest(< frame) keyframe.
  //                    else we seek to given frame.
  //  out_frame: receives the actual seek frame
  //  out_file_offset: receives the absolute file offset where you must
  //                   position the input memory stream.
  // Returns: success.
  bool SeekToFrame(uint32 frame, bool seek_to_keyframe,
                   uint32& out_frame, int64& out_file_offset);
  //  time: milliseconds
  bool SeekToTime(int64 time, bool seek_to_keyframe,
                  uint32& out_frame, int64& out_file_offset);

  // Build a CuePointTag containing a mapping of frame index -> file offset.
  // If no MOOV atom has been encountered returns NULL.
  // You have to delete the returned CuePointTag.
  CuePointTag* GenerateCuePointTableTag() const;

  void Clear();

 private:
  // Read next f4v atom from stream 'in' and store it in 'out'.
  // You have to delete *out when you're done.
  // Returns read success.
  TagDecodeStatus ReadAtom(io::MemoryStream& in, BaseAtom** out);
  // Read next f4v frame store it in 'out'.
  // You have to delete *out when you're done.
  // Returns read success.
  TagDecodeStatus ReadFrame(io::MemoryStream& in, Frame** out);
  // Read next f4v frame from stream 'in' and store it in 'out'.
  // You have to delete *out when you're done.
  // Returns read success.
  TagDecodeStatus IOReadFrame(io::MemoryStream& in, Frame** out);
  // Go through MOOV atom and gather frame headers. Output into mdat_frames_.
  void BuildFrames();
  // Clear mdat_frames_.
  void ClearFrames();

 public:
  // Internal state description.
  string ToString() const;

 private:
  // How much data we read from input stream. Used only for debugging.
  uint64 stream_position_;

  // True on external call to ReadAtom.
  // False on recursive calls to ReadAtom (because of reading subatoms).
  bool is_topmost_atom_;
  // Input stream size when starting to read a topmost atom.
  uint64 stream_size_on_topmost_atom_;

  // last MOOV atom seen
  MoovAtom* moov_atom_;

  // Current MDAT atom.
  // If this is not NULL, then we're reading frames from this atom.
  // Else we're reading atoms.
  MdatAtom* mdat_atom_;

  // the absolute stream offset for MDAT atom begin
  int64 mdat_begin_;
  // the absolute stream offset for the last byte in MDAT atom
  int64 mdat_end_;

  // the current absolute stream offset (inside MDAT atom)
  int64 mdat_offset_;

  // the complete list of frames for current MDAT atom,
  // ordered by frame offset
  vector<FrameHeader*> mdat_frames_;
  // the index in mdat_frames_ for the next frame to be read by IOReadFrame
  uint32 mdat_next_;
  // the previous frame returned by IOReadFrame(..)
  // Used for timestamp and internal offset checks.
  const FrameHeader* mdat_prev_frame_;

  // true => we read fixed size raw frames, without the need of the MOOV atom.
  //         Useful when the MDAT comes before the MOOV atom.
  // false => regular frames, we need the MOOV
  bool mdat_split_raw_frames_;
};

} // namespace f4v
} // namespace streaming

#endif // __MEDIA_F4V_F4V_DECODER_H__
