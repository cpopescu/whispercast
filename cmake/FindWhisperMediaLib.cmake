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
#  whispermedialib_INCLUDE_DIR - where to find faad.h, etc.
#  whispermedialib_LIBRARIES   - List of libraries when using whispermedialib.
#  whispermedialib_FOUND       - True if whispermedialib found.
#

if (whispermedialib_INCLUDE_DIR)
else (whispermedialib_INCLUDE_DIR)
  find_path(whispermedialib_INCLUDE_DIR whispermedialib/media/streaming.h
    /opt/local/include
    /usr/local/include
    /usr/include
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_INSTALL_PREFIX}/include
  )
endif (whispermedialib_INCLUDE_DIR)

if (whispermedialib_LIBRARY)
else (whispermedialib_LIBRARY)
  set(whispermedialib_NAMES whisper_medialib)
  find_library(whispermedialib_LIBRARY
    NAMES ${whispermedialib_NAMES}
    PATHS /usr/lib /usr/local/lib /opt/local/lib ${CMAKE_BINARY_DIR}/whispermedialib ${CMAKE_INSTALL_PREFIX}/lib
  )
endif (whispermedialib_LIBRARY)

if (whispermedialib_INCLUDE_DIR AND whispermedialib_LIBRARY)
   set(whispermedialib_FOUND TRUE)
   set(whispermedialib_LIBRARIES ${whispermedialib_LIBRARY} )
else (whispermedialib_INCLUDE_DIR AND whispermedialib_LIBRARY)
   set(whispermedialib_FOUND FALSE)
   set(whispermedialib_LIBRARIES)
endif (whispermedialib_INCLUDE_DIR AND whispermedialib_LIBRARY)

if (whispermedialib_FOUND)
   message(STATUS "====> Found whispermedialib: ${whispermedialib_LIBRARY}")
else (whispermedialib_FOUND)
  message(STATUS "Looked for whispermedialib libraries named ${whispermedialib_NAMES}.")
  message(STATUS "Include file detected: [${whispermedialib_INCLUDE_DIR}].")
  message(STATUS "Lib file detected: [${whispermedialib_LIBRARY}].")
  message(FATAL_ERROR "=========> Could NOT find whispermedialib library")
endif (whispermedialib_FOUND)

mark_as_advanced(
  whispermedialib_LIBRARY
  whispermedialib_INCLUDE_DIR
  )
