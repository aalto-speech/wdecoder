#!/usr/bin/python

import os
import sys
import shutil
import tempfile
import threading
import subprocess
from optparse import OptionParser


def segment(model, recipe, lnadir, batches, bidx):

    work_dir = os.path.join(lnadir, "segment_temp_%i" % bidx)
    if not os.path.exists(work_dir): os.makedirs(work_dir)

    pp_exe = 'phone_probs'
    pp_cmd = "%s -b %s -c %s -r %s -o %s -C %s --eval-ming=0.15 -B %i -I %i -i 1" %\
             (pp_exe, model, "%s.cfg" % model, recipe, work_dir, "%s.gcl" % model, batches, bidx)
    p = subprocess.Popen(pp_cmd, shell=True)
    p.wait()

    file_dir = os.path.dirname(os.path.realpath(__file__))
    segment_exe = os.path.join(file_dir, 'segment')
    model_ph = "%s.ph" % model
    model_dur = "%s.dur" % model
    batch_options = "-B %i -I %i" % (batches, bidx)
#    seg_cmd = "%s -t -l -s -d %s -n %s %s %s %s" % (segment_exe, model_dur, work_dir, batch_options, model_ph, recipe)
    seg_cmd = "%s -t -l -s -n %s %s %s %s" % (segment_exe, work_dir, batch_options, model_ph, recipe)
    p = subprocess.Popen(seg_cmd, shell=True)
    p.wait()

    shutil.rmtree(work_dir)


def main(argv=None):

    usage = "usage: %prog [options] MODEL RECIPE BATCHES THREADS"
    parser = OptionParser(usage=usage)

    (options, args) = parser.parse_args()
    if len(args) != 4:
        parser.print_help()
        return

    model = args[0]
    recipe = args[1]
    batchc = int(args[2])
    threadc = int(args[3])

    amodel_suffixes = ['ph', 'mc', 'gk', 'cfg', 'gcl']
    for suffix in amodel_suffixes:
        amfname = "%s.%s" % (model, suffix)
        if not os.path.exists(amfname):
            print >>sys.stderr, "%s not found" % amfname
            sys.exit()

    if not os.path.exists(recipe):
        print >>sys.stderr, "recipe %s not found" % recipe
        sys.exit()

    lnadir = tempfile.gettempdir()
    if not lnadir:
        print >>sys.stderr, "Could not find temporary directory for lnas"
        sys.exit()

    batch_idxs = range(1,batchc+1)
    while len(batch_idxs):
        threads = []
        for t in range(threadc):
            if not len(batch_idxs): break
            bidx = batch_idxs.pop(0)
            t = threading.Thread(target=segment, args=[model, recipe, lnadir, batchc, bidx])
            t.start()
            threads.append(t)
        for t in threads: t.join()


if __name__ == "__main__":
    sys.exit(main())

