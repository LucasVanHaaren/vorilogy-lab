#!/usr/bin/env python3

"""
Write a script virus1.py which copie itself in another file
"""

import random, os, sys, string

SRC_FILENAME = sys.argv[0]
DEST_FILENAME = ''.join(random.choices(string.ascii_lowercase + string.digits, k=25)) + ".py"


with open(SRC_FILENAME, 'rb') as src_file:
    with open(DEST_FILENAME, 'wb') as dst_file:
        dst_file.write(src_file.read())

os.chmod(DEST_FILENAME, 0o775)
