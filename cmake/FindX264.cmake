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
# Finds X264 library
#
#  X264_INCLUDE_DIR - where to find x264.h, etc.
#  X264_LIBRARIES   - List of libraries when using X264.
#  X264_FOUND       - True if X264 found.
#

if (X264_INCLUDE_DIR)
  # Already in cache, be silent
  set(X264_FIND_QUIETLY TRUE)
endif (X264_INCLUDE_DIR)

find_path(X264_INCLUDE_DIR x264.h
  /opt/local/include
  /usr/local/include
  /usr/include
)

set(X264_NAMES x264)
find_library(X264_LIBRARY
  NAMES ${X264_NAMES}
  PATHS /usr/lib /usr/local/lib /opt/local/lib
)

if (X264_INCLUDE_DIR AND X264_LIBRARY)
   set(X264_FOUND TRUE)
   set( X264_LIBRARIES ${X264_LIBRARY} )
else (X264_INCLUDE_DIR AND X264_LIBRARY)
   set(X264_FOUND FALSE)
   set(X264_LIBRARIES)
endif (X264_INCLUDE_DIR AND X264_LIBRARY)

if (X264_FOUND)
   if (NOT X264_FIND_QUIETLY)
      message(STATUS "Found X264: ${X264_LIBRARY}")
   endif (NOT X264_FIND_QUIETLY)
else (X264_FOUND)
   if (X264_FIND_REQUIRED)
      message(STATUS "Looked for X264 libraries named ${X264_NAMES}.")
      message(STATUS "Include file detected: [${X264_INCLUDE_DIR}].")
      message(STATUS "Lib file detected: [${X264_LIBRARY}].")
      message(FATAL_ERROR "=========> Could NOT find X264 library")
   endif (X264_FIND_REQUIRED)
endif (X264_FOUND)

mark_as_advanced(
  X264_LIBRARY
  X264_INCLUDE_DIR
  )
