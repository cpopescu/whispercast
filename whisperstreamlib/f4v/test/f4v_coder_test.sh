#!/bin/bash

if [ $# -ne 2 ]; then
  echo "Usage: $0 <executable> <test_data_dir>"
  echo "Where:"
  echo "  <executable>: path to 'f4v_coder_test' executable file"
  echo "  <test_data_dir>: path to 'test_data' directory"
  exit
fi

EXE=$1
TEST_DATA_DIR=$2

`$EXE --loglevel 2 --f4v_path $TEST_DATA_DIR/backcountry.f4v --f4v_log_level 3 2>> /tmp/f4v_coder_test-stderr.LOG`
`$EXE --loglevel 2 --f4v_path $TEST_DATA_DIR/sample1_150kbps.f4v --f4v_log_level 3 2>> /tmp/f4v_coder_test-stderr.LOG`
`$EXE --loglevel 2 --f4v_path $TEST_DATA_DIR/sample2_1000kbps.f4v --f4v_log_level 3 2>> /tmp/f4v_coder_test-stderr.LOG`
