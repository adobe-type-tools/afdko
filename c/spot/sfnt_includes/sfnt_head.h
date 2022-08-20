/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Font header table format definition.
 */

#ifndef FORMAT_HEAD_H
#define FORMAT_HEAD_H

#define head_VERSION VERSION(1, 0)

#define DATE_TIME_SIZE 8
typedef uint8_t longDateTime[DATE_TIME_SIZE];

typedef struct
{
    Fixed version;
    Fixed fontRevision;
    uint32_t checkSumAdjustment;
    uint32_t magicNumber;
#define head_MAGIC 0x5F0F3CF5
    uint16_t flags;
#define head_SET_LSB (1 << 1)
    uint16_t unitsPerEm;
    longDateTime created;
    longDateTime modified;
    FWord xMin;
    FWord yMin;
    FWord xMax;
    FWord yMax;
    uint16_t macStyle;
    uint16_t lowestRecPPEM;
    int16_t fontDirectionHint;
#define head_STRONGL2R 1
    int16_t indexToLocFormat;
#define head_LONGOFFSETSUSED 1
    int16_t glyphDataFormat;
} headTbl;

#endif /* FORMAT_HEAD_H */
