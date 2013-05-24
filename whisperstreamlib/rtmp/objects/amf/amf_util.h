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
// Author: Cosmin Tudorache & Catalin Popescu

#ifndef __NET_RTMP_OBJECTS_AMF_AMF_UTIL_H__
#define __NET_RTMP_OBJECTS_AMF_AMF_UTIL_H__

namespace rtmp {

//
// This static class encasulates a bunch of static defines
//
class AmfUtil {
 public:
  enum Version {
    AMF0_VERSION = 0,        // Supported
    AMF3_VERSION = 3         // ** not supported anywhere (yet) **
  };
  // Error codes used throughout the reading process of rtmp things
  enum ReadStatus {
    READ_OK              = 0,    // everything went OK
    READ_NO_DATA         = 1,    // not enough data available
    READ_CORRUPTED_DATA  = 2,    // some corruption in the input stream
    READ_NOT_IMPLEMENTED = 3,    // we found an object that we don't know how
                                 // to read
    READ_UNSUPPORTED_REFERENCES = 4,  // we don't support references, though
                                      // we pass through them
    READ_STRUCT_TOO_LONG = 5,    // We found a string (or array) that is
                                 // simply too long
    READ_OOM = 6,                // We were required to use too much memory
                                 // overrun our internal buffer limits
    READ_TOO_MANY_CHANNELS = 7,  // Client created too many channels - bad
  };
  static const char* ReadStatusName(ReadStatus stat);

  // TODO(cpopescu): shall we make these flags ?

  // From all strings we read at most 10MB..
  static const int kMaximumStringReadableData = 10 << 20;
  // We limit arrays and maps of at most this size
  static const int kMaximumArraySize = 10000;
};
}

#endif  // __NET_RTMP_OBJECTS_AMF_AMF_UTIL_H__
