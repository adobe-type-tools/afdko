/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)WDTH.h	1.2
 * Changed:    12/4/95 10:01:35
 ***********************************************************************/

/*
 * Width table format definition.
 */

#ifndef FORMAT_WDTH_H
#define FORMAT_WDTH_H

#define WDTH_VERSION VERSION(1,0)

typedef struct
	{
	Fixed version;
	Card16 flags;
#define LONG_OFFSETS	(1<<0)
	Card16 nMasters;
	Card16 nRanges;
	GlyphId *firstGlyph;	/* [nRanges + 1] */
	void *offset;	/* [nRanges + 1] (16- or 32-bit entries) */
	uFWord *width;	/* [variable] (on the order of (1|nGlyphs) * nMasters) */
	} WDTHTbl;

#define WDTH_TBL_SIZE(nranges, offsetsize) \
	(SIZEOF(WDTHTbl, version) + \
	 SIZEOF(WDTHTbl, flags) + \
	 SIZEOF(WDTHTbl, nMasters) + \
	 SIZEOF(WDTHTbl, nRanges) + \
	 ((SIZEOF(WDTHTbl, firstGlyph[0])) * ((nranges) + 1)) + \
	 ((offsetsize) * ((nranges) + 1)))
					    
#endif /* FORMAT_WDTH_H */
