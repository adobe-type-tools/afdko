# Copyright 2014 Adobe Systems Incorporated

import subprocess
import sys
import warnings


warnings.warn(
    "stemhist now calls psstemhist and will be removed from AFDKO soon. "
    "Please update your code to use psstemhist.",
    category=FutureWarning)


def main():
    args = ['psstemhist'] + sys.argv[1:]
    proc = subprocess.Popen(args)
    proc.communicate()

    return proc.returncode


if __name__ == '__main__':
    sys.exit(main())
