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
##
## Some common cmake junk used in all our root CMakeLists.txt
##

if (NOT INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "$ENV{HOME}/whispercast"
    CACHE PATH "installation directory prefix" FORCE)
else (NOT INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "${INSTALL_PREFIX}"
    CACHE PATH "installation directory prefix" FORCE)
endif (NOT INSTALL_PREFIX)

if (NOT IS_ABSOLUTE ${CMAKE_INSTALL_PREFIX})
  message(FATAL_ERROR "INSTALL_PREFIX: [${INSTALL_PREFIX}] is not an absolute path")
endif (NOT IS_ABSOLUTE ${CMAKE_INSTALL_PREFIX})

message(STATUS "Installing to: " ${CMAKE_INSTALL_PREFIX})


set(CMAKE_INSTALL_PREFIX_ETC "etc"
  CACHE PATH "installation directory prefix (etc)" FORCE)
set(CMAKE_INSTALL_PREFIX_VAR "var"
  CACHE PATH "installation directory prefix (var)" FORCE)
set(CMAKE_INSTALL_PREFIX_WWW "var/www"
  CACHE PATH "installation directory prefix (www)" FORCE)
set(CMAKE_INSTALL_PREFIX_SCRIPTS "scripts"
  CACHE PATH "installation directory prefix (scripts)" FORCE)
set(CMAKE_INSTALL_PREFIX_ADMIN "admin"
  CACHE PATH "installation directory prefix (admin)" FORCE)

include_directories("/usr/include")
include_directories("/opt/local/include")
include_directories("/usr/local/include")

######################################################################

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING
      "Options: Debug Release." FORCE)
endif (NOT CMAKE_BUILD_TYPE)

if (NOT RPC_TESTS)
  set(RPC_TESTS "None" CACHE STRING
      "Options: No RPC Tests (use RPC_TESTS=All or RPS_TESTS=Some." FORCE)
endif (NOT RPC_TESTS)

if (WITH_GSTREAMER AND NOT APPLE)
  add_definitions(-D__WITH_GSTREAMER__)
  set(WITH_GSTREAMER 1 CACHE STRING
      "Options: Enabling gstreamer dependent stuff." FORCE)
  message(STATUS "=====> WITH gstreamer ")
else (WITH_GSTREAMER AND NOT APPLE)
  add_definitions(-D__NO_GSTREAMER__)
  set(WITH_GSTREAMER 0 CACHE STRING
      "Options: Disabling gstreamer dependent stuff." FORCE)
  message(STATUS "=====> NO gstreamer ")
endif (WITH_GSTREAMER AND NOT APPLE)


if (WITH_ADMIN)
  set(WITH_ADMIN 1 CACHE STRING
      "Options: Enabling administration interface." FORCE)
  message(STATUS "=====> WITH administration interface ")
else (WITH_ADMIN)
  set(WITH_ADMIN 0 CACHE STRING
      "Options: Disabling administration interface." FORCE)
  message(STATUS "=====> NO administration interface ")
endif (WITH_ADMIN)

#
# Generic definitions for all build types
#
remove_definitions(-Wall -Wno-sign-compare -Werror)
remove_definitions(-D__STDC_FORMAT_MACROS)
add_definitions(-Wall -Wno-sign-compare -Werror)
add_definitions(-D__STDC_FORMAT_MACROS)

if (APPLE)
  remove_definitions(-D__APPLE__ -Wno-deprecated-declarations)
  add_definitions(-D__APPLE__ -Wno-deprecated-declarations)
endif (APPLE)

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "Compiling in Debug mode.")
  set(CMAKE_CXX_FLAGS_DEBUG
    "${CMAKE_CXX_FLAGS_DEBUG} -D _FILE_OFFSET_BITS=64 -ggdb3 -D _DEBUG")
else (CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "Compiling in Release mode.")
  set(CMAKE_CXX_FLAGS_RELEASE
    "${CMAKE_CXX_FLAGS_RELEASE} -D _FILE_OFFSET_BITS=64 -ggdb3 -O2 -fomit-frame-pointer")
