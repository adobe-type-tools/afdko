# Copyright 2016 Adobe. All rights reserved.

from __future__ import print_function, absolute_import

import os
import platform
import subprocess
import traceback

__doc__ = """
fdkutils.py v1.2.6 Jun 30 2018
A module of functions that are needed by several of the AFDKO scripts.
"""

curSystem = platform.system()


class FDKEnvError(KeyError):
    pass


def get_resources_dir():
    """ Look up the file path to find the "Tools" directory;
    then add the os.name for the executables, and 'FDKScripts'
    for the scripts.
    """
    cjk_dir = os.path.join(
        os.path.dirname(__file__),
        'resources')
    return cjk_dir


def runShellCmd(cmd):
    try:
        p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT).stdout
        log = p.read()
        return log
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
