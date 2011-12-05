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
# Finds DTS library
#
#  DTS_INCLUDE_DIR - where to find dts.h, etc.
#  DTS_LIBRARIES   - List of libraries when using DTS.
#  DTS_FOUND       - True if DTS found.
#

if (DTS_INCLUDE_DIR)
  # Already in cache, be silent
  set(DTS_FIND_QUIETLY TRUE)
endif (DTS_INCLUDE_DIR)

find_path(DTS_INCLUDE_DIR dts.h
  /opt/local/include
  /usr/local/include
  /usr/include
)

set(DTS_NAMES dts)
find_library(DTS_LIBRARY
  NAMES ${DTS_NAMES}
  PATHS /usr/lib /usr/local/lib /opt/local/lib
)

if (DTS_INCLUDE_DIR AND DTS_LIBRARY)
   set(DTS_FOUND TRUE)
   set( DTS_LIBRARIES ${DTS_LIBRARY} )
else (DTS_INCLUDE_DIR AND DTS_LIBRARY)
   set(DTS_FOUND FALSE)
   set(DTS_LIBRARIES)
endif (DTS_INCLUDE_DIR AND DTS_LIBRARY)

if (DTS_FOUND)
   if (NOT DTS_FIND_QUIETLY)
      message(STATUS "Found DTS: ${DTS_LIBRARY}")
   endif (NOT DTS_FIND_QUIETLY)
else (DTS_FOUND)
   if (DTS_FIND_REQUIRED)
      message(STATUS "Looked for DTS libraries named ${DTS_NAMES}.")
      message(STATUS "Include file detected: [${DTS_INCLUDE_DIR}].")
      message(STATUS "Lib file detected: [${DTS_LIBRARY}].")
      message(FATAL_ERROR "=========> Could NOT find DTS library")
   endif (DTS_FIND_REQUIRED)
endif (DTS_FOUND)

mark_as_advanced(
  DTS_LIBRARY
  DTS_INCLUDE_DIR
  )
