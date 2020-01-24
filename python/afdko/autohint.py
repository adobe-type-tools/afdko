# Copyright 2016 Adobe. All rights reserved.

import subprocess
import sys
import warnings


warnings.warn(
    "autohint now calls psautohint and will be removed from AFDKO soon. "
    "Please update your code to use psautohint.",
    category=FutureWarning)


def main():
    args = ['psautohint'] + sys.argv[1:]
    proc = subprocess.Popen(args)
    proc.communicate()

    return proc.returncode


if __name__ == '__main__':
    sys.exit(main())
