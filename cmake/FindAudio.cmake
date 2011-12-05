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
# Finds Audio library
#
#  Audio_INCLUDE_DIR - where to find audio.h, etc.
#  Audio_LIBRARIES   - List of libraries when using Audio.
#  Audio_FOUND       - True if Audio found.
#

if (Audio_INCLUDE_DIR)
  # Already in cache, be silent
  set(Audio_FIND_QUIETLY TRUE)
endif (Audio_INCLUDE_DIR)

find_path(Audio_INCLUDE_DIR audio/audio.h
  /opt/local/include
  /usr/local/include
  /usr/include
)

set(Audio_NAMES audio)
find_library(Audio_LIBRARY
  NAMES ${Audio_NAMES}
  PATHS /usr/lib /usr/local/lib /opt/local/lib
)

if (Audio_INCLUDE_DIR AND Audio_LIBRARY)
   set(Audio_FOUND TRUE)
   set( Audio_LIBRARIES ${Audio_LIBRARY} )
else (Audio_INCLUDE_DIR AND Audio_LIBRARY)
   set(Audio_FOUND FALSE)
   set(Audio_LIBRARIES)
endif (Audio_INCLUDE_DIR AND Audio_LIBRARY)

if (Audio_FOUND)
   if (NOT Audio_FIND_QUIETLY)
      message(STATUS "Found Audio: ${Audio_LIBRARY}")
   endif (NOT Audio_FIND_QUIETLY)
else (Audio_FOUND)
   if (Audio_FIND_REQUIRED)
      message(STATUS "Looked for Audio libraries named ${Audio_NAMES}.")
      message(STATUS "Include file detected: [${Audio_INCLUDE_DIR}].")
      message(STATUS "Lib file detected: [${Audio_LIBRARY}].")
      message(FATAL_ERROR "=========> Could NOT find Audio library")
   endif (Audio_FIND_REQUIRED)
endif (Audio_FOUND)

mark_as_advanced(
  Audio_LIBRARY
  Audio_INCLUDE_DIR
  )
