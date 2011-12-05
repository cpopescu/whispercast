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

#ifndef __MEDIA_SOURCE_MEDIA_INFO_UTIL_H__
#define __MEDIA_SOURCE_MEDIA_INFO_UTIL_H__

#include <string>

#include <whisperlib/common/base/types.h>
#include <whisperstreamlib/base/media_info.h>
#include <whisperstreamlib/f4v/atoms/movie/moov_atom.h>
#include <whisperstreamlib/flv/flv_tag.h>
#include <whisperstreamlib/mp3/mp3_frame.h>

namespace streaming {
namespace util {

// Extracts MediaInfo from a f4v MOOV atom.
bool ExtractMediaInfoFromMoov(const streaming::f4v::MoovAtom& moov,
    MediaInfo* out);

// Extract MediaInfo from 3 representative FLV tags: metadata,
// first audio and first video sample. 'audio' or 'video' may be missing.
bool ExtractMediaInfoFromFlv(const FlvTag& metadata, const FlvTag* audio,
    const FlvTag* video, MediaInfo* out);

// Extract MediaInfo from an MP3 tag.
// Nothing configurable for now..
bool ExtractMediaInfoFromMp3(const Mp3FrameTag& mp3_tag, MediaInfo* out);

// This is a blocking method for opening & reading a file to extract MediaInfo.
// Uses the utility methods above according to file type.
// f4v file: it reads just until the MOOV atom
// flv file: it reads just until Metadata + first Audio + first Video tag
// other file: may not be supported.
// Returns success.
bool ExtractMediaInfoFromFile(const string& filename, MediaInfo* out);

} // util
} // streaming

#endif // __MEDIA_SOURCE_MEDIA_INFO_H__
