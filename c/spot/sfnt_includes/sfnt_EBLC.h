/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef FORMAT_EBLC_H
#define FORMAT_EBLC_H

#define EBLC_VERSION VERSION(1, 0)

#include "sfnt_sbit.h"

typedef struct _EBLCBitmapSizeTable {
    uint32_t indexSubTableArrayOffset; /* offset to corresponding index */
                                     /* subtable array from beginning */
                                     /* of "EBLC"                     */
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
#define EBLC_FLAG_HORIZONTAL (1 << 0)
#define EBLC_FLAG_VERTICAL (1 << 1)
    uint8_t _index;
} EBLCBitmapSizeTable;

#define EBLCBITMAPSIZETABLE_SIZE (SIZEOF(EBLCBitmapSizeTable, indexSubTableArrayOffset) + \
                                  SIZEOF(EBLCBitmapSizeTable, indexTableSize) +           \
                                  SIZEOF(EBLCBitmapSizeTable, numberofIndexSubTables) +   \
                                  SIZEOF(EBLCBitmapSizeTable, colorRef) +                 \
                                  SBITLINEMETRICS_SIZE +                                  \
                                  SBITLINEMETRICS_SIZE +                                  \
                                  SIZEOF(EBLCBitmapSizeTable, startGlyphIndex) +          \
                                  SIZEOF(EBLCBitmapSizeTable, endGlyphIndex) +            \
                                  SIZEOF(EBLCBitmapSizeTable, ppemX) +                    \
                                  SIZEOF(EBLCBitmapSizeTable, ppemY) +                    \
                                  SIZEOF(EBLCBitmapSizeTable, bitDepth) +                 \
                                  SIZEOF(EBLCBitmapSizeTable, flags))

/* ...................EBLC SUBTABLES.................................... */

typedef struct _EBLCIndexSubHeader {
    uint16_t indexFormat;     /* sbitBitmapIndexFormats */
    uint16_t imageFormat;     /* sbitBitmapDataFormats  */
    uint32_t imageDataOffset; /*offset to corresponding image data from beginning of EBDT table*/
} EBLCIndexSubHeader;

#define EBLCINDEXSUBHEADER_SIZE (SIZEOF(EBLCIndexSubHeader, indexFormat) + \
                                 SIZEOF(EBLCIndexSubHeader, imageFormat) + \
                                 SIZEOF(EBLCIndexSubHeader, imageDataOffset))

/* format 1 has variable-length images of the same format for
   uncompressed PROPORTIONALLY-spaced glyphs */
typedef struct _EBLCIndexSubtable_Format1 {
    EBLCIndexSubHeader header;
    uint32_t _numoffsets;
    DCL_ARRAY(uint32_t, offsetArray); /* offsetArray[glyphIndex] + imageDataOffset ==> start of bitmap data for glyph. */
                                    /* sizeOfArray = lastGlyph - firstGlyph + 1                                      */
} EBLCIndexSubtable_Format1;

#define EBLCFORMAT1_SIZE(first, last)            \
    (SIZEOF(EBLCIndexSubtable_Format1, header) + \
     sizeof(uint32_t) * ((last) - (first) + 1))

/* format 2 has fixed-length images of the same format for
   MONO-spaced glyphs */
typedef struct _EBLCIndexSubtable_Format2 {
    EBLCIndexSubHeader header;
    uint32_t imageSize;            /* images may be compressed, bit-aligned or byte-aligned */
    sbitBigGlyphMetrics Metrics; /* all glyphs share same metrics */
} EBLCIndexSubtable_Format2;

#define EBLCFORMAT2_SIZE (SIZEOF(EBLCIndexSubtable_Format2, header) +    \
                          SIZEOF(EBLCIndexSubtable_Format2, imageSize) + \
                          SBITBIGGLYPHMETRICS_SIZE)

/* format 3 is similar to format 1, but with 16-bit offsets, for
   compressed PROPORTIONALLY-spaced glyphs.
   Must be padded to a long-word boundary. */
typedef struct _EBLCIndexSubtable_Format3 {
    EBLCIndexSubHeader header;
    uint32_t _numoffsets;
    DCL_ARRAY(uint16_t, offsetArray);
} EBLCIndexSubtable_Format3;

#define EBLCFORMAT3_SIZE(first, last)            \
    (SIZEOF(EBLCIndexSubtable_Format1, header) + \
     sizeof(uint16_t) * ((last) - (first) + 1))

