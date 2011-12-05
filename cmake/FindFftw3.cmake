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
# Finds Fftw3 library
#
#  Fftw3_INCLUDE_DIR - where to find fftw3.h, etc.
#  Fftw3_LIBRARIES   - List of libraries when using Fftw3.
#  Fftw3_FOUND       - True if Fftw3 found.
#

if (Fftw3_INCLUDE_DIR)
  # Already in cache, be silent
  set(Fftw3_FIND_QUIETLY TRUE)
endif (Fftw3_INCLUDE_DIR)

find_path(Fftw3_INCLUDE_DIR fftw3.h
  /opt/local/include
  /usr/local/include
  /usr/include
)

set(Fftw3_NAMES fftw3)
find_library(Fftw3_LIBRARY
  NAMES ${Fftw3_NAMES}
  PATHS /usr/lib /usr/local/lib /opt/local/lib
)
if (NOT Fftw3_LIBRARY)
  set(Fftw3_NAMES fftw3f)
  find_library(Fftw3_LIBRARY
    NAMES ${Fftw3_NAMES}
    PATHS /usr/lib /usr/local/lib /opt/local/lib
    )
endif (NOT Fftw3_LIBRARY)

if (Fftw3_INCLUDE_DIR AND Fftw3_LIBRARY)
   set(Fftw3_FOUND TRUE)
   set( Fftw3_LIBRARIES ${Fftw3_LIBRARY} )
else (Fftw3_INCLUDE_DIR AND Fftw3_LIBRARY)
   set(Fftw3_FOUND FALSE)
   set(Fftw3_LIBRARIES)
endif (Fftw3_INCLUDE_DIR AND Fftw3_LIBRARY)

if (Fftw3_FOUND)
   if (NOT Fftw3_FIND_QUIETLY)
      message(STATUS "Found Fftw3: ${Fftw3_LIBRARY}")
   endif (NOT Fftw3_FIND_QUIETLY)
else (Fftw3_FOUND)
   if (Fftw3_FIND_REQUIRED)
      message(STATUS "Looked for Fftw3 libraries named ${Fftw3_NAMES}.")
      message(STATUS "Include file detected: [${Fftw3_INCLUDE_DIR}].")
      message(STATUS "Lib file detected: [${Fftw3_LIBRARY}].")
      message(FATAL_ERROR "=========> Could NOT find Fftw3 library")
   endif (Fftw3_FIND_REQUIRED)
endif (Fftw3_FOUND)

mark_as_advanced(
  Fftw3_LIBRARY
  Fftw3_INCLUDE_DIR
  )
