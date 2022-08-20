/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * File input support.
 */

#include <stdbool.h>

#ifndef SFNTDIFF_DFILE_H_
#define SFNTDIFF_DFILE_H_

extern void sdFileOpen(uint8_t which, char *filename);
extern int sdFileIsOpened(uint8_t which);
extern void sdFileClose(uint8_t which);
extern void sdFileSeek(uint8_t which, uint32_t offset, int relative);
extern void sdFileSeekAbsNotBuffered(uint8_t which, uint32_t offset);
extern void sdFileReadBytes(uint8_t which, int32_t count, uint8_t *buf);
extern void sdFileReadObject(uint8_t which, int size, ...);
extern uint32_t sdFileSniff(uint8_t which);
extern bool isSupportedFontFormat(uint32_t value, char *fname);

/* Convenience macros */
#define SEEK_ABS(which, o)    sdFileSeek((which), (o), 0) /* From beginning of file */
#define SEEK_REL(which, o)    sdFileSeek((which), (o), 1) /* From current position  */
#define SEEK_SURE(which, o)   sdFileSeekAbsNotBuffered((which), (o))
#define IN_BYTES(which, c, b) sdFileReadBytes((which), (c), (b))
#define IN(which, o)          sdFileReadObject((which), sizeof(o), &(o))

#endif  // SFNTDIFF_DFILE_H_
