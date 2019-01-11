#!/usr/bin/env python

import numpy as np
import argparse
import os,sys,math
import time
from scipy.spatial import distance
from sklearn.metrics.pairwise import euclidean_distances


def parse_args():
    """Parse input arguments."""
    parser = argparse.ArgumentParser(description='test')
    parser.add_argument('--num', dest='num', help='num iterations', type=int)
    args = parser.parse_args()
    return args

# 1 -- fastest way on 128-size float vectors
def eudis1(v1, v2):
    return np.linalg.norm(v1-v2)

# 2
def eudis2(v1, v2):
    return distance.euclidean(v1, v2)

# 5
def eudis5(v1, v2):
    dist = [(a - b)**2 for a, b in zip(v1, v2)]
    dist = math.sqrt(sum(dist))
    return dist

gsum = 0
v1 = np.zeros(128, dtype=np.float32)
v2 = np.zeros(128, dtype=np.float32)

def test(n):
    global v1
    global v2
    global gsum
    v1 = v1 + n
    v2 = v2 - n
    gsum = gsum + eudis1(v1, v2)
    gsum = gsum + np.dot(v1, v2)


if __name__ == '__main__':
    args = parse_args()
    print 'iterations: ', args.num

    start = time.time()

    for i in range(args.num):
        test(i)

    end = time.time()
    print 'total:', (end - start)*1000, 'ms'
    print 'sum: ', gsum


