/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/*
 * File handling.
 */

#ifndef FILE_H
#define FILE_H

#include "cb.h"
#include "lstdio.h"

typedef struct {
	char *name;
	FILE *fp;
	cbCtx h;
} File;

void fileOpen(File *file, cbCtx h, char *filename, char *mode);
int fileExists(char *filename);
#define fileRead1(f)	getc((f)->fp)
int fileReadN(File *file, size_t count, void *ptr);
#define fileWrite1(f,c) putc((c),(f)->fp)
int fileWriteN(File *file, size_t count, void *ptr);
char *fileGetLine(File *file, char *s, int n);
void fileSeek(File *file, long offset, int wherefrom);
long fileTell(File *file);
void fileClose(File *file);
void fileError(File *file);

#endif /* FILE_H */
