/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

#ifndef OS_2_H
#define OS_2_H

#include "common.h"

#define OS_2_   TAG('O', 'S', '/', '2')

/* Standard functions */
void OS_2New(hotCtx g);
int OS_2Fill(hotCtx g);
void OS_2Write(hotCtx g);
void OS_2Reuse(hotCtx g);
void OS_2Free(hotCtx g);

/* Supplementary functions */
void OS_2SetUnicodeRanges(hotCtx        g,
                          unsigned long ulUnicodeRange1,
                          unsigned long ulUnicodeRange2,
                          unsigned long ulUnicodeRange3,
                          unsigned long ulUnicodeRange4);
void OS_2SetCodePageRanges(hotCtx        g,
                           unsigned long ulCodePageRange1,
                           unsigned long ulCodePageRange2);
void OS_2SetCharIndexRange(hotCtx         g,
                           unsigned short usFirstCharIndex,
                           unsigned short usLastCharIndex);
void OS_2SetMaxContext(hotCtx g, unsigned maxContext);
void OS_2SetPanose(hotCtx g, char *panose);
void OS_2SetFSType(hotCtx g, unsigned short fsType);
void OS_2SetWeightClass(hotCtx g, unsigned short weightClass);
void OS_2SetWidthClass(hotCtx g, unsigned short widthClass);
void OS_2LowerOpticalPointSize(hotCtx g, unsigned short opSize);
void OS_2UpperOpticalPointSize(hotCtx g, unsigned short opSize);

#endif /* OS_2_H */
