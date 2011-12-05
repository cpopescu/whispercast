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
// Author: Mihai Ianculescu

//
// We have here a bunch of utility classes for parsing stuff from strings.
//

#ifndef __COMMON_BASE_PARSE_H__
#define __COMMON_BASE_PARSE_H__

#include <string>
#include <whisperlib/common/base/strutil.h>
#include <whisperlib/common/base/log.h>

// our namespace
namespace parsable {

class Parsable {
 public:
  Parsable() : set_(false) {}
  virtual ~Parsable() {}

  bool IsSet() const { return set_; }
  void Reset() { set_ = false; }

  virtual bool Parse(const char* in,
                     const char** next,
                     char terminator = '\0') = 0;
  virtual string ToString() const = 0;

 protected:
  bool set_;
};

template<typename TTraits> class TParsable : public Parsable {
 public:
  TParsable() {
  }
  explicit TParsable(const TParsable<TTraits>& other) {
    value_ = other.value_;
    set_ = other.set_;
  }
  explicit TParsable(typename TTraits::RType value) {
    TTraits::Assign(value_, value);
    set_ = true;
  }

  operator const typename TTraits::LType& () const {
    return value_;
  }
  const typename TTraits::LType& Value() const {
    return value_;
  }

  const typename TTraits::LType&
  operator=(const typename TTraits::LType& value) {
    value_ = value;
    set_ = true;
    return value_;
  }

  bool operator ==(const TParsable<TTraits>& other) const {
    if (IsSet()) {
      if (other.IsSet()) {
        return Value() == other.Value();
      }
      return false;
    }
    return !other.IsSet();
  }
  // a not set parsable is never equal to a specific value
  bool operator ==(typename TTraits::RType other) const {
    if (IsSet()) {
      return Value() == other;
    }
    return false;
  }
  bool operator <(const TParsable<TTraits>& other) const {
    if (IsSet()) {
      if (other.IsSet()) {
        return Value() < other.Value();
      }
      return false;
    }
    return other.IsSet();
  }
  // a not set parsable is always less than a specific value
  bool operator <(typename TTraits::RType other) const {
    if (IsSet()) {
      return Value() < other;
    }
    return true;
  }

  virtual bool Parse(const char* in,
                     const char** next,
                     char terminator = '\0') {
    if (TTraits::Parse(value_, in, next, terminator)) {
      set_ = true;
      return true;
    }
    return false;
  }

  virtual string ToString() const {
    return TTraits::ToString(value_);
  }

 private:
  typename TTraits::LType value_;
};

template<typename T>
class TSimpleTraits {
 public:
  typedef T RType;
  typedef T LType;

  static void Assign(T& value, T from) {
    value = from;
  }
  static T Return(const T& value) {
    return value;
  }
};

// string
class TStringTraits {
 public:
  typedef const char* RType;
  typedef string LType;

  static void Assign(string& value, const char* from) {
    value = (from != NULL) ? from : "";
  }
  static const char* Return(const string& value) {
    return value.c_str();
  }
  static bool Parse(string& value,
                    const char* in,
                    const char** next,
                    char terminator) {
    CHECK_NOT_NULL(in);
    value = in;
    if (next != NULL) {
      *next = const_cast<char*>(in + value.size());
    }
    return true;
  }
  static string ToString(const string& value) {
    return value;
  }
};

typedef TParsable<TStringTraits> String;

// bool
class TBooleanTraits : public TSimpleTraits<bool> {
 public:
  static bool Parse(bool& value,
                    const char* in,
                    const char** next,
                    char terminator) {
    CHECK_NOT_NULL(in);

    char* _next = const_cast<char*>(in);
    value = true;
    if (*in != '\0') {
      if (strutil::StrCasePrefix(in, "true")) {
        value = true;
        _next += 4;
      } else if (strutil::StrCasePrefix(in, "false")) {
        value = false;
        _next += 5;
      } else if (strutil::StrCasePrefix(in, "on")) {
        value = true;
        _next += 2;
      } else if (strutil::StrCasePrefix(in, "off")) {
        value = false;
        _next += 3;
      } else if (strutil::StrCasePrefix(in, "yes")) {
        value = true;
        _next += 3;
      } else if (strutil::StrCasePrefix(in, "no")) {
        value = false;
        _next += 2;
      } else if (strutil::StrCasePrefix(in, "1")) {
        value = true;
        _next += 1;
      } else if (strutil::StrCasePrefix(in, "0")) {
        value = false;
        _next += 1;
      } else {
        return false;
      }

      if (*_next != terminator)
        return false;
    }

    if (next != NULL) {
      *next = _next;
    }
    return true;
  }
  static string ToString(const bool& value) {
    return value ? "1" : "0";
  }
};
typedef TParsable<TBooleanTraits> Boolean;

// generic integer type
template <typename TIntegerType>
class TIntegerTraits : public TSimpleTraits<TIntegerType> {
 public:
  static bool Parse(TIntegerType& value,
                    const char* in,
                    const char** next,
                    char terminator) {
    char* _next;
    value = DoParse(in, &_next);
    if ( _next != in ) {
      if ( *_next != terminator ) {
        return false;
      }
      if ( next != NULL ) {
        *next = _next;
      }
      return true;
    }
    return false;
  }
  static string ToString(const TIntegerType& value);

 protected:
  static TIntegerType DoParse(const char* in, char** next);
};

// int32
template<> inline
int32 TIntegerTraits<int32>::DoParse(const char* in, char** next) {
  return static_cast<int32>((strtol(in, next, 10)));
}
template<> inline
string TIntegerTraits<int32>::ToString(const int32& value) {
  return strutil::IntToString(value);
}
typedef TParsable<TIntegerTraits<int32> > Int32;

// uint32
template<> inline
uint32 TIntegerTraits<uint32>::DoParse(const char* in,
                                       char** next) {
  return static_cast<uint32>(strtoul(in, next, 10));
}
template<> inline
string TIntegerTraits<uint32>::ToString(const uint32& value) {
  return strutil::UintToString(value);
}
typedef TParsable<TIntegerTraits<uint32> > UInt32;

// int64
template<> inline
int64 TIntegerTraits<int64>::DoParse(const char* in, char** next) {
  return static_cast<int64>(strtoll(in, next, 10));
}
template<> inline
string TIntegerTraits<int64>::ToString(const int64& value) {
  return strutil::Int64ToString(value);
}
typedef TParsable<TIntegerTraits<int64> > Int64;

// uint64
template<> inline
uint64 TIntegerTraits<uint64>::DoParse(const char* in,
                                       char** next) {
  return static_cast<uint64>(strtoull(in, next, 10));
}
template<> inline
string TIntegerTraits<uint64>::ToString(const uint64& value) {
  return strutil::Uint64ToString(value);
}
typedef TParsable<TIntegerTraits<uint64> > UInt64;

}   // namespace parsable

#endif  // __COMMON_BASE_PARSE_H__
