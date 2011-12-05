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
# Finds Asound library
#
#  Asound_INCLUDE_DIR - where to find asoundlib.h, etc.
#  Asound_LIBRARIES   - List of libraries when using Asound.
#  Asound_FOUND       - True if Asound found.
#

if (Asound_INCLUDE_DIR)
  # Already in cache, be silent
  set(Asound_FIND_QUIETLY TRUE)
endif (Asound_INCLUDE_DIR)

find_path(Asound_INCLUDE_DIR alsa/asoundlib.h
  /opt/local/include
  /usr/local/include
  /usr/include
)

set(Asound_NAMES asound)
find_library(Asound_LIBRARY
  NAMES ${Asound_NAMES}
  PATHS /usr/lib /usr/local/lib /opt/local/lib
)

if (Asound_INCLUDE_DIR AND Asound_LIBRARY)
   set(Asound_FOUND TRUE)
   set( Asound_LIBRARIES ${Asound_LIBRARY} )
else (Asound_INCLUDE_DIR AND Asound_LIBRARY)
   set(Asound_FOUND FALSE)
   set(Asound_LIBRARIES)
endif (Asound_INCLUDE_DIR AND Asound_LIBRARY)

if (Asound_FOUND)
   if (NOT Asound_FIND_QUIETLY)
      message(STATUS "Found Asound: ${Asound_LIBRARY}")
   endif (NOT Asound_FIND_QUIETLY)
else (Asound_FOUND)
   if (Asound_FIND_REQUIRED)
      message(STATUS "Looked for Asound libraries named ${Asound_NAMES}.")
      message(STATUS "Include file detected: [${Asound_INCLUDE_DIR}].")
      message(STATUS "Lib file detected: [${Asound_LIBRARY}].")
      message(FATAL_ERROR "=========> Could NOT find Asound library")
   endif (Asound_FIND_REQUIRED)
endif (Asound_FOUND)

mark_as_advanced(
  Asound_LIBRARY
  Asound_INCLUDE_DIR
  )
