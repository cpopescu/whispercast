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

#include "common/base/system.h"

#include <stdlib.h>
#include <execinfo.h>
#include <stdio.h>

#ifndef _NO_BFD
#include <bfd.h>
#include <cxxabi.h>
#endif

#include <string>

#include "common/base/log.h"
#include "common/base/signal_handlers.h"
#include "common/base/strutil.h"
#include "common/base/gflags.h"
#include "net/base/dns_resolver.h"

//////////////////////////////////////////////////////////////////////

DEFINE_bool(loop_on_exit,
            false,
            "If this is turned on, we loop on exit waiting for your debuger");

DEFINE_bool(loop_on_crash,
            false,
            "Causes the program to hang on bad signals waiting for "
            "your debugger.");

//////////////////////////////////////////////////////////////////////

namespace {

static char* bfd_filename;

#ifdef _NO_BFD

bool DecodeBacktrace(const vector<string>& addr,
                     vector<string>* ret) {
  ret->push_back(string("For Stacktrace run this (cut and paste): "));
  ret->push_back(string("addr2line -e ") + bfd_filename + " \\");
  for ( size_t i = 0; i < addr.size(); i++ ) {
    ret->push_back(strutil::StringPrintf("%s \\", addr[i].c_str()));
  }
  ret->push_back("");
  return true;
}

#else

static void* SlurpSymtab(bfd* abfd) {
  int64 symcount;
  // size_t size;
  unsigned int size;
  if ( (bfd_get_file_flags(abfd) & HAS_SYMS) == 0 ) {
    return false;
  }
  void* syms = NULL;
  symcount = bfd_read_minisymbols(abfd, false, &syms, &size);
  if ( symcount == 0 ) {
    // true -> dynamic
    symcount = bfd_read_minisymbols(abfd, true, &syms, &size);
  }
  if ( symcount < 0 ) {
    return NULL;
  }
  return syms;
}

struct ScanData {
  asymbol** syms;
  bfd_vma pc;
  bfd_boolean found;
  const char* filename;
  const char* functionname;
  unsigned int line;
};


static void FindAddressInSection(bfd* abfd,
                                 asection* section,
                                 void* data) {
  ScanData* sd = reinterpret_cast<ScanData*>(data);
  bfd_vma vma;
  bfd_size_type size;
  if ( sd->found )
    return;
  if ( (bfd_get_section_flags(abfd, section) & SEC_ALLOC) == 0 )
    return;
  vma = bfd_get_section_vma(abfd, section);
  if ( sd->pc < vma )
    return;
  size = bfd_get_section_size(section);
  if ( sd->pc >= vma + size )
    return;
  sd->found = bfd_find_nearest_line(
    abfd, section, sd->syms, sd->pc - vma,
    &sd->filename, &sd->functionname, &sd->line);
}

static void TranslateAddresses(bfd* abfd,
                               asymbol** syms,
                               const vector<string>& addr,
                               vector<string>* ret) {
  ScanData sd;
  sd.syms = syms;
  for ( size_t i = 0; i < addr.size(); i++ ) {
    sd.found = false;
    sd.pc = bfd_scan_vma(addr[i].c_str(), NULL, 16);
    bfd_map_over_sections(abfd, FindAddressInSection, &sd);
    string fileline;
    string function;
    if ( !sd.found ) {
      fileline = "??:0";
      function = "??";
    } else {
      if ( sd.functionname == NULL || *sd.functionname == '\0' ) {
        function = "??";
      } else {
        char buff[1024];
        size_t len = sizeof(buff);
        memset(buff, 0, len);
        int status;
        __cxxabiv1::__cxa_demangle(sd.functionname, buff, &len, &status);
        if ( *buff ) {
          function = buff;
        } else {
          function = sd.functionname;
        }
      }
      fileline = strutil::StringPrintf(
        "[ %80s ]",
        strutil::StringPrintf("%s:%d",
                              (sd.filename ? sd.filename : "??"),
                              static_cast<int>(sd.line)).c_str());
      ret->push_back(fileline + " - " + function);
    }
  }
}

bool DecodeBacktrace(const vector<string>& addr,
                     vector<string>* ret) {
  char** matching;
  bfd* abfd = bfd_openr(bfd_filename, NULL);

  if ( abfd == NULL )
    return false;
  if ( bfd_check_format(abfd, bfd_archive) )
    return false;

  if ( !bfd_check_format_matches(abfd, bfd_object, &matching) )
    return false;
  asymbol** syms =  reinterpret_cast<asymbol**>(SlurpSymtab(abfd));
  if ( syms == NULL ) {
    return false;
  }
  TranslateAddresses(abfd, syms, addr, ret);

  if ( syms != NULL )
    free(syms);
  return true;
}
#endif  // _NO_BFD
}

namespace common {

void InternalSystemExit(int error) {
  if ( FLAGS_loop_on_exit ) {
    while ( true ) {
      if ( log_internal::gLogger != NULL )
        LOG_INFO << " Looping on exit - attach to us !";
      FlushLogger();
      sleep(40);
    }
  }
  if ( log_internal::gLogger != NULL )
    LOG_INFO << " Exiting with error : " << error;
  FlushLogger();
}

void InternalSystemAtExit() {
  if ( log_internal::gLogger != NULL )
    delete log_internal::gLogger;
  log_internal::gLogger = NULL;
}


// This is used by the unittest to test error-exit code
int Exit(int error, bool forced) {
  net::DnsExit();
  FlushLogger();
  if ( forced ) {
    // forcefully exit, avoiding any destructor/atexit()/etc calls
    cerr << "Forcefully exiting with error: " << error << endl;
    ::_exit(error);
  } else {
    InternalSystemExit(error);
    ::exit(error);
  }
  // keep things simple in main()...
  return error;
}

void Init(int argc, char* argv[]) {
  bfd_filename = strdup(argv[0]);
  string full_command(
    strutil::JoinStrings(const_cast<const char**>(argv), argc, " "));
  char cwd[2048] = { 0, };
  fprintf(stderr, "CWD: [%s] CMD: [%s]\n",
          getcwd(cwd, sizeof(cwd)), full_command.c_str());
  google::ParseCommandLineFlags(&argc, &argv, true);
  InitDefaultLogger(strutil::Basename(argv[0]));
  net::DnsInit();

#ifndef _NO_BFD
  bfd_init();
#endif

  LOG_INFO << "Started : " << argv[0];
  CHECK(InstallDefaultSignalHandlers(FLAGS_loop_on_crash));
  LOG_INFO << "Command Line: " << full_command;
  atexit(&InternalSystemAtExit);
}

string GetBacktrace() {
  void* trace[100];
  const int trace_size = backtrace(trace, sizeof(trace) / sizeof(*trace));
  vector<string> addrs;
  vector<string> fitered_messages;
  for ( int i = 0; i < trace_size; i++ ) {
    addrs.push_back(strutil::StringPrintf(
                        "0x%lx",
                        reinterpret_cast<unsigned long int>(trace[i])));
  }
  DecodeBacktrace(addrs, &fitered_messages);
  return strutil::JoinStrings(fitered_messages, "\n");
}

const char* ByteOrderName(ByteOrder order) {
  switch ( order ) {
    CONSIDER(BIGENDIAN);
    CONSIDER(LILENDIAN);
  default: LOG_FATAL << " Unsupported byte order: " << order;
  }
  return "Unknown";
}

}   // end namespace system
