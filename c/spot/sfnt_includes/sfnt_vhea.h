/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Vertical header table format definition.
 */

#ifndef FORMAT_VHEA_H
#define FORMAT_VHEA_H

#define vhea_VERSION VERSION(1, 0)

typedef struct
{
    Fixed version;
    FWord vertTypoAscender;
    FWord vertTypoDescender;
    FWord vertTypoLineGap;
    uFWord advanceHeightMax;
    FWord minTopSideBearing;
    FWord minBottomSideBearing;
    FWord yMaxExtent;
    int16_t caretSlopeRise;
    int16_t caretSlopeRun;
    int16_t caretOffset;
    int16_t reserved[4];
    int16_t metricDataFormat;
    uint16_t numberOfLongVerMetrics;
} vheaTbl;

#endif /* FORMAT_VHEA_H */
