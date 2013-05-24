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

#ifndef __MEDIA_F4V_F4V_TYPES_H__
#define __MEDIA_F4V_F4V_TYPES_H__

#include <string>
#include <whisperlib/common/base/date.h>

namespace streaming {
namespace f4v {

// 32-bit unsigned fixed point-number (16.16).
struct FPU16U16 {
  uint16 integer_;
  uint16 fraction_;
  FPU16U16(uint16 integer)
      : integer_(integer), fraction_(0) {}
  FPU16U16(uint16 integer, uint16 fraction)
      : integer_(integer), fraction_(fraction) {};
  bool operator==(const FPU16U16& other) const {
    return integer_ == other.integer_ &&
           fraction_ == other.fraction_;
  }
};

enum TagDecodeStatus {
  TAG_DECODE_SUCCESS, // tag read successfully
  TAG_DECODE_NO_DATA, // not enough data to read tag
  TAG_DECODE_ERROR,   // bad data in stream. You should abort reading tags.
};
const char* TagDecodeStatusName(TagDecodeStatus status);


// useful for Mac time <=> Unix time translation
extern const int64 kSecondsFrom1904to1970;

// time: seconds since January 1, 1904 UTC
timer::Date MacDate(int64 time);

// receives a 3 letter language abbreviation,
// returns the MAC code.
// e.g. "eng" => 5575
uint16 MacLanguageCode(const string& lang);
// receives a MAC language code,
// returns the 3 letter language abbreviation.
string MacLanguageName(uint16 code);

} // namespace f4v
} // namespace streaming

#endif // __MEDIA_F4V_F4V_TYPES_H__
