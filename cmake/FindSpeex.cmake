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

# Finds Speex library
#
#  Speex_INCLUDE_DIR - where to find speex.h, etc.
#  Speex_LIBRARIES   - List of libraries when using Speex.
#  Speex_FOUND       - True if Speex found.
#

if (Speex_INCLUDE_DIR)
  # Already in cache, be silent
  set(Speex_FIND_QUIETLY TRUE)
endif (Speex_INCLUDE_DIR)

find_path(Speex_INCLUDE_DIR speex/speex.h
  /opt/local/include
  /usr/local/include
  /usr/include
)

set(Speex_NAMES speex)
find_library(Speex_LIBRARY
  NAMES ${Speex_NAMES}
  PATHS /usr/lib /usr/local/lib /opt/local/lib
)

if (Speex_INCLUDE_DIR AND Speex_LIBRARY)
   set(Speex_FOUND TRUE)
   set( Speex_LIBRARIES ${Speex_LIBRARY} )
else (Speex_INCLUDE_DIR AND Speex_LIBRARY)
   set(Speex_FOUND FALSE)
   set(Speex_LIBRARIES)
endif (Speex_INCLUDE_DIR AND Speex_LIBRARY)

if (Speex_FOUND)
   if (NOT Speex_FIND_QUIETLY)
      message(STATUS "Found Speex: ${Speex_LIBRARY}")
   endif (NOT Speex_FIND_QUIETLY)
else (Speex_FOUND)
   if (Speex_FIND_REQUIRED)
      message(STATUS "Looked for Speex libraries named ${Speex_NAMES}.")
      message(STATUS "Include file detected: [${Speex_INCLUDE_DIR}].")
      message(STATUS "Lib file detected: [${Speex_LIBRARY}].")
      message(FATAL_ERROR "=========> Could NOT find Speex library")
   endif (Speex_FIND_REQUIRED)
endif (Speex_FOUND)

mark_as_advanced(
  Speex_LIBRARY
  Speex_INCLUDE_DIR
  )
