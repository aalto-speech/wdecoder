#!python

import sys

def triphonize(letters):
    triphones = []
    for i in range(1, len(letters)-1):
        triphones.append("%s-%s+%s" % (letters[i-1], letters[i], 
letters[i+1]))
    return triphones


for wrd in sys.argv[1:]:
    letters = list("_" + wrd + "_")
    triphones = triphonize(letters) 
    print "%s(1.0) %s" % (wrd, " ".join(triphones))