endif (CMAKE_BUILD_TYPE STREQUAL "Debug")

if (APPLE)
   SET(CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS
       "${CMAKE_SHARED_MODULE_CREATE_CXX_FLAGS} -flat_namespace -undefined suppress")
   set (COMMON_LINK_LIBRARIES pthread glog gflags stdc++)
else (APPLE)
   set (COMMON_LINK_LIBRARIES pthread glog gflags crypt)
endif (APPLE)

######################################################################

find_package(Epoll)
if (EPOLL_FOUND)
  message(STATUS "====> Using epoll")
  set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D __USE_EPOLL__")
  set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -D __USE_EPOLL__")
else (EPOLL_FOUND)
  message(STATUS "====> Using just poll - WARNING - performance not that good")
  set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D __USE_POLL__")
  set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -D __USE_POLL__")
endif (EPOLL_FOUND)

######################################################################

find_package(EventFd)
if (EVENTFD_FOUND)
  message(STATUS "====> Using eventfd")
  set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D __USE_EVENTFD__")
  set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -D __USE_EVENTFD__")
endif(EVENTFD_FOUND)

######################################################################

find_package(Icu REQUIRED)

if (Icu_LIBRARY_PATH)
  set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${Icu_LIBRARY_PATH}")
  set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${Icu_LIBRARY_PATH}")
endif (Icu_LIBRARY_PATH)

######################################################################

find_package(Gflags REQUIRED)
if (Gflags_LIBRARY_PATH)
  set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${Gflags_LIBRARY_PATH}")
  set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${Gflags_LIBRARY_PATH}")
endif (Gflags_LIBRARY_PATH)

find_package(Glog REQUIRED)
if (Glog_LIBRARY_PATH)
  set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${Glog_LIBRARY_PATH}")
  set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${Glog_LIBRARY_PATH}")
endif (Glog_LIBRARY_PATH)

######################################################################

if ( ENABLE_BFD )
  find_package(Bfd)
endif ( ENABLE_BFD )

if ( Bfd_FOUND )
  message(STATUS "====> Enabling BFD.")
  set( COMMON_LINK_LIBRARIES "${COMMON_LINK_LIBRARIES} ${Bfd_LIBRARY}")
else (Bfd_FOUND)
  message(STATUS "====> Disabling BFD.")
  set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D _NO_BFD")
  set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -D _NO_BFD")
endif (Bfd_FOUND)

######################################################################

find_package(GooglePerfTools)

if (profiler_LIBRARY)
  message(STATUS "====> Enabling profiler")
  set( COMMON_LINK_LIBRARIES "${COMMON_LINK_LIBRARIES} ${profiler_LIBRARIES}")
endif (profiler_LIBRARY)

if (tcmalloc_LIBRARY)
  message(STATUS "====> Enabling tcmalloc")
  set( COMMON_LINK_LIBRARIES "${COMMON_LINK_LIBRARIES} ${tcmalloc_LIBRARIES}")
endif (tcmalloc_LIBRARY)

if (google_perftools_INCLUDE_DIR)
  message(STATUS "====> Enabling google-perftools")
  include_directories (${google_perftools_INCLUDE_DIR})
  set (CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D _HAVE_GOOGLE_PERFTOOLS")
  set (CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -D _HAVE_GOOGLE_PERFTOOLS")
endif (google_perftools_INCLUDE_DIR)

######################################################################

## Simple macro for test definition

macro(WHISPER_TEST NAME SRCS DEPS LIBS)
  add_executable(${NAME} ${SRCS})
  add_dependencies(${NAME} ${DEPS})
  target_link_libraries(${NAME} ${LIBS})
 add_test(${NAME} ${NAME})
endmacro(WHISPER_TEST)

macro(WHISPER_ADD_LIBRARY NAME SRCS)
  add_library(${NAME} STATIC ${SRCS})
  set_target_properties(${NAME} PROPERTIES SOURCES ${SRCS})
endmacro(WHISPER_ADD_LIBRARY)

######################################################################

exec_program(date ARGS +%s OUTPUT_VARIABLE BUILD_RAND_SEED)

######################################################################

enable_testing()
