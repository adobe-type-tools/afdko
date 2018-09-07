/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/* File input support. */

#ifndef FILE_H
#define FILE_H

typedef struct /* File data */
{
    char *name;
    FILE *fp;
} File;

extern void fileOpenRead(char *filename, File *fyl);
extern void fileOpenWrite(char *filename, File *fyl);
extern long fileLength(File *fyl);
extern int fileIsOpened(File *fyl);
extern int fileExists(char *filename);
extern void fileClose(File *fyl);
extern Card32 fileTell(File *fyl);
extern void fileSeek(File *file, long offset, int wherefrom);
extern int fileReadN(File *file, size_t count, void *ptr);
extern int fileWriteN(File *file, size_t count, void *ptr);
extern void fileReadObject(File *fyl, IntX size, void *obj);
extern void fileWriteObject(File *fyl, IntX size, Card32 value);
extern void fileCopy(File *src, File *dst, size_t count);

#endif /* FILE_H */
