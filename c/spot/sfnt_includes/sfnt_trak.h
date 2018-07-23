/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)trak.h	1.1
 * Changed:    4/13/95 10:15:28
 ***********************************************************************/

/*
 * Track kerning table format definition.
 */

#ifndef FORMAT_TRAK_H
#define FORMAT_TRAK_H

#define trak_VERSION VERSION(1,0)

typedef struct
	{
	Fixed level;
	Card16 nameId;
	Card16 offset;
	Int16 *value;	/* Actually part of track data structure */
	} Entry;
#define ENTRY_SIZE (SIZEOF(Entry, level) + \
					SIZEOF(Entry, nameId) + \
					SIZEOF(Entry, offset))

typedef struct
	{
	Card16 nTracks;
	Card16 nSizes;
	Card32 sizeOffset;
	DCL_ARRAY(Entry, track);
	Fixed *size;
	/* Track values here */
	} Data;
#define DATA_HDR_SIZE (SIZEOF(Data, nTracks) + \
					   SIZEOF(Data, nSizes) + \
					   SIZEOF(Data, sizeOffset))

typedef struct
	{
	Fixed version;
	Card16 format;
	Card16 horizOffset;
	Card16 vertOffset;
	Card16 reserved;
	Data horiz;
	Data vert;
	} trakTbl;
#define TBL_HDR_SIZE (SIZEOF(trakTbl, version) + \
					  SIZEOF(trakTbl, format) + \
					  SIZEOF(trakTbl, horizOffset) + \
					  SIZEOF(trakTbl, vertOffset) + \
					  SIZEOF(trakTbl, reserved))

#endif /* FORMAT_TRAK_H */
