/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Machine dependent file operations.
 */

#ifndef DSYS_H
#define DSYS_H

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
#define sysPathSep ("\\")
#else
#define sysPathSep ("/")
#endif

#define sysDIRECTORY -111

#endif /* DSYS_H */
