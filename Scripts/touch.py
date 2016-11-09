# script for cross platform touch file

import os, sys

fname = sys.argv[1]

with open(fname, 'a'):
	os.utime(fname, None)
