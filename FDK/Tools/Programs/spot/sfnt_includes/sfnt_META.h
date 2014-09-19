/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */
/***********************************************************************
 * SCCS Id:    @(#)META.h	1.2
 * Changed:    8/10/95 15:35:26
 ***********************************************************************/

/*
 * META table format definition.
 */

#ifndef FORMAT_META_H
#define FORMAT_META_H


typedef struct 
	{
	Card16 labelID; /* Metadata label identifier */
	Card16 stringLen;
	Card32 stringOffset; /* offset in bytes from start of META table,
						either 16-bit or 32-bit offset */
	} METAString;

typedef struct
	{
	GlyphId glyphID;
	Card16 nMetaEntry; /* number of Metadata string entries for this glyph */
	Card32 hdrOffset; /* offset from start of META table to
					beginning of this glyph's array of 
					nMetaEntry METAstring entries,
					either 16-bit or 32-bit offset */
	da_DCL(METAString, stringentry); 
	} METARecord;


typedef struct
	{
	Card16 tableVersionMajor;  
	Card16 tableVersionMinor;  
	Card16 metaEntriesVersionMajor; 
	Card16 metaEntriesVersionMinor; 
	Card32 unicodeVersion; /* major.minor.update label MMmmuu digits */
	Card16 metaFlags;
#define META_FLAGS_2BYTEOFFSETS 0x0
#define META_FLAGS_4BYTEOFFSETS 0x1
	Card16 nMetaRecs; /* total number of Metadata records */
	da_DCL(METARecord, record); /* in ascending sorted order, by glyphID */
	/* arrays of METAStrings are written after the METARecords */
	da_DCL(Card8, pool); /* pool of UTF-8 strings */
	} METATbl;

#endif /* FORMAT_META_H */
