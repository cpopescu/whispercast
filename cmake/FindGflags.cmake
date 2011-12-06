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
# Finds Gflags library
#
#  Gflags_INCLUDE_DIR - where to find gflags.h, etc.
#  Gflags_LIBRARIES   - List of libraries when using Gflags.
#  Gflags_FOUND       - True if Gflags found.
#

if (Gflags_INCLUDE_DIR)
  # Already in cache, be silent
  set(Gflags_FIND_QUIETLY TRUE)
endif (Gflags_INCLUDE_DIR)

find_path(Gflags_INCLUDE_DIR gflags/gflags.h
  /opt/local/include
  /usr/local/include
  /usr/include
)

set(Gflags_NAMES gflags)
find_library(Gflags_LIBRARY
  NAMES ${Gflags_NAMES}
  PATHS /usr/lib /usr/local/lib /opt/local/lib
)

if (Gflags_INCLUDE_DIR AND Gflags_LIBRARY)
   set(Gflags_FOUND TRUE)
   set( Gflags_LIBRARIES ${Gflags_LIBRARY} )
else (Gflags_INCLUDE_DIR AND Gflags_LIBRARY)
   set(Gflags_FOUND FALSE)
   set(Gflags_LIBRARIES)
endif (Gflags_INCLUDE_DIR AND Gflags_LIBRARY)

if (Gflags_FOUND)
   if (NOT Gflags_FIND_QUIETLY)
      message(STATUS "Found Gflags: ${Gflags_LIBRARY}")
   endif (NOT Gflags_FIND_QUIETLY)
else (Gflags_FOUND)
   if (Gflags_FIND_REQUIRED)
      message(STATUS "Looked for Gflags libraries named ${Gflags_NAMES}.")
      message(STATUS "Include file detected: [${Gflags_INCLUDE_DIR}].")
      message(STATUS "Lib file detected: [${Gflags_LIBRARY}].")
      message(FATAL_ERROR "=========> Could NOT find Gflags library")
   endif (Gflags_FIND_REQUIRED)
endif (Gflags_FOUND)

mark_as_advanced(
  Gflags_LIBRARY
  Gflags_INCLUDE_DIR
  )
