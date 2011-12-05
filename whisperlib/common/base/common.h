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

#ifndef __COMMON_BASE_COMMON_H__
#define __COMMON_BASE_COMMON_H__

#include <time.h>
#include <unistd.h>

#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_RELEASE 16
#define VERSION_TEXT "1.0.16"

#include <whisperlib/common/base/types.h>

#define NOTHING

#define UNUSED_ALWAYS(param) ((void)param)

#if defined(__GNUC__) && defined(__GNUC_MINOR__)
  #define HAVE_GCC(major, minor) \
    ((__GNUC__ > (major)) || \
     (__GNUC__ == (major) && __GNUC_MINOR__ >= (minor)))
#else
  #define HAVE_GCC(major, minor) 0
#endif

//
// Functions using this attribute cause a warning if the variable
// argument list does not contain a NULL pointer.
//
#ifndef G_GNUC_NULL_TERMINATED
  #if HAVE_GCC(4, 0)
    #define G_GNUC_NULL_TERMINATED __attribute__((__sentinel__))
  #else   // GCC < 4
    #define G_GNUC_NULL_TERMINATED
  #endif  // GCC >= 4
#endif    // G_GNUC_NULL_TERMINATED

#endif  // __COMMON_BASE_COMMON_H__
