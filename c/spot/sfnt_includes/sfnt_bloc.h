/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef FORMAT_BLOC_H
#define FORMAT_BLOC_H

#define BLOC_VERSION VERSION(1, 0)

#include "sfnt_sbit.h"

typedef struct _blocBitmapSizeTable {
    uint32_t indexSubTableArrayOffset; /* offset to corresponding index */
                                     /* subtable array from beginning */
                                     /* of "bloc"                     */
    uint32_t indexTableSize;           /* length of corresponding index subtables and array   */
    uint32_t numberofIndexSubTables;   /* number of index subtables.                          */
                                     /* There is a subtable for each range or format change */
    uint32_t colorRef;                 /* set to 0. ignore for now                            */
    sbitLineMetrics hori;
    sbitLineMetrics vert;
    GlyphId startGlyphIndex;
    GlyphId endGlyphIndex;
    uint8_t ppemX; /* target horizontal pixels/em */
    uint8_t ppemY;
    uint8_t bitDepth;
    int8_t flags;
#define BLOC_FLAG_HORIZONTAL (1 << 0)
#define BLOC_FLAG_VERTICAL (1 << 1)
    uint8_t _index;
} blocBitmapSizeTable;

#define BLOCBITMAPSIZETABLE_SIZE (SIZEOF(blocBitmapSizeTable, indexSubTableArrayOffset) + \
                                  SIZEOF(blocBitmapSizeTable, indexTableSize) +           \
                                  SIZEOF(blocBitmapSizeTable, numberofIndexSubTables) +   \
                                  SIZEOF(blocBitmapSizeTable, colorRef) +                 \
                                  SBITLINEMETRICS_SIZE +                                  \
                                  SBITLINEMETRICS_SIZE +                                  \
                                  SIZEOF(blocBitmapSizeTable, startGlyphIndex) +          \
                                  SIZEOF(blocBitmapSizeTable, endGlyphIndex) +            \
                                  SIZEOF(blocBitmapSizeTable, ppemX) +                    \
                                  SIZEOF(blocBitmapSizeTable, ppemY) +                    \
                                  SIZEOF(blocBitmapSizeTable, bitDepth) +                 \
                                  SIZEOF(blocBitmapSizeTable, flags))

/* ...................BLOC SUBTABLES.................................... */

typedef struct _blocIndexSubHeader {
    uint16_t indexFormat;     /* sbitBitmapIndexFormats */
    uint16_t imageFormat;     /* sbitBitmapDataFormats  */
    uint32_t imageDataOffset; /*offset to corresponding image data from beginning of bdat table*/
} blocIndexSubHeader;

#define BLOCINDEXSUBHEADER_SIZE (SIZEOF(blocIndexSubHeader, indexFormat) + \
                                 SIZEOF(blocIndexSubHeader, imageFormat) + \
                                 SIZEOF(blocIndexSubHeader, imageDataOffset))

/* format 1 has variable-length images of the same format for
   uncompressed PROPORTIONALLY-spaced glyphs */
typedef struct _blocIndexSubtable_Format1 {
    blocIndexSubHeader header;
    uint32_t _numoffsets;
    DCL_ARRAY(uint32_t, offsetArray); /* offsetArray[glyphIndex] + imageDataOffset ==> start of bitmap data for glyph. */
                                    /* sizeOfArray = lastGlyph - firstGlyph + 1                                      */
} blocIndexSubtable_Format1;

#define BLOCFORMAT1_SIZE(first, last)            \
    (SIZEOF(blocIndexSubtable_Format1, header) + \
     sizeof(uint32_t) * ((last) - (first) + 1))

/* format 2 has fixed-length images of the same format for
   MONO-spaced glyphs */
typedef struct _blocIndexSubtable_Format2 {
    blocIndexSubHeader header;
    uint32_t imageSize;            /* images may be compressed, bit-aligned or byte-aligned */
    sbitBigGlyphMetrics Metrics; /* all glyphs share same metrics */
} blocIndexSubtable_Format2;

#define BLOCFORMAT2_SIZE (SIZEOF(blocIndexSubtable_Format2, header) +    \
                          SIZEOF(blocIndexSubtable_Format2, imageSize) + \
                          SBITBIGGLYPHMETRICS_SIZE)

/* format 3 is similar to format 1, but with 16-bit offsets, for
   compressed PROPORTIONALLY-spaced glyphs.
   Must be padded to a long-word boundary. */
typedef struct _blocIndexSubtable_Format3 {
    blocIndexSubHeader header;
    uint32_t _numoffsets;
    DCL_ARRAY(uint16_t, offsetArray);
} blocIndexSubtable_Format3;

