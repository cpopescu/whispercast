#!/usr/bin/python

# Starts a bunch of fetches for a page w/ wget

import sys
import alarm
import threading
import random

class fetcher(threading.Thread):
    def __init__(self, tid, min_time, max_time, url):
        threading.Thread.__init__(self)
        self.id = tid
        self.min_time = min_time
        self.max_time = max_time
        self.url = url
    def run(self):
        cmd = ["wget", "-q", "-O", "/dev/null", self.url]
        while True:
            to = random.randrange(self.min_time, self.max_time)
            print " %s > Fetching for: %s - %s" % (self.id, to, cmd)
            alarm.Alarm(to, cmd)

if __name__ == "__main__":
    num_threads = int(sys.argv[1])
    min_time = int(sys.argv[2])
    max_time = int(sys.argv[3])
    url = sys.argv[4]
    random.seed()
    threads = []
    threading.stack_size(65536)
    for i in range(num_threads):
        threads.append(fetcher(i, min_time, max_time, url))
        threads[-1].start()
    for t in threads:
        t.join()
