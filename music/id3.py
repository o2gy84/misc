#!/usr/bin/env python

import argparse
import os,sys
import eyed3
import shutil
from pathlib import Path,PurePosixPath

def parse_args():
    parser = argparse.ArgumentParser(description='id3 tags utility')
    parser.add_argument('--file',             dest='file', help='path to file')
    parser.add_argument('--dir',              dest='dir', help='path to dir')
    parser.add_argument('--r_level',          dest='r_level', help='hierarchy level for reorganize', default=2)
    parser.add_argument('--reorganize',       dest='reorganize', action='store_true', help='if specified, script will move all data into directory on `reorganize_level` hierarchy')
    parser.add_argument('--exclude',          dest='exclude', help='example: --exclude dir1,dir2, exclude dirs from reorganize')
    parser.add_argument('--dry',              dest='dry', action='store_true', help='check files without changes')
    args = parser.parse_args()
    return args

# returns tuple [ok, tags_changed]
def process_file(path, dryrun):
    ext = Path(path).suffix.lower()
    if ext != ".mp3":
        print("[-] file ignored, not mp3: {}".format(path))
        return True, False

    print("[+] file: {}".format(path))
    try:
        audiofile = eyed3.load(path)
        if dryrun:
            if audiofile.tag is None:
                print("\tartist: unknown, title: unknown (no tags parsed)")
                return True, False

            artist = "unknown"
            title = "unknown"
            if audiofile.tag.artist != None:
                artist = audiofile.tag.artist
            if audiofile.tag.title != None:
                title = audiofile.tag.title
            print("\tartist: {}, title: {}".format(artist, title))
            return True, False

        if audiofile.tag is None:
            audiofile.tag = eyed3.id3.Tag()

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
        print("\terror load tags from: {}, err: {}".format(path, e))
        return False, False

def dir_fix_id3(path, dryrun):
    changed = 0
    notchanged = 0
    errors = 0
    total = 0

    for root, dirs, files in os.walk(path):
        for filename in files:
            total = total + 1
            ok, ch = process_file(os.path.join(root, filename), dryrun)
            if ok:
                if ch:
                    changed = changed + 1
                else:
                    notchanged = notchanged + 1
            else:
                errors = errors + 1

    print("total: {}, notchanged: {}, changed: {}, errors: {}".format(int(total), int(notchanged), int(changed), int(errors)))

# returns ok, dict
def dir_reorganize(path, exclude_dirs, level):
    uniq = {}
    ex_dirs = []
    if exclude_dirs is not None:
        ex_dirs = exclude_dirs.split(",")

    for root, dirs, files in os.walk(path):
        for f in files:
            ext = Path(f).suffix.lower()
            if ext != ".mp3":
                continue

            if f in uniq:
                print("Found collision file1: {} and file2: {}, need rename".format(os.path.join(root, f), uniq[f]['origin']))
                return  False, {}

            p = PurePosixPath(root)

            is_excluded = False
            for ex_dir in ex_dirs:
                if p.is_relative_to(ex_dir):
                    print("[-] exclude: {}/{}, because exclude prefix: {}".format(p, f, ex_dir))
                    is_excluded = True
                    break

            if is_excluded:
                continue

            #relative: PostRock/Amera/1
            rel = p.relative_to(path)

            if rel is None:
                continue
            if len(str(rel)) <= 2:
                continue
            if len(rel.parts) <= level - 1:
                continue

            target = path
            counter = 0
            for part in rel.parts:
                target = os.path.join(target, rel.parts[counter])
                counter = counter + 1
                if counter == level - 1:
                    break

            uniq[f] = {
                    'target': target,
                    'origin': os.path.join(root, f)
            }

    return True, uniq

def dir_move(path, level, d):
    for k, v in d.items():
        src = v['origin']
        dst = os.path.join(v['target'], k)
        os.rename(src, dst)

    # todo Physically remove empty folders
    #counter = 1
    #if level > 1:
        #l = os.listdir(path=path)

if __name__ == '__main__':
    args = parse_args()

    if args.file != None:
        ok, changed = process_file(args.file, args.dry)
        if ok:
            if changed:
                print("ok, id3 changed")
            else:
                print("ok, id3 not changed")
        else:
            print("error")
        sys.exit(0)

    if args.dir != None:
        if args.reorganize:
            ok, d = dir_reorganize(args.dir, args.exclude, int(args.r_level))
            if not ok:
                sys.exit(1)
            for k, v in d.items():
                print("file: {} --> {}".format(v['origin'], os.path.join(v['target'], k)))
            if args.dry:
                sys.exit(0)
            dir_move(args.dir, int(args.r_level), d)
        else:
            dir_fix_id3(args.dir, args.dry)

