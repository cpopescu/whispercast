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
// Author: Catalin Popescu

#include "aac/aac_tag_splitter.h"

#include <whisperlib/common/base/log.h>

namespace streaming {

const Tag::Type AacFrameTag::kType = Tag::TYPE_AAC;
const TagSplitter::Type AacTagSplitter::kType = TagSplitter::TS_AAC;
const int AacTagSplitter::kAacHeaderLen = 2048;


AacTagSplitter::AacTagSplitter(const string& name)
    : streaming::TagSplitter(kType, name),
      aac_handle_(faacDecOpen()),
      samplerate_(0),
      channels_(0),
      header_extracted_(false),
      stream_offset_ms_(0.0) {
  faacDecConfigurationPtr config =
    faacDecGetCurrentConfiguration(aac_handle_);
  config->defObjectType = LC;
  config->outputFormat = FAAD_FMT_16BIT;
  config->downMatrix = 0;
  config->useOldADTSFormat = 0;
  faacDecSetConfiguration(aac_handle_, config);
}

AacTagSplitter::~AacTagSplitter() {
  faacDecClose(aac_handle_);
}

streaming::TagReadStatus AacTagSplitter::ReadHeader(
  io::MemoryStream* in, scoped_ref<Tag>* tag) {
  uint8 buf2[2];
  // Skip until we get an 0xff 0xfX
  while ( true ) {
    if ( 2 != in->Peek(buf2, 2) ) {
      return streaming::READ_NO_DATA;
    }
    if ( buf2[0] == 0xff && (buf2[1] & 0xf0) == 0xf0 ) {
      break;
    }
    in->Skip(1);
  }

  CHECK(!header_extracted_);
  if ( in->Size() < kAacHeaderLen ) {
    return streaming::READ_NO_DATA;
  }
  // LOG_INFO << "Header at: \n" << in->DumpContent(16);
  in->MarkerSet();
  unsigned char buffer[kAacHeaderLen];
  CHECK_EQ(in->Read(buffer, kAacHeaderLen), kAacHeaderLen);

  // WARNING: we did not convince this guy to be portable

  // If one of these gives errors, change the other path of if:
#ifdef __FAAD_USE_UINT32_T
  uint32_t sr;
#else
  unsigned long int sr;
#endif

  const int size = faacDecInit(aac_handle_,
                               buffer, sizeof(buffer),
                               &sr, &channels_);
  samplerate_ = sr;

  if ( size < 0 ) {
    LOG_ERROR << name() << "Error decoding AAC header: "
             << faacDecGetErrorMessage(frame_info_.error);
    return streaming::READ_CORRUPTED_FAIL;
  }
  in->MarkerRestore();
  in->Skip(size);

  *tag = new AacFrameTag(Tag::ATTR_CAN_RESYNC,
                         kDefaultFlavourMask,
                         0,
                         0,
                         samplerate_,
                         channels_,
                         0,
                         reinterpret_cast<const char*>(buffer),
                         size);

  header_extracted_ = true;
  return streaming::READ_OK;
}

streaming::TagReadStatus AacTagSplitter::MaybeFinalizeFrame(
    scoped_ref<Tag>* tag, const char* buffer) {
  // Check seek / time limits
  int64 timestamp_to_send = static_cast<int64>(stream_offset_ms_);
  // use 'CORE' AAC samplerate - which is half of the actual samplerate
  const double crt_time_len = (frame_info_.samples * 500.0 /
                               frame_info_.samplerate);
  *tag = new AacFrameTag(Tag::ATTR_AUDIO
                         | Tag::ATTR_CAN_RESYNC
                         | Tag::ATTR_DROPPABLE,
                         kDefaultFlavourMask,
                         timestamp_to_send,
                         crt_time_len,
                         frame_info_.samplerate,
                         frame_info_.channels,
                         frame_info_.samples,
                         buffer,
                         frame_info_.bytesconsumed);

  stream_offset_ms_ += crt_time_len;
  return streaming::READ_OK;
}

streaming::TagReadStatus AacTagSplitter::GetNextTagInternal(
    io::MemoryStream* in, scoped_ref<Tag>* tag, bool is_at_eos) {
  if ( !header_extracted_ ) {
    return ReadHeader(in, tag);
  }
  *tag = NULL;

  while ( true ) {
    // assume NULL return + not enough data error
    void* samplebuffer = NULL;
    frame_info_.error = 13;
    // If you get a faad crash: "ifilter_bank () from /usr/lib/libfaad.so.2"
    // it's because faacDecDecode has a problem with decoding few data.
    // So: Make sure we have a significant amount of data available.
    if ( last_stream_.size() > 1000 ) {
      samplebuffer = faacDecDecode(
        aac_handle_, &frame_info_,
        const_cast<unsigned char*>(
          reinterpret_cast<const unsigned char*>(last_stream_.data())),
        last_stream_.size());
    }
    if ( samplebuffer == NULL ) {
      // Error Path..
      if ( frame_info_.error != 13 ) {
        // This is a bad error
        LOG_ERROR << name() << ": aac decode error '"
                  << faacDecGetErrorMessage(frame_info_.error) << "'";
        return streaming::READ_CORRUPTED_FAIL;
      }
      // Not enough data
      if ( in->IsEmpty() ) {
        return streaming::READ_NO_DATA;
      }
      const char* buffer = NULL;
      int size = 0;
      in->ReadNext(&buffer, &size);
      last_stream_.append(buffer, size);
      continue;
    }

    // Success - one frame decoded
    const streaming::TagReadStatus ret =
      MaybeFinalizeFrame(tag, last_stream_.data());
    last_stream_.erase(0, frame_info_.bytesconsumed);
    VLOG(10) << (*tag)->ToString();
    if ( ret == streaming::READ_SKIP ) {
      continue;
    }
    return ret;
  }
  LOG_FATAL << "Unreachable line";
  return streaming::READ_UNKNOWN;
}

string AacFrameTag::ToStringBody() const {
  return strutil::StringPrintf(
    " AACFRAME %d smp. [%d Hz, %d] len: %u",
    num_samples_, samplerate_, channels_,
    static_cast<unsigned int>(data_.Size()));
}
}
