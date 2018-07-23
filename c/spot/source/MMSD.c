/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    %W%
 * Changed:    %G% %U%
 ***********************************************************************/

#include "MMSD.h"
#include "sfnt_MMSD.h"

static MMSDTbl *(MMSD) = NULL;
static IntX loaded = 0;

void MMSDRead(LongN start, Card32 length)
	{
	if (loaded)
		return;

	MMSD = (MMSDTbl *)memNew(sizeof(MMSDTbl));
	SEEK_ABS(start);
	
	loaded = 1;
	}

void MMSDDump(IntX level, LongN start)
	{
	DL(1, (stderr, "### [MMSD] (%08lx)\n", start));
	}

void MMSDFree(void)
	{
    if (!loaded)
		return;
	memFree(MMSD); MMSD = NULL;
	loaded = 0;
	}

