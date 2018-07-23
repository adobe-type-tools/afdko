/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)TYP1.h	1.2
 * Changed:    7/14/95 18:28:18
 ***********************************************************************/

/*
 * Type 1 data table format definition.
 */

#ifndef FORMAT_TYP1_H
#define FORMAT_TYP1_H

#define TYP1_VERSION VERSION(1,0)

typedef struct
	{
	Fixed Version;
	Card16 Flags;
#define TYP1_NO_SEAC (1<<0)
	Card16 GlyphCount;
	Card32 TotalLength;
	Card32 AsciiLength;
	Card32 BinaryLength;
	Int32 SubrMaxLength;
	} TYP1Tbl;
#define TYP1_HDR_SIZE (SIZEOF(TYP1Tbl, Version) + \
					   SIZEOF(TYP1Tbl, Flags) + \
					   SIZEOF(TYP1Tbl, GlyphCount) + \
					   SIZEOF(TYP1Tbl, TotalLength) + \
					   SIZEOF(TYP1Tbl, AsciiLength) + \
					   SIZEOF(TYP1Tbl, BinaryLength) + \
					   SIZEOF(TYP1Tbl, SubrMaxLength))

#endif /* FORMAT_TYP1_H */
