import sys
import cython

from libc.stdlib cimport malloc, free
from libc.string cimport strcpy, strcmp, strlen

cdef extern char *FDK_VERSION

try:
    from afdko.fdkutils import fdk_version
    tmp_str = fdk_version().encode('UTF-8')
    tmp_s = cython.declare(cython.p_char, tmp_str)
    global FDK_VERSION
    if strcmp(FDK_VERSION, "unknown") != 0:
        free(FDK_VERSION)
    FDK_VERSION = <char*> malloc(sizeof(char) * strlen(tmp_s) + 1)
    strcpy(FDK_VERSION, tmp_s)
except:
    pass

ctypedef int (*mainesque_t)(int argc, char *argv[])

cdef extern int main__detype1(int argc, char *argv[])
cdef extern int main__addfeatures(int argc, char *argv[])
cdef extern int main__mergefonts(int argc, char *argv[])
cdef extern int main__rotatefont(int argc, char *argv[])
cdef extern int main__sfntdiff(int argc, char *argv[])
cdef extern int main__sfntedit(int argc, char *argv[])
cdef extern int main__spot(int argc, char *argv[])
cdef extern int main__tx(int argc, char *argv[])
cdef extern int main__type1(int argc, char *argv[])

cdef extern char *rotN(const char *str, int N)

cdef int call_mainesque(mainesque_t mfunc, args):
    ec = 0
    cdef char **argv = NULL
    arge = []
    if args is None:
        args = sys.argv
    try:
        argc = len(args)
        if argc != 0:
            argv = <char **>malloc(argc * sizeof(char *))
            # space for encoded strings during lifetime of argv
            for i in range(argc):
                arge.append(args[i].encode())
                argv[i] = arge[i]
        ec = mfunc(argc, argv)
    finally:
        free(argv)
    return ec

def detype1(args = None, noexit = False):
    ec = call_mainesque(&main__detype1, args)
    if not noexit:
        sys.exit(ec)
    return ec

def addfeatures(args = None, noexit = False):
    ec = call_mainesque(&main__addfeatures, args)
    if not noexit:
        sys.exit(ec)
    return ec

def mergefonts(args = None, noexit = False):
    ec = call_mainesque(&main__mergefonts, args)
    if not noexit:
        sys.exit(ec)
    return ec

def rotatefont(args = None, noexit = False):
    ec = call_mainesque(&main__rotatefont, args)
    if not noexit:
        sys.exit(ec)
    return ec

def sfntdiff(args = None, noexit = False):
    ec = call_mainesque(&main__sfntdiff, args)
    if not noexit:
        sys.exit(ec)
    return ec

def sfntedit(args = None, noexit = False):
    ec = call_mainesque(&main__sfntedit, args)
    if not noexit:
        sys.exit(ec)
    return ec

def spot(args = None, noexit = False):
    ec = call_mainesque(&main__spot, args)
    if not noexit:
        sys.exit(ec)
    return ec

def tx(args = None, noexit = False):
    ec = call_mainesque(&main__tx, args)
    if not noexit:
        sys.exit(ec)
    return ec

def type1(args = None, noexit = False):
    ec = call_mainesque(&main__type1, args)
    if not noexit:
        sys.exit(ec)
    return ec
