# Copyright 2016 Adobe. All rights reserved.

"""
fdkutils.py v1.3.3 Aug 1 2019
A module of functions that are needed by several of the AFDKO scripts.
"""

import os
import subprocess
import tempfile

__version__ = "1.3.3"


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


def run_shell_command(args, suppress_output=False):
    """
    Runs a shell command.
    Returns True if the command was successful, and False otherwise.
    """
    sup = subprocess.DEVNULL if suppress_output else None

    try:
        subprocess.check_call(args, stderr=sup, stdout=sup)
        return True
    except (subprocess.CalledProcessError, OSError) as err:
        print(err)
        return False


def run_shell_command_logging(args):
    try:
        proc = subprocess.Popen(args,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT)
        while True:
            out = proc.stdout.readline().rstrip()
            if out:
                print(out.decode())
            if proc.poll() is not None:
                out = proc.stdout.readline().rstrip()
                if out:
                    print(out.decode())
                break
        return True
    except (subprocess.CalledProcessError, OSError) as err:
        msg = " ".join(args)
        print(f"Error executing command '{msg}'\n{err}")
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
        str_output = bytes_output.decode('utf-8', 'backslashreplace')
        success = True

    except (subprocess.CalledProcessError, OSError) as err:
        print(err)
        success = False
        str_output = None

    return success, str_output


def runShellCmd(cmd):
    try:
        p = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT)
        stdoutdata, _ = p.communicate()
        str_output = stdoutdata.decode('utf-8', 'backslashreplace')

    except (subprocess.CalledProcessError, OSError) as err:
        msg = f"Error executing command '{cmd}'\n{err}"
        print(msg)
        str_output = ""

    return str_output


def runShellCmdLogging(cmd):
    try:
        proc = subprocess.Popen(cmd, shell=True, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT)
        while 1:
            output = proc.stdout.readline().rstrip()
            if output:
                print(output.decode('utf-8', 'backslashreplace'))

            if proc.poll() is not None:
                output = proc.stdout.readline().rstrip()
                if output:
                    print(output.decode('utf-8', 'backslashreplace'))
                break

    except (subprocess.CalledProcessError, OSError) as err:
        msg = f"Error executing command '{cmd}'\n{err}"
        print(msg)
        return 1

    return 0
