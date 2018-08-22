/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Machine dependent file operations.
 */

#ifndef SYS_H
#define SYS_H

#include "Eglobal.h"

FILE *sysOpenRead(char *filename);
FILE *sysOpenWrite(char *filename);
FILE *sysOpenSearchpath(char *filename);
int sysFileExists(char *filename);
long sysFileLen(FILE *f);

#endif /* SYS_H */
