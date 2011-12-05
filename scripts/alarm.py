#!/usr/bin/python
#
# This swift utility runs a command with a timeout
#
#
# e.g
#   alarm 1.5 sleep 1000         # will end in 1.5 seconds the sleep
#
import sys
import subprocess
import time
import threading

class killer(threading.Thread):
  def __init__ (self, event, proc, timeout):
    threading.Thread.__init__(self)
    self.event = event
    self.proc = proc
    self.timeout = timeout

  def run(self):
    ret = self.event.wait(self.timeout)
    if not ret:
       try:
         self.proc.terminate()
         time.sleep(1)
         self.proc.kill()
       except OSError:
         pass


def Alarm(timeout, command):
   p = subprocess.Popen(command)
   ev = threading.Event()
   k = killer(ev, p, timeout)
   k.start()
   p.wait()
   ev.set();

if __name__ == "__main__":
   if len(sys.argv) < 3:
      sys.exit("%s <timeout sec> <command ...>")
   timeout = float(sys.argv[1])
   Alarm(timeout, sys.argv[2:])
