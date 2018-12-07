# Copyright 2016 Adobe. All rights reserved.

"""
fdkutils.py v1.2.8 Dec 7 2018
A module of functions that are needed by several of the AFDKO scripts.
"""

from __future__ import print_function, absolute_import

import os
import subprocess
import tempfile

from fontTools.misc.py23 import tounicode, tostr


def get_temp_file_path():
    file_descriptor, path = tempfile.mkstemp()
    os.close(file_descriptor)
    return path


def get_resources_dir():
    return os.path.join(os.path.dirname(__file__), 'resources')


def run_shell_command(args):
    """
    Runs a shell command.
    Returns True if the command was successful, and False otherwise.
    """
    try:
        subprocess.check_call(args)
        return True
    except (subprocess.CalledProcessError, OSError) as err:
        print(err)
        return False


def runShellCmd(cmd):
    try:
        p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT)
        stdoutdata, _ = p.communicate()
        return tounicode(stdoutdata, encoding='utf-8')
    except (subprocess.CalledProcessError, OSError) as err:
        msg = "Error executing command '%s'\n%s" % (cmd, err)
        print(msg)
        return ""


def runShellCmdLogging(cmd):
    try:
        proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT)
        while 1:
            output = proc.stdout.readline().rstrip()
            if output:
                print(tostr(output))
            if proc.poll() is not None:
                output = proc.stdout.readline().rstrip()
                if output:
                    print(tostr(output))
                break
    except (subprocess.CalledProcessError, OSError) as err:
        msg = "Error executing command '%s'\n%s" % (cmd, err)
        print(msg)
        return 1
    return 0
