/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)post.h	1.5
 * Changed:    8/10/95 15:35:27
 ***********************************************************************/

/*
 * PostScript printer table format definition.
 */

#ifndef FORMAT_POST_H
#define FORMAT_POST_H

typedef struct
	{
	Card16 numberGlyphs;
	Card16 *glyphNameIndex;
	Card8 *names;
	} Format2_0;

typedef struct
	{
	Card16 numberGlyphs;
	Int8 *offset;
	} Format2_5;

typedef struct
	{
	Card16 *code;
	} Format4_0;

typedef struct
	{
	Fixed version;
	Fixed italicAngle;
	FWord underlinePosition;
	FWord underlineThickness;
	Card32 isFixedPitch;
	Card32 minMemType42;
	Card32 maxMemType42;
	Card32 minMemType1;
	Card32 maxMemType1;
	void *format;
	} postTbl;
#define TBL_HDR_SIZE (SIZEOF(postTbl, version) + \
					  SIZEOF(postTbl, italicAngle) + \
					  SIZEOF(postTbl, underlinePosition) + \
					  SIZEOF(postTbl, underlineThickness) + \
					  SIZEOF(postTbl, isFixedPitch) + \
					  SIZEOF(postTbl, minMemType42) + \
					  SIZEOF(postTbl, maxMemType42) + \
					  SIZEOF(postTbl, minMemType1) + \
					  SIZEOF(postTbl, maxMemType1))

#endif /* FORMAT_POST_H */
