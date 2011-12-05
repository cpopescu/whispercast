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
# - Look for GNU flex, the lexer generator.
# Defines the following:
#  FLEX_EXECUTABLE - path to the flex executable
#  FLEX_FILE - parse a file with flex
#  FLEX_PREFIX_OUTPUTS - Set to true to make FLEX_FILE produce outputs of
#                        lex.${filename}.c, not lex.yy.c . Passes -P to flex. 

if(NOT DEFINED FLEX_PREFIX_OUTPUTS)
  set(FLEX_PREFIX_OUTPUTS FALSE)
endif(NOT DEFINED FLEX_PREFIX_OUTPUTS) 

if(NOT FLEX_EXECUTABLE)
  message(STATUS "Looking for flex")
  find_program(FLEX_EXECUTABLE flex)
  if(FLEX_EXECUTABLE)
    message(STATUS "Looking for flex -- ${FLEX_EXECUTABLE}")
  endif(FLEX_EXECUTABLE)
endif(NOT FLEX_EXECUTABLE) 

if(FLEX_EXECUTABLE)
  macro(FLEX_FILE FILENAME)
    get_filename_component(PATH "${FILENAME}" PATH)
    if("${PATH}" STREQUAL "")
      set(PATH_OPT "")
    else("${PATH}" STREQUAL "")
      set(PATH_OPT "/${PATH}")
    endif("${PATH}" STREQUAL "")
    if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}${PATH_OPT}")
      file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}${PATH_OPT}")
    endif(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}${PATH_OPT}")
    if(FLEX_PREFIX_OUTPUTS)
      get_filename_component(PREFIX "${FILENAME}" NAME_WE)
    else(FLEX_PREFIX_OUTPUTS)
      set(PREFIX "yy")
    endif(FLEX_PREFIX_OUTPUTS)
    set(OUTFILE "${CMAKE_CURRENT_BINARY_DIR}${PATH_OPT}/lex.${PREFIX}.c")
    add_custom_command(
      OUTPUT "${OUTFILE}"
      COMMAND "${FLEX_EXECUTABLE}"
      ARGS "--prefix=${PREFIX}"
      "--outfile=${OUTFILE}"
      "${CMAKE_CURRENT_SOURCE_DIR}/${FILENAME}"
      DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${FILENAME}")
    set_source_files_properties("${OUTFILE}" PROPERTIES GENERATED TRUE)
  endmacro(FLEX_FILE)
else(FLEX_EXECUTABLE)
  message(FATAL_ERROR "=========> Could NOT find Flex executable")
endif(FLEX_EXECUTABLE)
