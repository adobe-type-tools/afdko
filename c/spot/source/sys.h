/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Machine dependent file operations.
 */

#ifndef SYS_H
#define SYS_H

#include "global.h"

IntX sysOpen(Byte8 *filename);
IntX sysOpenSearchpath(Byte8 *filename);
IntX sysFileExists(Byte8 *filename);
void sysClose(IntX fd, Byte8 *filename);
Int32 sysTell(IntX fd, Byte8 *filename);
void sysSeek(IntX fd, Int32 offset, IntX relative, Byte8 *filename);
Int32 sysFileLen(IntX fd);
IntX sysRead(IntX fd, Card8 *buf, IntX size, Byte8 *filename);
Byte8 *sysOurtime(void);
#if MACINTOSH
IntX sysOpenMac(Byte8 *OUTfilename);
IntX sysMacOpenRes(Byte8 *filename);
FILE *sysPutMacFILE(Byte8 *defaultname, Byte8 *OUTfilename);
FILE *sysPutMacDesktopFILE(Byte8 *defaultname, Byte8 *OUTfilename);
IntX sysMacDesktopFileExists(Byte8 *defaultname, Byte8 *OUTfilename);
#endif
IntX sysOpenSearchpath(Byte8 *filename);

#if WIN32
#define SAFE_LOCALTIME(x, y) localtime_s(y, x)
#else
#define SAFE_LOCALTIME(x, y) localtime_r(x, y)
#endif

#endif /* SYS_H */
