/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Control point table format definition.
 */

#ifndef FORMAT_CNPT_H
#define FORMAT_CNPT_H

#define CNPT_VERSION VERSION(1, 0)

typedef struct
{
    uint16_t firstPoint;
    uint16_t nPoints;
} Element0;
#define ELEMENT0_SIZE (SIZEOF(Element0, firstPoint) + \
                       SIZEOF(Element0, nPoints))

typedef struct
{
    Element0 *index;
} Format0;

typedef struct
{
    GlyphId glyphId;
    uint16_t firstPoint;
    uint16_t nPoints;
} Element1;
#define ELEMENT1_SIZE (SIZEOF(Element1, glyphId) +    \
                       SIZEOF(Element1, firstPoint) + \
                       SIZEOF(Element1, nPoints))

typedef struct
{
    uint16_t searchRange;
    uint16_t entrySelector;
    uint16_t rangeShift;
    Element1 *index;
} Format1;
#define FORMAT1_SIZE (SIZEOF(Format1, searchRange) +   \
                      SIZEOF(Format1, entrySelector) + \
                      SIZEOF(Format1, rangeShift))

typedef struct
{
    Fixed version;
    uint16_t format;
    uint16_t flags;
    uint32_t indexLength;
    GlyphId firstGlyph;
    uint16_t nElements;
    void *formatSpecific;
    struct
    {
        uint32_t cnt;
        CNPTPoint *point;
    } points;
} CNPTTbl;

enum {
    CNPT_DENSE_FORMAT,
    CNPT_SPARSE_FORMAT
};

#endif /* FORMAT_CNPT_H */
