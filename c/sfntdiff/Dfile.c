/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Machine independent file operations. The ANSI standard buffered i/o library
 * can't be used here because the Macintosh version doesn't support resource
 * reading.
 *
 * This code assumes that 32 bits is large enough to hold a file offset. This
 * seems a safe assumption since other code would break long before
 * fonts/resource files reached this size.
 */

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <stdbool.h>

#include "Dglobal.h"
#include "Dfile.h"
#include "sfile.h"

typedef struct {
    sFile file;
    uint8_t buf[BUFSIZ]; /* Input buffer */
    uint8_t *next;       /* Next character to read */
    uint8_t *end;        /* One past end of filled buffer */
} _file_;

static _file_ file1;
static _file_ file2;
static _file_ *filep;

/* Open file read-only */
void sdFileOpen(uint8_t which, char *filename) {
    filep = (which == 1) ? &file1 : &file2;
    sFileOpen(&filep->file, filename, "rb");
    filep->end = filep->next = filep->buf;
    memset(filep->buf, 0, BUFSIZ);
}

int sdFileIsOpened(uint8_t which) {
    filep = (which == 1) ? &file1 : &file2;
    return filep->file.fp != NULL;
}

/* Close file */
void sdFileClose(uint8_t which) {
    filep = (which == 1) ? &file1 : &file2;
    if (filep->file.fp != NULL)
        sFileClose(&filep->file);
    filep->next = filep->end = NULL;
}

void sdFileSeek(uint8_t which, uint32_t offset, int relative) {
    uint32_t at;
    uint32_t to;

    filep = (which == 1) ? &file1 : &file2;

    at = sFileTell(&filep->file);
    to = relative ? at + offset : offset;

    if (to >= at - (filep->end - filep->buf) && to < at)
        /* Offset already within current buffer */
        filep->next = filep->end - (at - to);
    else {
        /* Offset outside current buffer */
        sFileSeek(&filep->file, to, 0);
        filep->next = filep->end;
    }
}

void sdFileSeekAbsNotBuffered(uint8_t which, uint32_t offset) {
    filep = (which == 1) ? &file1 : &file2;
    sFileSeek(&filep->file, offset, 0);
    filep->next = filep->end = filep->buf;
    memset(filep->buf, 0, BUFSIZ);
}

/* Fill buffer */
static void fillBuf(_file_ *file) {
    int count = sFileReadN(&file->file, BUFSIZ, file->buf);
    if (count == 0)
        sdFatal("file error <premature EOF> [%s]\n", file->file.name);
    file->end = file->buf + count;
    file->next = file->buf;
}

void sdFileReadBytes(uint8_t which, int32_t count, uint8_t *buf) {
    filep = (which == 1) ? &file1 : &file2;

    while (count > 0) {
        int size;
        int left = filep->end - filep->next;
        if (left == 0) {
            fillBuf(filep);
            left = filep->end - filep->next;
        }

        size = (left < count) ? left : count;

        memcpy(buf, filep->next, size);
        filep->next += size;
        buf += size;
        count -= size;
    }
}

/* 1, 2, and 4 byte big-endian machine independent input */
void sdFileReadObject(uint8_t which, int size, ...) {
    int32_t value;
    va_list ap;

    filep = (which == 1) ? &file1 : &file2;

    va_start(ap, size);
    if (filep->end - filep->next >= size)
        /* Request doesn't cross block boundary */
        switch (size) {
            case 1:
                *va_arg(ap, int8_t *) = *filep->next++;
                break;
            case 2:
                value = *filep->next++;
                *va_arg(ap, int16_t *) = (int16_t)(value << 8 | *filep->next++);
                break;
            case 4:
                value = *filep->next++;
                value = value << 8 | *filep->next++;
                value = value << 8 | *filep->next++;
                *va_arg(ap, int32_t *) = value << 8 | *filep->next++;
                break;
            default:
                sdFatal("bad input object size [%d]\n", size);
                break;
        } else {
        /* Read across block boundary */
        if (filep->next == filep->end) fillBuf(filep);
        switch (size) {
            case 1:
                *va_arg(ap, int8_t *) = *filep->next++;
                break;
            case 2:
                value = *filep->next++;
                if (filep->next == filep->end) fillBuf(filep);
                *va_arg(ap, int16_t *) = (int16_t)(value << 8 | *filep->next++);
                break;
            case 4:
                value = *filep->next++;
                if (filep->next == filep->end) fillBuf(filep);
                value = value << 8 | *filep->next++;
                if (filep->next == filep->end) fillBuf(filep);
                value = value << 8 | *filep->next++;
                if (filep->next == filep->end) fillBuf(filep);
                *va_arg(ap, int32_t *) = value << 8 | *filep->next++;
                break;
            default:
                sdFatal("bad input object size [%d]\n", size);
                break;
        }
    }
    va_end(ap);
}

uint32_t sdFileSniff(uint8_t which) {
    int count = 0;
    uint32_t value = 0;

    sdFileSeekAbsNotBuffered(which, 0);

    filep = (which == 1) ? &file1 : &file2;

    count = sFileReadN(&filep->file, 4, filep->buf);
    if (count == 0)
        value = 0xBADBAD;
    else {
        filep->end = filep->buf + 4;
        filep->next = filep->buf;
        value = *filep->next++;
        value = value << 8 | *filep->next++;
        value = value << 8 | *filep->next++;
        value = value << 8 | *filep->next++;
    }

    sdFileSeekAbsNotBuffered(which, 0);
    return value;
}

bool isSupportedFontFormat(uint32_t value, char *fname) {
    switch (value) {
        case bits_:
        case typ1_:
        case true_:
        case OTTO_:
        case VERSION(1, 0):
            return true;
            break;
        case 256:
        case ttcf_:
            sdWarning("unsupported file [%s] (ignored)\n", fname);
            return false;
            break;
        default:
            sdWarning("unsupported/bad file [%s] (ignored)\n", fname);
            return false;
    }
}
