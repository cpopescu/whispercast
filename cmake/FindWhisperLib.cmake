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
# Finds whisper_lib library
#
#  whisperlib_INCLUDE_DIR - where to find faad.h, etc.
#  whisperlib_LIBRARIES   - List of libraries when using whisperlib.
#  whisperlib_FOUND       - True if whisperlib found.
#

if (whisperlib_INCLUDE_DIR)
else (whisperlib_INCLUDE_DIR)
  find_path(whisperlib_INCLUDE_DIR whisperlib/common/base/types.h
    /opt/local/include
    /usr/local/include
    /usr/include
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_INSTALL_PREFIX}/include
  )
endif (whisperlib_INCLUDE_DIR)

if (whisperlib_LIBRARY)
else (whisperlib_LIBRARY)
  set(whisperlib_NAMES whisper_lib)
  find_library(whisperlib_LIBRARY
    NAMES ${whisperlib_NAMES}
    PATHS /usr/lib /usr/local/lib /opt/local/lib ${CMAKE_BINARY_DIR}/whisperlib ${CMAKE_INSTALL_PREFIX}/lib
  )
endif (whisperlib_LIBRARY)

if (whisperlib_INCLUDE_DIR AND whisperlib_LIBRARY)
   set(whisperlib_FOUND TRUE)
   set(whisperlib_LIBRARIES ${whisperlib_LIBRARY} )
else (whisperlib_INCLUDE_DIR AND whisperlib_LIBRARY)
   set(whisperlib_FOUND FALSE)
   set(whisperlib_LIBRARIES)
endif (whisperlib_INCLUDE_DIR AND whisperlib_LIBRARY)

if (whisperlib_FOUND)
   message(STATUS "====> Found whisperlib: ${whisperlib_LIBRARY}")
else (whisperlib_FOUND)
  message(STATUS "Looked for whisperlib libraries named ${whisperlib_NAMES}.")
  message(STATUS "Include file detected: [${whisperlib_INCLUDE_DIR}].")
  message(STATUS "Lib file detected: [${whisperlib_LIBRARY}].")
  message(FATAL_ERROR "=========> Could NOT find whisperlib library")
endif (whisperlib_FOUND)

mark_as_advanced(
  whisperlib_LIBRARY
  whisperlib_INCLUDE_DIR
  )
