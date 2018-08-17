/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
  except.c
*/

#include "supportenvironment.h"
#include "supportexcept.h"
#include "supportcanthappen.h"

PUBLIC procedure os_raise(_Exc_Buf *buf, int code, char *msg) {
#ifdef USE_CXX_EXCEPTION
    (void)buf;
    _Exc_Buf e(code, msg);
    throw e;
#else
    /* Replacing setjmp/longjmp  with a simple call to exit().
    buf->Code = code;
    buf->Message = msg;

    longjmp(buf->Environ, 1);
    */
    exit(code);
#endif
}