typedef struct _EBLCCodeOffsetPair {
    GlyphId glyphCode; /* code of glyph present */
    uint16_t offset;     /* location in "EBDT" */
} EBLCCodeOffsetPair;
#define EBLCCODEOFFSETPAIR_SIZE (SIZEOF(EBLCCodeOffsetPair, glyphCode) + \
                                 SIZEOF(EBLCCodeOffsetPair, offset))

/* format 4 is for sparsely-embedded glyph data for
   PROPORTIONAL metrics */
typedef struct _EBLCIndexSubtable_Format4 {
    EBLCIndexSubHeader header;
    uint32_t numGlyphs;
    DCL_ARRAY(EBLCCodeOffsetPair, glyphArray); /* one per glyph */
} EBLCIndexSubtable_Format4;

#define EBLCFORMAT4_SIZE(numglyphs)                 \
    (SIZEOF(EBLCIndexSubtable_Format4, header) +    \
     SIZEOF(EBLCIndexSubtable_Format4, numGlyphs) + \
     (EBLCCODEOFFSETPAIR_SIZE) * (numglyphs))

/* format 5 is for sparsely-embedded glyph data of
   fixed-sized, MONO-spaced metrics */
typedef struct _EBLCIndexSubtable_Format5 {
    EBLCIndexSubHeader header;
    uint32_t imageSize;
    sbitBigGlyphMetrics Metrics;
    uint32_t numGlyphs;
    DCL_ARRAY(EBLCCodeOffsetPair, glyphArray); /* one per glyph */
} EBLCIndexSubtable_Format5;

#define EBLCFORMAT5_SIZE(numglyphs)                 \
    (SIZEOF(EBLCIndexSubtable_Format5, header) +    \
     SIZEOF(EBLCIndexSubtable_Format5, imageSize) + \
     SBITBIGGLYPHMETRICS_SIZE +                     \
     SIZEOF(EBLCIndexSubtable_Format5, numGlyphs) + \
     (EBLCCODEOFFSETPAIR_SIZE) * (numglyphs))

/* ...................EBLC TABLE.................................... */

typedef union _EBLCFormat {
    EBLCIndexSubtable_Format1 *f1;
    EBLCIndexSubtable_Format2 *f2;
    EBLCIndexSubtable_Format3 *f3;
    EBLCIndexSubtable_Format4 *f4;
    EBLCIndexSubtable_Format5 *f5;
} EBLCFormat;

typedef struct _EBLCFormats /* internal: for construction only */
{
    uint32_t _bytelen;
    sbitBitmapIndexFormats _fmttype;
    uint16_t _index;
    EBLCFormat _fmt;
} EBLCFormats;

typedef struct _EBLCIndexSubTableArrayElt {
    GlyphId firstGlyphIndex;
    GlyphId lastGlyphIndex;
    uint32_t additionalOffsetToIndexSubtable; /* add to indexSubTableArrayOffset to get offset from beginning of "EBLC" table */
    EBLCFormats _subtable;
    uint16_t _index; /* index in subtable array */
} EBLCIndexSubTableArrayElt;

#define EBLCINDEXSUBTABLEARRAYELT_SIZE (SIZEOF(EBLCIndexSubTableArrayElt, firstGlyphIndex) + \
                                        SIZEOF(EBLCIndexSubTableArrayElt, lastGlyphIndex) +  \
                                        SIZEOF(EBLCIndexSubTableArrayElt, additionalOffsetToIndexSubtable))

typedef struct _EBLCIndexSubTableArray {
    uint16_t _numarrayelts;
    DCL_ARRAY(EBLCIndexSubTableArrayElt, _elts);
    uint16_t _index; /* index in Main EBLC SubTableArray */
} EBLCIndexSubTableArray;

typedef struct _EBLCTableHeader {
#define EBLC_HEADER_VERSION 0x00020000
    Fixed version;
    uint32_t numSizes;
    DCL_ARRAY(EBLCBitmapSizeTable, bitmapSizeTable); /* array [numSizes] */
    /* for construction: array[numSizes].
       one-to-one correspondence with bitmapSizeTable  */
    DCL_ARRAY(EBLCIndexSubTableArray, _indexSubTableArray);
} EBLCTableHeader;

#define EBLCTABLEHEADER_SIZE(numsizes)   \
    (SIZEOF(EBLCTableHeader, version) +  \
     SIZEOF(EBLCTableHeader, numSizes) + \
     EBLCBITMAPSIZETABLE_SIZE * (numsizes))

#endif /* FORMAT_EBLC_H */
