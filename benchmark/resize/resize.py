#!/usr/bin/env python

import cv2
import argparse
import os,sys,math
import time
import multiprocessing
from multiprocessing import Lock
from multiprocessing.sharedctypes import Value

def parse_args():
    """Parse input arguments."""
    parser = argparse.ArgumentParser(description='test')
    parser.add_argument('--path', dest='path', help='path to image')
    parser.add_argument('--num', dest='num', help='num iterations', type=int)

    args = parser.parse_args()
    return args

counter = None

def init(arg1):
    ''' store the counter for later use '''
    global counter
    counter = arg1

def test(v):
    global counter
    with counter.get_lock():
        counter.value += 1
    new_im = cv2.resize(img, (256, 256))
    new_im = new_im[64:128, 64:128]

if __name__ == '__main__':
    args = parse_args()
    print 'iterations: ', args.num

    img = cv2.imread(args.path)
    if img is None:
        print 'error image read: ', args.path
        sys.exit(-1)

    print 'shape: ', img.shape

    counter = Value('i', 0)

    array = [img] * args.num

    start = time.time()

    pool = multiprocessing.Pool(10, initializer = init, initargs = (counter, ))
    pool.map(test, array)

    end = time.time()
    print 'total:', (end - start)*1000, 'ms'
    print 'counter: ', counter.value


