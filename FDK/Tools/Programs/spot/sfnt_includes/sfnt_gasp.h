/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)gasp.h	1.1
 * Changed:    10/12/95 14:59:09
 ***********************************************************************/

/*
 * Grid-fitting And Scan-conversion Procedure format definition.
 */

#ifndef FORMAT_GASP_H
#define FORMAT_GASP_H

typedef struct
	{
	Card16 rangeMaxPPEM;
	Card16 rangeGaspBehavior;
#define GASP_GRIDFIT	(1<<0)
#define GASP_DOGRAY		(1<<1)
	} GaspRange;

typedef struct
	{
	Card16 version;
	Card16 numRanges;
	GaspRange *gaspRange;	/* [numRanges] */
	} gaspTbl;

#endif /* FORMAT_GASP_H */
