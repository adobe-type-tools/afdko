/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Vertical Origin table definition.
 */

#ifndef FORMAT_VORG_H
#define FORMAT_VORG_H

#define VORG_VERSION VERSION(1, 0)

typedef struct
{
    uint16_t glyphIndex;
    int16_t vertOriginY;
} vertOriginYMetric;

typedef struct
{
    uint16_t major, minor;
    int16_t defaultVertOriginY;
    uint16_t numVertOriginYMetrics;
    vertOriginYMetric *vertMetrics;
} VORGTbl;

#endif /* FORMAT_VMTX_H */
