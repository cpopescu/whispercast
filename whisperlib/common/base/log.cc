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

#include <sys/time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdio.h>
#include <fcntl.h>
#include <iomanip>
#include "common/base/log.h"
#include "common/base/strutil.h"
#include "common/sync/mutex.h"
#include "common/base/system.h"
#include "common/base/gflags.h"

//////////////////////////////////////////////////////////////////////

DEFINE_int32(loglevel,
             LINFO,
             "We initialize the log at this maximum level."
             "(we log messages with levels less then this)");

DEFINE_string(logdir,
              "/tmp",
              "We write the log file unde this directory.");

DEFINE_bool(alsologtostderr,
            false,
            "If this is turned on, we also write log lines to this stderr");

DEFINE_bool(logcolors,
            false,
            "If this is turned on, the log will use bash colors");

DEFINE_bool(logtid,
            false,
            "Log the thread ID");

DEFINE_bool(logflush,
            false,
            "Turn this on to flush log after each line");

//////////////////////////////////////////////////////////////////////

void InitLogger(int level, const char* filename) {
  if ( log_internal::gLogger != NULL ) return;
  log_internal::gLogger = new log_internal::Log(level, filename);
}
void InitDefaultLogger(const char* progname) {
  if ( log_internal::gLogger != NULL ) return;
  const string log_file(FLAGS_logdir + "/" + progname + ".LOG");
  cerr << "Writing log to: " << log_file << endl;
  log_internal::gLogger =
    new log_internal::Log(FLAGS_loglevel, log_file.c_str());
}
void FlushLogger() {
  if ( log_internal::gLogger != NULL ) {
    log_internal::gLogger->Flush();
  }
}

namespace log_internal {

Log* gLogger       = NULL;


//////////////////////////////////////////////////////////////////////

static const char* kLogHeader[] = { "F", "E", "W", "I", "D", "X" };
static const char* kLogColors[] = { "\033[1;31m",  // FATAL   Red Bold
                                    "\033[31m",    // ERROR   Red
                                    "\033[35m",    // WARNING Magenta(Purple)
                                    "\033[30m",    // INFO    Black
                                    "\033[37m",    // DEBUG   Gray
                                    "\033[34m"};   // other   Blue

static void LogHeader(std::ostream& stream, unsigned level) {
  struct timeval tv;
  struct timezone tz;
  struct tm tm_time;
  gettimeofday(&tv, &tz);
  localtime_r(&tv.tv_sec, &tm_time);
  static const int kNumLevels = sizeof(kLogHeader) / sizeof(*kLogHeader);
  if ( level >= kNumLevels ) {
    level = kNumLevels - 1;
  }
  if ( FLAGS_logcolors ) {
    // print color
    stream << kLogColors[level];
  }
  const string header(strutil::StringPrintf(
                          "%s [%04d%02d%02d %02d%02d%02d.%03ld] %s",
                          kLogHeader[level],
                          1900 + tm_time.tm_year,
                          1 + tm_time.tm_mon,
                          tm_time.tm_mday,
                          tm_time.tm_hour,
                          tm_time.tm_min,
                          tm_time.tm_sec,
                          (tv.tv_usec / 1000),
                          FLAGS_logtid
                          ? strutil::StringPrintf(
                              "[TID: 0x%08llx] ",
#ifdef __APPLE__
    reinterpret_cast<long long unsigned int>(::pthread_self()) ).c_str()
#else
   static_cast<long long unsigned int>(::pthread_self()) ).c_str()
#endif
                           : ""));
  stream << header;
}

//////////////////////////////////////////////////////////////////////

LogLine::LogLine(Log* log, int level)
    : log_(log),
      is_fatal_(level == LFATAL) {
  LogHeader(stream_, level);
}

LogLine::~LogLine() {
  if ( FLAGS_logcolors ) {
    stream_ << "\033[0m";
  }
  stream_ << "\n";
  log_->OutputString(stream_.str());
  if ( is_fatal_ ) {
    ::abort();
  }
}

//////////////////////////////////////////////////////////////////////

Log::Log(int level, const char* filename)
  : mutex_(new synch::Mutex(true)),  // reentrant
    level_(level),
    log_file_(NULL),
    tee_file_(NULL) {
  log_file_ = fopen(filename, "a");
  if ( log_file_ == NULL ) {
    fprintf(stderr,
            "Cound not open the log file: [%s]. Using stderr.\n",
            filename);
    log_file_ = stderr;
  } else {
    if ( FLAGS_alsologtostderr ) {
      tee_file_ = stderr;
    }
  }
}

Log::~Log() {
  if ( log_file_ != stderr ) {
    fclose(log_file_);
  }
  delete mutex_;
}

void Log::Flush() {
  fflush(log_file_);
}

void Log::OutputString(const string& s) {
  mutex_->Lock();
  fprintf(log_file_, "%s", s.c_str());
  if ( tee_file_ != NULL ) {
    fprintf(tee_file_, "%s", s.c_str());
  }
  if ( FLAGS_logflush ) {
    fflush(log_file_);
  }
  mutex_->Unlock();
}
}
