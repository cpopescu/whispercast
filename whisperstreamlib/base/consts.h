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

// This contains the definitions for general constants and types
// used throughout the media subsystem.

#ifndef __MEDIA_BASE_CONSTS_H__
#define __MEDIA_BASE_CONSTS_H__

#include <string>
#include <whisperlib/net/base/address.h>
#include <whisperlib/common/base/system.h>

namespace streaming {

// When we read a tag we can return the following codes:
//   READ_OK - we read one tag and processed it (may decode one again from
//             the element). VALID tag returned.
//   READ_NO_DATA - tag unreadable because not enough data available.
//                  NULL tag returned.
//   READ_SKIP - we got an ok tag, but we decided to skip. NULL tag returned.
//   READ_CORRUPTED - the in input is corrupted and we cannot recover
//         from the error. NULL tag returned.
//   READ_EOF - basically we reached the end of the file (or what we
//         were supposed to read). NULL tag returned.
enum TagReadStatus {
  READ_NO_DATA,
  READ_OK,
  READ_SKIP,
  READ_CORRUPTED,
  READ_EOF,
};
const char* TagReadStatusName(TagReadStatus status);

//////////

// TagSplitters and TagSerializers are associated with these types.
enum MediaFormat {
  MFORMAT_F4V,      // F4V file (or MP4 file)
  MFORMAT_FLV,      // FLV file
  MFORMAT_MP3,      // MP3 file
  MFORMAT_AAC,      // AAC file
  MFORMAT_MTS,      // MPEG TS, file
  MFORMAT_RAW,      // raw data, no structure
  MFORMAT_INTERNAL, // TODO(cosmin): remove this format if possible
};
// This is not a valid MediaFormat. Used in Requests that
// advertise they accept any incoming format.
extern const MediaFormat kAnyMediaFormat;
const char* MediaFormatName(MediaFormat format);

// file extension correspondence
const char* MediaFormatToExtension(MediaFormat format);
bool MediaFormatFromExtension(const string& ext, MediaFormat* out_format);

// http content type correspondence
const char* MediaFormatToContentType(MediaFormat format);
bool MediaFormatFromContentType(const string& content_type, MediaFormat* out_format);

// small type correspondence
const char* MediaFormatToSmallType(MediaFormat format);
bool MediaFormatFromSmallType(const string& small_type, MediaFormat* out_format);

// utility functions: uses a combination of the above conversion
bool ExtensionToContentType(const string& ext, string* content_type);

//////////

// Here we have the most simple protocol used
// internally to wrap frames from the encoder, in order
// to allow the downstream servers to split the media
// into tags without any knowledge of the actual media structure.
enum ProtocolFlags {
  HEADER         = 1 << 0,

  AUDIO          = 1 << 1,
  VIDEO          = 1 << 2,
  METADATA       = 1 << 3,

  DELTA          = 1 << 4,
  DISCONTINUITY  = 1 << 5,

  HAS_TIMESTAMP  = 1 << 6,
  HAS_MIME_TYPE  = 1 << 7,
};

// The frame flags.
typedef uint8 ProtocolFrameFlags;
// The frame length.
typedef int32 ProtocolFrameLength;
// The frame timestamp, in miliseconds.
typedef int32 ProtocolFrameTimeStamp;

// The byte order used by the protocol.
const common::ByteOrder kProtocolByteOrder = common::LILENDIAN;

// The frame header, as a structure.
#pragma pack(push, 0)
struct ProtocolFrameHeader {
  ProtocolFrameFlags flags_;
  ProtocolFrameLength length_;
  ProtocolFrameTimeStamp timestamp_;
  ProtocolFrameHeader() : flags_(0), length_(0), timestamp_(0) {}
  ProtocolFrameHeader(const ProtocolFrameHeader& other)
    : flags_(other.flags_),
      length_(other.length_),
      timestamp_(other.timestamp_) {}
};
#pragma pack(pop)

// The mime-type of the media encoded by our internal protocol.
static const char kInternalMimeType[]  = "media/x-whisper-internal";
// The HTTP header name of the actually encoded mime type.
static const char kInternalHttpHeaderName[]  = "X-Whisper-Content-Type";

/////////////////////////////////////////////////////////////////////

static const uint32 kAllFlavours = 0xFFFFFFFF;
static const uint32 kDefaultFlavourId = 0;
static const uint32 kDefaultFlavourMask = (1 << kDefaultFlavourId);
static const uint32 kNumFlavours = 32;

// Flavour manipulation function
// Basically this returns the id of the bit set.. (-1 if no bit or more bits
// are set).
// e.g:
//   FlavourId(1) == 0
//   FlavourId(128) == 7
//   FlavourId(0) == -1
//   FlavourId(3) == -1
//
int FlavourId(uint32 mask);
// The flavourId of the rightmost set bit and updates mask to unset that bit
// You can use in a construction:

// while ( flavour_mask ) {
//   const int flavour_id = RightmostFlavourId(&flavour_mask);
//   * do something w/ flavour_id
// }
inline int RightmostFlavourId(uint32& mask) {
  const uint32 new_mask = mask & (mask - 1);
  const int ret = FlavourId(mask & ~new_mask);
  mask &= new_mask;
  return ret;
}

//////////////////////////////////////////////////////////////////////

}
#endif  // __MEDIA_BASE_CONSTS_H__
