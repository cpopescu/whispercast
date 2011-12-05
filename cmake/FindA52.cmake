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
# Finds A52 library
#
#  A52_INCLUDE_DIR - where to find a52.h, etc.
#  A52_LIBRARIES   - List of libraries when using A52.
#  A52_FOUND       - True if A52 found.
#

if (A52_INCLUDE_DIR)
  # Already in cache, be silent
  set(A52_FIND_QUIETLY TRUE)
endif (A52_INCLUDE_DIR)

find_path(A52_INCLUDE_DIR a52dec/a52.h
  /opt/local/include
  /usr/local/include
  /usr/include
)

set(A52_NAMES a52)
find_library(A52_LIBRARY
  NAMES ${A52_NAMES}
  PATHS /usr/lib /usr/local/lib /opt/local/lib
)

if (A52_INCLUDE_DIR AND A52_LIBRARY)
   set(A52_FOUND TRUE)
   set( A52_LIBRARIES ${A52_LIBRARY} )
else (A52_INCLUDE_DIR AND A52_LIBRARY)
   set(A52_FOUND FALSE)
   set(A52_LIBRARIES)
endif (A52_INCLUDE_DIR AND A52_LIBRARY)

if (A52_FOUND)
   if (NOT A52_FIND_QUIETLY)
      message(STATUS "Found A52: ${A52_LIBRARY}")
   endif (NOT A52_FIND_QUIETLY)
else (A52_FOUND)
   if (A52_FIND_REQUIRED)
      message(STATUS "Looked for A52 libraries named ${A52_NAMES}.")
      message(STATUS "Include file detected: [${A52_INCLUDE_DIR}].")
      message(STATUS "Lib file detected: [${A52_LIBRARY}].")
      message(FATAL_ERROR "=========> Could NOT find A52 library")
   endif (A52_FIND_REQUIRED)
endif (A52_FOUND)

mark_as_advanced(
  A52_LIBRARY
  A52_INCLUDE_DIR
  )
