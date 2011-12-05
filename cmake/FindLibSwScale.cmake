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

# Finds SwScale library
#
#  SwScale_INCLUDE_DIR - where to find swscale.h, etc.
#  SwScale_LIBRARIES   - List of libraries when using SwScale.
#  SwScale_FOUND       - True if SwScale found.
#

find_path(SwScale_INCLUDE_DIR1 libswscale/swscale.h
  /opt/local/include
  /usr/local/include
  /usr/include
  )
find_path(SwScale_INCLUDE_DIR2 ffmpeg/swscale.h
  /opt/local/include
  /usr/local/include
  /usr/include
  )
if ( SwScale_INCLUDE_DIR1 )
  set (SW_SCALE_INCLUDE_FILE libswscale/swscale.h)
  set (SwScale_INCLUDE_DIR ${SwScale_INCLUDE_DIR1})
else  ( SwScale_INCLUDE_DIR1 )
  if ( SwScale_INCLUDE_DIR2 )
    set (SW_SCALE_INCLUDE_FILE ffmpeg/swscale.h)
    set (SwScale_INCLUDE_DIR ${SwScale_INCLUDE_DIR2})
  endif  ( SwScale_INCLUDE_DIR2 )
endif  ( SwScale_INCLUDE_DIR1 )

set(SwScale_NAMES swscale)
find_library(SwScale_LIBRARY
  NAMES ${SwScale_NAMES}
  PATHS /usr/lib /usr/local/lib /opt/local/lib
)

if (SwScale_INCLUDE_DIR AND SwScale_LIBRARY)
   set(SwScale_FOUND TRUE)
   set( SwScale_LIBRARIES ${SwScale_LIBRARY} )
else (SwScale_INCLUDE_DIR AND SwScale_LIBRARY)
   set(SwScale_FOUND FALSE)
   set(SwScale_LIBRARIES)
endif (SwScale_INCLUDE_DIR AND SwScale_LIBRARY)

if (SwScale_FOUND)
   if (NOT SwScale_FIND_QUIETLY)
      message(STATUS "Found SwScale: ${SwScale_LIBRARY}")
   endif (NOT SwScale_FIND_QUIETLY)
else (SwScale_FOUND)
   if (SwScale_FIND_REQUIRED)
      message(STATUS "Looked for SwScale libraries named ${SwScale_NAMES}.")
      message(STATUS "Include file detected: [${SwScale_INCLUDE_DIR}].")
      message(STATUS "Lib file detected: [${SwScale_LIBRARY}].")
      message(FATAL_ERROR "=========> Could NOT find SwScale library")
   endif (SwScale_FIND_REQUIRED)
endif (SwScale_FOUND)

mark_as_advanced(
  SwScale_LIBRARY
  SwScale_INCLUDE_DIR
  )
