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
#if USE_STDARG
#include <stdarg.h>
#endif
#include <string.h>
#include "spot_global.h"
#include "file.h"
#include "sfile.h"

#ifndef CTL_CDECL
#define CTL_CDECL
#endif

static struct
{
    sFile file;           /* File identifier */
    uint8_t buf[BUFSIZ]; /* Input buffer */
    uint8_t *next;       /* Next character to read */
    uint8_t *end;        /* One past end of filled buffer */
} file;

int sdFileExists(char *filename) {
    return (sFileExists(filename));
}

/* Open file read-only */
void fileOpen(char *filename) {
    sFileOpen(&file.file, filename, "rb");
    file.end = file.next = file.buf;
    memset(file.buf, 0, BUFSIZ);
}

int fileIsOpened(void) {
    return (file.file.fp != NULL);
}

/* Close file */
void fileClose(void) {
    if (file.file.fp != NULL)
        sFileClose(&file.file);
    file.next = file.end = NULL;
}

/* Return file position */
uint32_t fileTell(void) {
    return sFileTell(&file.file) - (file.end - file.next);
}

/* Seek to absolute or relative offset */
void fileSeek(uint32_t offset, int relative) {
    uint32_t at = sFileTell(&file.file);
    uint32_t to = relative ? at + offset : offset;
    /* require that any SEEK invalidates buffer */
    sFileSeek(&file.file, to, 0);
    file.next = file.end = file.buf;
}

void fileSeekAbsNotBuffered(uint32_t offset) {
    fileSeek(offset, 0);
}

/* Fill buffer */
static void fillBuf(void) {
    int count = sFileReadN(&file.file, BUFSIZ, file.buf);
    if (count == 0)
        spotFatal(SPOT_MSG_EARLYEOF, file.file.name);
    file.end = file.buf + count;
    file.next = file.buf;
}

/* Supply specified number of bytes */
void fileReadBytes(int32_t count, char *buf) {
    while (count > 0) {
        int size;
        int left = file.end - file.next;

        if (left == 0) {
            fillBuf();
            left = file.end - file.next;
        }

        size = (left < count) ? left : count;

        memcpy(buf, file.next, size);
        file.next += size;
        buf += size;
        count -= size;
    }
}

#if USE_STDARG
/* 1, 2, and 4 byte big-endian machine independent input */
void CTL_CDECL fileReadObject(int size, ...) {
    int32_t value;
    va_list ap;

    va_start(ap, size);
    if (file.end - file.next >= size) {
        /* Request doesn't cross block boundary */
        switch (size) {
            case 1:
                *va_arg(ap, int8_t *) = *file.next++;
                break;
            case 2:
                value = *file.next++;
                *va_arg(ap, int16_t *) = value << 8 | *file.next++;
                break;
            case 4:
                value = *file.next++;
                value = value << 8 | *file.next++;
                value = value << 8 | *file.next++;
                *va_arg(ap, int32_t *) = value << 8 | *file.next++;
                break;
            default:
                spotFatal(SPOT_MSG_BADREADSIZE, size);
                break;
        }
    } else {
        /* Read across block boundary */
        if (file.next == file.end) fillBuf();
        switch (size) {
            case 1:
                *va_arg(ap, int8_t *) = *file.next++;
                break;
            case 2:
                value = *file.next++;
                if (file.next == file.end) fillBuf();
                *va_arg(ap, int16_t *) = value << 8 | *file.next++;
                break;
            case 4:
                value = *file.next++;
                if (file.next == file.end) fillBuf();
                value = value << 8 | *file.next++;
                if (file.next == file.end) fillBuf();
                value = value << 8 | *file.next++;
                if (file.next == file.end) fillBuf();
                *va_arg(ap, int32_t *) = value << 8 | *file.next++;
                break;
            default:
                spotFatal(SPOT_MSG_BADREADSIZE, size);
                break;
        }
    }
    va_end(ap);
}
#else
/* 1, 2, and 4 byte big-endian machine independent input */
void fileReadObject(int size, void *ap) {
    int32_t value, final;

    if (file.end - file.next >= size) {
        /* Request doesn't cross block boundary */
        switch (size) {
            case 1:
                final = *file.next++;
                *((int8_t *)ap) = (int8_t)final;
                break;
            case 2:
                value = *file.next++;
                final = value << 8 | *file.next++;
                *((int16_t *)ap) = (int16_t)final;
                break;
            case 4:
                value = *file.next++;
                value = value << 8 | *file.next++;
                value = value << 8 | *file.next++;
                final = value << 8 | *file.next++;
                *((int32_t *)ap) = (int32_t)final;
                break;
            default:
                spotFatal(SPOT_MSG_BADREADSIZE, size);
                break;
        }
    } else {
        /* Read across block boundary */
        if (file.next == file.end) fillBuf();
        switch (size) {
            case 1:
                final = *file.next++;
                *((int8_t *)ap) = (int8_t)final;
                break;
            case 2:
                value = *file.next++;
                if (file.next == file.end) fillBuf();
                final = value << 8 | *file.next++;
                *((int16_t *)ap) = (int16_t)final;
                break;
            case 4:
                value = *file.next++;
                if (file.next == file.end) fillBuf();
                value = value << 8 | *file.next++;
                if (file.next == file.end) fillBuf();
                value = value << 8 | *file.next++;
                if (file.next == file.end) fillBuf();
                final = value << 8 | *file.next++;
                *((int32_t *)ap) = (int32_t)final;
                break;
            default:
                spotFatal(SPOT_MSG_BADREADSIZE, size);
                break;
        }
    }
}
#endif

char *fileName(void) {
    return file.file.name;
}

uint32_t fileSniff(void) {
    int count = 0;
    uint32_t value;
    count = sFileReadN(&file.file, 4, file.buf);
    file.end = file.buf + 4;
    if (count == 0)
        value = 0xBADBAD;
    else {
        file.next = file.buf;
        value = *file.next++;
        value = value << 8 | *file.next++;
        value = value << 8 | *file.next++;
        value = value << 8 | *file.next++;
    }
    return value;
}
