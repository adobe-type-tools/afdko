# Copyright 2016 Adobe. All rights reserved.

"""
A collection of functions used by multiple AFDKO scripts.
"""

import argparse
import os
import subprocess
import tempfile

__version__ = '1.3.7'


def validate_path(path_str):
    valid_path = os.path.abspath(os.path.realpath(path_str))
    if not os.path.exists(valid_path):
        raise argparse.ArgumentTypeError(
            f"'{path_str}' is not a valid path.")
    return valid_path


def get_temp_file_path(directory=None):
    file_descriptor, path = tempfile.mkstemp(dir=directory)
    os.close(file_descriptor)
    return path


def get_temp_dir_path(path_comp=None):
    path = tempfile.mkdtemp()
    if path_comp:
        path = os.path.join(path, path_comp)
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
            elif head == b'ttcf':
                return 'TTC'
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
    """
    Runs a shell command while logging both standard output and standard error.

    An earlier version of this function that used Popen.stdout.readline()
    was failing intermittently in CI, so now we're trying this version that
    uses Popen.communicate().
    """
    try:
        proc = subprocess.Popen(args,
                                stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT,
                                universal_newlines=True)
        out = proc.communicate()[0]
        print(out, end='')
        if proc.returncode != 0:
            msg = " ".join(args)
            print(f"Error executing command '{msg}'")
            return False
        else:
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


def runShellCmd(cmd, shell=True, timeout=None):
    try:
        p = subprocess.Popen(cmd, shell=shell, stdout=subprocess.PIPE,
                             stderr=subprocess.STDOUT)
        stdoutdata, _ = p.communicate(timeout=timeout)
        str_output = stdoutdata.decode('utf-8', 'backslashreplace')

        if p.returncode != 0:  # must be called *after* communicate()
            print(f"Error executing command '{cmd}'\n{str_output}")
            str_output = ""

    except subprocess.TimeoutExpired as err:
        p.kill()
        print(f"{err}")  # the 'err' will contain the command
        str_output = ""

    except (subprocess.CalledProcessError, OSError) as err:
        print(f"Error executing command '{cmd}'\n{err}")
        str_output = ""

    return str_output


def runShellCmdLogging(cmd, shell=True):
    try:
        proc = subprocess.Popen(cmd, shell=shell, stdout=subprocess.PIPE,
                                stderr=subprocess.STDOUT)
        while 1:
            output = proc.stdout.readline().rstrip()
            if output:
                print(output.decode('utf-8', 'backslashreplace'))

            if proc.poll() is not None:
                if proc.returncode != 0:  # must be called *after* poll()
                    print(f"Error executing command '{cmd}'")
                    return 1

                output = proc.stdout.readline().rstrip()
                if output:
                    print(output.decode('utf-8', 'backslashreplace'))
                break

    except (subprocess.CalledProcessError, OSError) as err:
        print(f"Error executing command '{cmd}'\n{err}")
        return 1

    return 0
