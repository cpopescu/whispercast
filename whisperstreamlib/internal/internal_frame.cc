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
// Author: Mihai Ianculescu

#include <whisperlib/common/base/log.h>
#include <whisperlib/common/io/buffer/memory_stream.h>
#include <whisperlib/common/io/num_streaming.h>
#include "internal/internal_frame.h"

namespace streaming {

const Tag::Type InternalFrameTag::kType = Tag::TYPE_INTERNAL;

string InternalFrameTag::ToStringBody() const {
  return strutil::StringPrintf(
    " INTERNAL FRAME {%c%c%c%c%c%c%c%c - %08d, %08d, %s}",
    header_.flags_ & streaming::HEADER ? 'H' : '-',
    header_.flags_ & streaming::AUDIO ? 'A' : '-',
    header_.flags_ & streaming::VIDEO ? 'V' : '-',
    header_.flags_ & streaming::METADATA ? 'M' : '-',
    header_.flags_ & streaming::DELTA ? 'D' : '-',
    header_.flags_ & streaming::DISCONTINUITY ? 'X' : '-',
    header_.flags_ & streaming::HAS_TIMESTAMP ? 'T' : '-',
    header_.flags_ & streaming::HAS_MIME_TYPE ? 'M' : '-',
    header_.length_, header_.timestamp_, mime_type_.c_str());
}

streaming::TagReadStatus InternalFrameTag::Read(io::MemoryStream* in) {
  State state = kHeaderFlags;
  uint8 mime_type_length = 0;

  while (true) {
    switch (state) {
      case kHeaderFlags: {
        // we need the first byte - FLAGS
        if (in->Size() < 1) {
          return streaming::READ_NO_DATA;
        }

        // get the flags
        in->MarkerSet();
        CHECK_EQ(in->Read(&header_.flags_, 1), 1);
        // move on
        state = kHeaderLength;
        break;
      }
      case kHeaderLength: {
        // we need at least the LENGTH
        if (in->Size() < sizeof(streaming::ProtocolFrameLength)) {
          in->MarkerRestore();
          return streaming::READ_NO_DATA;
        }

        header_.length_ = io::NumStreamer::ReadInt32(
            in, streaming::kProtocolByteOrder);
        // consider the flags and move on
        if ((header_.flags_ & streaming::HAS_TIMESTAMP) != 0) {
          state = kHeaderTimeStamp;
        } else if ((header_.flags_ & streaming::HAS_MIME_TYPE) != 0) {
          state = kHeaderMimeType;
        } else {
          state = kData;
        }
        break;
      }
      case kHeaderTimeStamp: {
        // we need at least the TIMESTAMP
        if (in->Size() < sizeof(streaming::ProtocolFrameTimeStamp)) {
          in->MarkerRestore();
          return streaming::READ_NO_DATA;
        }

        header_.timestamp_ = io::NumStreamer::ReadInt32(
            in, streaming::kProtocolByteOrder);
        // consider the flags and move on
        if ((header_.flags_ & streaming::HAS_MIME_TYPE) != 0) {
          state = kHeaderMimeType;
        } else {
          state = kData;
        }
        break;
      }
      case kHeaderMimeType: {
        // we need at least the MIME_TYPE length
        if (in->Size() < sizeof(uint8)) {
          in->MarkerRestore();
          return streaming::READ_NO_DATA;
        }

        mime_type_length = io::NumStreamer::ReadByte(in);
        // move on
        state = kHeaderMimeTypeData;
        break;
      }
      case kHeaderMimeTypeData: {
        // TODO(cpopescu) : clarify this w/ Mihai !

        // we need at least the MIME_TYPE length
        if (in->Size() < mime_type_length) {
          in->MarkerRestore();
          return streaming::READ_NO_DATA;
        }
        const int32 cb = in->ReadString(&mime_type_, mime_type_length);
        CHECK_EQ(mime_type_length, cb);
        // move on
        state = kData;
        break;
      }
      case kData: {
        // we need the entire frame
        if (in->Size() < header_.length_) {
          in->MarkerRestore();
          return streaming::READ_NO_DATA;
        }

        CHECK_EQ(in->ReadString(&data_, header_.length_), header_.length_);
        DLOG(10) << "Extracted an internal header: " << ToString();

        in->MarkerClear();
        return streaming::READ_OK;
      }
    }
  }
  LOG_FATAL << "Illegal state: " << state;
  return streaming::READ_UNKNOWN;
}
}
