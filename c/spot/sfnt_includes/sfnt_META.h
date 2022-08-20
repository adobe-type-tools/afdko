/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * META table format definition.
 */

#ifndef FORMAT_META_H
#define FORMAT_META_H

typedef struct
{
    uint16_t labelID; /* Metadata label identifier */
    uint16_t stringLen;
    uint32_t stringOffset; /* offset in bytes from start of META table, */
                         /* either 16-bit or 32-bit offset            */
} METAString;

typedef struct
{
    GlyphId glyphID;
    uint16_t nMetaEntry; /* number of Metadata string entries for this glyph */
    uint32_t hdrOffset;  /* offset from start of META table to beginning of */
                       /* this glyph's array of nMetaEntry METAstring     */
                       /* entries, either 16-bit or 32-bit offset         */

    da_DCL(METAString, stringentry);
} METARecord;

typedef struct
{
    uint16_t tableVersionMajor;
    uint16_t tableVersionMinor;
    uint16_t metaEntriesVersionMajor;
    uint16_t metaEntriesVersionMinor;
    uint32_t unicodeVersion; /* major.minor.update label MMmmuu digits */
    uint16_t metaFlags;
#define META_FLAGS_2BYTEOFFSETS 0x0
#define META_FLAGS_4BYTEOFFSETS 0x1
    uint16_t nMetaRecs;           /* total number of Metadata records */
    da_DCL(METARecord, record); /* in ascending sorted order, by glyphID */
    /* arrays of METAStrings are written after the METARecords */
    da_DCL(uint8_t, pool); /* pool of UTF-8 strings */
} METATbl;

#endif /* FORMAT_META_H */
