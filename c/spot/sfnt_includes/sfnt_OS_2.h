/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * OS/2 table format definition.
 */

#ifndef FORMAT_OS_2_H
#define FORMAT_OS_2_H

typedef struct
{
    uint16_t version;

    uint16_t averageWidth;
    uint16_t weightClass;
    uint16_t widthClass;
    uint16_t type;
    uint16_t subscriptXSize;
    uint16_t subscriptYSize;
    int16_t subscriptXOffset;
    int16_t subscriptYOffset;
    uint16_t superscriptXSize;
    uint16_t superscriptYSize;
    int16_t superscriptXOffset;
    int16_t superscriptYOffset;
    uint16_t strikeoutSize;
    int16_t strikeoutPosition;
    uint16_t familyClass;
    uint8_t panose[10];
    uint32_t charRange[4];
    uint8_t vendor[4];
    uint16_t selection;
    uint16_t firstChar;
    uint16_t lastChar;
    int16_t typographicAscent;
    int16_t typographicDescent;
    int16_t typographicLineGap;
    uint16_t windowsAscent;
    uint16_t windowsDescent;
    uint32_t CodePageRange[2];        /* Version 1 */
    int16_t XHeight;                  /* Version 2 */
    int16_t CapHeight;                /* Version 2 */
    uint16_t DefaultChar;             /* Version 2 */
    uint16_t BreakChar;               /* Version 2 */
    int16_t maxContext;               /* Version 2 */
    uint16_t usLowerOpticalPointSize; /* Version 5 */
    uint16_t usUpperOpticalPointSize; /* Version 5 */
} OS_2Tbl;

#endif /* FORMAT_OS_2_H */
