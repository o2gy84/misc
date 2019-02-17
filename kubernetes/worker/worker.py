#!/usr/bin/env python

import requests, sys, time, random

# queue should be either 'queue1' or 'queue2'
queue = sys.argv[1]
host_port = sys.argv[2]
url = 'http://{0}/take?{1}=1'.format(host_port, queue)

def cpu_load(count, x):
    while count > 0:
        count = count - 1
        x * x

if __name__ == "__main__":
    while True:
        r = requests.get(url)
        q_size = r.json()[queue]

        print 'sizes: ', r.json()

        if q_size == 0:
            time.sleep(1)
            continue

        load = random.randint(100000, 1000000)
        cpu_load(load, q_size)

