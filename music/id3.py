#!/usr/bin/env python

import argparse
import os,sys,math
import time
import eyed3

def parse_args():
    """Parse input arguments."""
    parser = argparse.ArgumentParser(description='id3 tags utility')
    parser.add_argument('--file', dest='file', help='path to file')
    parser.add_argument('--dir', dest='dir', help='path to dir')
    args = parser.parse_args()
    return args

if __name__ == '__main__':
    args = parse_args()
    print("args.path: ", args.file)

    try:
        audiofile = eyed3.load(args.file)
        print("tags: ", audiofile)
    except Exception as e:
        print("Error load tags from: {}, err: {}".format(args.file, e))

