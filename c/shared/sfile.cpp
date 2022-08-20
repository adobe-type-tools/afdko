/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

/*
 * File input/output.
 */

#include "sfile.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <filesystem>

#include "supportdefines.h"
#include "slogger.h"
#include "smem.h"

/* Print file error message and quit */
void sFileError(sFile *f) {
    sLog(sFATAL, "file error <%s> [%s]", strerror(errno), f->name);
}

/* Open file and check for errors */
void sFileOpen(sFile *f, const char *filename, const char *mode) {
    int l = strlen(filename)+1;
    f->name = (char *) sMemNew(l);
    STRCPY_S(f->name, l, filename);
    f->fp = fopen(filename, mode);
    if (f->fp == NULL) {
        sFileError(f);
    }
}

int sFileLen(sFile *f) {
    int at = fseek(f->fp, 0, SEEK_END);
    if (at == -1)
        return -1;
    at = fseek(f->fp, 0, SEEK_CUR);
    return at;
}

/* Checks whether file exists */
int sFileExists(const char *filename) {
    auto s = std::filesystem::status(filename);
    return std::filesystem::exists(s);
}

/* Checks whether file exists */
int sFileIsDir(const char *filename) {
    auto s = std::filesystem::status(filename);
    return std::filesystem::is_directory(s);
}

/* Read from file */
int sFileReadN(sFile *f, size_t count, void *ptr) {
    size_t n = fread(ptr, 1, count, f->fp);
    if (n == 0 && ferror(f->fp)) {
        sFileError(f);
    }
    return n;
}

/* Write to file */
int sFileWriteN(sFile *f, size_t count, void *ptr) {
    size_t n = fwrite(ptr, 1, count, f->fp);
    if (n != count) {
        sFileError(f);
    }
    return n;
}

/* Get line from file */
char *sFileGetLine(sFile *f, char *s, int n) {
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
void sFileSeek(sFile *f, long offset, int wherefrom) {
    if (fseek(f->fp, offset, wherefrom) != 0) {
        sFileError(f);
    }
}

/* Return file stream position */
long sFileTell(sFile *f) {
    long posn = ftell(f->fp);
    if (posn == -1) {
        sFileError(f);
    }
    return posn;
}

/* Check for errors and close file */
void sFileClose(sFile *f) {
    if (ferror(f->fp)) {
        sFileError(f);
    }
    fclose(f->fp);
    f->fp = NULL;
    sMemFree(f->name);
    f->name = NULL;
}

/* 1, 2, and 4 byte big-endian machine independent input */
void sFileReadObject(sFile *f, int size, void *obj) {
    uint32_t value;
    uint8_t b;
#define GET1B sFileReadN(f, 1, &b)

    switch (size) {
        case 1:
            GET1B;
            *((uint8_t *)obj) = b;
            break;
        case 2:
            GET1B;
            value = b;
            GET1B;
            value = value << 8 | b;
            *((uint16_t *)obj) = (uint16_t)value;
            break;
        case 4:
            GET1B;
            value = b;
            GET1B;
            value = value << 8 | b;
            GET1B;
            value = value << 8 | b;
            GET1B;
            value = value << 8 | b;
            *((uint32_t *)obj) = value;
            break;
        default:
            sLog(sFATAL, "Bad input object size [%d]", size);
            break;
    }
}

/* 1, 2, and 4 byte big-endian machine independent output */
void sFileWriteObject(sFile *f, int size, uint32_t value) {
    uint8_t b;
#define PUT1B sFileWriteN(f, 1, &(b))

    switch (size) {
        case 1:
            b = (uint8_t)(value & 0xFF);
            PUT1B;
            break;
        case 2:
            b = (uint8_t)(value >> 8 & 0xFF);
            PUT1B;
            b = (uint8_t)(value & 0xFF);
            PUT1B;
            break;
        case 4:
            b = (uint8_t)(value >> 24 & 0xFF);
            PUT1B;
            b = (uint8_t)(value >> 16 & 0xFF);
            PUT1B;
            b = (uint8_t)(value >> 8 & 0xFF);
            PUT1B;
            b = (uint8_t)(value & 0xFF);
            PUT1B;
            break;
        default:
            sLog(sFATAL, "Bad output object size [%d]", size);
            break;
    }
}

int sFileReadInputDir(const char *dirname, char ***filenamelist) {
    const std::filesystem::path dpath{dirname};
    int fileCnt = 0;

    for (auto &file : std::filesystem::directory_iterator{dpath})
        if (!(file.path().c_str()[0] == '.'))
            fileCnt++;

    *filenamelist = (char **)sMemNew(fileCnt + 1 * sizeof(char *));

    int i = 0;
    for (auto &file : std::filesystem::directory_iterator{dpath}) {
        const std::string fp {file.path().string()};
        if (!(fp[0] == '.')) {
            (*filenamelist)[i] = (char *)sMemNew(fp.length() + 1);
            STRCPY_S((*filenamelist)[i], fp.length()+1, fp.c_str());
            i++;
        }
    }
    return fileCnt;
}
