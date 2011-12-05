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
# Finds Esd library
#
#  Esd_INCLUDE_DIR - where to find esd.h, etc.
#  Esd_LIBRARIES   - List of libraries when using Esd.
#  Esd_FOUND       - True if Esd found.
#

if (Esd_INCLUDE_DIR)
  # Already in cache, be silent
  set(Esd_FIND_QUIETLY TRUE)
endif (Esd_INCLUDE_DIR)

find_path(Esd_INCLUDE_DIR esd.h
  /opt/local/include
  /usr/local/include
  /usr/include
)

set(Esd_NAMES esd)
find_library(Esd_LIBRARY
  NAMES ${Esd_NAMES}
  PATHS /usr/lib /usr/local/lib /opt/local/lib
)

if (Esd_INCLUDE_DIR AND Esd_LIBRARY)
   set(Esd_FOUND TRUE)
   set( Esd_LIBRARIES ${Esd_LIBRARY} )
else (Esd_INCLUDE_DIR AND Esd_LIBRARY)
   set(Esd_FOUND FALSE)
   set(Esd_LIBRARIES)
endif (Esd_INCLUDE_DIR AND Esd_LIBRARY)

if (Esd_FOUND)
   if (NOT Esd_FIND_QUIETLY)
      message(STATUS "Found Esd: ${Esd_LIBRARY}")
   endif (NOT Esd_FIND_QUIETLY)
else (Esd_FOUND)
   if (Esd_FIND_REQUIRED)
      message(STATUS "Looked for Esd libraries named ${Esd_NAMES}.")
      message(STATUS "Include file detected: [${Esd_INCLUDE_DIR}].")
      message(STATUS "Lib file detected: [${Esd_LIBRARY}].")
      message(FATAL_ERROR "=========> Could NOT find Esd library")
   endif (Esd_FIND_REQUIRED)
endif (Esd_FOUND)

mark_as_advanced(
  Esd_LIBRARY
  Esd_INCLUDE_DIR
  )
