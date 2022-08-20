/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * PostScript printer table format definition.
 */

#ifndef FORMAT_POST_H
#define FORMAT_POST_H

typedef struct
{
    uint16_t numberGlyphs;
    uint16_t *glyphNameIndex;
    uint8_t *names;
} Format2_0;

typedef struct
{
    uint16_t numberGlyphs;
    int8_t *offset;
} Format2_5;

typedef struct
{
    uint16_t *code;
} Format4_0;

typedef struct
{
    Fixed version;
    Fixed italicAngle;
    FWord underlinePosition;
    FWord underlineThickness;
    uint32_t isFixedPitch;
    uint32_t minMemType42;
    uint32_t maxMemType42;
    uint32_t minMemType1;
    uint32_t maxMemType1;
    void *format;
} postTbl;
#define TBL_HDR_SIZE (SIZEOF(postTbl, version) +            \
                      SIZEOF(postTbl, italicAngle) +        \
                      SIZEOF(postTbl, underlinePosition) +  \
                      SIZEOF(postTbl, underlineThickness) + \
                      SIZEOF(postTbl, isFixedPitch) +       \
                      SIZEOF(postTbl, minMemType42) +       \
                      SIZEOF(postTbl, maxMemType42) +       \
                      SIZEOF(postTbl, minMemType1) +        \
                      SIZEOF(postTbl, maxMemType1))

#endif /* FORMAT_POST_H */
