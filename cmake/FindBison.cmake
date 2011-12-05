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
# - Look for GNU Bison, the parser generator
# Based off a news post from Andy Cedilnik at Kitware
# Defines the following:
#  BISON_EXECUTABLE - path to the bison executable
#  BISON_FILE - parse a file with bison
#  BISON_PREFIX_OUTPUTS - Set to true to make BISON_FILE produce prefixed
#                         symbols in the generated output based on filename.
#                         So for ${filename}.y, you'll get ${filename}parse(), etc.
#                         instead of yyparse().
#  BISON_GENERATE_DEFINES - Set to true to make BISON_FILE output the matching
#                           .h file for a .c file. You want this if you're using
#                           flex.

if(NOT DEFINED BISON_PREFIX_OUTPUTS)
  set(BISON_PREFIX_OUTPUTS FALSE)
endif(NOT DEFINED BISON_PREFIX_OUTPUTS)

if(NOT DEFINED BISON_GENERATE_DEFINES)
  set(BISON_GENERATE_DEFINES FALSE)
endif(NOT DEFINED BISON_GENERATE_DEFINES)

if(NOT BISON_EXECUTABLE)
  message(STATUS "Looking for bison")
  find_program(BISON_EXECUTABLE bison)
  if(BISON_EXECUTABLE)
    message(STATUS "Looking for bison -- ${BISON_EXECUTABLE}")
  endif(BISON_EXECUTABLE)
endif(NOT BISON_EXECUTABLE)

if(BISON_EXECUTABLE)
  macro(BISON_FILE FILENAME)
    get_filename_component(PATH "${FILENAME}" PATH)
    if("${PATH}" STREQUAL "")
      set(PATH_OPT "")
    else("${PATH}" STREQUAL "")
      set(PATH_OPT "/${PATH}")
    endif("${PATH}" STREQUAL "")

    get_filename_component(HEAD "${FILENAME}" NAME_WE)
    if(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}${PATH_OPT}")
      file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}${PATH_OPT}")
    endif(NOT EXISTS "${CMAKE_CURRENT_BINARY_DIR}${PATH_OPT}")

    if(BISON_PREFIX_OUTPUTS)
      set(PREFIX "${HEAD}")
    else(BISON_PREFIX_OUTPUTS)
      set(PREFIX "yy")
    endif(BISON_PREFIX_OUTPUTS)

    set(OUTFILE "${CMAKE_CURRENT_BINARY_DIR}${PATH_OPT}/${HEAD}.tab.c")
    if(BISON_GENERATE_DEFINES)
      set(HEADER "${CMAKE_CURRENT_BINARY_DIR}${PATH_OPT}/${HEAD}.tab.h")
      add_custom_command(
        OUTPUT "${OUTFILE}" "${HEADER}"
        COMMAND "${BISON_EXECUTABLE}"
        ARGS "--name-prefix=${PREFIX}"
        "--defines"
        "--output-file=${OUTFILE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/${FILENAME}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${FILENAME}")
      set_source_files_properties("${OUTFILE}" "${HEADER}"
        PROPERTIES GENERATED TRUE)
      set_source_files_properties("${HEADER}" PROPERTIES HEADER_FILE_ONLY TRUE)
    else(BISON_GENERATE_DEFINES)
      add_custom_command(
        OUTPUT "${OUTFILE}"
        COMMAND "${BISON_EXECUTABLE}"
        ARGS "--name-prefix=${PREFIX}"
        "--output-file=${OUTFILE}"
        "${CMAKE_CURRENT_SOURCE_DIR}/${FILENAME}"
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${FILENAME}")
      set_source_files_properties("${OUTFILE}" PROPERTIES GENERATED TRUE)
    endif(BISON_GENERATE_DEFINES)
  endmacro(BISON_FILE)
else(BISON_EXECUTABLE)
  message(FATAL_ERROR "=========> Could NOT find Bison executable")
endif(BISON_EXECUTABLE)
