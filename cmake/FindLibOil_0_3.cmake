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

# Finds LibOil_0_3 library
#
#  LibOil_0_3_INCLUDE_DIR - where to find liboil.h, etc.
#  LibOil_0_3_LIBRARIES   - List of libraries when using LibOil.
#  LibOil_0_3_FOUND       - True if LibOil found.
#

if (LibOil_0_3_INCLUDE_DIR)
  # Already in cache, be silent
  set(LibOil_0_3_FIND_QUIETLY TRUE)
endif (LibOil_0_3_INCLUDE_DIR)

find_path(LibOil_0_3_INCLUDE_DIR liboil/liboil.h
  /opt/local/include
  /usr/local/include
  /usr/include
  /opt/local/include/liboil-0.3
  /usr/local/include/liboil-0.3
  /usr/include/liboil-0.3
)

set(LibOil_0_3_NAMES oil-0.3)
find_library(LibOil_0_3_LIBRARY
  NAMES ${LibOil_0_3_NAMES}
  PATHS /usr/lib /usr/local/lib /opt/local/lib
)

if (LibOil_0_3_INCLUDE_DIR AND LibOil_0_3_LIBRARY)
   set(LibOil_0_3_FOUND TRUE)
   set( LibOil_0_3_LIBRARIES ${LibOil_0_3_LIBRARY} )
else (LibOil_0_3_INCLUDE_DIR AND LibOil_0_3_LIBRARY)
   set(LibOil_0_3_FOUND FALSE)
   set(LibOil_0_3_LIBRARIES)
endif (LibOil_0_3_INCLUDE_DIR AND LibOil_0_3_LIBRARY)

if (LibOil_0_3_FOUND)
   if (NOT LibOil_0_3_FIND_QUIETLY)
      message(STATUS "Found LibOil_0_3: ${LibOil_0_3_LIBRARY}")
   endif (NOT LibOil_0_3_FIND_QUIETLY)
else (LibOil_0_3_FOUND)
   if (LibOil_0_3_FIND_REQUIRED)
      message(STATUS "Looked for LibOil_0_3 libraries named ${LibOil_0_3_NAMES}.")
      message(STATUS "Include file detected: [${LibOil_0_3_INCLUDE_DIR}].")
      message(STATUS "Lib file detected: [${LibOil_0_3_LIBRARY}].")
      message(FATAL_ERROR "=========> Could NOT find LibOil_0_3 library")
   endif (LibOil_0_3_FIND_REQUIRED)
endif (LibOil_0_3_FOUND)

mark_as_advanced(
  LibOil_0_3_LIBRARY
  LibOil_0_3_INCLUDE_DIR
  )