#define BLOCFORMAT3_SIZE(first, last)            \
    (SIZEOF(blocIndexSubtable_Format1, header) + \
     sizeof(uint16_t) * ((last) - (first) + 1))

typedef struct _blocCodeOffsetPair {
    GlyphId glyphCode; /* code of glyph present */
    uint16_t offset;     /* location in "bdat" */
} blocCodeOffsetPair;
#define BLOCCODEOFFSETPAIR_SIZE (SIZEOF(blocCodeOffsetPair, glyphCode) + \
                                 SIZEOF(blocCodeOffsetPair, offset))

/* format 4 is for sparsely-embedded glyph data for
   PROPORTIONAL metrics */
typedef struct _blocIndexSubtable_Format4 {
    blocIndexSubHeader header;
    uint32_t numGlyphs;
    DCL_ARRAY(blocCodeOffsetPair, glyphArray); /* one per glyph */
} blocIndexSubtable_Format4;

#define BLOCFORMAT4_SIZE(numglyphs)                 \
    (SIZEOF(blocIndexSubtable_Format4, header) +    \
     SIZEOF(blocIndexSubtable_Format4, numGlyphs) + \
     (BLOCCODEOFFSETPAIR_SIZE) * (numglyphs))

/* format 5 is for sparsely-embedded glyph data of
   fixed-sized, MONO-spaced metrics */
typedef struct _blocIndexSubtable_Format5 {
    blocIndexSubHeader header;
    uint32_t imageSize;
    sbitBigGlyphMetrics Metrics;
    uint32_t numGlyphs;
    DCL_ARRAY(blocCodeOffsetPair, glyphArray); /* one per glyph */
} blocIndexSubtable_Format5;

#define BLOCFORMAT5_SIZE(numglyphs)                 \
    (SIZEOF(blocIndexSubtable_Format5, header) +    \
     SIZEOF(blocIndexSubtable_Format5, imageSize) + \
     SBITBIGGLYPHMETRICS_SIZE +                     \
     SIZEOF(blocIndexSubtable_Format5, numGlyphs) + \
     (BLOCCODEOFFSETPAIR_SIZE) * (numglyphs))

/* ...................BLOC TABLE.................................... */

typedef union _blocFormat {
    blocIndexSubtable_Format1 *f1;
    blocIndexSubtable_Format2 *f2;
    blocIndexSubtable_Format3 *f3;
    blocIndexSubtable_Format4 *f4;
    blocIndexSubtable_Format5 *f5;
} blocFormat;

typedef struct _blocFormats /* internal: for construction only */
{
    uint32_t _bytelen;
    sbitBitmapIndexFormats _fmttype;
    uint16_t _index;
    blocFormat _fmt;
} blocFormats;

typedef struct _blocIndexSubTableArrayElt {
    GlyphId firstGlyphIndex;
    GlyphId lastGlyphIndex;
    uint32_t additionalOffsetToIndexSubtable; /* add to indexSubTableArrayOffset to get offset from beginning of "bloc" table */
    blocFormats _subtable;
    uint16_t _index; /* index in subtable array */
} blocIndexSubTableArrayElt;

#define BLOCINDEXSUBTABLEARRAYELT_SIZE (SIZEOF(blocIndexSubTableArrayElt, firstGlyphIndex) + \
                                        SIZEOF(blocIndexSubTableArrayElt, lastGlyphIndex) +  \
                                        SIZEOF(blocIndexSubTableArrayElt, additionalOffsetToIndexSubtable))

typedef struct _blocIndexSubTableArray {
    uint16_t _numarrayelts;
    DCL_ARRAY(blocIndexSubTableArrayElt, _elts);
    uint16_t _index; /* index in Main bloc SubTableArray */
} blocIndexSubTableArray;

typedef struct _blocTableHeader {
#define BLOC_HEADER_VERSION 0x00020000
    Fixed version;
    uint32_t numSizes;
    DCL_ARRAY(blocBitmapSizeTable, bitmapSizeTable); /* array [numSizes] */
    /* for construction: array[numSizes].
       one-to-one correspondence with bitmapSizeTable  */
    DCL_ARRAY(blocIndexSubTableArray, _indexSubTableArray);
} blocTableHeader;

#define BLOCTABLEHEADER_SIZE(numsizes)   \
    (SIZEOF(blocTableHeader, version) +  \
     SIZEOF(blocTableHeader, numSizes) + \
     BLOCBITMAPSIZETABLE_SIZE * (numsizes))

#endif /* FORMAT_BLOC_H */
