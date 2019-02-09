# Copyright 2016 Adobe. All rights reserved.

"""
fdkutils.py v1.3.0 Feb 8 2019
A module of functions that are needed by several of the AFDKO scripts.
"""

from __future__ import print_function, absolute_import

import os
import sys
import subprocess
import tempfile

from fontTools.misc.py23 import tounicode, tostr


def get_temp_file_path():
    file_descriptor, path = tempfile.mkstemp()
    os.close(file_descriptor)
    return path


def get_resources_dir():
    return os.path.join(os.path.dirname(__file__), 'resources')


def _font_is_ufo(path):
    from fontTools.ufoLib import UFOReader
    from fontTools.ufoLib.errors import UFOLibError
    try:
        UFOReader(path, validate=False)
        return True
    except (UFOLibError, KeyError, TypeError):
        return False


def get_font_format(font_file_path):
    if _font_is_ufo(font_file_path):
        return 'UFO'
    elif os.path.isfile(font_file_path):
        with open(font_file_path, 'rb') as f:
            head = f.read(4)
            if head == b'OTTO':
                return 'OTF'
            elif head in (b'\x00\x01\x00\x00', b'true'):
                return 'TTF'
            elif head[0:2] == b'\x01\x00':
                return 'CFF'
            elif head[0:2] == b'\x80\x01':
                return 'PFB'
            elif head in (b'%!PS', b'%!Fo'):
                for fullhead in (b'%!PS-AdobeFont', b'%!FontType1',
                                 b'%!PS-Adobe-3.0 Resource-CIDFont'):
                    f.seek(0)
                    if f.read(len(fullhead)) == fullhead:
                        if b"CID" not in fullhead:
                            return 'PFA'
                        else:
                            return 'PFC'
    return None


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


def get_shell_command_output(args, std_error=False):
    """
    Runs a shell command and captures its output.
    To also capture standard error call with std_error=True.

    Returns a tuple; the first element will be True if the command was
    successful, and False otherwise. The second element will be a
    Unicode-encoded string, or None.
    """
    stderr = subprocess.STDOUT if std_error else None
    try:
        bytes_output = subprocess.check_output(args, stderr=stderr)
        try:
            str_output = tounicode(bytes_output, encoding='utf-8')
        except UnicodeDecodeError:
            str_output = tounicode(bytes_output,
                                   encoding=sys.getfilesystemencoding())
        return True, str_output
    except (subprocess.CalledProcessError, OSError) as err:
        print(err)
        return False, None


def runShellCmd(cmd):
    try:
        p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT)
        stdoutdata, _ = p.communicate()
        try:
            return tounicode(stdoutdata, encoding='utf-8')
        except UnicodeDecodeError:
            return tounicode(stdoutdata, encoding=sys.getfilesystemencoding())
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
