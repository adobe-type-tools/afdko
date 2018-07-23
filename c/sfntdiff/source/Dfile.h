/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)file.h	1.3
 * Changed:    12/15/97 08:57:35
 ***********************************************************************/

/*
 * File input support.
 */

#ifndef FILE_H
#define FILE_H

extern void fileOpen(Int8 which, Byte8 *filename);
extern IntX fileIsOpened(Card8 which);
extern IntX fileExists(Byte8 *filename);
extern void fileClose(Card8 which);
extern Card32 fileTell(Card8 which);
extern void fileSeek(Card8 which, Card32 offset, int relative);
extern void fileSeekAbsNotBuffered(Card8 which, Card32 offset);
extern void fileReadBytes(Card8 which, Int32 count, Card8 *buf);
extern void fileReadObject(Card8 which, IntX size, ...);
extern Byte8 *fileName(Card8 which);
extern Card32 fileSniff(Card8 which);


/* Convienience macros */
#define TELL(which)			fileTell((which))
#define SEEK_ABS(which,o)	fileSeek((which),(o),0)		/* From beginning of file */
#define SEEK_REL(which,o)	fileSeek((which),(o),1)		/* From current postion */
#define SEEK_SURE(which,o)  fileSeekAbsNotBuffered((which),(o))
#define IN_BYTES(which,c,b) fileReadBytes((which),(c),(b))
#define IN(which,o)			fileReadObject((which),sizeof(o),&(o))
#endif /* FILE_H */
