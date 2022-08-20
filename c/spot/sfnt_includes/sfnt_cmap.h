/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Character mapping table format definition.
 */

#ifndef FORMAT_CMAP_H
#define FORMAT_CMAP_H

/* Macintosh */
#define cmap_MAC 1 /* Platform: Macintosh */

/* Microsoft */
#define cmap_MS 3        /* Platform: Microsoft */
#define cmap_MS_SYMBOL 0 /* Script: Undefined index scheme */
#define cmap_MS_UGL 1    /* Script: UGL */

/* Custom */
#define cmap_CUSTOM 4         /* Platform: Custom */
#define cmap_CUSTOM_DEFAULT 0 /* Script: Default */

/* Common header for all formats */
typedef struct
{
    uint16_t format;
    uint16_t length;
    uint16_t languageId; /* Documented as revision */
} FormatHdr;

typedef struct
{
    uint16_t format;
    uint16_t length;
    uint16_t languageId; /* Documented as revision */
    uint8_t glyphId[256];
} Format0;
#define FORMAT0_SIZE (SIZEOF(Format0, format) +     \
                      SIZEOF(Format0, length) +     \
                      SIZEOF(Format0, languageId) + \
                      SIZEOF(Format0, glyphId[0]) * 256)

typedef struct
{
    uint16_t firstCode;
    uint16_t entryCount;
    int16_t idDelta;
    uint16_t idRangeOffset;
} Segment2;
#define SEGMENT2_SIZE (SIZEOF(Segment2, firstCode) +  \
                       SIZEOF(Segment2, entryCount) + \
                       SIZEOF(Segment2, idDelta) +    \
                       SIZEOF(Segment2, idRangeOffset))

typedef struct
{
    uint16_t format;
    uint16_t length;
    uint16_t languageId; /* Documented as revision */
    uint16_t segmentKeys[256];
    uint16_t _nSegments;
    DCL_ARRAY(Segment2, segment);
    uint16_t _nGlyphs;
    DCL_ARRAY(GlyphId, glyphId);
} Format2;
#define FORMAT2_SIZE(segs, glyphs) (SIZEOF(Format2, format) +               \
                                    SIZEOF(Format2, length) +               \
                                    SIZEOF(Format2, languageId) +           \
                                    SIZEOF(Format2, segmentKeys[0]) * 256 + \
                                    SEGMENT2_SIZE * (segs) +                \
                                    sizeof(GlyphId) * (glyphs))

typedef struct
{
    uint16_t format;
    uint16_t length;
    uint16_t languageId; /* Documented as revision */
    uint16_t segCountX2;
    uint16_t searchRange;
    uint16_t entrySelector;
    uint16_t rangeShift;
    uint16_t *endCode;
    uint16_t password;
    uint16_t *startCode;
    int16_t *idDelta;
    uint16_t *idRangeOffset;
    DCL_ARRAY(GlyphId, glyphId);
} Format4;
#define FORMAT4_SIZE(segs, glyphs)         \
    (SIZEOF(Format4, format) +             \
     SIZEOF(Format4, length) +             \
     SIZEOF(Format4, languageId) +         \
     SIZEOF(Format4, segCountX2) +         \
     SIZEOF(Format4, searchRange) +        \
     SIZEOF(Format4, entrySelector) +      \
     SIZEOF(Format4, rangeShift) +         \
     SIZEOF(Format4, password) +           \
     (SIZEOF(Format4, endCode[0]) +        \
      SIZEOF(Format4, startCode[0]) +      \
      SIZEOF(Format4, idDelta[0]) +        \
      SIZEOF(Format4, idRangeOffset[0])) * \
         (segs) +                          \
     sizeof(GlyphId) * (glyphs))

typedef struct
{
    uint16_t format;
    uint16_t length;
    uint16_t languageId;
    uint16_t firstCode;
    uint16_t entryCount;
    uint16_t *glyphId;
} Format6;
#define FORMAT6_SIZE(glyphs) (SIZEOF(Format6, format) +     \
                              SIZEOF(Format6, length) +     \
                              SIZEOF(Format6, languageId) + \
                              SIZEOF(Format6, firstCode) +  \
                              SIZEOF(Format6, entryCount) + \
                              SIZEOF(Format6, glyphId[0]) * (glyphs))

typedef struct
{
    uint32_t startCharCode;
    uint32_t endCharCode;
    uint32_t startGlyphID;
} Format12Group;

typedef struct
{
    uint16_t format;
    uint16_t reserved;
    uint32_t length;
    uint32_t languageId; /* Documented as revision */
    uint32_t nGroups;
    DCL_ARRAY(Format12Group, group);
} Format12;

typedef struct
{
    uint32_t uv;     /* first Unicode value in range of UV's for default UVS's */
    uint8_t addlCnt; /* Number of consecutive UV's after first */
} DefaultUVSRecord;

typedef struct
{
    uint32_t uv;      /*  Unicode value  */
    uint16_t glyphID; /*glyphID for UVS glyph */
} ExtendedUVSRecord;

typedef struct
{
    uint32_t uvs;
    uint32_t defaultUVSoffset;
    uint32_t extUVSOffset;
    uint32_t numDefEntries;
    uint32_t numExtEntries;
    DCL_ARRAY(DefaultUVSRecord, defUVSEntries);
    DCL_ARRAY(ExtendedUVSRecord, extUVSEntries);
} UVSRecord;

typedef struct
{
    uint16_t format;
    uint32_t length;
    uint32_t numUVSRecords; /* Documented as revision */
    DCL_ARRAY(UVSRecord, uvsRecs);
} Format14;

#if 0
#define FORMAT12_SIZE(segs, glyphs) \
    (SIZEOF(Format12, format) +     \
     SIZEOF(Format12, length) +     \
     SIZEOF(Format12, languageId) + \
     SIZEOF(Format12, nGroups) +    \
     sizeof(Format12Group) * (glyphs))
#endif

typedef struct
{
    uint16_t platformId;
    uint16_t scriptId;
    uint32_t offset;
    void *format;
} Encoding;
#define ENCODING_SIZE (SIZEOF(Encoding, platformId) + \
                       SIZEOF(Encoding, scriptId) +   \
                       SIZEOF(Encoding, offset))

typedef struct
{
    uint16_t version;
    uint16_t nEncodings;
    DCL_ARRAY(Encoding, encoding);
} cmapTbl;
#define TBL_HDR_SIZE (SIZEOF(cmapTbl, version) + \
                      SIZEOF(cmapTbl, nEncodings))

#endif /* FORMAT_CMAP_H */
