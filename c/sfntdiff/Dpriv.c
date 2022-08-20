/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Miscellaneous functions and system calls.
 */

#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#include "Dglobal.h"
#include "slogger.h"

/* ### Error reporting */

/* Print fatal message */
void sdFatal(char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    svLog(sFATAL, fmt, ap);
    // Not reached
    va_end(ap);
    exit(1);
}

/* Print informational message */
void sdMessage(char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    svLog(sINFO, fmt, ap);
    va_end(ap);
}

void sdNote(char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    vfprintf(stdout, fmt, ap);
    va_end(ap);
}

/* Print warning message */
void sdWarning(char *fmt, ...) {
    va_list ap;

    va_start(ap, fmt);
    svLog(sWARNING, fmt, ap);
    va_end(ap);
}
