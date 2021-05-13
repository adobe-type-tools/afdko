/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Machine dependent file operations.
 */

#ifndef ESYS_H
#define ESYS_H

#include "Eglobal.h"

FILE *sysOpenRead(const char *filename);
FILE *sysOpenWrite(const char *filename);
FILE *sysOpenSearchpath(const char *filename);
int sysFileExists(const char *filename);
long sysFileLen(FILE *f);

#endif /* ESYS_H */
