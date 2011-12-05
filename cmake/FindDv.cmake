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
# Finds Dv library
#
#  Dv_INCLUDE_DIR - where to find dv.h, etc.
#  Dv_LIBRARIES   - List of libraries when using Dv.
#  Dv_FOUND       - True if Dv found.
#

if (Dv_INCLUDE_DIR)
  # Already in cache, be silent
  set(Dv_FIND_QUIETLY TRUE)
endif (Dv_INCLUDE_DIR)

find_path(Dv_INCLUDE_DIR libdv/dv.h
  /opt/local/include
  /usr/local/include
  /usr/include
)

set(Dv_NAMES dv)
find_library(Dv_LIBRARY
  NAMES ${Dv_NAMES}
  PATHS /usr/lib /usr/local/lib /opt/local/lib
)

if (Dv_INCLUDE_DIR AND Dv_LIBRARY)
   set(Dv_FOUND TRUE)
   set( Dv_LIBRARIES ${Dv_LIBRARY} )
else (Dv_INCLUDE_DIR AND Dv_LIBRARY)
   set(Dv_FOUND FALSE)
   set(Dv_LIBRARIES)
endif (Dv_INCLUDE_DIR AND Dv_LIBRARY)

if (Dv_FOUND)
   if (NOT Dv_FIND_QUIETLY)
      message(STATUS "Found Dv: ${Dv_LIBRARY}")
   endif (NOT Dv_FIND_QUIETLY)
else (Dv_FOUND)
   if (Dv_FIND_REQUIRED)
      message(STATUS "Looked for Dv libraries named ${Dv_NAMES}.")
      message(STATUS "Include file detected: [${Dv_INCLUDE_DIR}].")
      message(STATUS "Lib file detected: [${Dv_LIBRARY}].")
      message(FATAL_ERROR "=========> Could NOT find Dv library")
   endif (Dv_FIND_REQUIRED)
endif (Dv_FOUND)

mark_as_advanced(
  Dv_LIBRARY
  Dv_INCLUDE_DIR
  )
