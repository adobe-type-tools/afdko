/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)trak.c	1.7
 * Changed:    5/19/99 18:59:20
 ***********************************************************************/

#include "trak.h"
#include "sfnt_trak.h"

static trakTbl *trak = NULL;
static IntX loaded = 0;

/* Read track data table */
static void readData(Data *data, LongN start, Card16 offset)
	{
	IntX i;
	IntX j;
	
	if (offset == 0)
		return;

	SEEK_ABS(start + offset);

	IN1(data->nTracks);
	IN1(data->nSizes);
	IN1(data->sizeOffset);

	/* Read track table */
	data->track = memNew(sizeof(Entry) * data->nTracks);
	for (i = 0; i < data->nTracks; i++)
		{
		Entry *entry = &data->track[i];

		IN1(entry->level);
		IN1(entry->nameId);
		IN1(entry->offset);
		}

	/* Read size table */
	data->size = memNew(sizeof(data->size[i]) * data->nSizes);
	for (i = 0; i < data->nSizes; i++)
		IN1(data->size[i]);

	/* Read tracking values */
	for (i = 0; i < data->nTracks; i++)
		{
		Entry *entry = &data->track[i];

		SEEK_ABS(start + entry->offset);

		entry->value = memNew(sizeof(entry->value[0]) * data->nSizes);
		for (j = 0; j < data->nSizes; j++)
			IN1(entry->value[j]);
		}
	}

void trakRead(LongN start, Card32 length)
	{
	if (loaded)
		return;

	trak = (trakTbl *)memNew(sizeof(trakTbl));

	SEEK_ABS(start);

	/* Read header */
	IN1(trak->version);
	IN1(trak->format);
	IN1(trak->horizOffset);
	IN1(trak->vertOffset);
	IN1(trak->reserved);

	readData(&trak->horiz, start, trak->horizOffset);
	readData(&trak->vert, start, trak->vertOffset);

	loaded = 1;
	}

/* Dump track data table */
static void dumpData(Data *data, Byte8 *name, IntX level)
	{
	IntX i;
	IntX j;

	DL(2, (OUTPUTBUFF, "%s", name));
	DLu(2, "nTracks   =", data->nTracks);
	DLu(2, "nSizes    =", data->nSizes);
	DLX(2, "sizeOffset=", data->sizeOffset);

	DL(2, (OUTPUTBUFF, "--- track[index]={level,nameId,offset}\n"));	
	for (i = 0; i < data->nTracks; i++)
		{
		Entry *entry = &data->track[i];

		DL(2, (OUTPUTBUFF, "[%d]={%1.3f (%08lx),%hu,%04hx} ", i, 
			   FIXED_ARG(entry->level), entry->nameId, entry->offset));
		}
	DL(2, (OUTPUTBUFF, "\n"));

	DL(2, (OUTPUTBUFF, "--- size[index]=value\n"));	
	for (i = 0; i < data->nSizes; i++)
		DL(2, (OUTPUTBUFF, "[%d]=%1.3f (%08lx) ", i, FIXED_ARG(data->size[i])));
	DL(2, (OUTPUTBUFF, "\n"));

	DL(2, (OUTPUTBUFF, "--- value[index]=value\n"));	
	j = 0;
	for (i = 0; i < data->nTracks; i++)
		{
		IntX k;
		Entry *entry = &data->track[i];
		
		for (k = 0; k < data->nSizes; k++)
			DL(2, (OUTPUTBUFF, "[%d]=%hd ", j++, entry->value[k]));
		}
	DL(2, (OUTPUTBUFF, "\n"));
	}

void trakDump(IntX level, LongN start)
	{
	DL(1, (OUTPUTBUFF, "### [trak] (%08lx)\n", start));

	DLV(2, "version    =", trak->version);
	DLu(2, "format     =", trak->format);
	DLx(2, "horizOffset=", trak->horizOffset);
	DLx(2, "vertOffset =", trak->vertOffset);
	DLu(2, "reserved   =", trak->reserved);

	if (trak->horizOffset != 0)
		dumpData(&trak->horiz, "--- horiz\n", level);
	if (trak->vertOffset != 0)
		dumpData(&trak->vert, "--- vert\n", level);
	}

static void freeData(Data *data)
	{
	IntX i;

	for (i = 0; i < data->nTracks; i++)
		memFree(data->track[i].value);
	memFree(data->track);
	memFree(data->size);
	}

void trakFree(void)
	{
    if (!loaded)
		return;

	if (trak->horizOffset != 0)
		freeData(&trak->horiz);
	if (trak->vertOffset != 0)
		freeData(&trak->vert);
	memFree(trak); trak = NULL;
	loaded = 0;
	}

