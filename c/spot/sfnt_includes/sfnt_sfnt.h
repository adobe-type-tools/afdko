/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * sfnt table format definition.
 */

#ifndef FORMAT_SFNT_H
#define FORMAT_SFNT_H

/* TrueType Collection Header */
typedef struct
{
    uint32_t TTCTag;
    Fixed Version;
    uint32_t DirectoryCount;
    uint32_t *TableDirectory; /* [DirectoryCount] */
    uint32_t DSIGTag;
    uint32_t DSIGLength;
    uint32_t DSIGOffset;
} ttcfTbl;

#define TTC_HDR_SIZEV1 (SIZEOF(ttcfTbl, TTCTag) +  \
                        SIZEOF(ttcfTbl, Version) + \
                        SIZEOF(ttcfTbl, DirectoryCount))
/* SIZEOF(ttcfTbl, TableDirectory) + \ The  directory table size is added in later */

#define TTC_HDR_SIZEV2 (SIZEOF(ttcfTbl, TTCTag) +         \
                        SIZEOF(ttcfTbl, Version) +        \
                        SIZEOF(ttcfTbl, DirectoryCount) + \
                        SIZEOF(ttcfTbl, DSIGTag) +        \
                        SIZEOF(ttcfTbl, DSIGLength) +     \
                        SIZEOF(ttcfTbl, DSIGOffset))
                    /* SIZEOF(ttcfTbl, TableDirectory) + \ The  directory table size is added in later */

typedef struct
{
    uint32_t tag;
    uint32_t checksum;
    uint32_t offset;
    uint32_t length;
} Entry;
#define ENTRY_SIZE (SIZEOF(Entry, tag) +      \
                    SIZEOF(Entry, checksum) + \
                    SIZEOF(Entry, offset) +   \
                    SIZEOF(Entry, length))

typedef struct
{
    Fixed version;
    uint16_t numTables;
    uint16_t searchRange;
    uint16_t entrySelector;
    uint16_t rangeShift;
    Entry *directory; /* [numTables] */
} sfntTbl;
#define DIR_HDR_SIZE (SIZEOF(sfntTbl, version) +       \
                      SIZEOF(sfntTbl, numTables) +     \
                      SIZEOF(sfntTbl, searchRange) +   \
                      SIZEOF(sfntTbl, entrySelector) + \
                      SIZEOF(sfntTbl, rangeShift))

#endif /* FORMAT_SFNT_H */
