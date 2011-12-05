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

#include <whisperstreamlib/f4v/f4v_file_reader.h>

namespace streaming {
namespace f4v {

FileReader::FileReader()
  : buf_(), file_(), eof_(false), decoder_(), stats_() {
}
FileReader::~FileReader() {
}

const ElementSplitterStats& FileReader::stats() const {
  return stats_;
}
streaming::f4v::Decoder& FileReader::decoder() {
  return decoder_;
}

bool FileReader::Open(const string& filename) {
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
void FileReader::Close() {
  buf_.Clear();
  file_.Close();
  file_size_ = -1;
  eof_ = false;
  decoder_.Clear();
  stats_.Clear();
}

TagDecodeStatus FileReader::Read(scoped_ref<Tag>* out) {
  if ( eof_ ) {
    return TAG_DECODE_NO_DATA;
  }

  while ( true ) {
    TagDecodeStatus result = decoder_.ReadTag(buf_, out);
    if ( result == TAG_DECODE_NO_DATA ) {
      CHECK_NULL(out->get());
      // read more data from file and try DecodeTag again
      char* scratch_buffer;
      int32 scratch_size;
      buf_.GetScratchSpace(&scratch_buffer, &scratch_size);
      int32 read = file_.Read(scratch_buffer, scratch_size);
      if ( read < 0 ) {
        buf_.ConfirmScratch(0);
        LOG_ERROR << "Failed to read file: " << file_.filename()
                  << ", at position: " << file_.Position()
                  << ", error: " << GetLastSystemErrorDescription();
        return TAG_DECODE_ERROR;
      }
      buf_.ConfirmScratch(read);
      if ( read == 0 ) {
        // file EOS reached
        eof_ = true;
        return TAG_DECODE_NO_DATA;
      }
      continue;
    }
    if ( result == TAG_DECODE_ERROR ) {
      CHECK_NULL(out->get());
      LOG_ERROR << "Failed to decode tag"
                   ", result: " << TagDecodeStatusName(result)
                << ", at file position: " << Position();
      return TAG_DECODE_ERROR;
    }
    CHECK_EQ(result, TAG_DECODE_SUCCESS);
    CHECK_NOT_NULL(out->get());

    // update file stats
    F4vTag* tag = out->get();
    stats_.num_decoded_tags_++;
    if ( tag->is_atom() &&
         tag->atom()->type() == ATOM_MDAT ) {
      stats_.total_tag_size_ += tag->atom()->is_extended_size() ? 16 : 8;
    } else {
      stats_.total_tag_size_ += tag->size();
      stats_.max_tag_size_ = max(stats_.max_tag_size_,
          static_cast<int64>(tag->size()));
    }
    if ( tag->is_frame() &&
         tag->frame()->header().type() == FrameHeader::VIDEO_FRAME ) {
      stats_.num_video_tags_++;
      stats_.num_video_bytes_ += tag->size();
    }
    if ( tag->is_frame() &&
         tag->frame()->header().type() == FrameHeader::AUDIO_FRAME ) {
      stats_.num_audio_tags_++;
      stats_.num_audio_bytes_ += tag->size();
    }

    return TAG_DECODE_SUCCESS;
  }
}
TagDecodeStatus FileReader::Read(scoped_ref<Tag>* out_tag,
                                 io::MemoryStream* out_data) {
  buf_.MarkerSet();
  int64 start_pos = file_.Position() - buf_.Size();
  TagDecodeStatus result = Read(out_tag);
  int64 end_pos = file_.Position() - buf_.Size();
  if ( result != TAG_DECODE_SUCCESS ) {
    // error reading tag, abort raw data reading
    buf_.MarkerClear();
    return result;
  }
  // tag read OK, reading raw data
  CHECK_EQ(result, TAG_DECODE_SUCCESS);
  // end_pos == start_pos When reading tags in
  //                      timestamp order and current tag has been cached
  CHECK(end_pos >= start_pos)
      << " end_pos: " << end_pos
      << " start_pos: " << start_pos
      << " on tag: " << (*out_tag)->ToString();
  buf_.MarkerRestore();
  int64 size = end_pos - start_pos;
  out_data->AppendStream(&buf_, size);
  return result;
}

bool FileReader::IsEof() const {
  return eof_;
}
uint64 FileReader::Position() const {
  CHECK_GE(file_.Position(), buf_.Size());
  return file_.Position() - buf_.Size();
}
uint64 FileReader::Remaining() const {
  const int64 position = Position();
  CHECK(position <= file_size_) << " file_.Position(): " << file_.Position()
                                << ", buf_.Size(): " << buf_.Size();
  return file_size_ - position;
}
void FileReader::Rewind() {
  file_.SetPosition(0, io::File::FILE_SET);
  eof_ = false;
  stats_ = ElementSplitterStats();
}

} // namespace f4v
} // namespace streaming
