/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)MMFX.h	1.1
 * Changed:    7/7/98 16:07:23
 ***********************************************************************/

/*
 * Multiple master  table format definition.
 */

#ifndef FORMAT_MMFX_H
#define FORMAT_MMFX_H

#define MMFX_VERSION VERSION(1,0)

typedef struct _MMFXMetric
	{
	Card16 id;		/* Metric id */
	Int16 length;			/* Charstring length */
	Int32 index;				/* Charstring index */
	} MMFXMetric;

typedef struct _MMFXTbl
	{
	Fixed version;
	Card16 nMetrics;
	Card16 offSize;  /* 2 or 4 byte length indicator */
	Int32 *offset;			/* [nMetrics] */
	Card8 *cstrs;	/* Charstrings */
	} MMFXTbl;

#endif /* FORMAT_MMFX_H */
