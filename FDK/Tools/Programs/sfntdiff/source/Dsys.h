/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)sys.h	1.2
 * Changed:    12/15/97 08:58:26
 ***********************************************************************/

/*
 * Machine dependent file operations.
 */

#ifndef SYS_H
#define SYS_H

#include "Dglobal.h"

IntX sysIsDir(Byte8 *name);
IntX sysOpen(Byte8 *filename);
IntX sysOpenSearchpath(Byte8 *filename);
IntX sysFileExists(Byte8 *filename);
void sysClose(IntX fd, Byte8 *filename);
Int32 sysTell(IntX fd, Byte8 *filename);
void sysSeek(IntX fd, Int32 offset, IntX relative, Byte8 *filename);
Int32 sysFileLen(IntX fd);
IntX sysRead(IntX fd, Card8 *buf, IntX size, Byte8 *filename);
IntX sysReadInputDir(Byte8 *dirName, Byte8 ***fileNameList);

#if WIN32 || MSDOS
#define sysPathSep  ("\\")
#else
#define sysPathSep  ("/")
#endif

#define sysDIRECTORY  -111

#endif /* SYS_H */
