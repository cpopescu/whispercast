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
#include <whisperstreamlib/f4v/atoms/movie/stsz_atom.h>

namespace streaming {
namespace f4v {
namespace util {

// Go through MOOV atom and gather audio or video frame headers.
void ExtractFrames(const MoovAtom& moov, bool audio,
    vector<FrameHeader*>* out);

// Extract audio&video information from 'moov' atom, output into MediaInfo.
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
StcoAtom* GetStcoAtom(MoovAtom& moov, bool audio);
// returns a pointer to the StszAtom inside 'moov'.
// Don't delete the pointer! Returns NULL if not found.
StszAtom* GetStszAtom(MoovAtom& moov, bool audio);
const StszAtom* GetStszAtom(const MoovAtom& moov, bool audio);

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
