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

if [ $(ls -l $1/test/test_data/media/*.flv | wc -l) -ne 4 ]; then
   echo "Probably you don't have all the test flv files - skipping"
   exit 0
fi

rm -rf /tmp/state_instance_1
mkdir -p /tmp/state_instance_1

rm -rf /tmp/state_instance_2
mkdir -p /tmp/state_instance_2

OLD_PIDS=$(ps ax -www | grep "whispercast --http_port=8481 --rtmp_port=8482" \
    | grep -v grep | awk '{print $1}')
[[ -n $OLD_PIDS ]]  && kill $OLD_PIDS; sleep 2; kill -9 $OLD_PIDS

$2/whispercast \
    --http_port=8481 \
    --rtmp_port=8482 \
    --base_media_dir=$1/test/test_data/media/ \
    --media_state_dir=/tmp/state_instance_1/ \
    --media_config_dir=$1/test/test_data/config/instance_1/ \
    --rtmp_connection_media_chunk_size_ms=0 \
    --element_libraries_dir=$2/../whisperstreamlib/elements/ &

PID_0=$!

sleep 10

if [ "$3" == "--longtest" ]; then
  $2/rtmp_client --server_host=127.0.0.1 --server_port=8482 \
      --stream_name=ns --log_received_events > \
      /tmp/rtmp_stream_s.out  &
  PID_1=$!
fi

$2/rtmp_client --server_host=127.0.0.1 --server_port=8482 \
    --stream_name=f/whispercast_test1.flv --log_received_events > \
    /tmp/rtmp_stream_f_1.out &
PID_2=$!

$2/rtmp_client --server_host=127.0.0.1 --server_port=8482 \
    --stream_name=f/whispercast_test1.flv?seek_pos=14000  \
    --log_received_events > \
    /tmp/rtmp_stream_f_1_seek_14000.out &
PID_3=$!

$2/rtmp_client --server_host=127.0.0.1 --server_port=8482 \
    --stream_name=f/whispercast_test4.flv --log_received_events > \
    /tmp/rtmp_stream_f_4.out &
PID_4=$!

if [ "$3" == "--longtest" ]; then
   echo "Waiting "$PID_1
   wait $PID_1
fi

echo "Waiting "$PID_2
wait $PID_2
echo "Waiting "$PID_3
wait $PID_3
echo "Waiting "$PID_4
wait $PID_4

kill -9 $PID_0

if [ "$3" == "--longtest" ]; then
  echo "Diffing rtmp_stream_s.out... "
  diff /tmp/rtmp_stream_s.out $1/test/test_data/rtmp_stream_s.out || exit 1
  echo "OK"
fi

echo "Diffing rtmp_stream_f_1.out... "
diff /tmp/rtmp_stream_f_1.out $1/test/test_data/rtmp_stream_f_1.out || exit 1
echo "OK"

echo "Diffing rtmp_stream_f_1_seek_14000.out..."
diff /tmp/rtmp_stream_f_1_seek_14000.out \
    $1/test/test_data/rtmp_stream_f_1_seek_14000.out || exit 1
echo "OK"

echo "Diffing rtmp_stream_f_4.out..."
diff /tmp/rtmp_stream_f_4.out $1/test/test_data/rtmp_stream_f_4.out || exit 1
echo "OK"

echo "PASS"
