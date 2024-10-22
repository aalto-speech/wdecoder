#!/usr/bin/python3
# -*- coding: utf-8 -*-

import argparse
import os
import re
import sys
from collections import OrderedDict


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

        sub_dir = os.path.split(trfname)[0].replace(r"/work/asr/c/speechdat-fi/phn/", "")
        full_dir = os.path.join(ptxtdir, sub_dir)
        if not os.path.exists(full_dir): os.makedirs(full_dir)

        ptxtfname = "%s.ptxt" % os.path.splitext(os.path.split(trfname)[1])[0]
        ptxtfname = os.path.join(full_dir, ptxtfname)

        ptxtf = open(ptxtfname, 'w')
        print(" ".join(words), file=ptxtf)
        ptxtf.close()

        line['text'] = ptxtfname
        line['lna'] = os.path.split(line['lna'])[1]
        new_line = list()
        for key,val in line.items():
            new_line.append("%s=%s" % (key,val))
        print(" ".join(new_line))


def main():

    parser = argparse.ArgumentParser()
    parser.add_argument("RECIPE",
                        help="Recipe file")
    parser.add_argument("TEXTDIR",
                        help="Directory for text files")
    args = parser.parse_args()

    lines = parse_recipe(args.RECIPE)

    ptxtdir = os.path.abspath(args.TEXTDIR)
    if not os.path.exists(ptxtdir):
        os.makedirs(ptxtdir)

    create_ptxts(lines, ptxtdir)


if __name__ == "__main__":
    sys.exit(main())
