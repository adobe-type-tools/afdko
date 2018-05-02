/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)hdmx.c	1.7
 * Changed:    5/19/99 18:59:19
 ***********************************************************************/

#include "hdmx.h"
#include "sfnt_hdmx.h"
#include "maxp.h"

static hdmxTbl *hdmx = NULL;
static IntX loaded = 0;
static Card16 nGlyphs;

void hdmxRead(LongN start, Card32 length)
	{
	IntX i;
	LongN recordOffset = start + TBL_HDR_SIZE;

	if (loaded)
		return;

	hdmx = (hdmxTbl *)memNew(sizeof(hdmxTbl));

	if (maxpGetNGlyphs(&nGlyphs, hdmx_))
		return;

	SEEK_ABS(start);

	IN1(hdmx->version);
	IN1(hdmx->nRecords);
	IN1(hdmx->recordSize);

	hdmx->record = memNew(sizeof(DeviceRecord) * hdmx->nRecords);
	for (i = 0; i < hdmx->nRecords; i++)
		{
		IntX j;
		DeviceRecord *rec = &hdmx->record[i];

		IN1(rec->pixelsPerEm);
		IN1(rec->maxWidth);
		
		rec->widths = memNew(sizeof(rec->widths[0]) * nGlyphs);
		for (j = 0; j < nGlyphs; j++)
			IN1(rec->widths[j]);

		recordOffset += hdmx->recordSize;
		SEEK_ABS(recordOffset);
		}
	
	loaded = 1;
	}

void hdmxDump(IntX level, LongN start)
	{
	IntX i;

	DL(1, (OUTPUTBUFF, "### [hdmx->] (%08lx)\n", start));
	
	DLu(2, "version   =", hdmx->version);
	DLu(2, "nRecords  =", hdmx->nRecords);
	DLU(2, "recordSize=", hdmx->recordSize);

	for (i = 0; i < hdmx->nRecords; i++)
		{
		IntX j;
		DeviceRecord *rec = &hdmx->record[i];

		DL(2, (OUTPUTBUFF, "--- device record[%d]\n", i));
		DLu(2, "pixelsPerEm=", (Card16)rec->pixelsPerEm);
		DLu(2, "maxWidth   =", (Card16)rec->maxWidth);
		
		DL(3, (OUTPUTBUFF, "--- widths[index]=value\n"));
		for (j = 0; j < nGlyphs; j++)
			DL(3, (OUTPUTBUFF, "[%d]=%u ", j, rec->widths[j]));
		DL(3, (OUTPUTBUFF, "\n"));
		}
	}

void hdmxFree(void)
	{
	IntX i;

    if (!loaded)
		return;

	for (i = 0; i < hdmx->nRecords; i++)
		memFree(hdmx->record[i].widths);
	memFree(hdmx->record);
	memFree(hdmx); hdmx = NULL;
	loaded = 0;
	}

