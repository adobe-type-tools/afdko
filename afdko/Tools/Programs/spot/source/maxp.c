/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)maxp.c	1.9
 * Changed:    5/19/99 17:51:54
 ***********************************************************************/

#include "maxp.h"
#include "sfnt_maxp.h"
#include "sfnt.h"

static maxpTbl *maxp = NULL;
static IntX loaded = 0;

void maxpRead(LongN start, Card32 length)
	{
	if (loaded)
		return;

	maxp = (maxpTbl *)memNew(sizeof(maxpTbl));

	SEEK_ABS(start);

	IN1(maxp->version);
	IN1(maxp->numGlyphs);
	if (maxp->version > 0x00005000)
		{
		IN1(maxp->maxPoints);
		IN1(maxp->maxContours);
		IN1(maxp->maxCompositePoints);
		IN1(maxp->maxCompositeContours);
		IN1(maxp->maxZones);
		IN1(maxp->maxTwilightPoints);
		IN1(maxp->maxStorage);
		IN1(maxp->maxFunctionDefs);
		IN1(maxp->maxInstructionDefs);
		IN1(maxp->maxStackElements);
		IN1(maxp->maxSizeOfInstructions);
		IN1(maxp->maxComponentElements);
		IN1(maxp->maxComponentDepth);
		}

	loaded = 1;
	}

void maxpDump(IntX level, LongN start)
	{
	DL(1, (OUTPUTBUFF, "### [maxp] (%08lx)\n", start));

	DLV(2, "version              =", maxp->version);
	DLu(2, "numGlyphs            =", maxp->numGlyphs);
	if (maxp->version > 0x00005000)
		{
		DLu(2, "maxPoints            =", maxp->maxPoints);
		DLu(2, "maxContours          =", maxp->maxContours);
		DLu(2, "maxCompositePoints   =", maxp->maxCompositePoints);
		DLu(2, "maxCompositeContours =", maxp->maxCompositeContours);
		DLu(2, "maxZones             =", maxp->maxZones);
		DLu(2, "maxTwilightPoints    =", maxp->maxTwilightPoints);
		DLu(2, "maxStorage           =", maxp->maxStorage);
		DLu(2, "maxFunctionDefs      =", maxp->maxFunctionDefs);
		DLu(2, "maxInstructionDefs   =", maxp->maxInstructionDefs);
		DLu(2, "maxStackElements     =", maxp->maxStackElements);
		DLu(2, "maxSizeOfInstructions=", maxp->maxSizeOfInstructions);
		DLu(2, "maxComponentElements =", maxp->maxComponentElements);
		DLu(2, "maxComponentDepth    =", maxp->maxComponentDepth);
		}
	}

void maxpFree(void)
	{
	  if (!loaded) return;
	  memFree(maxp); maxp = NULL;
	loaded = 0;
	}

/* Return maxp->numGlyphs */
IntX maxpGetNGlyphs(Card16 *nGlyphs, Card32 client)
	{
	if (!loaded)
		{
		if (sfntReadTable(maxp_))
			return tableMissing(maxp_, client);
		}
	*nGlyphs = maxp->numGlyphs;
	return 0;
	}

/* Return maxp->maxComponentElements */
IntX maxpGetMaxComponents(Card16 *nComponents, Card32 client)
	{
	if (!loaded)
		{
		if (sfntReadTable(maxp_))
			return tableMissing(maxp_, client);
		}
	*nComponents = maxp->maxComponentElements;
	return 0;
	}
