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

ADD_EXECUTABLE(f4v_coder_test
  f4v_coder_test.cc)
ADD_DEPENDENCIES(f4v_coder_test
  whisper_lib
  whisper_streamlib)
TARGET_LINK_LIBRARIES(f4v_coder_test
  whisper_lib
  whisper_streamlib)
ADD_TEST(f4v_coder_test ${CMAKE_CURRENT_SOURCE_DIR}/f4v_coder_test.sh
                        ${CMAKE_CURRENT_BINARY_DIR}/f4v_coder_test
                        ${CMAKE_CURRENT_SOURCE_DIR}/test_data)

ADD_EXECUTABLE(f4v_fixer
  f4v_fixer.cc)
ADD_DEPENDENCIES(f4v_fixer
  whisper_lib
  whisper_streamlib)
TARGET_LINK_LIBRARIES(f4v_fixer
  whisper_lib
  whisper_streamlib)
INSTALL (TARGETS f4v_fixer
         DESTINATION bin)
