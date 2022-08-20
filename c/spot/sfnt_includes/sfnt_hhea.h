/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Horizontal header table format definition.
 */

#ifndef FORMAT_HHEA_H
#define FORMAT_HHEA_H

#define hhea_VERSION VERSION(1, 0)

typedef struct
{
    Fixed version;
    FWord ascender;
    FWord descender;
    FWord lineGap;
    uFWord advanceWidthMax;
    FWord minLeftSideBearing;
    FWord minRightSideBearing;
    FWord xMaxExtent;
    int16_t caretSlopeRise;
    int16_t caretSlopeRun;
    int16_t caretOffset;
    int16_t reserved[4];
    int16_t metricDataFormat;
    uint16_t numberOfLongHorMetrics;
} hheaTbl;

#endif /* FORMAT_HHEA_H */
