/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)vhea.c	1.7
 * Changed:    5/19/99 17:51:54
 ***********************************************************************/

#include "vhea.h"
#include "sfnt_vhea.h"
#include "sfnt.h"

static vheaTbl *vhea = NULL;
static IntX loaded = 0;

void vheaRead(LongN start, Card32 length)
	{
	if (loaded)
		return;

	vhea = (vheaTbl *)memNew(sizeof(vheaTbl));

	SEEK_ABS(start);

	IN1(vhea->version);
	IN1(vhea->vertTypoAscender);
	IN1(vhea->vertTypoDescender);
	IN1(vhea->vertTypoLineGap);
	IN1(vhea->advanceHeightMax);
	IN1(vhea->minTopSideBearing);
	IN1(vhea->minBottomSideBearing);
	IN1(vhea->yMaxExtent);
	IN1(vhea->caretSlopeRise);
	IN1(vhea->caretSlopeRun);
	IN1(vhea->caretOffset);
	IN1(vhea->reserved[0]);
	IN1(vhea->reserved[1]);
	IN1(vhea->reserved[2]);
	IN1(vhea->reserved[3]);
	IN1(vhea->metricDataFormat);
	IN1(vhea->numberOfLongVerMetrics);

	loaded = 1;
	}

void vheaDump(IntX level, LongN start)
	{
	DL(1, (OUTPUTBUFF, "### [vhea] (%08lx)\n", start));

	DLV(2, "version               =", vhea->version);
	DLs(2, "vertTypoAscender      =", vhea->vertTypoAscender);
	DLs(2, "vertTypoDescender     =", vhea->vertTypoDescender);
	DLs(2, "vertTypoLineGap       =", vhea->vertTypoLineGap);
	DLu(2, "advanceHeightMax      =", vhea->advanceHeightMax);
	DLs(2, "minTopSideBearing     =", vhea->minTopSideBearing);
	DLs(2, "minBottomSideBearing  =", vhea->minBottomSideBearing);
	DLs(2, "yMaxExtent            =", vhea->yMaxExtent);
	DLs(2, "caretSlopeRise        =", vhea->caretSlopeRise);
	DLs(2, "caretSlopeRun         =", vhea->caretSlopeRun);
	DLs(2, "caretOffset           =", vhea->caretOffset);
	DLs(2, "reserved[0]           =", vhea->reserved[0]);
	DLs(2, "reserved[1]           =", vhea->reserved[1]);
	DLs(2, "reserved[2]           =", vhea->reserved[2]);
	DLs(2, "reserved[3]           =", vhea->reserved[3]);
	DLs(2, "metricDataFormat      =", vhea->metricDataFormat);
	DLu(2, "numberOfLongVerMetrics=", vhea->numberOfLongVerMetrics);
	}

void vheaFree(void)
	{
	  if (!loaded) return;
	  memFree(vhea); vhea = NULL;
	loaded = 0;
	}

/* Return vhea->numberOfLongVerMetrics */
IntX vheaGetNLongVerMetrics(Card16 *nLongVerMetrics, Card32 client)
	{
	if (!loaded)
		{
		if (sfntReadTable(vhea_))
			return tableMissing(vhea_, client);
		}
	*nLongVerMetrics = vhea->numberOfLongVerMetrics;
	return 0;
	}
