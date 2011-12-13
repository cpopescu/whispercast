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

#ifndef __COMMON_BASE_LOG_H__
#define __COMMON_BASE_LOG_H__

#include <glog/logging.h>
#include <whisperlib/common/base/errno.h>

#include <iomanip>

// To be used in context of VLOG(..)
static const int LDEBUG    = 4;
static const int LINFO     = 3;
static const int LWARNING  = 2;
static const int LERROR    = 1;
static const int LFATAL    = 0;


#ifndef PATH_SEPARATOR
#define PATH_SEPARATOR  '/'
#endif

#ifdef _DEBUG
#define LOG_DEBUG   LOG(INFO)
#else
#define LOG_DEBUG   if ( false ) LOG(INFO)
#endif

#define LOG_INFO    LOG(INFO)
#define LOG_WARNING LOG(WARNING)
#define LOG_ERROR   LOG(ERROR)
#define LOG_FATAL   LOG(FATAL)

#define LOG_FATAL_IF(cond) if ( cond ) LOG_FATAL
#define LOG_ERROR_IF(cond) if ( cond ) LOG_ERROR
#define LOG_INFO_IF(cond) if ( cond ) LOG_INFO

// Log the given value in hexadecimal with 0x prefix
#define LHEX(v) showbase << hex << (v) << dec << noshowbase
#define LTIMESTAMP(v) right << setw(8) << (v) << setw(0) << left

#define CHECK_NOT_NULL(p)                                               \
  if ( (p) != NULL );                                                   \
  else                                                                  \
    LOG_FATAL << " Pointer is NULL. "
#define CHECK_NULL(p)                                                   \
  if ( (p) == NULL );                                                   \
  else                                                                  \
    LOG_FATAL << " Pointer " << #p << "(" << LHEX(p) << ") is not NULL. "

#define CHECK_SYS_FUN(a, b)                                             \
  do { const int __tmp_res__ = (a);                                     \
    if ( __tmp_res__ != (b) ) {                                         \
      LOG_FATAL << #a << " Failed w/ result: " << __tmp_res__           \
                << "(" << GetSystemErrorDescription(__tmp_res__) << ")" \
                << " expected: " << #b;                                 \
    }                                                                   \
  } while ( false )


//////////////////////////////////////////////////////////////////////

// What we have below is a compile time check helper
// that will trigger a compile error if the given constant
// expression evaluates to false.
template <bool>
struct CompileAssert {
};

#undef  COMPILE_ASSERT
#define COMPILE_ASSERT(expr, msg)                  \
  typedef CompileAssert<(static_cast<bool>(expr))> \
  msg[static_cast<bool>(expr) ? 1 : -1]

//////////////////////////////////////////////////////////////////////

#ifdef _DEBUG

#define DLOG_DEBUG   LOG(INFO)
#define DLOG_INFO    LOG(INFO)
#define DLOG_WARNING LOG(WARNING)
#define DLOG_ERROR   LOG(ERROR)
#define DLOG_FATAL   LOG(FATAL)

#define DLOG_FATAL_IF(cond) if ( cond ) LOG_FATAL
#define DLOG_INFO_IF(cond) if ( cond ) LOG_INFO

#else

#define DLOG_DEBUG   if ( false ) LOG(INFO)
#define DLOG_INFO    if ( false ) LOG(INFO)
#define DLOG_WARNING if ( false ) LOG(WARNING)
#define DLOG_ERROR   if ( false ) LOG(ERROR)
#define DLOG_FATAL   if ( false ) LOG(FATAL)

#define DLOG_FATAL_IF(cond) if ( false ) LOG(FATAL)
#define DLOG_INFO_IF(cond) if ( false ) LOG(INFO)
#endif

#endif    // __COMMON_BASE_LOG_H__
