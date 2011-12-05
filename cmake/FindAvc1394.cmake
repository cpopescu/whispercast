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
# Finds Avc1394 library
#
#  Avc1394_INCLUDE_DIR - where to find avc1394.h, etc.
#  Avc1394_LIBRARIES   - List of libraries when using Avc1394.
#  Avc1394_FOUND       - True if Avc1394 found.
#

if (Avc1394_INCLUDE_DIR)
  # Already in cache, be silent
  set(Avc1394_FIND_QUIETLY TRUE)
endif (Avc1394_INCLUDE_DIR)

find_path(Avc1394_INCLUDE_DIR libavc1394/avc1394.h
  /opt/local/include
  /usr/local/include
  /usr/include
)

set(Avc1394_NAMES avc1394)
find_library(Avc1394_LIBRARY
  NAMES ${Avc1394_NAMES}
  PATHS /usr/lib /usr/local/lib /opt/local/lib
)

if (Avc1394_INCLUDE_DIR AND Avc1394_LIBRARY)
   set(Avc1394_FOUND TRUE)
   set( Avc1394_LIBRARIES ${Avc1394_LIBRARY} )
else (Avc1394_INCLUDE_DIR AND Avc1394_LIBRARY)
   set(Avc1394_FOUND FALSE)
   set(Avc1394_LIBRARIES)
endif (Avc1394_INCLUDE_DIR AND Avc1394_LIBRARY)

if (Avc1394_FOUND)
   if (NOT Avc1394_FIND_QUIETLY)
      message(STATUS "Found Avc1394: ${Avc1394_LIBRARY}")
   endif (NOT Avc1394_FIND_QUIETLY)
else (Avc1394_FOUND)
   if (Avc1394_FIND_REQUIRED)
      message(STATUS "Looked for Avc1394 libraries named ${Avc1394_NAMES}.")
      message(STATUS "Include file detected: [${Avc1394_INCLUDE_DIR}].")
      message(STATUS "Lib file detected: [${Avc1394_LIBRARY}].")
      message(FATAL_ERROR "=========> Could NOT find Avc1394 library")
   endif (Avc1394_FIND_REQUIRED)
endif (Avc1394_FOUND)

mark_as_advanced(
  Avc1394_LIBRARY
  Avc1394_INCLUDE_DIR
  )
