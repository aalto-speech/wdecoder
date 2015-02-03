#!/usr/bin/python

import os
import re
import sys
import glob
import tempfile
import threading
import subprocess
from collections import OrderedDict
from optparse import OptionParser


def parse_recipe(recipefname):

    lines = open(recipefname).readlines()
    recipe_lines = []
    for line in lines:
        line = line.strip()
        if not line: continue
        items = line.split()
        recipe_line = OrderedDict()
        for item in items:
            (key, val) = item.split("=")
            recipe_line[key] = val
        recipe_lines.append(recipe_line)
    return recipe_lines


def create_ptxts(lines, ptxtdir):

    for line in lines:
        trfname = line['transcript']
        trf = open(trfname)

        word = ""
        words = list()
        for trl in trf:
            trl = trl.strip()
            if trl.endswith(".0"):
                trip = trl.split()[-1]
                if not "-" in trip:
                    if word:
                        words.append(word)
                        word = ""
                    continue
                m = re.search("\-(.)\+", trip)
                word += m.group(1)

        ptxtfname = "%s.ptxt" % os.path.splitext(os.path.split(trfname)[1])[0]
        ptxtfname = os.path.join(ptxtdir, ptxtfname)
        ptxtf = open(ptxtfname, 'w')
        print >>ptxtf, " ".join(words)
        ptxtf.close()

        line['text'] = ptxtfname
        new_line = list()
        for key,val in line.items():
            new_line.append("%s=%s" % (key,val))
        print " ".join(new_line)


def main(argv=None):

    usage = "usage: %prog [options] RECIPE TEXTDIR"
    parser = OptionParser(usage=usage)

    (options, args) = parser.parse_args()
    if len(args) != 2:
        parser.print_help()
        return

    recipe = args[0]
    lines = parse_recipe(recipe)

    ptxtdir = os.path.abspath(args[1])
    if not os.path.exists(ptxtdir):
        os.makedirs(ptxtdir)

    create_ptxts(lines, ptxtdir)


if __name__ == "__main__":
    sys.exit(main())


