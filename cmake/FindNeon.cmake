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

# Finds Neon library
#
#  Neon_INCLUDE_DIR - where to find neon.h, etc.
#  Neon_LIBRARIES   - List of libraries when using Neon.
#  Neon_FOUND       - True if Neon found.
#

if (Neon_INCLUDE_DIR)
  # Already in cache, be silent
  set(Neon_FIND_QUIETLY TRUE)
endif (Neon_INCLUDE_DIR)

find_path(Neon_INCLUDE_DIR ne_207.h
  /opt/local/include
  /usr/local/include
  /usr/include
  /opt/local/include/neon
  /usr/local/include/neon
  /usr/include/neon
)

set(Neon_NAMES neon)
find_library(Neon_LIBRARY
  NAMES ${Neon_NAMES}
  PATHS /usr/lib /usr/local/lib /opt/local/lib
)

if (Neon_INCLUDE_DIR AND Neon_LIBRARY)
   set(Neon_FOUND TRUE)
   set( Neon_LIBRARIES ${Neon_LIBRARY} )
else (Neon_INCLUDE_DIR AND Neon_LIBRARY)
   set(Neon_FOUND FALSE)
   set(Neon_LIBRARIES)
endif (Neon_INCLUDE_DIR AND Neon_LIBRARY)

if (Neon_FOUND)
   if (NOT Neon_FIND_QUIETLY)
      message(STATUS "Found Neon: ${Neon_LIBRARY}")
   endif (NOT Neon_FIND_QUIETLY)
else (Neon_FOUND)
   if (Neon_FIND_REQUIRED)
      message(STATUS "Looked for Neon libraries named ${Neon_NAMES}.")
      message(STATUS "Include file detected: [${Neon_INCLUDE_DIR}].")
      message(STATUS "Lib file detected: [${Neon_LIBRARY}].")
      message(FATAL_ERROR "=========> Could NOT find Neon library")
   endif (Neon_FIND_REQUIRED)
endif (Neon_FOUND)

mark_as_advanced(
  Neon_LIBRARY
  Neon_INCLUDE_DIR
  )
