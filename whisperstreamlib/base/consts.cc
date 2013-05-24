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

#include "base/consts.h"

namespace streaming {

const char* TagReadStatusName(TagReadStatus status) {
  switch ( status ) {
    CONSIDER(READ_OK);
    CONSIDER(READ_NO_DATA);
    CONSIDER(READ_SKIP);
    CONSIDER(READ_CORRUPTED);
    CONSIDER(READ_EOF);
  }
  LOG_FATAL << "Illegal TagReadStatus: " << status;
  return "==UNKNOWN==";
};

///////

const MediaFormat kAnyMediaFormat = (MediaFormat)-2;

const char* MediaFormatName(MediaFormat format) {
  switch ( format ) {
    CONSIDER(MFORMAT_F4V);
    CONSIDER(MFORMAT_FLV);
    CONSIDER(MFORMAT_MP3);
    CONSIDER(MFORMAT_AAC);
    CONSIDER(MFORMAT_MTS);
    CONSIDER(MFORMAT_RAW);
    CONSIDER(MFORMAT_INTERNAL);
  }
  if ( format == kAnyMediaFormat ) {
    return "MFORMAT_ANY";
  }
  LOG_FATAL << "Illegal MediaFormat: " << (int)format;
  return "unknown";
}
const char* MediaFormatToExtension(MediaFormat format) {
  switch ( format ) {
    case MFORMAT_F4V: return "f4v";
    case MFORMAT_FLV: return "flv";
    case MFORMAT_MP3: return "mp3";
    case MFORMAT_AAC: return "aac";
    case MFORMAT_MTS: return "ts";
    case MFORMAT_RAW: return "raw";
    case MFORMAT_INTERNAL: return "internal";
  }
  LOG_FATAL << "Illegal MediaFormat: " << (int)format;
  return "unknown";
}
bool MediaFormatFromExtension(const string& ext2, MediaFormat* out_format) {
  const string ext = strutil::StrToLower(ext2);
  if ( ext == "flv" ) { *out_format = MFORMAT_FLV; return true; }
  if ( ext == "f4v" || ext == "mp4" || ext == "mp4a" || ext == "m4v" ||
       ext == "mov" ) { *out_format = MFORMAT_F4V; return true; }
  if ( ext == "mp3" ) { *out_format = MFORMAT_MP3; return true; }
  if ( ext == "aac" ) { *out_format = MFORMAT_AAC; return true; }
  if ( ext == "ts" ) { *out_format = MFORMAT_MTS; return true; }
  return false;
}

const char* MediaFormatToContentType(MediaFormat format) {
  switch ( format ) {
    case MFORMAT_FLV:      return "video/x-flv";
    case MFORMAT_F4V:      return "video/x-f4v";
    case MFORMAT_MP3:      return "audio/mpeg";
    case MFORMAT_AAC:      return "audio/aac";
    case MFORMAT_MTS:      return "video/mp2t"; // Or: "video/x-mpegts" ?
    case MFORMAT_RAW:      return "application/octet-stream";
    case MFORMAT_INTERNAL: return kInternalMimeType;
  }
  LOG_FATAL << "Illegal MediaFormat: " << (int)format;
  return "unknown";
}
bool MediaFormatFromContentType(const string& content_type,
    MediaFormat* out_format) {
  if ( content_type == "video/x-flv" ) {
    *out_format = MFORMAT_FLV;
    return true;
  }
  if ( content_type == "video/x-f4v" ||
       content_type == "video/x-m4v" ) {
    *out_format = MFORMAT_F4V;
    return true;
  }
  if ( content_type == "audio/mpeg" ) {
    *out_format = MFORMAT_MP3;
    return true;
  }
  if ( content_type == "audio/aac" ||
       content_type == "audio/aacp" ) {
    *out_format = MFORMAT_AAC;
    return true;
  }
  if ( content_type == "video/mp2t" ||
       content_type == "video/x-mpegts" ) {
    *out_format = MFORMAT_MTS;
    return true;
  }
  if ( content_type == kInternalMimeType ||
       content_type == "media/x-emoticon-internal" ) {
    *out_format = MFORMAT_INTERNAL;
    return true;
  }
  LOG_ERROR << "Unknown content type: [" << content_type << "]";
  return false;
}

const char* MediaFormatToSmallType(MediaFormat format) {
  switch ( format ) {
    case MFORMAT_F4V: return "f4v";
    case MFORMAT_FLV: return "flv";
    case MFORMAT_MP3: return "mp3";
    case MFORMAT_AAC: return "aac";
    case MFORMAT_MTS: return "mts";
    case MFORMAT_RAW: return "raw";
    case MFORMAT_INTERNAL: return "internal";
  }
  LOG_FATAL << "Illegal MediaFormat: " << (int)format;
  return "unknown";
}
bool MediaFormatFromSmallType(const string& small_type, MediaFormat* out_format) {
  if ( small_type == "flv" ) { *out_format = MFORMAT_FLV; return true; }
  if ( small_type == "f4v" ) { *out_format = MFORMAT_F4V; return true; }
  if ( small_type == "mp3" ) { *out_format = MFORMAT_MP3; return true; }
  if ( small_type == "aac" ) { *out_format = MFORMAT_AAC; return true; }
  return false;
}

//////////////////////////////////////////////////////////////////////

bool ExtensionToContentType(const string& ext, string* content_type) {
  MediaFormat format;
  if ( !MediaFormatFromExtension(ext, &format) ) {
    return false;
  }
  *content_type = MediaFormatToContentType(format);
  return true;
}

//////////////////////////////////////////////////////////////////////

static const int kFlavourIds0[] = {
   0,  0,  1,  0,  2,  0,  0,  0,  3,  0,  0,  0,  0,  0,  0,  0,
   4,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   5,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   6,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   7,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};
static const int kFlavourIds1[] = {
   0,  8,  9,  0, 10,  0,  0,  0, 11,  0,  0,  0,  0,  0,  0,  0,
  12,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  13,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  14,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  15,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};
static const int kFlavourIds2[] = {
   0, 16, 17,  0, 18,  0,  0,  0, 19,  0,  0,  0,  0,  0,  0,  0,
  20,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  21,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  22,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  23,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};
static const int kFlavourIds3[] = {
   0, 24, 25,  0, 26,  0,  0,  0, 27,  0,  0,  0,  0,  0,  0,  0,
  28,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  29,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  30,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
  31,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,
   0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0
};
int FlavourId(uint32 mask) {
  if ( !mask || (mask & (mask - 1) ) ) {
    return -1;
  }
  return (kFlavourIds0[(mask      ) &  0xff] +
          kFlavourIds1[(mask >>  8) &  0xff] +
          kFlavourIds2[(mask >> 16) &  0xff] +
          kFlavourIds3[(mask >> 24) &  0xff]);
}

}
