/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)lcar.h	1.1
 * Changed:    4/13/95 10:15:25
 ***********************************************************************/

/*
 * Ligature caret table format definition.
 */

#ifndef FORMAT_LCAR_H
#define FORMAT_LCAR_H

#define lcar_VERSION VERSION(1,0)

typedef struct
	{
	GlyphId ligGlyph;	/* [Not in format] */
	Card16 cnt;
	Int16 *partial;
	} LigCaretEntry;

typedef struct
	{
	Fixed version;
	Card16 format;
	Lookup lookup;
	DCL_ARRAY(LigCaretEntry, entry);
	} lcarTbl;
#define TBL_HDR_SIZE (SIZEOF(lcarTbl, version) + \
					  SIZEOF(lcarTbl, format))

#endif /* FORMAT_LCAR_H */
