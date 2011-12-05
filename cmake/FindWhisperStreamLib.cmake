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
# Finds whisper_medialib library
#
#  whisperstreamlib_INCLUDE_DIR - where to find faad.h, etc.
#  whisperstreamlib_LIBRARIES   - List of libraries when using whisperstreamlib.
#  whisperstreamlib_FOUND       - True if whisperstreamlib found.
#

if (whisperstreamlib_INCLUDE_DIR)
else (whisperstreamlib_INCLUDE_DIR)
  find_path(whisperstreamlib_INCLUDE_DIR whisperstreamlib/base/consts.h
    /opt/local/include
    /usr/local/include
    /usr/include
    ${CMAKE_INSTALL_PREFIX}/include
    ${CMAKE_SOURCE_DIR}

)
endif (whisperstreamlib_INCLUDE_DIR)


if (whisperstreamlib_LIBRARY)
else (whisperstreamlib_LIBRARY)
  set(whisperstreamlib_NAMES whisper_streamlib)
  find_library(whisperstreamlib_LIBRARY
    NAMES ${whisperstreamlib_NAMES}
    PATHS /usr/lib /usr/local/lib /opt/local/lib ${CMAKE_BINARY_DIR}/whisperstreamlib ${CMAKE_INSTALL_PREFIX}/lib
  )
endif (whisperstreamlib_LIBRARY)

if (whisperstreamlib_INCLUDE_DIR AND whisperstreamlib_LIBRARY)
   set(whisperstreamlib_FOUND TRUE)
   set(whisperstreamlib_LIBRARIES ${whisperstreamlib_LIBRARY} )
else (whisperstreamlib_INCLUDE_DIR AND whisperstreamlib_LIBRARY)
   set(whisperstreamlib_FOUND FALSE)
   set(whisperstreamlib_LIBRARIES)
endif (whisperstreamlib_INCLUDE_DIR AND whisperstreamlib_LIBRARY)

if (whisperstreamlib_FOUND)
   message(STATUS "====> Found whisperstreamlib: ${whisperstreamlib_LIBRARY}")
else (whisperstreamlib_FOUND)
  message(STATUS "Looked for whisperstreamlib libraries named ${whisperstreamlib_NAMES}.")
  message(STATUS "Include file detected: [${whisperstreamlib_INCLUDE_DIR}].")
  message(STATUS "Lib file detected: [${whisperstreamlib_LIBRARY}].")
  message(FATAL_ERROR "=========> Could NOT find whisperstreamlib library")
endif (whisperstreamlib_FOUND)

mark_as_advanced(
  whisperstreamlib_LIBRARY
  whisperstreamlib_INCLUDE_DIR
  )
