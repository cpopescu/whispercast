#!/bin/bash
#
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

PERF_SERVER_EXECUTORS=10
#  if you want more, increase the TCP connect timeout in
#  rpc_client_connection_http.cc:38
PERF_CLIENT_THREADS=500
PERF_CLIENT_SECONDS=30

function KillFromCommand {
  CMD=$1

  BINARY=$(basename /$(echo $CMD | awk '{print $1}'))
  PIDS=$(ps --no-headers -o pid= -C $BINARY)
  [[ -n $PIDS ]] && kill $PIDS && sleep 2 && kill -9 $PIDS
}
function KillWithWait {
  PID=$1
  kill -2 $PID

  WAIT=10
  i=0
  while [[ $i -lt $WAIT && `ps h -p $SERVER_PID` ]]; do
    echo -n "."
    sleep 1
    let "i+=1"
  done
}

function RpcTest {
  NOLOG=0
  [[ "$1" == "nolog" ]] && NOLOG=1 && shift
  TEST_NAME=$1
  SERVER_CMD=$2
  CLIENT_CMD=$3
  [[ -z $TEST_NAME ]] && \
      echo "[$0] missing TEST_NAME (first parameter)" && exit -1
  [[ -z $CLIENT_CMD ]] && \
      echo "[$0] missing CLIENT_CMD (second parameter)" && exit -1
  [[ -z $SERVER_CMD ]] && \
      echo "[$0] missing SERVER_CMD (third parameter)" && exit -1

  if [[ $NOLOG -eq 0 ]]; then
    CLIENT_LOG="run_all.$TEST_NAME.client.log"
    SERVER_LOG="run_all.$TEST_NAME.server.log"
    [[ -e $CLIENT_LOG ]] && \
        echo "[$0] [$TEST_NAME] removing $CLIENT_LOG" && rm -f $CLIENT_LOG;
    [[ -e $SERVER_LOG ]] && \
        echo "[$0] [$TEST_NAME] removing $SERVER_LOG" && rm -f $SERVER_LOG;
  else
    CLIENT_LOG="/dev/null"
    SERVER_LOG="/dev/null"
  fi

  # Killing any old instances..
  KillFromCommand $SERVER_CMD
  KillFromCommand $CLIENT_CMD

  echo -n "[$0] [$TEST_NAME] Starting server.. "
  echo $SERVER_CMD
  $SERVER_CMD &> $SERVER_LOG &
  SERVER_PID=$!

  echo -n "pid=$SERVER_PID .. "

  sleep 5
  if [[ -z `ps h -p $SERVER_PID` ]]; then
    echo -e "\033[31mFailed\033[0m."
    KillFromCommand $SERVER_CMD
    KillFromCommand $CLIENT_CMD
    exit -1
  fi

  echo -e "\033[32mDone\033[0m."

  echo "[$0] [$TEST_NAME] Running client.."
  echo $CLIENT_CMD
  `$CLIENT_CMD &> $CLIENT_LOG`
  CLIENT_RET=$?

  if [[ $CLIENT_RET != 0 ]]; then
    echo -e "[$0] [$TEST_NAME] \033[31mFailed. "\
            "Client return code: $CLIENT_RET \033[0m"
    KillFromCommand $SERVER_CMD
    KillFromCommand $CLIENT_CMD
    exit -1
  else
    echo -e "[$0] [$TEST_NAME] \033[32mSucceeded. "\
            "Client return code: $CLIENT_RET \033[0m"
  fi

  if [[ -z `ps h -p $SERVER_PID` ]]; then
    echo "[$0] [$TEST_NAME] server died unexpectedly"
    KillFromCommand $SERVER_CMD
    KillFromCommand $CLIENT_CMD
    exit -1
  fi

  echo -n "[$0] [$TEST_NAME] Stopping server.. "
  KillWithWait  $SERVER_PID


  if [[ `ps h -p $SERVER_PID` ]]; then
      echo -e "\033[31mFailed\033[0m."
      KillFromCommand $SERVER_CMD
      KillFromCommand $CLIENT_CMD
      exit -1
  fi
  KillFromCommand $SERVER_CMD
  KillFromCommand $CLIENT_CMD

  echo -e "\033[32mDone\033[0m.";

  echo -e "[$0] [$TEST_NAME] \033[32mTest OK. \033[0m"
}
