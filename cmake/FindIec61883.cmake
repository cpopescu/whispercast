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
# Finds Iec61883 library
#
#  Iec61883_INCLUDE_DIR - where to find iec61883.h, etc.
#  Iec61883_LIBRARIES   - List of libraries when using Iec61883.
#  Iec61883_FOUND       - True if Iec61883 found.
#

if (Iec61883_INCLUDE_DIR)
  # Already in cache, be silent
  set(Iec61883_FIND_QUIETLY TRUE)
endif (Iec61883_INCLUDE_DIR)

find_path(Iec61883_INCLUDE_DIR libiec61883/iec61883.h
  /opt/local/include
  /usr/local/include
  /usr/include
)

set(Iec61883_NAMES iec61883)
find_library(Iec61883_LIBRARY
  NAMES ${Iec61883_NAMES}
  PATHS /usr/lib /usr/local/lib /opt/local/lib
)

if (Iec61883_INCLUDE_DIR AND Iec61883_LIBRARY)
   set(Iec61883_FOUND TRUE)
   set( Iec61883_LIBRARIES ${Iec61883_LIBRARY} )
else (Iec61883_INCLUDE_DIR AND Iec61883_LIBRARY)
   set(Iec61883_FOUND FALSE)
   set(Iec61883_LIBRARIES)
endif (Iec61883_INCLUDE_DIR AND Iec61883_LIBRARY)

if (Iec61883_FOUND)
   if (NOT Iec61883_FIND_QUIETLY)
      message(STATUS "Found Iec61883: ${Iec61883_LIBRARY}")
   endif (NOT Iec61883_FIND_QUIETLY)
else (Iec61883_FOUND)
   if (Iec61883_FIND_REQUIRED)
      message(STATUS "Looked for Iec61883 libraries named ${Iec61883_NAMES}.")
      message(STATUS "Include file detected: [${Iec61883_INCLUDE_DIR}].")
      message(STATUS "Lib file detected: [${Iec61883_LIBRARY}].")
      message(FATAL_ERROR "=========> Could NOT find Iec61883 library")
   endif (Iec61883_FIND_REQUIRED)
endif (Iec61883_FOUND)

mark_as_advanced(
  Iec61883_LIBRARY
  Iec61883_INCLUDE_DIR
  )
