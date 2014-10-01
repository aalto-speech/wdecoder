#!/usr/bin/python

import subprocess
import sys
import os


if len(sys.argv) != 2:
    print >>sys.stderr, "give .dot file as an argument"
    sys.exit()

dotname = sys.argv[1]
(basename, ext) = os.path.splitext(dotname)
epsname = basename + ".eps"
p = subprocess.Popen("dot -Teps -Gcharset=latin1 %s > %s" % (dotname, epsname), shell=True)
p.wait()
p = subprocess.Popen("epspdf %s" % epsname, shell=True)
p.wait()
os.remove(epsname)

pdfname = basename + ".pdf"
subprocess.Popen("evince %s" % pdfname, shell=True)

