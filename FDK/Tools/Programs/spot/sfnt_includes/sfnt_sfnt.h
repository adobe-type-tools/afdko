/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************
 * SCCS Id:    @(#)sfnt.h	1.3
 * Changed:    10/12/95 15:01:07
 ***********************************************************************/

/*
 * sfnt table format definition.
 */

#ifndef FORMAT_SFNT_H
#define FORMAT_SFNT_H

/* TrueType Collection Header */
typedef struct
	{
	Card32 TTCTag;
	Fixed Version;
	Card32 DirectoryCount;
	Card32 *TableDirectory;					/* [DirectoryCount] */
	Card32 DSIGTag;
	Card32 DSIGLength;
	Card32 DSIGOffset;
	} ttcfTbl;
	
#define TTC_HDR_SIZEV1 (SIZEOF(ttcfTbl, TTCTag) + \
					  SIZEOF(ttcfTbl, Version) + \
					  SIZEOF(ttcfTbl, DirectoryCount))
					 /* SIZEOF(ttcfTbl, TableDirectory) + \ The  dir table size is added in later */
					  
#define TTC_HDR_SIZEV2  (SIZEOF(ttcfTbl, TTCTag) + \
					  SIZEOF(ttcfTbl, Version) + \
					  SIZEOF(ttcfTbl, DirectoryCount) + \
					  SIZEOF(ttcfTbl, DSIGTag) + \
					  SIZEOF(ttcfTbl, DSIGLength) + \
					  SIZEOF(ttcfTbl, DSIGOffset))
					 /* SIZEOF(ttcfTbl, TableDirectory) + \ The  dir table size is added in later */ \

typedef struct
	{
	Card32 tag;
	Card32 checksum;
	Card32 offset;
	Card32 length;
	} Entry;
#define ENTRY_SIZE (SIZEOF(Entry, tag) + \
					SIZEOF(Entry, checksum) + \
					SIZEOF(Entry, offset) + \
					SIZEOF(Entry, length))

typedef struct
	{
	Fixed version;
	Card16 numTables;
	Card16 searchRange;
	Card16 entrySelector;
	Card16 rangeShift;
	Entry *directory;						/* [numTables] */
	} sfntTbl;
#define DIR_HDR_SIZE (SIZEOF(sfntTbl, version) + \
					  SIZEOF(sfntTbl, numTables) + \
					  SIZEOF(sfntTbl, searchRange) + \
					  SIZEOF(sfntTbl, entrySelector) + \
					  SIZEOF(sfntTbl, rangeShift))

#endif /* FORMAT_SFNT_H */
