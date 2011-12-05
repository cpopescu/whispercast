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

make
if [ $? != 0 ]; then
  echo "[$0] Make failed"
  exit -1
fi

source rpc_test_base.sh

echo -e "[$0] \033[34;1mPerformance settings:\033[0m"
echo -e "[$0] \033[34;1m\t\tPERF_SERVER_EXECUTORS = $PERF_SERVER_EXECUTORS\033[0m"
echo -e "[$0] \033[34;1m\t\tPERF_CLIENT_THREADS = $PERF_CLIENT_THREADS\033[0m"
echo -e "[$0] \033[34;1m\t\tPERF_CLIENT_SECONDS = $PERF_CLIENT_SECONDS\033[0m"

RpcTest sample_tcp \
        "Client-Server-sample/server/rpc_tcp_server_sample --alsologtostderr --loglevel 4 --logcolors"\
        "Client-Server-sample/client/rpc_tcp_client_sample --alsologtostderr --loglevel 4 --logcolors"

RpcTest sample_http \
        "Client-Server-sample/server/rpc_http_server_sample --alsologtostderr --loglevel 4 --logcolors"\
        "Client-Server-sample/client/rpc_http_client_sample --alsologtostderr --loglevel 4 --logcolors"

RpcTest performance_conformance_tcp \
        "Client-Server-performance/server/rpc_server_tcp_performance --alsologtostderr --loglevel 4 --logcolors --nExecutors 1"\
        "Client-Server-performance/client/rpc_client_tcp_performance --alsologtostderr --loglevel 4 --logcolors --nThreads 1 --nSeconds 10"

RpcTest performance_conformance_http \
        "Client-Server-performance/server/rpc_server_http_performance --alsologtostderr --loglevel 4 --logcolors --nExecutors 1"\
        "Client-Server-performance/client/rpc_client_http_performance --alsologtostderr --loglevel 4 --logcolors --nThreads 1 --nSeconds 10"

echo -e "[$0] \033[34;1mPerformance conformance looks OK. Running true performance test..\033[0m"

RpcTest performance_tcp \
        "Client-Server-performance/server/rpc_server_tcp_performance --alsologtostderr --loglevel 0 --logcolors --nExecutors $PERF_SERVER_EXECUTORS"\
        "Client-Server-performance/client/rpc_client_tcp_performance --alsologtostderr --loglevel 0 --logcolors --nThreads $PERF_CLIENT_THREADS --nSeconds $PERF_CLIENT_SECONDS"

grep "Performance: " run_all.performance_tcp.client.log

RpcTest performance_tcp_simple \
        "Client-Server-performance/server/rpc_server_tcp_simple_performance --alsologtostderr --loglevel 0 --logcolors"\
        "Client-Server-performance/client/rpc_client_tcp_performance --alsologtostderr --loglevel 0 --logcolors --nThreads $PERF_CLIENT_THREADS --nSeconds $PERF_CLIENT_SECONDS"

grep "Performance: " run_all.performance_tcp.client.log

RpcTest performance_http \
        "Client-Server-performance/server/rpc_server_http_performance --alsologtostderr --loglevel 0 --logcolors --nExecutors $PERF_SERVER_EXECUTORS"\
        "Client-Server-performance/client/rpc_client_http_performance --alsologtostderr --loglevel 0 --logcolors --nThreads $PERF_CLIENT_THREADS --nSeconds $PERF_CLIENT_SECONDS"

grep "Performance: " run_all.performance_http.client.log

RpcTest performance_http_simple \
        "Client-Server-performance/server/rpc_server_http_simple_performance --alsologtostderr --loglevel 0 --logcolors"\
        "Client-Server-performance/client/rpc_client_http_performance --alsologtostderr --loglevel 0 --logcolors --nThreads $PERF_CLIENT_THREADS --nSeconds $PERF_CLIENT_SECONDS"

grep "Performance: " run_all.performance_http_simple.client.log

RpcTest performance_http2 \
        "Client-Server-performance/server/rpc_server_http2_performance --alsologtostderr --loglevel 0 --logcolors "\
        "Client-Server-performance/client/rpc_client_http_performance --alsologtostderr --loglevel 0 --logcolors --nThreads $PERF_CLIENT_THREADS --nSeconds $PERF_CLIENT_SECONDS"

grep "Performance: " run_all.performance_http2.client.log

echo "[$0] Bye."
