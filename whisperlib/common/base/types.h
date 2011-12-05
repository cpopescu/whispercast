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

#ifndef __COMMON_BASE_TYPES_H__
#define __COMMON_BASE_TYPES_H__

// for PRId64 , PRIu64 , ...
#include <inttypes.h>

#include <sys/types.h>
#include <stddef.h>
#include <string>

//////////////////////////////////////////////////////////////////////

typedef signed char int8;
typedef unsigned char uint8;

typedef signed short int16;
typedef unsigned short uint16;

typedef int32_t int32;
typedef u_int32_t uint32;

typedef int64_t int64;
typedef u_int64_t uint64;

//////////////////////////////////////////////////////////////////////

static const int8  kMaxInt8  = (static_cast<int8>(0x7f));
static const int8  kMinInt8  = (static_cast<int8>(0x80));
static const int16 kMaxInt16 = (static_cast<int16>(0x7fff));
static const int16 kMinInt16 = (static_cast<int16>(0x8000));
static const int32 kMaxInt32 = (static_cast<int32>(0x7fffffff));
static const int32 kMinInt32 = (static_cast<int32>(0x80000000));
static const int64 kMaxInt64 = (static_cast<int64>(0x7fffffffffffffffLL));
static const int64 kMinInt64 = (static_cast<int64>(0x8000000000000000LL));

static const uint8  kMaxUInt8  = (static_cast<uint8>(0xff));
static const uint16 kMaxUInt16 = (static_cast<uint16>(0xffff));
static const uint32 kMaxUInt32 = (static_cast<uint32>(0xffffffff));
static const uint64 kMaxUInt64 = (static_cast<uint64>(0xffffffffffffffffLL));

//////////////////////////////////////////////////////////////////////


// None of this trouble for now ..
// The current character type.
// #ifdef _UNICODE
// typedef wchar_t char_t;
// typedef wstring tstring;
// #else
// typedef char char_t;
// typedef string tstring;
// #endif // _UNICODE

//////////////////////////////////////////////////////////////////////

#define CONSIDER(cond) case cond: return #cond;
#define NUMBEROF(things)                                            \
    ((sizeof(things) / sizeof(*(things))) /                         \
    static_cast<size_t>(!(sizeof(things) % sizeof(*(things)))))

#define INVALID_FD_VALUE (-1)

// A macro to disallow the evil copy constructor and operator= functions
// This should be used in the private: declarations for a class
#define DISALLOW_EVIL_CONSTRUCTORS(TypeName)    \
  TypeName(const TypeName&);                    \
  void operator=(const TypeName&)

//////////////////////////////////////////////////////////////////////

// Hash helper functions for where is undefined

#if defined(__GLIBCXX__) || (defined(__GLIBCPP__) && __GLIBCPP__ >= 20020514)

// we accept 'std' and '__gnu_cxx' namespaces without full specification
// to reduce the clutter
namespace std { }
namespace  __gnu_cxx { }

using namespace std;
using namespace __gnu_cxx;

# if !defined(__GNUC__) || __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 3)
#   define WHISPER_HAS_HASH_FUN    1
#   define WHISPER_HASH_FUN_HEADER <ext/hash_fun.h>

#   define WHISPER_HASH_SET_HEADER <ext/hash_set>
#   define WHISPER_HASH_MAP_HEADER <ext/hash_map>

# else
#   define WHISPER_HAS_FUN         0
#   define WHISPER_HASH_FUN_HEADER <backward/hash_fun>

#   define WHISPER_HASH_SET_HEADER <tr1/unordered_set>
#   define WHISPER_HASH_MAP_HEADER <tr1/unordered_map>

#   define hash_map tr1::unordered_map
#   define hash_set tr1::unordered_set
# endif

#else
# error "Please use at leaset  GCC >= 3.1.0"
#endif

#if WHISPER_HAS_HASH_FUN

#include WHISPER_HASH_FUN_HEADER

namespace __gnu_cxx {
#if __WORDSIZE != 64 || defined(__APPLE__)
  template<> struct hash<int64> {
    size_t operator()(const int64 in) const {
      const int64 ret = (in >> 32L) ^ (in & 0xFFFFFFFF);
      return static_cast<size_t>(ret);
    }
  };
  template<> struct hash<uint64> {
    size_t operator()(const uint64 in) const {
      const int64 ret = (in >> 32L) ^ (in & 0xFFFFFFFF);
      return static_cast<size_t>(ret);
    }
  };
#endif
  template<typename T> struct hash<T*> {
    size_t operator()(const T* p) const {
      return (size_t) p;
    }
  };
  template<> struct hash<string> {
    size_t operator()(const string& s) const {
      uint32 a = 63689;
      uint32 b = 378551;
      const char* p = s.data();
      int len =  s.size();
      uint32 h = 0;
      while ( len-- ) {
        h = h * a + *p++;
        a *= b;
      }
      return h;
    }
  };
}

#ifdef __APPLE__
#define O_LARGEFILE 0
#define lseek64 lseek
#define fdatasync fsync
#endif

#endif  // WHISPER_HAS_FUN

//////////////////////////////////////////////////////////////////////


#endif   // __COMMON_BASE_TYPES_H__
