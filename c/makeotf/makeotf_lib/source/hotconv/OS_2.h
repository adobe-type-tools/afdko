/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************/

#ifndef HOTCONV_OS_2_H
#define HOTCONV_OS_2_H

#include "common.h"

#define OS_2_ TAG('O', 'S', '/', '2')

/* Standard functions */
void OS_2New(hotCtx g);
int OS_2Fill(hotCtx g);
void OS_2Write(hotCtx g);
void OS_2Reuse(hotCtx g);
void OS_2Free(hotCtx g);

/* Supplementary functions */
void OS_2SetUnicodeRanges(hotCtx g,
                          uint32_t ulUnicodeRange1,
                          uint32_t ulUnicodeRange2,
                          uint32_t ulUnicodeRange3,
                          uint32_t ulUnicodeRange4);
void OS_2SetCodePageRanges(hotCtx g,
                           uint32_t lCodePageRange1,
                           uint32_t ulCodePageRange2);
void OS_2SetCharIndexRange(hotCtx g,
                           uint16_t usFirstCharIndex,
                           uint16_t usLastCharIndex);
void OS_2SetMaxContext(hotCtx g, unsigned maxContext);
void OS_2SetPanose(hotCtx g, char *panose);
void OS_2SetFSType(hotCtx g, uint16_t fsType);
void OS_2SetWeightClass(hotCtx g, uint16_t weightClass);
void OS_2SetWidthClass(hotCtx g, uint16_t widthClass);
void OS_2LowerOpticalPointSize(hotCtx g, uint16_t opSize);
void OS_2UpperOpticalPointSize(hotCtx g, uint16_t opSize);
void OS_2FamilyClass(hotCtx g, uint16_t familyClass);

#endif /* HOTCONV_OS_2_H */
