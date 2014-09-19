/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)head.h	1.4
 * Changed:    7/1/98 10:53:28
 ***********************************************************************/

/*
 * Font header table format definition.
 */

#ifndef FORMAT_HEAD_H
#define FORMAT_HEAD_H

#define head_VERSION VERSION(1,0)

#define DATE_TIME_SIZE 8
typedef Card8 longDateTime[DATE_TIME_SIZE];

typedef struct
	{
	Fixed version;
	Fixed fontRevision;
	Card32 checkSumAdjustment;
	Card32 magicNumber;
#define head_MAGIC 0x5F0F3CF5
	Card16 flags;
#define head_SET_LSB	(1<<1)
	Card16 unitsPerEm;
	longDateTime created;
	longDateTime modified;
	FWord xMin;
	FWord yMin;
	FWord xMax;
	FWord yMax;
	Card16 macStyle;
	Card16 lowestRecPPEM;
	Int16 fontDirectionHint;
#define head_STRONGL2R 1
	Int16 indexToLocFormat;
#define head_LONGOFFSETSUSED 1
	Int16 glyphDataFormat;
	} headTbl;

#endif /* FORMAT_HEAD_H */
