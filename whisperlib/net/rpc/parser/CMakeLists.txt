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

project (whisperlib)

find_package(Flex REQUIRED)

SET(RPC_PARSER_DIR ${CMAKE_CURRENT_SOURCE_DIR}
  CACHE INTERNAL "rpc parser source directory")

SET(RPC_PARSER_EXE ${CMAKE_CURRENT_BINARY_DIR}/rpc_parser
    CACHE INTERNAL "rpc parser executable")

ADD_CUSTOM_COMMAND (
  DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/rpc-parser.yy
  OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/rpc-parser.cc
  PRE_BUILD
  COMMAND ${FLEX_EXECUTABLE} -+ -o ${CMAKE_CURRENT_SOURCE_DIR}/rpc-parser.cc ${CMAKE_CURRENT_SOURCE_DIR}/rpc-parser.yy
  )

ADD_EXECUTABLE (rpc_parser
  export/rpc-exporter.cc
  export/rpc-base-types.cc
  export/C++/rpc-exporter-cc.cc
  export/Javascript/rpc-exporter-js.cc
  export/Php/rpc-exporter-php.cc
  export/Python/rpc-exporter-python.cc
  common.cc
  rpc-ptypes.cc
  rpc-ptypes-check.cc
  rpc-parser.cc
  main.cc)


ADD_DEPENDENCIES(rpc_parser whisper_lib)
TARGET_LINK_LIBRARIES(rpc_parser whisper_lib)

INSTALL (TARGETS rpc_parser
         DESTINATION bin)
