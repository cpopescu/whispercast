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

#include <dlfcn.h>

#include <whisperlib/common/base/gflags.h>
#include <whisperlib/common/base/log.h>

#include "rtmp/rtmp_consts.h"
#include "rtmp/rtmp_handshake.h"

//////////////////////////////////////////////////////////////////////

DEFINE_string(rtmp_handshake_library,
              "",
              "If specifired we load the handshake function from this "
              "library");


DEFINE_string(element_libraries_dir,
              "",
              "Where are placed the modules with the implementations for the "
              "elements that we can create ?");

//////////////////////////////////////////////////////////////////////

namespace {

void* glb_lib_handle = NULL;
bool (*glb_handshake_fun)(const char* data,
                          int cb,
                          const char** response);
pthread_once_t  glb_once_control = PTHREAD_ONCE_INIT;

void InitHandshakeLib() {
  glb_handshake_fun = NULL;
  glb_lib_handle = NULL;
  if ( !FLAGS_rtmp_handshake_library.empty() ) {
    glb_lib_handle = dlopen(FLAGS_rtmp_handshake_library.c_str(), RTLD_LAZY);
    if ( glb_lib_handle == NULL ) {
      if ( !FLAGS_rtmp_handshake_library.empty() ) {
        LOG_FATAL << " Cannot load rtmp_handshake_library: "
                  << FLAGS_rtmp_handshake_library;
        return;
      }
    }
    LOG_INFO << " Loaded handshake library from: ["
             << FLAGS_rtmp_handshake_library
             << "]";
  } else {
    glb_lib_handle = dlopen(
        (FLAGS_element_libraries_dir +
         "/libwhisper_streamlib_rtmp_extra.so").c_str(),
        RTLD_LAZY);
    if ( glb_lib_handle == NULL ) {
      return;
    }
    LOG_INFO << "Loaded handshake library from : ["
             << (FLAGS_element_libraries_dir +
                 "/libwhisper_streamlib_rtmp_extra.so]");
  }
  glb_handshake_fun =
      reinterpret_cast<bool (*)(const char*, int, const char**)>(
          dlsym(glb_lib_handle, "PrepareServerHandshake"));
  const char* const error = dlerror();
  if ( error != NULL ) {
    LOG_FATAL << " Error linking PrepareServerHandshake function: "
              << error;
  }
  // We leave the lib loaded until we die ..
}
}

namespace rtmp {

bool PrepareServerHandshake(const char* data, int cb,
                            const char** response) {
  // LOG_INFO << " ==============>>>> VERSION: "
  //          << static_cast<int>(data[4]) << "."
  //          << static_cast<int>(data[5]) << "."
  //          << static_cast<int>(data[6]) << "."
  //          << static_cast<int>(data[7]);
  pthread_once(&glb_once_control, InitHandshakeLib);
  if ( glb_handshake_fun != NULL ) {
   return (*glb_handshake_fun)(data, cb, response);
  }
  uint8* p = new uint8[2 * kHandshakeSize];
  *response = reinterpret_cast<const char*>(p);
  memcpy(p, kServerV3HandshakeBlock, kHandshakeSize);
  memcpy(p + kHandshakeSize, kServerV3HandshakeBlock, kHandshakeSize);
  return true;
}

//////////////////////////////////////////////////////////////////////
}
