/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)MMVR.c	1.6
 * Changed:    5/19/99 17:21:00
 ***********************************************************************/

#include "MMVR.h"
#include "sfnt_MMVR.h"

static MMVRTbl *MMVR = NULL;
static IntX loaded = 0;

void MMVRRead(LongN start, Card32 length)
	{
	IntX i;
	
	if (loaded)
		return;

	MMVR = (MMVRTbl *)memNew(sizeof(MMVRTbl));
	SEEK_ABS(start);

	IN1(MMVR->Version);
	IN1(MMVR->Flags);
	IN1(MMVR->AxisCount);

	MMVR->axis = memNew(sizeof(Axis) * MMVR->AxisCount);
	for (i = 0; i < MMVR->AxisCount; i++)
		{
		Axis *axis = &MMVR->axis[i];
		
		IN1(axis->Tag);
		IN1(axis->Default);
		IN1(axis->Scale);
		}

	loaded = 1;
	}

void MMVRDump(IntX level, LongN start)
	{
	IntX i;

	DL(1, (OUTPUTBUFF, "### [MMVR] (%08lx)\n", start));

	DLV(2, "Version  =", MMVR->Version);
	DLx(2, "Flags    =", MMVR->Flags);
	DLu(2, "AxisCount=", MMVR->AxisCount);

	for (i = 0; i < MMVR->AxisCount; i++)
		{
		Axis *axis = &MMVR->axis[i];
		
		DL(2, (OUTPUTBUFF, "--- axis[%d]\n", i));
		DLT(2, "Tag    =", axis->Tag);
		DLu(2, "Default=", axis->Default);
		DLu(2, "Scale  =", axis->Scale);
		}
	}

void MMVRFree(void)
	{
	if (!loaded)
		return;

	memFree(MMVR->axis);
	memFree(MMVR); MMVR = NULL;
	loaded = 0;
	}
