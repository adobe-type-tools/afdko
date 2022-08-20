/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. 
   This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

/*
 * File handling.
 */

#ifndef SHARED_INCLUDE_SFILE_H_
#define SHARED_INCLUDE_SFILE_H_

#include <stdint.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    char *name;
    FILE *fp;
} sFile;

void sFileOpen(sFile *file, const char *filename, const char *mode);
int sFileExists(const char *filename);
int sFileIsDir(const char *filename);
int sFileLen(sFile *file);
#define sFileRead1(f) getc((f)->fp)
int sFileReadN(sFile *file, size_t count, void *ptr);
#define sFileWrite1(f, c) putc((c), (f)->fp)
int sFileWriteN(sFile *file, size_t count, void *ptr);
char *sFileGetLine(sFile *file, char *s, int n);
void sFileSeek(sFile *file, long offset, int wherefrom);
long sFileTell(sFile *file);
void sFileClose(sFile *file);
void sFileReadObject(sFile *file, int size, void *obj);
void sFileWriteObject(sFile *file, int size, uint32_t obj);
void sFileError(sFile *file);
int sFileReadInputDir(const char *dirname, char ***filenamelist);

#ifdef __cplusplus
}
#endif

#endif  // SHARED_INCLUDE_SFILE_H_
