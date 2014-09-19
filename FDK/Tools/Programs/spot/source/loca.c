/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)loca.c	1.10
 * Changed:    7/14/99 09:24:29
 ***********************************************************************/

#include "loca.h"
#include "sfnt_loca.h"
#include "maxp.h"
#include "head.h"
#include "sfnt.h"

static locaTbl *loca = NULL;
static IntX loaded = 0;
static Card16 nGlyphs;
static Card16 locFormat;

static Format0 *readFormat0(void)
	{
	IntX i;
	Format0 *format = memNew(sizeof(Format0));

	format->offsets = memNew(sizeof(format->offsets[0]) * (nGlyphs + 1));
	for (i = 0; i < nGlyphs + 1; i++)
		IN1(format->offsets[i]);

	return format;
	}

static Format1 *readFormat1(void)
	{
	IntX i;
	Format1 *format = memNew(sizeof(Format1));

	format->offsets = memNew(sizeof(format->offsets[0]) * (nGlyphs + 1));
	for (i = 0; i < nGlyphs + 1; i++)
		IN1(format->offsets[i]);

	return format;
	}

void locaRead(LongN start, Card32 length)
	{
	if (loaded)
		return;

	loca = (locaTbl *)memNew(sizeof(locaTbl));

    if (maxpGetNGlyphs(&nGlyphs, loca_) ||
		headGetLocFormat(&locFormat, loca_))
		return;

	SEEK_ABS(start);

	switch (locFormat)
		{
	case 0:
		loca->format = readFormat0();
		break;
	case 1:
		loca->format = readFormat1();
		break;
	default:
		warning(SPOT_MSG_locaBADFMT, locFormat);
		return;
		}
	
	loaded = 1;
	}

static void dumpFormat0(Format0 *format, IntX level)
	{
	IntX i;

	DL(2, (OUTPUTBUFF, "--- offsets[index]=short (byte offset)\n"));
	for (i = 0; i < nGlyphs + 1; i++)
		DL(2, (OUTPUTBUFF, "[%d]=%04hx (%08x) ", i,
			   format->offsets[i], format->offsets[i] * 2));
	DL(2, (OUTPUTBUFF, "\n"));
	}

static void dumpFormat1(Format1 *format, IntX level)
	{
	IntX i;

	DL(2, (OUTPUTBUFF, "--- offsets[index]=long\n"));
	for (i = 0; i < nGlyphs + 1; i++)
		DL(2, (OUTPUTBUFF, "[%d]=%08lx ", i, format->offsets[i]));
	DL(2, (OUTPUTBUFF, "\n"));
	}

void locaDump(IntX level, LongN start)
	{
	DL(1, (OUTPUTBUFF, "### [loca] (%08lx)\n", start));

	switch (locFormat)
		{
	case 0:
		dumpFormat0(loca->format, level);
		break;
	case 1:
		dumpFormat1(loca->format, level);
		break;
		}
	}

void locaFree(void)
	{
    if (!loaded)
		return;

	switch (locFormat)
		{
	case 0:
		memFree(((Format0 *)loca->format)->offsets);
		memFree(loca->format);
		break;
	case 1:
		memFree(((Format1 *)loca->format)->offsets);
		memFree(loca->format);
		break;
		}

	memFree(loca); loca = NULL;
	loaded = 0;
	}

IntX locaGetOffset(GlyphId glyphId, LongN *offset, Card32 *length,
				   Card32 client)
	{
	if (!loaded)
		{
		if (sfntReadTable(loca_))
			return tableMissing(loca_, client);
		}

	switch (locFormat)
		{
	case 0:
		{
		Card16 *offsets = ((Format0 *)loca->format)->offsets;
		*offset = offsets[glyphId] * 2;
		*length = offsets[glyphId + 1] * 2 - *offset;
		break;
		}
	case 1:
		{
		Card32 *offsets = ((Format1 *)loca->format)->offsets;
		*offset = offsets[glyphId];
		*length = offsets[glyphId + 1] - *offset;
		break;
		}
		}
	return 0;
	}
