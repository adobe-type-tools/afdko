/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

/*
 * File input/output.
 */

#include "file.h"
#include "lstdio.h"
#include "lstring.h"
#include "lerrno.h"

#if SUNOS
/* SUNOS doesn't define strerror in libc */
char *strerror(int errno) {
    extern int sys_nerr;
    extern char *sys_errlist[];
    return (errno >= 0 && errno <= sys_nerr) ? sys_errlist[errno] : "unknown error";
}

#endif /* SUNOS */

/* Print file error message and quit */
void fileError(File *f) {
    cbFatal(f->h, "file error <%s> [%s]", strerror(errno), f->name);
}

/* Open file and check for errors */
void fileOpen(File *f, cbCtx h, char *filename, char *mode) {
    f->h = h;
    f->name = filename;
    f->fp = fopen(filename, mode);
    if (f->fp == NULL) {
        fileError(f);
    }
}

/* Checks whether file exists */
int fileExists(char *filename) {
    FILE *fp = fopen(filename, "r");
    if (fp == NULL) {
        return 0;
    } else {
        fclose(fp);
        return 1;
    }
}

/* Read from file */
int fileReadN(File *f, size_t count, void *ptr) {
    size_t n = fread(ptr, 1, count, f->fp);
    if (n == 0 && ferror(f->fp)) {
        fileError(f);
    }
    return n;
}

/* Write to file */
int fileWriteN(File *f, size_t count, void *ptr) {
    size_t n = fwrite(ptr, 1, count, f->fp);
    if (n != count) {
        fileError(f);
    }
    return n;
}

/* Get line from file */
char *fileGetLine(File *f, char *s, int n) {
    char *newLine = fgets(s, n, f->fp);
    if (newLine == NULL) {
        return newLine;
    }

    if (feof(f->fp)) {
        int len = strlen(s);
        if (len + 1 < n) {
            /*  If we hit EOF in a line, then the string in 's' is terminated without a final newline.
             In order to satisfy the logic in cbAliasDBRead(), we add a terminal newline */
            s[len] = '\n';
            s[len + 1] = '\0';
        }
    }
    return newLine;
}

/* Seek on file */
void fileSeek(File *f, long offset, int wherefrom) {
    if (fseek(f->fp, offset, wherefrom) != 0) {
        fileError(f);
    }
}

/* Return file stream position */
long fileTell(File *f) {
    long posn = ftell(f->fp);
    if (posn == -1) {
        fileError(f);
    }
    return posn;
}

/* Check for errors and close file */
void fileClose(File *f) {
    if (ferror(f->fp)) {
        fileError(f);
    }
    fclose(f->fp);
}
