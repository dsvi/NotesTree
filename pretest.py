#!/usr/bin/env python3

import shutil
import os
from os.path import expanduser

home = expanduser("~")
dst = home + '/OTest'
src = home + '/OTest1'
shutil.rmtree(dst)
shutil.copytree(src, dst)