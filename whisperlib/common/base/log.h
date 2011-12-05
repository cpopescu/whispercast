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

#include <string.h>
#include <sstream>
#include <string>
#include <whisperlib/common/base/types.h>
#include <whisperlib/common/base/errno.h>

static const int LDEBUG    = 4;
static const int LINFO     = 3;
static const int LWARNING  = 2;
static const int LERROR    = 1;
static const int LFATAL    = 0;


//////////////////////////////////////////////////////////////////////

namespace synch {
class Mutex;
}

#ifndef PATH_SEPARATOR
#define PATH_SEPARATOR  '/'
#endif

namespace log_internal {

class Log {
 public:
  Log(int level, const char* filename);
  virtual ~Log();

  int level() const { return level_; }

  void Flush();

  void OutputString(const string& s);
 private:
  // We synchronize log access w/ this guy
  // (use * to not require including mutex.h)
  // TODO(cpopescu): amazingly important !!! Make this reentrant !!!
  synch::Mutex* mutex_;
  // we log only under this level
  const int level_;
  // our log file - main stuff - everything goes here
  FILE* log_file_;
  // a tee - writes to logfile and cerr at the same time (if enabled w/
  // --alsowritetostderr
  FILE* tee_file_;

  friend class LogLine;
};

class LogLine {
 public:
  LogLine(Log* log, int level);
  ~LogLine();

  std::ostream& stream() { return stream_; }

 private:
  Log* const log_;
  std::ostringstream stream_;
  const bool is_fatal_;
};

extern Log* gLogger;

// We also need this utility here so this file does not depend on more
// advanced includes
inline const char* Basename(const char* filename) {
  const char* sep = strrchr(filename, PATH_SEPARATOR);
  return sep ? sep + 1 : filename;
}
}

//////////////////////////////////////////////////////////////////////

// Pure log macros

// The trick is to instantiate a LogLine w/ a scope of one C++ statement
#define LOG(log_level) \
  if ( log_internal::gLogger != NULL && \
       log_level <= log_internal::gLogger->level() ) \
    log_internal::LogLine(log_internal::gLogger, log_level).stream()    \
        << log_internal::Basename(__FILE__) << ":" << __LINE__  << " : "
#ifdef _DEBUG
#define DLOG(log_level) LOG(log_level)
#else
#define DLOG(log_level) if (false) LOG(log_level)
#endif

#define LOG_INFO    LOG(LINFO)
#define LOG_DEBUG   LOG(LDEBUG)
#define LOG_WARNING LOG(LWARNING)
#define LOG_ERROR   LOG(LERROR)
#define LOG_FATAL   LOG(LFATAL)

#define LOG_FATAL_IF(cond) if ( cond ) LOG_FATAL
#define LOG_ERROR_IF(cond) if ( cond ) LOG_ERROR
#define LOG_INFO_IF(cond) if ( cond ) LOG_INFO

// Pure log macros, the same set as above
// but with the addition of the file name/line number
// as there are instances when we get those from somewhere else

// The trick is to instantiate a LogLine w/ a scope of one C++ statement
#define LOG_FILE_LINE(log_level, file, line)                            \
  if ( log_level <= log_internal::gLogger->level() )                    \
    log_internal::LogLine(log_internal::gLogger, log_level).stream()    \
        << log_internal::Basename(file) << ":" << (line) << " : "
#ifdef _DEBUG
#define LOG_FILE_LINE_DEBUG(file, line)         \
  LOG_FILE_LINE(LDEBUG, file, line)
#else
#define LOG_FILE_LINE_DEBUG(file, line)         \
  if (false) LOG_FILE_LINE(LDEBUG, file, line)
#endif

#define LOG_FILE_LINE_INFO(file, line)    LOG_FILE_LINE(LINFO, file, line)
#define LOG_FILE_LINE_WARNING(file, line) LOG_FILE_LINE(LWARNING, file, line)
#define LOG_FILE_LINE_ERROR(file, line)   LOG_FILE_LINE(LERROR, file, line)
#define LOG_FILE_LINE_FATAL(file, line)   LOG_FILE_LINE(LFATAL, file, line)

#define LOG_FILE_LINE_FATAL_IF(cond, file, line) \
  if ( cond ) LOG_FILE_LINE_FATAL(file, line)
#define LOG_FILE_LINE_INFO_IF(cond, file, line) \
  if ( cond ) LOG_FILE_LINE_INFO(file, line)

// Log the given value in hexadecimal with 0x prefix
#define LHEX(v) showbase << hex << v << dec << noshowbase


//////////////////////////////////////////////////////////////////////

// Conditional macros - a little better then ASSERTs
#define CHECK(cond)                                                     \
  if ( cond );                                                          \
  else                                                                  \
    LOG_FATAL << #cond << " failed. "
#define CHECK_NOT_NULL(p)                                               \
  if ( (p) != NULL );                                                   \
  else                                                                  \
    LOG_FATAL << " Pointer is NULL. "
#define CHECK_NULL(p)                                                   \
  if ( (p) == NULL );                                                   \
  else                                                                  \
    LOG_FATAL << " Pointer " << #p << "(" << LHEX(p) << ") is not NULL. "


