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

FILE(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/auto)

SET(RPC_INPUT_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/../GigelMitica.services.rpc
  ${CMAKE_CURRENT_SOURCE_DIR}/../GigelMitica.types.rpc
  ${CMAKE_CURRENT_SOURCE_DIR}/../tribes.rpc)
SET(RPC_OUTPUT_FILES
  ${CMAKE_CURRENT_SOURCE_DIR}/auto/types.h
  ${CMAKE_CURRENT_SOURCE_DIR}/auto/types.cc
  ${CMAKE_CURRENT_SOURCE_DIR}/auto/wrappers.h
  ${CMAKE_CURRENT_SOURCE_DIR}/auto/wrappers.cc)

ADD_CUSTOM_COMMAND (
  DEPENDS ${RPC_INPUT_FILES} ${RPC_PARSER_EXE}
  OUTPUT  ${RPC_OUTPUT_FILES}
  PRE_BUILD
  COMMAND ${RPC_PARSER_EXE} ${RPC_INPUT_FILES} --cc='fClientTypes="${CMAKE_CURRENT_SOURCE_DIR}/auto/types", fClientWrappers="${CMAKE_CURRENT_SOURCE_DIR}/auto/wrappers"'
  DEPENDS ${RPC_INPUT_FILES} ${RPC_PARSER_TARGET} )

SET(EXECUTABLE rpc_tcp_client_sample)
ADD_EXECUTABLE(${EXECUTABLE}  ${RPC_OUTPUT_FILES}
  test.cc
  client_tcp.cc)
ADD_DEPENDENCIES(${EXECUTABLE}
  whisper_lib)
TARGET_LINK_LIBRARIES(${EXECUTABLE}
  whisper_lib)


SET(EXECUTABLE rpc_http_client_sample)
ADD_EXECUTABLE(${EXECUTABLE}  ${RPC_OUTPUT_FILES}
  test.cc
  client_http.cc)
ADD_DEPENDENCIES(${EXECUTABLE}
  whisper_lib)
TARGET_LINK_LIBRARIES(${EXECUTABLE}
  whisper_lib)
