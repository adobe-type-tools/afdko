/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)bsln.h	1.1
 * Changed:    4/13/95 10:15:20
 ***********************************************************************/

/*
 * bsln table format definition.
 */

#ifndef FORMAT_BSLN_H
#define FORMAT_BSLN_H

#define bsln_VERSION VERSION(1,0)

typedef struct
	{
	Card16 deltas[32];
	} Format0;

typedef struct
	{
	Card16 deltas[32];
	Lookup *mapping;
	} Format1;

typedef struct
	{
	GlyphId stdGlyph;
	Card16 ctlPoints[32];
	} Format2;

typedef struct
	{
	GlyphId stdGlyph;
	Card16 ctlPoints[32];
	Lookup *mapping;
	} Format3;

typedef struct
	{
	Fixed version;
	Card16 format;
	Card16 defaultBaseline;
	void *part;
	} bslnTbl;

#endif /* FORMAT_BSLN_H */
