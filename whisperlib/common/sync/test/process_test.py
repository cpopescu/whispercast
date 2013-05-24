#!/usr/bin/python

import sys
import time

def log(msg):
  sys.stdout.write("process2_test.py STDOUT: " + msg)
  sys.stdout.flush()
  sys.stderr.write("process2_test.py STDERR: " + msg)
  sys.stderr.flush()
def logln(msg):
  log(msg+"\n")

def main():
  logln("Running with " + str(len(sys.argv)) + " arguments")
  for i,a in enumerate(sys.argv):
    logln("Argument %d: %s" % (i, a))
    time.sleep(1)
  logln("Exit")

if __name__ == "__main__":
  sys.exit(main())

