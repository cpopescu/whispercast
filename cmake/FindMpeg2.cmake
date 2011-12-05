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

# Finds Mpeg2 library
#
#  Mpeg2_INCLUDE_DIR - where to find mpeg2.h, etc.
#  Mpeg2_LIBRARIES   - List of libraries when using Mpeg2.
#  Mpeg2_FOUND       - True if Mpeg2 found.
#

if (Mpeg2_INCLUDE_DIR)
  # Already in cache, be silent
  set(Mpeg2_FIND_QUIETLY TRUE)
endif (Mpeg2_INCLUDE_DIR)

find_path(Mpeg2_INCLUDE_DIR mpeg2.h
  /opt/local/include
  /usr/local/include
  /usr/include
  /usr/include/mpeg2dec
  /opt/local/include/mpeg2dec
  /usr/local/include/mpeg2dec
)

set(Mpeg2_NAMES mpeg2)
find_library(Mpeg2_LIBRARY
  NAMES ${Mpeg2_NAMES}
  PATHS /usr/lib /usr/local/lib /opt/local/lib
)

if (Mpeg2_INCLUDE_DIR AND Mpeg2_LIBRARY)
   set(Mpeg2_FOUND TRUE)
   set( Mpeg2_LIBRARIES ${Mpeg2_LIBRARY} )
else (Mpeg2_INCLUDE_DIR AND Mpeg2_LIBRARY)
   set(Mpeg2_FOUND FALSE)
   set(Mpeg2_LIBRARIES)
endif (Mpeg2_INCLUDE_DIR AND Mpeg2_LIBRARY)

if (Mpeg2_FOUND)
   if (NOT Mpeg2_FIND_QUIETLY)
      message(STATUS "Found Mpeg2: ${Mpeg2_LIBRARY}")
   endif (NOT Mpeg2_FIND_QUIETLY)
else (Mpeg2_FOUND)
   if (Mpeg2_FIND_REQUIRED)
      message(STATUS "Looked for Mpeg2 libraries named ${Mpeg2_NAMES}.")
      message(STATUS "Include file detected: [${Mpeg2_INCLUDE_DIR}].")
      message(STATUS "Lib file detected: [${Mpeg2_LIBRARY}].")
      message(FATAL_ERROR "=========> Could NOT find Mpeg2 library")
   endif (Mpeg2_FIND_REQUIRED)
endif (Mpeg2_FOUND)

mark_as_advanced(
  Mpeg2_LIBRARY
  Mpeg2_INCLUDE_DIR
  )
