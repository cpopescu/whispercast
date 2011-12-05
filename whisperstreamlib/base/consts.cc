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
    CONSIDER(READ_CORRUPTED_CONT);
    CONSIDER(READ_CORRUPTED_FAIL);
    CONSIDER(READ_OVERSIZED_TAG);
    CONSIDER(READ_UNKNOWN);
    CONSIDER(READ_EOF);
  }
  LOG_FATAL << "Illegal TagReadStatus: " << status;
  return "==UNKNOWN==";
};

///////

bool GetStreamType(const string& str, Tag::Type* out) {
  if ( str == "flv" )      { *out = Tag::TYPE_FLV; return true; }
  if ( str == "mp3" )      { *out = Tag::TYPE_MP3; return true; }
  if ( str == "aac" )      { *out = Tag::TYPE_AAC; return true; }
  if ( str == "internal" ) { *out = Tag::TYPE_INTERNAL; return true; }
  if ( str == "f4v" )      { *out = Tag::TYPE_F4V; return true; }
  if ( str == "raw" )      { *out = Tag::TYPE_RAW; return true; }
  return false;
}

Tag::Type GetStreamTypeFromContentType(const string& content_type) {
  if ( content_type == "video/x-flv" ) {
    return Tag::TYPE_FLV;
  }
  if ( content_type == "audio/mpeg" ) {
    return Tag::TYPE_MP3;
  }
  if ( content_type == "audio/aac" ||
       content_type == "audio/aacp" ) {
    return Tag::TYPE_AAC;
  }
  if ( content_type == kInternalMimeType ||
       content_type == "media/x-emoticon-internal" ) {
    return Tag::TYPE_INTERNAL;
  }
  if ( content_type == "video/x-f4v" ) {
    return Tag::TYPE_F4V;
  }
  return Tag::TYPE_RAW;
}

const char* GetContentTypeFromStreamType(Tag::Type tag_type) {
  switch ( tag_type ) {
    case Tag::TYPE_FLV:       return "video/x-flv";
    case Tag::TYPE_MP3:       return "audio/mpeg";
    case Tag::TYPE_AAC:       return "audio/aac";
    case Tag::TYPE_INTERNAL:  return kInternalMimeType;
    case Tag::TYPE_F4V:       return "video/x-f4v";
    default:                  return "application/octet-stream";
  }
}
const char* GetSmallTypeFromStreamType(Tag::Type tag_type) {
  switch ( tag_type ) {
    case Tag::TYPE_FLV:        return "flv";
    case Tag::TYPE_MP3:        return "mp3";
    case Tag::TYPE_AAC:        return "aac";
    case Tag::TYPE_INTERNAL:   return "internal";
    case Tag::TYPE_F4V:        return "f4v";
    case Tag::TYPE_RAW:        return "raw";
    default:                   return "error";
  }
}

const char* GetSmallTypeFromContentType(const string& content_type) {
  if ( content_type == "video/x-flv" ) {
    return "flv";
  }
  if ( content_type == "audio/mpeg" ) {
    return "mp3";
  }
  if ( content_type == "audio/aac" ||
       content_type == "audio/aacp" ) {
    return "aac";
  }
  if ( content_type == kInternalMimeType ) {
    return "internal";
  }
  if ( content_type == "video/x-f4v" ) {
    return "f4v";
  }
  return content_type.c_str();
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
