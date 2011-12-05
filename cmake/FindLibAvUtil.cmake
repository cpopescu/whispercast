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

# Finds AvUtil library
#
#  AvUtil_INCLUDE_DIR - where to find avutil.h, etc.
#  AvUtil_LIBRARIES   - List of libraries when using AvUtil.
#  AvUtil_FOUND       - True if AvUtil found.
#

find_path(AvUtil_INCLUDE_DIR1 libavutil/avutil.h
  /opt/local/include
  /usr/local/include
  /usr/include
  )
find_path(AvUtil_INCLUDE_DIR2 ffmpeg/avutil.h
  /opt/local/include
  /usr/local/include
  /usr/include
  )
if ( AvUtil_INCLUDE_DIR1 )
  set (AV_UTIL_INCLUDE_FILE libavutil/avutil.h)
  set (AvUtil_INCLUDE_DIR ${AvUtil_INCLUDE_DIR1})
else ( AvUtil_INCLUDE_DIR1 )
  if ( AvUtil_INCLUDE_DIR2 )
    set (AV_UTIL_INCLUDE_FILE ffmpeg/avutil.h)
    set (AvUtil_INCLUDE_DIR ${AvUtil_INCLUDE_DIR2})
  endif  ( AvUtil_INCLUDE_DIR2 )
endif ( AvUtil_INCLUDE_DIR1 )

set(AvUtil_NAMES avutil)
find_library(AvUtil_LIBRARY
  NAMES ${AvUtil_NAMES}
  PATHS /usr/lib /usr/local/lib /opt/local/lib
)

if (AvUtil_INCLUDE_DIR AND AvUtil_LIBRARY)
   set(AvUtil_FOUND TRUE)
   set( AvUtil_LIBRARIES ${AvUtil_LIBRARY} )
else (AvUtil_INCLUDE_DIR AND AvUtil_LIBRARY)
   set(AvUtil_FOUND FALSE)
   set(AvUtil_LIBRARIES)
endif (AvUtil_INCLUDE_DIR AND AvUtil_LIBRARY)

if (AvUtil_FOUND)
   if (NOT AvUtil_FIND_QUIETLY)
      message(STATUS "Found AvUtil: ${AvUtil_LIBRARY}")
   endif (NOT AvUtil_FIND_QUIETLY)
else (AvUtil_FOUND)
   if (AvUtil_FIND_REQUIRED)
      message(STATUS "Looked for AvUtil libraries named ${AvUtil_NAMES}.")
      message(STATUS "Include file detected: [${AvUtil_INCLUDE_DIR}].")
      message(STATUS "Lib file detected: [${AvUtil_LIBRARY}].")
      message(FATAL_ERROR "=========> Could NOT find AvUtil library")
   endif (AvUtil_FIND_REQUIRED)
endif (AvUtil_FOUND)

mark_as_advanced(
  AvUtil_LIBRARY
  AvUtil_INCLUDE_DIR
  )
