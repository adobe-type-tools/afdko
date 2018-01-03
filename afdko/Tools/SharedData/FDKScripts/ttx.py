#!/usr/bin/python

__copyright__ = """Copyright 2015 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
"""

# wrapper for the real ttx.py, which is in:
# FDK/Tools/osx/Python/AFDKOPython27/lib/python2.7/site-packages/FontTools/fontTools/ttx.py
# or the equivalent for win or linux os.

import sys
from fontTools import ttx

ttx.main(sys.argv[1:])
