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
# Finds GoolgePerfTools:
#   - tcmalloc
#   - stacktrace
#   - profiler
find_path(google_perftools_INCLUDE_DIR google/heap-profiler.h
  /usr/local/include
  /usr/include
  )

# set(tcmalloc_NAMES ${tcmalloc_NAMES} tcmalloc_debug)
set(tcmalloc_NAMES ${tcmalloc_NAMES} tcmalloc)
find_library(tcmalloc_LIBRARY
  NAMES ${tcmalloc_NAMES}
  PATHS /usr/lib /usr/local/lib
  )

set(stacktrace_NAMES ${stacktrace_NAMES} stacktrace)
find_library(stacktrace_LIBRARY
  NAMES ${stacktrace_NAMES}
  PATHS /usr/lib /usr/local/lib
  )

if (stacktrace_LIBRARY AND google_perftools_INCLUDE_DIR)
  set(stacktrace_LIBRARIES ${stacktrace_LIBRARY})
endif (stacktrace_LIBRARY AND google_perftools_INCLUDE_DIR)

set(profiler_NAMES ${profiler_NAMES} profiler)
find_library(profiler_LIBRARY
  NAMES ${profiler_NAMES}
  PATHS /usr/lib /usr/local/lib
  )

if (profiler_LIBRARY AND google_perftools_INCLUDE_DIR)
  set(profiler_LIBRARIES ${profiler_LIBRARY})
endif (profiler_LIBRARY AND google_perftools_INCLUDE_DIR)

if (tcmalloc_LIBRARY AND google_perftools_INCLUDE_DIR)
  set(tcmalloc_LIBRARIES ${tcmalloc_LIBRARY})
  set(google_perftools_FOUND TRUE)
else (tcmalloc_LIBRARY AND google_perftools_INCLUDE_DIR)
  set(google_perftools_FOUND FALSE)
endif (tcmalloc_LIBRARY AND google_perftools_INCLUDE_DIR)

if (google_perftools_FOUND)
  if (NOT google_perftools_FIND_QUIETLY)
    message(STATUS "Found Google perftools: ${google_perftools_LIBRARIES}")
  endif (NOT google_perftools_FIND_QUIETLY)
else (google_perftools_FOUND)
  if (google_perftools_FIND_REQUIRED)
    message(FATAL_ERROR "Could not find Google perftools library")
  endif (google_perftools_FIND_REQUIRED)
endif (google_perftools_FOUND)

mark_as_advanced(
  tcmalloc_LIBRARY
  stacktrace_LIBRARY
  profiler_LIBRARY
  google_perftools_INCLUDE_DIR
  )
