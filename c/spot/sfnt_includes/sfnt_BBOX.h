/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)BBOX.h	1.2
 * Changed:    11/30/95 16:23:03
 ***********************************************************************/

/*
 * Bounding box table format definition.
 */

#ifndef FORMAT_BBOX_H
#define FORMAT_BBOX_H

#define BBOX_VERSION VERSION(1,0)

typedef struct
	{
	FWord *left;	/* [nMasters] */
	FWord *bottom;	/* [nMasters] */
	FWord *right;	/* [nMasters] */
	FWord *top;		/* [nMasters] */
	} BBox;

typedef struct
	{
	Fixed version;
	Card16 flags;
	Card16 nGlyphs;
	Card16 nMasters;
	BBox *bbox;		/* [nGlyphs] */
	} BBOXTbl;

#endif /* FORMAT_BBOX_H */
