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

# Finds Theora library
#
#  Theora_INCLUDE_DIR - where to find theora.h, etc.
#  Theora_LIBRARIES   - List of libraries when using Theora.
#  Theora_FOUND       - True if Theora found.
#

if (Theora_INCLUDE_DIR)
  # Already in cache, be silent
  set(Theora_FIND_QUIETLY TRUE)
endif (Theora_INCLUDE_DIR)

find_path(Theora_INCLUDE_DIR theora/theora.h
  /opt/local/include
  /usr/local/include
  /usr/include
)

set(Theora_NAMES theora)
find_library(Theora_LIBRARY
  NAMES ${Theora_NAMES}
  PATHS /usr/lib /usr/local/lib /opt/local/lib
)

if (Theora_INCLUDE_DIR AND Theora_LIBRARY)
   set(Theora_FOUND TRUE)
   set( Theora_LIBRARIES ${Theora_LIBRARY} )
else (Theora_INCLUDE_DIR AND Theora_LIBRARY)
   set(Theora_FOUND FALSE)
   set(Theora_LIBRARIES)
endif (Theora_INCLUDE_DIR AND Theora_LIBRARY)

if (Theora_FOUND)
   if (NOT Theora_FIND_QUIETLY)
      message(STATUS "Found Theora: ${Theora_LIBRARY}")
   endif (NOT Theora_FIND_QUIETLY)
else (Theora_FOUND)
   if (Theora_FIND_REQUIRED)
      message(STATUS "Looked for Theora libraries named ${Theora_NAMES}.")
      message(STATUS "Include file detected: [${Theora_INCLUDE_DIR}].")
      message(STATUS "Lib file detected: [${Theora_LIBRARY}].")
      message(FATAL_ERROR "=========> Could NOT find Theora library")
   endif (Theora_FIND_REQUIRED)
endif (Theora_FOUND)

mark_as_advanced(
  Theora_LIBRARY
  Theora_INCLUDE_DIR
  )