#define DEFINE_CHECK_OP(name, op)                                       \
  template <class T1, class T2>                                         \
  inline void name(const T1& a, const T2& b, const char* _fn, int _line) { \
    if ( a op b );                                                      \
    else                                                                \
      log_internal::LogLine(log_internal::gLogger, LFATAL).stream()     \
          << _fn << ":" << _line << " : "                               \
          << "Check failed: [" << (a) << "] " #op " [" << (b) << "]";   \
  }

#define DEFINE_CHECK_FUN(name, fun, res)                                \
  template <class T1, class T2>                                         \
  inline void name(const T1& a, const T2& b, const char* _fn, int _line) { \
    if ( fun(a, b) == res );                                            \
    else                                                                \
      log_internal::LogLine(log_internal::gLogger, LFATAL).stream()     \
          << _fn << ":" << _line << " : "                               \
          << "Check failed: " << #fun "("                               \
          << a << "," << (b) << ") != " << res;                         \
  }

DEFINE_CHECK_OP(CHECK_EQ_FUN, ==)
DEFINE_CHECK_OP(CHECK_LT_FUN, <)
DEFINE_CHECK_OP(CHECK_LE_FUN, <=)
DEFINE_CHECK_OP(CHECK_GT_FUN, >)
DEFINE_CHECK_OP(CHECK_GE_FUN, >=)
DEFINE_CHECK_OP(CHECK_NE_FUN, !=)
#define CHECK_EQ(a,b) CHECK_EQ_FUN((a),(b), __FILE__, __LINE__)
#define CHECK_LT(a,b) CHECK_LT_FUN((a),(b), __FILE__, __LINE__)
#define CHECK_LE(a,b) CHECK_LE_FUN((a),(b), __FILE__, __LINE__)
#define CHECK_GT(a,b) CHECK_GT_FUN((a),(b), __FILE__, __LINE__)
#define CHECK_GE(a,b) CHECK_GE_FUN((a),(b), __FILE__, __LINE__)
#define CHECK_NE(a,b) CHECK_NE_FUN((a),(b), __FILE__, __LINE__)

DEFINE_CHECK_FUN(CHECK_STREQ_FUN, strcmp, 0)
DEFINE_CHECK_FUN(CHECK_STRLT_FUN, strcmp, -1)
DEFINE_CHECK_FUN(CHECK_STRGT_FUN, strcmp, 1)
#define CHECK_STREQ(a,b) CHECK_STREQ_FUN((a),(b), __FILE__, __LINE__)
#define CHECK_STRLT(a,b) CHECK_STRLT_FUN((a),(b), __FILE__, __LINE__)
#define CHECK_STRGT(a,b) CHECK_STRGT_FUN((a),(b), __FILE__, __LINE__)

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

#define DCHECK(cond)   CHECK(cond)
#define DCHECK_EQ(a, b) CHECK_EQ(a, b)
#define DCHECK_LT(a, b) CHECK_LT(a, b)
#define DCHECK_LE(a, b) CHECK_LE(a, b)
#define DCHECK_GT(a, b) CHECK_GT(a, b)
#define DCHECK_GE(a, b) CHECK_GE(a, b)
#define DCHECK_NE(a, b) CHECK_NE(a, b)

#define DLOG_DEBUG   LOG(LDEBUG)
#define DLOG_INFO    LOG(LINFO)
#define DLOG_WARNING LOG(LWARNING)
#define DLOG_ERROR   LOG(LERROR)
#define DLOG_FATAL   LOG(LFATAL)

#define DLOG_FATAL_IF(cond) if ( cond ) LOG_FATAL
#define DLOG_INFO_IF(cond) if ( cond ) LOG_INFO

#else
#define DCHECK(cond)    if ( false ) LOG(LDEBUG)
#define DCHECK_EQ(a, b) if ( false ) LOG(LDEBUG)
#define DCHECK_LT(a, b) if ( false ) LOG(LDEBUG)
#define DCHECK_LE(a, b) if ( false ) LOG(LDEBUG)
#define DCHECK_GT(a, b) if ( false ) LOG(LDEBUG)
#define DCHECK_GE(a, b) if ( false ) LOG(LDEBUG)
#define DCHECK_NE(a, b) if ( false ) LOG(LDEBUG)

#define DLOG_DEBUG   if ( false ) LOG(LDEBUG)
#define DLOG_INFO    if ( false ) LOG(LINFO)
#define DLOG_WARNING if ( false ) LOG(LWARNING)
#define DLOG_ERROR   if ( false ) LOG(LERROR)
#define DLOG_FATAL   if ( false ) LOG(LFATAL)

#define DLOG_FATAL_IF(cond) if (false) LOG_FATAL
#define DLOG_INFO_IF(cond) if ( false ) LOG_INFO

#endif


// Internal initialization/deinitialization functions -
// you should not call these
void InitLogger(int level, const char* filename);
void InitDefaultLogger(const char* progname);
void FlushLogger();

#endif    // __COMMON_BASE_LOG_H__
