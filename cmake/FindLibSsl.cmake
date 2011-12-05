# Copyright (c) 2009, Whispersoft s.r.l.
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#
# * Redistributions of source code must retain the above copyright
# notice, this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above
# copyright notice, this list of conditions and the following disclaimer
# in the documentation and/or other materials provided with the
# distribution.
# * Neither the name of Whispersoft s.r.l. nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
# A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
# OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
# SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
# LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
# THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
# (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
# Finds LibSsl library
#
#  LibSsl_INCLUDE_DIR - where to find ssl.h, etc.
#  LibSsl_LIBRARIES   - List of libraries when using LibSsl.
#  LibSsl_FOUND       - True if LibSsl found.
#

if (LibSsl_INCLUDE_DIR)
  # Already in cache, be silent
  set(LibSsl_FIND_QUIETLY TRUE)
endif (LibSsl_INCLUDE_DIR)

find_path(LibSsl_INCLUDE_DIR ssl.h
  /opt/local/include
  /usr/local/include
  /usr/include
  /usr/include/openssl
)

set(LibSsl_NAMES ssl)
find_library(LibSsl_LIBRARY
  NAMES ${LibSsl_NAMES}
  PATHS /usr/lib /usr/local/lib /opt/local/lib
)

if (LibSsl_INCLUDE_DIR AND LibSsl_LIBRARY)
   set(LibSsl_FOUND TRUE)
   set( LibSsl_LIBRARIES ${LibSsl_LIBRARY} )
else (LibSsl_INCLUDE_DIR AND LibSsl_LIBRARY)
   set(LibSsl_FOUND FALSE)
   set(LibSsl_LIBRARIES)
endif (LibSsl_INCLUDE_DIR AND LibSsl_LIBRARY)

if (LibSsl_FOUND)
   if (NOT LibSsl_FIND_QUIETLY)
      message(STATUS "Found LibSsl: ${LibSsl_LIBRARY}")
   endif (NOT LibSsl_FIND_QUIETLY)
else (LibSsl_FOUND)
   if (LibSsl_FIND_REQUIRED)
      message(STATUS "Looked for LibSsl libraries named ${LibSsl_NAMES}.")
      message(STATUS "Include file detected: [${LibSsl_INCLUDE_DIR}].")
      message(STATUS "Lib file detected: [${LibSsl_LIBRARY}].")
      message(FATAL_ERROR "=========> Could NOT find LibSsl library")
   endif (LibSsl_FIND_REQUIRED)
endif (LibSsl_FOUND)

mark_as_advanced(
  LibSsl_LIBRARY
  LibSsl_INCLUDE_DIR
  )
