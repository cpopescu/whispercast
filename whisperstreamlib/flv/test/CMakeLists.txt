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

project (whisperstreamlib)

ADD_EXECUTABLE(flv_join_files
  flv_join_files.cc)
ADD_DEPENDENCIES(flv_join_files
  whisper_lib
  whisper_streamlib)
TARGET_LINK_LIBRARIES(flv_join_files
  whisper_lib
  whisper_streamlib)

ADD_EXECUTABLE(flv_split_file
  flv_split_file.cc)
ADD_DEPENDENCIES(flv_split_file
  whisper_lib
  whisper_streamlib)
TARGET_LINK_LIBRARIES(flv_split_file
  whisper_lib
  whisper_streamlib)

ADD_CUSTOM_TARGET(flv_tag_test)
ADD_DEPENDENCIES(flv_tag_test media_file_printer)
ADD_TEST(flv_tag_test ${CMAKE_CURRENT_SOURCE_DIR}/flv_tag_test.sh ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_BINARY_DIR}/whisperstreamlib/utils)

ADD_CUSTOM_TARGET(flv_join_test)
ADD_DEPENDENCIES(flv_join_test flv_join_files)
if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test_data/join_test.flv)
  ADD_TEST(flv_join_test ${CMAKE_CURRENT_SOURCE_DIR}/flv_join_test.sh ${CMAKE_CURRENT_SOURCE_DIR} ${CMAKE_CURRENT_BINARY_DIR})
endif (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/test_data/join_test.flv)

##############################

# Actually these utilities are rather handy ...
INSTALL (TARGETS flv_join_files
         DESTINATION bin)
INSTALL (TARGETS flv_split_file
         DESTINATION bin)
