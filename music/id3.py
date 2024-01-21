#!/usr/bin/env python

import argparse
import os,sys
import time
import eyed3
from pathlib import Path

def parse_args():
    parser = argparse.ArgumentParser(description='id3 tags utility')
    parser.add_argument('--file', dest='file', help='path to file')
    parser.add_argument('--dir', dest='dir', help='path to dir')
    args = parser.parse_args()
    return args

def process_file(path):
    print("+file: {}".format(path))
    try:
        audiofile = eyed3.load(path)
        change = False

        if audiofile.tag.title == None:
            fileName = Path(path).stem
            audiofile.tag.title = fileName
            change = True

        #if audiofile.tag.artist == None:
            #audiofile.tag.artist = "artist"
            #change = True

        if change:
            audiofile.tag.save()
            return True, True

        return True, False
    except Exception as e:
        print("error load tags from: {}, err: {}".format(path, e))
        return False, False

def process_dir(path):
    changed = 0
    notchanged = 0
    errors = 0
    total = 0

    for root, dirs, files in os.walk(path):
        for filename in files:
            total = total + 1
            ok, changed = process_file(os.path.join(root, filename))
            if ok:
                if changed:
                    changed = changed + 1
                else:
                    notchanged = notchanged + 1
            else:
                errors = errors + 1

    print("total: {}, notchanged: {}, changed: {}, errors: {}".format(int(total), int(notchanged), int(changed), int(errors)))

if __name__ == '__main__':
    args = parse_args()

    if args.file != None:
        ok, changed = process_file(args.file)
        if ok:
            if changed:
                print("ok, id3 changed")
            else:
                print("ok, id3 not changed")
        else:
            print("error")
        sys.exit(0)

    if args.dir != "":
        process_dir(args.dir)

