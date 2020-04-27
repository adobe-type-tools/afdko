/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0.
This license is available at: http://opensource.org/licenses/Apache-2.0. */

#include <stdio.h>
#include <stdarg.h>

#include "hotconv.h"
#include "cb.h"

extern void myfatal(void *ctx);
extern void message(void *ctx, int type, char *text);

extern int KeepGoing;

/* Print fatal error message */
void cbFatal(cbCtx h, char *fmt, ...) {
    char text[512];
    va_list ap;
    va_start(ap, fmt);
    VSPRINTF_S(text, sizeof(text), fmt, ap);
    message(h, hotFATAL, text);
    va_end(ap);
    if (!KeepGoing) {
        myfatal(h);
    }
}

/* Print warning message */
void cbWarning(cbCtx h, char *fmt, ...) {
    char text[512];
    va_list ap;
    va_start(ap, fmt);
    VSPRINTF_S(text, sizeof(text), fmt, ap);
    message(h, hotWARNING, text);
    va_end(ap);
}
