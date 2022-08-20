/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Track kerning table format definition.
 */

#ifndef FORMAT_TRAK_H
#define FORMAT_TRAK_H

#define trak_VERSION VERSION(1, 0)

typedef struct
{
    Fixed level;
    uint16_t nameId;
    uint16_t offset;
    int16_t *value; /* Actually part of track data structure */
} Entry;
#define ENTRY_SIZE (SIZEOF(Entry, level) +  \
                    SIZEOF(Entry, nameId) + \
                    SIZEOF(Entry, offset))

typedef struct
{
    uint16_t nTracks;
    uint16_t nSizes;
    uint32_t sizeOffset;
    DCL_ARRAY(Entry, track);
    Fixed *size;
    /* Track values here */
} Data;
#define DATA_HDR_SIZE (SIZEOF(Data, nTracks) + \
                       SIZEOF(Data, nSizes) +  \
                       SIZEOF(Data, sizeOffset))

typedef struct
{
    Fixed version;
    uint16_t format;
    uint16_t horizOffset;
    uint16_t vertOffset;
    uint16_t reserved;
    Data horiz;
    Data vert;
} trakTbl;
#define TBL_HDR_SIZE (SIZEOF(trakTbl, version) +     \
                      SIZEOF(trakTbl, format) +      \
                      SIZEOF(trakTbl, horizOffset) + \
                      SIZEOF(trakTbl, vertOffset) +  \
                      SIZEOF(trakTbl, reserved))

#endif /* FORMAT_TRAK_H */
