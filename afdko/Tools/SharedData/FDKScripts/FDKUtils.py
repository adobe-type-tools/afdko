# Copyright 2016 Adobe. All rights reserved.

from __future__ import print_function, absolute_import

import os
import platform
import subprocess
import sys
import traceback

__doc__ = """
FDKUtils.py v1.2.5 May 17 2018
A module of functions that are needed by several of the AFDKO scripts.
"""

curSystem = platform.system()

AdobeCMAPS = "Adobe Cmaps"


class FDKEnvError(KeyError):
    pass


def findFDKDirs():
    """ Look up the file path to find the "Tools" directory;
    then add the os.name for the executables, and 'FDKScripts'
    for the scripts.
    """
    fdkSharedDataDir = None
    fdkToolsDir = None
    _dir = os.path.dirname(__file__)

    while _dir:
        if os.path.basename(_dir) == "Tools":
            fdkScriptsDir = os.path.join(_dir, "SharedData", "FDKScripts")
            if curSystem == "Darwin":
                fdkToolsDir = os.path.join(_dir, "osx")
            elif curSystem == "Windows":
                fdkToolsDir = os.path.join(_dir, "win")
            elif curSystem == "Linux":
                fdkToolsDir = os.path.join(_dir, "linux")
            else:
                print("Fatal error: un-supported platform %s %s." % (
                    os.name, sys.platform))
                raise FDKEnvError

            if (not (os.path.exists(fdkScriptsDir) and
               os.path.exists(fdkToolsDir))):
                print("Fatal error: could not find  the FDK scripts dir %s "
                      "and the tools directory %s." %
                      (fdkScriptsDir, fdkToolsDir))
                raise FDKEnvError

            # the FDK.py bootstrap program already adds fdkScriptsDir
            # to the  sys.path; this is useful only when running the
            # calling script directly using an external Python.
            if fdkScriptsDir not in sys.path:
                sys.path.append(fdkScriptsDir)
            fdkSharedDataDir = os.path.join(_dir, "SharedData")
            break
        _dir = os.path.dirname(_dir)
    return fdkToolsDir, fdkSharedDataDir


def findFDKFile(fdkDir, fileName):
    path = os.path.join(fdkDir, fileName)
    if os.path.exists(path):
        return path
    p1 = path + ".exe"
    if os.path.exists(p1):
        return p1
    p2 = path + ".cmd"
    if os.path.exists(p2):
        return p2
    if fileName not in ["addGlobalColor"]:
        print("Fatal error: could not find '%s or %s or %s'." % (path, p1, p2))
    raise FDKEnvError


def runShellCmd(cmd):
    try:
        p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT).stdout
        log = p.read()
        return log.decode("latin-1")
    except (OSError, ValueError):
        msg = "Error executing command '%s'. %s" % (cmd, traceback.print_exc())
        print(msg)
        return ""


def runShellCmdLogging(cmd):
    try:
        proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT)
        while 1:
            output = proc.stdout.readline()
            if output:
                print(output, end=' ')
            if proc.poll() is not None:
                output = proc.stdout.readline()
                if output:
                    print(output, end=' ')
                break
    except (OSError, ValueError):
        msg = "Error executing command '%s'. %s" % (cmd, traceback.print_exc())
        print(msg)
        return 1
    return 0
