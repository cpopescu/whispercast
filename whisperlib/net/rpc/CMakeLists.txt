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

ADD_SUBDIRECTORY (parser)
if (NOT RPC_TESTS STREQUAL "None")
  ADD_SUBDIRECTORY (tests)
endif (NOT RPC_TESTS STREQUAL "None")

FILE(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX_SCRIPTS}/php/rpc)
install(FILES
  php/rpc_base.php
  php/rpc_common.php
  php/rpc_connection.php
  php/rpc_json.php
  php/rpc_log.php
  php/rpc_types.php
  DESTINATION ${CMAKE_INSTALL_PREFIX_SCRIPTS}/php/rpc)

if (WITH_ADMIN)
  FILE(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX_ADMIN}/rpc)
  install(FILES
    php/rpc_base.php
    php/rpc_common.php
    php/rpc_connection.php
    php/rpc_json.php
    php/rpc_log.php
    php/rpc_types.php
    DESTINATION ${CMAKE_INSTALL_PREFIX_ADMIN}/rpc)
endif (WITH_ADMIN)

FILE(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX_SCRIPTS}/js/rpc)
install(FILES
  js/log.js
  js/rpc_base.js
  js/rpc_connection.js
  js/rpc_json.js
  js/rpc_standard.js
  js/rpc_types.js
  js/auto_forms_log.html
  DESTINATION ${CMAKE_INSTALL_PREFIX_SCRIPTS}/js/rpc)

FILE(MAKE_DIRECTORY ${CMAKE_INSTALL_PREFIX_SCRIPTS}/python/rpc)
install(FILES
  python/whisperrpc.py
  DESTINATION ${CMAKE_INSTALL_PREFIX_SCRIPTS}/python/rpc)
