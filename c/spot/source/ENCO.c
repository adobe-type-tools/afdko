/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)ENCO.c	1.9
 * Changed:    7/14/99 09:24:28
 ***********************************************************************/

#include "ENCO.h"
#include "sfnt_ENCO.h"
#include "FNAM.h"

static ENCOTbl *ENCO = NULL;
static IntX loaded = 0;
static Card16 nEncodings;

static Format0 *readFormat0(void)
	{
	Format0 *format = memNew(sizeof(Format0));

	format->format = 0;
	return format;
	}

static Format1 *readFormat1(void)
	{
	IntX i;
	Format1 *format = memNew(sizeof(Format1));

	format->format = 1;
	IN1(format->count);

	format->glyphId = memNew(sizeof(format->glyphId[0]) * format->count);
	for (i = 0; i < format->count; i++)
		IN1(format->glyphId[i]);

	format->code = memNew(sizeof(format->code[0]) * format->count);
	for (i = 0; i < format->count; i++)
		IN1(format->code[i]);

	return format;
	}

static Format2 *readFormat2(void)
	{
	IntX i;
	Format2 *format = memNew(sizeof(Format2));

	format->format = 2;
	
	for (i = 0; i < 256; i++)
		IN1(format->glyphId[i]);
	
	return format;
	}

void ENCORead(LongN start, Card32 length)
	{
	IntX i;

	if (loaded)
		return;

	ENCO = (ENCOTbl *)memNew(sizeof(ENCOTbl));

	if (FNAMGetNEncodings(&nEncodings, ENCO_))
		return;


	SEEK_ABS(start);
	
	IN1(ENCO->version);
	
	ENCO->offset = memNew(sizeof(ENCO->offset[0]) * (nEncodings + 1));
	for (i = 0; i < nEncodings + 1; i++)
		IN1(ENCO->offset[i]);

	ENCO->encoding = memNew(sizeof(ENCO->encoding[0]) * nEncodings);
	for (i = 0; i < nEncodings; i++)
		{
		Card16 format;
		
		SEEK_ABS(ENCO->offset[i] + start);

		IN1(format);
		switch (format)
			{
		case ENCO_STANDARD:
			ENCO->encoding[i] = readFormat0();
			break;
		case ENCO_SPARSE:
			ENCO->encoding[i] = readFormat1();
			break;
		case ENCO_DENSE:
			ENCO->encoding[i] = readFormat2();
			break;
		default:
			warning(SPOT_MSG_ENCOUNKENCOD, format);
			}
		}
	
	loaded = 1;
	}

/* Dump format 0 encoding table */
static void dumpFormat0(Format0 *format, IntX level)
	{
	DLu(2, "format=", format->format);
	}

/* Dump format 1 encoding table */
static void dumpFormat1(Format1 *format, IntX level)
	{
	IntX i;

	DLu(2, "format=", format->format);
	DLu(2, "count =", format->count);

	DL(3, (OUTPUTBUFF, "--- glyphId[index]=glyphId\n"));
	for (i = 0; i < format->count; i++)
		DL(3, (OUTPUTBUFF, "[%d]=%hu ", i, format->glyphId[i]));
	DL(3, (OUTPUTBUFF, "\n"));

	DL(3, (OUTPUTBUFF, "--- code[index]=code\n"));
	for (i = 0; i < format->count; i++)
		DL(3, (OUTPUTBUFF, "[%d]=%d ", i, format->code[i]));
	DL(3, (OUTPUTBUFF, "\n"));
	}

/* Dump format 2 encoding table */
static void dumpFormat2(Format2 *format, IntX level)
	{
	IntX i;

	DLu(2, "format=", format->format);

	DL(3, (OUTPUTBUFF, "--- glyphId[index]=glyphId\n"));
	for (i = 0; i < 256; i++)
		DL(3, (OUTPUTBUFF, "[%d]=%hu ", i, format->glyphId[i]));
	DL(3, (OUTPUTBUFF, "\n"));
	}

void ENCODump(IntX level, LongN start)
	{
	IntX i;

	DL(1, (OUTPUTBUFF, "### [ENCO] (%08lx)\n", start));

	DLV(2, "version  =", ENCO->version);

	DL(2, (OUTPUTBUFF, "--- offset[index]=value\n"));
	for (i = 0; i <= nEncodings; i++)
		DL(2, (OUTPUTBUFF, "[%d]=%08x ", i, ENCO->offset[i]));
	DL(2, (OUTPUTBUFF, "\n"));

	for (i = 0; i < nEncodings; i++)
		{
		void *encoding = ENCO->encoding[i];

		DL(2, (OUTPUTBUFF, "--- encoding[%d]\n", i));

		switch (*(Card16 *)encoding)
			{
		case ENCO_STANDARD:
			dumpFormat0(encoding, level);
			break;
		case ENCO_SPARSE:
			dumpFormat1(encoding, level);
			break;
		case ENCO_DENSE:
			dumpFormat2(encoding, level);
			break;
			}
		}
	}

static void freeFormat1(Format1 *format)
	{
	memFree(format->glyphId);
	memFree(format->code);
	}

void ENCOFree(void)
	{
	IntX i;

    if (!loaded)
		return;

	for (i = 0; i < nEncodings; i++)
		{
		void *encoding = ENCO->encoding[i];

		switch (*(Card16 *)encoding)
			{
		case ENCO_SPARSE:
			freeFormat1(encoding);
			memFree(encoding);
			break;
		case ENCO_STANDARD:
		case ENCO_DENSE:
			memFree(encoding);
			break;
			}
		}
	memFree(ENCO->encoding);
	memFree(ENCO->offset);
	memFree(ENCO); ENCO = NULL;
	loaded = 0;
	}
