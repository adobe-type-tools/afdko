# Copyright 2017 Adobe. All rights reserved.

from __future__ import print_function, division, absolute_import

import os
import sys

from afdko.fdkutils import runShellCmd

try:
    import xml.etree.cElementTree as ET
except ImportError:
    import xml.etree.ElementTree as ET

XMLElement = ET.Element
xmlToString = ET.tostring

__usage__ = """
buildmasterotfs.py  1.9.0 Jun 7 2019
Build master source OpenType/CFF fonts from a Superpolator design space file
and the UFO master source fonts.

python buildmasterotfs.py -h
python buildmasterotfs.py -u
python buildmasterotfs.py  <path to design space file>
"""

__help__ = __usage__ + """
Options:
--mkot   Allows passing a comma-separated set of options to makeotf

The script makes a number of assumptions.
1) all the master source fonts are blend compatible in all their data.
2) Each master source font is in a separate directory from the other master
source fonts, and each of these directories provides the default meta data
files expected by the 'makeotf' command:
- features/features.fea
- font.ufo
- FontMenuNameDB within the directory or no more than 4 parent directories up.
- GlyphAliasAndOrderDB within the directory or no more than 4 parent
  directories up.
3) The Adobe AFDKO is installed: the script calls 'tx' and 'makeotf'.
4) <source> element for the default font in the design space file has
the child element <info copy="1">; this is used to identify which master is
the default font.

The script will build a master source OpenType/CFF font for each master source
UFO font, using the same file name, but with the extension '.otf'.

UFO masters with partial glyphsets are supported.
"""


def compatibilizePaths(otfPath):
    tempPathCFF = otfPath + ".temp.cff"
    command = "tx -cff +b -std -no_opt \"%s\" \"%s\" 2>&1" % (otfPath,
                                                              tempPathCFF)
    report = runShellCmd(command)
    if "fatal" in str(report):
        print(report)
        sys.exit(1)

    command = "sfntedit -a \"CFF \"=\"%s\" \"%s\" 2>&1" % (tempPathCFF,
                                                           otfPath)
    report = runShellCmd(command)
    if "FATAL" in str(report):
        print(report)
        sys.exit(1)

    os.remove(tempPathCFF)


def parse_makeotf_options(mkot_opts):
    """Converts a comma-separated string into a space-separated string"""
    return ' '.join(mkot_opts.split(','))


def main(args=None):
    if args is None:
        args = sys.argv[1:]
    if not args:
        args = ["-u"]

    mkot_options = ''

    if '-u' in args:
        print(__usage__)
        return
    if '-h' in args:
        print(__help__)
        return
    if '--mkot' in args:
        index = args.index('--mkot')
        args.pop(index)
        mkot_options = parse_makeotf_options(args.pop(index))

    (dsPath,) = args
    rootET = ET.parse(dsPath)
    ds = rootET.getroot()
    sourceET = ds.find('sources')
    masterList = sourceET.findall('source')
    master_paths = [et.attrib['filename'] for et in masterList]

    print("Building local otf's for master font paths...")
    curDir = os.getcwd()
    dsDir = os.path.dirname(dsPath)
    for master_path in master_paths:
        master_path = os.path.join(dsDir, master_path)
        masterDir = os.path.dirname(master_path)
        ufoName = os.path.basename(master_path)
        otfName = os.path.splitext(ufoName)[0]
        otfName = "{}.otf".format(otfName)
        if masterDir:
            os.chdir(masterDir)
        cmd = "makeotf -nshw -f \"%s\" -o \"%s\" -r -nS %s 2>&1" % (
            ufoName, otfName, mkot_options)
        log = runShellCmd(cmd)
        if ("FATAL" in log) or ("Failed to build" in log):
            print(log)

        if "Built" not in str(log):
            print("Error building OTF font for", master_path, log)
            print("makeotf cmd was '%s' in %s." % (cmd, masterDir))
        else:
            print("Built OTF font for", master_path)
            compatibilizePaths(otfName)
        os.chdir(curDir)


if __name__ == "__main__":
    main()
