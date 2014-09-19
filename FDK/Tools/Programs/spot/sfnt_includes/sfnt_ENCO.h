/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)ENCO.h	1.1
 * Changed:    4/13/95 10:15:16
 ***********************************************************************/

/*
 * Encoding table format definition.
 */

#ifndef FORMAT_ENCO_H
#define FORMAT_ENCO_H

#define ENCO_VERSION VERSION(1,0)

typedef struct
	{
	Card16 format;
	} Format0;
#define FORMAT0_SIZE SIZEOF(Format0, format)

typedef struct
	{
	Card16 format;
	Card16 count;
	GlyphId *glyphId;
	Card8 *code;
	} Format1;
#define FORMAT1_SIZE(n) (SIZEOF(Format1, format) + \
						 SIZEOF(Format1, count) + \
						 (SIZEOF(Format1, glyphId[0]) + \
						  SIZEOF(Format1, code[0])) * (n))

typedef struct
	{
	Card16 format;
	GlyphId glyphId[256];
	} Format2;
#define FORMAT2_SIZE (SIZEOF(Format2, format) + \
					  SIZEOF(Format2, glyphId[0]) * 256)

typedef struct
	{
	Fixed version;
	Card32 *offset;
	DCL_ARRAY(void *, encoding);
	} ENCOTbl;

enum
	{
	ENCO_STANDARD,
	ENCO_SPARSE,
	ENCO_DENSE
	};

#endif /* FORMAT_ENCO_H */
