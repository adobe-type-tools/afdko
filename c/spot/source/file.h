/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)file.h	1.5
 * Changed:    2/12/99 13:40:22
 ***********************************************************************/

/*
 * File input support.
 */

#ifndef FILE_H
#define FILE_H

extern void fileOpen(Byte8 *filename);
extern IntX fileIsOpened(void);
extern IntX fileExists(Byte8 *filename);
extern void fileClose(void);
extern Card32 fileTell(void);
extern void fileSeek(Card32 offset, int relative);
extern void fileSeekAbsNotBuffered(Card32 offset);
extern void fileReadBytes(Int32 count, Card8 *buf);
#if USE_STDARG
extern void fileReadObject(IntX size, ...);
#else
extern void fileReadObject(IntX size, void *ptr);
#endif
extern Byte8 *fileName(void);
extern Card32 fileSniff(void);

#if MACINTOSH
extern void fileOpenMacRes(Byte8 *filename);
#endif

/* Convienience macros */
#define TELL()			fileTell()
#define SEEK_ABS(o)		fileSeek((o),0)		/* From beginning of file */
#define SEEK_REL(o)		fileSeek((o),1)		/* From current postion */
#define SEEK_SURE(o)  fileSeekAbsNotBuffered((o))
#define IN_BYTES(c,b) 	fileReadBytes((c),(b))
#define IN1(o)			fileReadObject(sizeof(o),&(o))
#define IN3(o)		{unsigned char ui8; ui8 = fileReadObject(sizeof(o),&(o))

#endif /* FILE_H */
