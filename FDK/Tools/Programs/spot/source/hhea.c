/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)hhea.c	1.8
 * Changed:    5/19/99 17:51:53
 ***********************************************************************/

#include "hhea.h"
#include "sfnt_hhea.h"
#include "sfnt.h"

static hheaTbl *hhea = NULL;
static IntX loaded = 0;

void hheaRead(LongN start, Card32 length)
	{
	if (loaded)
		return;

	hhea = (hheaTbl *)memNew(sizeof(hheaTbl));

	SEEK_ABS(start);

	IN1(hhea->version);
	IN1(hhea->ascender);
	IN1(hhea->descender);
	IN1(hhea->lineGap);
	IN1(hhea->advanceWidthMax);
	IN1(hhea->minLeftSideBearing);
	IN1(hhea->minRightSideBearing);
	IN1(hhea->xMaxExtent);
	IN1(hhea->caretSlopeRise);
	IN1(hhea->caretSlopeRun);
	IN1(hhea->caretOffset);
	IN1(hhea->reserved[0]);
	IN1(hhea->reserved[1]);
	IN1(hhea->reserved[2]);
	IN1(hhea->reserved[3]);
	IN1(hhea->metricDataFormat);
	IN1(hhea->numberOfLongHorMetrics);

	loaded = 1;
	}

void hheaDump(IntX level, LongN start)
	{
	DL(1, (OUTPUTBUFF, "### [hhea] (%08lx)\n", start));

	DLV(2, "version               =", hhea->version);
	DLs(2, "ascender              =", hhea->ascender);
	DLs(2, "descender             =", hhea->descender);
	DLs(2, "lineGap               =", hhea->lineGap);
	DLu(2, "advanceWidthMax       =", hhea->advanceWidthMax);
	DLs(2, "minLeftSideBearing    =", hhea->minLeftSideBearing);
	DLs(2, "minRightSideBearing   =", hhea->minRightSideBearing);
	DLs(2, "xMaxExtent            =", hhea->xMaxExtent);
	DLs(2, "caretSlopeRise        =", hhea->caretSlopeRise);
	DLs(2, "caretSlopeRun         =", hhea->caretSlopeRun);
	DLs(2, "caretOffset           =", hhea->caretOffset);
	DLs(2, "reserved[0]           =", hhea->reserved[0]);
	DLs(2, "reserved[1]           =", hhea->reserved[1]);
	DLs(2, "reserved[2]           =", hhea->reserved[2]);
	DLs(2, "reserved[3]           =", hhea->reserved[3]);
	DLs(2, "metricDataFormat      =", hhea->metricDataFormat);
	DLu(2, "numberOfLongHorMetrics=", hhea->numberOfLongHorMetrics);
	}

void hheaFree(void)
	{
	  if (!loaded) return;
	  memFree(hhea); hhea = NULL;
	loaded = 0;
	}

/* Return hhea->numberOfLongHorMetrics */
IntX hheaGetNLongHorMetrics(Card16 *nLongHorMetrics, Card32 client)
	{
	if (!loaded)
		{
		if (sfntReadTable(hhea_))
			return tableMissing(hhea_, client);
		}
	*nLongHorMetrics = hhea->numberOfLongHorMetrics;
	return 0;
	}

void hheaGetTypocenders(Int32 *ascender, Int32*descender)
{
	if (!loaded)
		{
		  if (sfntReadTable(hhea_))
			{	
				*ascender=0;
				*descender=0;
			}
		}
	*ascender=hhea->ascender;
	*descender=hhea->descender;
}
