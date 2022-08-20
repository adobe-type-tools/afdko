/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * File input support.
 */

#ifndef FILE_H
#define FILE_H

extern void fileOpen(char *filename);
extern int fileIsOpened(void);
extern int sdFileExists(char *filename);
extern void fileClose(void);
extern uint32_t fileTell(void);
extern void fileSeek(uint32_t offset, int relative);
extern void fileSeekAbsNotBuffered(uint32_t offset);
extern void fileReadBytes(int32_t count, char *buf);
#if USE_STDARG
extern void fileReadObject(int size, ...);
#else
extern void fileReadObject(int size, void *ptr);
#endif
extern char *fileName(void);
extern uint32_t fileSniff(void);

/* Convenience macros */
#define TELL() fileTell()
#define SEEK_ABS(o) fileSeek((o), 0) /* From beginning of file */
#define SEEK_REL(o) fileSeek((o), 1) /* From current position */
#define SEEK_SURE(o) fileSeekAbsNotBuffered((o))
#define IN_BYTES(c, b) fileReadBytes((c), (b))
#define IN1(o) fileReadObject(sizeof(o), &(o))
#define IN3(o)             \
    {                      \
        unsigned char ui8; \
        ui8 = fileReadObject(sizeof(o), &(o))

#endif /* FILE_H */
