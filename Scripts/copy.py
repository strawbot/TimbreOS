# cross platform copy file script

import sys, shutil

src,dst = sys.argv[1:3]

shutil.copyfile(src,dst)