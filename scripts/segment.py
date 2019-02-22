#!/usr/bin/python3
# -*- coding: utf-8 -*-

import argparse
import os
import sys
import shutil
import tempfile
import threading
import subprocess

pp_exe = 'phone_probs'
segment_exe = 'segment'
segment_beam = 500.0
segment_num_tokens = 500

def segment(model, recipe, lexicon, duration_model, lnadir, batches, bidx):

    work_dir = os.path.join(lnadir, "segment_temp_%i" % bidx)
    if not os.path.exists(work_dir): os.makedirs(work_dir)

    pp_cmd = "%s -b %s -c %s -r %s -o %s -C %s --eval-ming=0.15 -B %i -I %i -i 1" %\
             (pp_exe, model, "%s.cfg" % model, recipe, work_dir, "%s.gcl" % model, batches, bidx)
    p = subprocess.Popen(pp_cmd, shell=True)
    p.wait()

    model_ph = "%s.ph" % model
    model_dur = "%s.dur" % model
    options = "-b %f -m %i -l -s -n %s -B %i -I %i" % (segment_beam, segment_num_tokens, work_dir, batches, bidx)
    if lexicon:
        options = "%s -t -x %s" % (options, lexicon)
    if duration_model:
        options = "%s -d %s" % (options, model_dur)
    seg_cmd = "%s %s %s %s" % (segment_exe, options, model_ph, recipe)

    p = subprocess.Popen(seg_cmd, shell=True)
    p.wait()

    shutil.rmtree(work_dir)


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("MODEL", help="Acoustic model basename")
    parser.add_argument("RECIPE", help="Recipe file")
    parser.add_argument("NUM_BATCHES", type=int, help="Number of batches")
    parser.add_argument("NUM_THREADS", type=int, help="Number of threads")
    parser.add_argument("-d", "--duration_model", action="store_true", help="Enable duration model")
    parser.add_argument("-l", "--lexicon", action="store", help="Lexicon file, enables segmentation from recipe text field")
    parser.add_argument("-t", "--lna_directory", action="store",
                        help="Temporary directory for lna files, default: use directory given by tempfile module")
    args = parser.parse_args()

    amodel_suffixes = ['ph', 'mc', 'gk', 'cfg', 'gcl']
    if args.duration_model: amodel_suffixes.append('dur')
    for suffix in amodel_suffixes:
        amfname = "%s.%s" % (args.MODEL, suffix)
        if not os.path.exists(amfname):
            print("%s not found" % amfname, file=sys.stderr)
            sys.exit()

    if not os.path.exists(args.RECIPE):
        print("recipe %s not found" % args.RECIPE, file=sys.stderr)
        sys.exit()

    lnadir = args.lna_directory or tempfile.gettempdir()
    if not lnadir:
        print("Could not find temporary directory for lnas", file=sys.stderr)
        sys.exit()
    else:
        print("Writing temporary lna files under %s" % lnadir, file=sys.stderr)

    batch_idxs = list(range(1,args.NUM_BATCHES+1))
    while len(batch_idxs):
        threads = []
        for t in range(args.NUM_THREADS):
            if not len(batch_idxs): break
            bidx = batch_idxs.pop(0)
            t = threading.Thread(target=segment, args=[args.MODEL,
                                                       args.RECIPE,
                                                       args.lexicon,
                                                       args.duration_model,
                                                       lnadir,
                                                       args.NUM_BATCHES,
                                                       bidx])
            t.start()
            threads.append(t)
        for t in threads: t.join()


if __name__ == "__main__":
    sys.exit(main())
