/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)WDTH.c	1.7
 * Changed:    5/19/99 17:21:02
 ***********************************************************************/

#include "WDTH.h"
#include "sfnt_WDTH.h"

static WDTHTbl *WDTH = NULL;
static IntX loaded = 0;

void WDTHRead(LongN start, Card32 length)
	{
	IntX i;
	IntX size;
	IntX nElements;
	IntX nOffsets;

	if (loaded)
		return;
	WDTH = (WDTHTbl *)memNew(sizeof(WDTHTbl));

	SEEK_ABS(start);
	
	IN1(WDTH->version);
	IN1(WDTH->flags);
	IN1(WDTH->nMasters);
	IN1(WDTH->nRanges);

	size = (WDTH->flags & LONG_OFFSETS) ? sizeof(Card32) : sizeof(Card16);
	nElements = WDTH->nRanges + 1;

	/* Read first glyphs */
	WDTH->firstGlyph = memNew(sizeof(GlyphId) * nElements);
	for (i = 0; i < nElements; i++)
		IN1(WDTH->firstGlyph[i]);

	/* Read offsets */
	if (WDTH->flags & LONG_OFFSETS)
		{
		Card32 *offset = WDTH->offset = memNew(sizeof(Card32) * nElements);
		for (i = 0; i < nElements; i++)
			IN1(offset[i]);	
		nOffsets = offset[WDTH->nRanges] - offset[0];
		}
	else
		{
		Card16 *offset = WDTH->offset = memNew(sizeof(Card16) * nElements);
		for (i = 0; i < nElements; i++)
			IN1(offset[i]);	
		nOffsets = offset[WDTH->nRanges] - offset[0];
		}

	/* Read widths */
	WDTH->width = memNew(sizeof(uFWord) * WDTH->nMasters * nOffsets);
	for (i = 0; i < nOffsets; i++)
		IN1(WDTH->width[i]);

	loaded = 1;
	}

void WDTHDump(IntX level, LongN start)
	{
	IntX i;
	IntX j;
	IntX k;
	IntX iWidth;
	IntX nElements = WDTH->nRanges + 1;

	DL(1, (OUTPUTBUFF, "### [WDTH] (%08lx)\n", start));

	DLV(2, "version =", WDTH->version);
	DLu(2, "flags   =", WDTH->flags);
	DLu(2, "nMasters=", WDTH->nMasters);
	DLu(2, "nRanges =", WDTH->nRanges);

	DL(3, (OUTPUTBUFF, "--- firstGlyph[index]=glyphId\n"));
	for (i = 0; i < nElements; i++)
		DL(3, (OUTPUTBUFF, "[%d]=%hu ", i, WDTH->firstGlyph[i]));
	DL(3, (OUTPUTBUFF, "\n"));

	DL(3, (OUTPUTBUFF, "--- offset[index]=offset\n"));
	if (WDTH->flags & LONG_OFFSETS)
		for (i = 0; i < nElements; i++)
			DL(3, (OUTPUTBUFF, "[%d]=%08x ", i, ((Card32 *)WDTH->offset)[i]));
	else
		for (i = 0; i < nElements; i++)
			DL(3, (OUTPUTBUFF, "[%d]=%04hx ", i, ((Card16 *)WDTH->offset)[i]));
	DL(3, (OUTPUTBUFF, "\n"));

	iWidth = 0;
	if (WDTH->nMasters == 1)
		{
		DL(3, (OUTPUTBUFF, "--- width[offset]=value\n"));
		if (WDTH->flags & LONG_OFFSETS)
			{
			Card32 *offset = WDTH->offset;
			for (i = 0; i < WDTH->nRanges; i++)
				{
				IntX span = (offset[i + 1] - offset[i]) / sizeof(uFWord);
				for (j = 0; j < span; j++)
					DL(3, (OUTPUTBUFF, "[%08lx]=%hu ", offset[i] + j * sizeof(uFWord),
						   WDTH->width[iWidth++]));
				}
			}
		else
			{
			Card16 *offset = WDTH->offset;
			for (i = 0; i < WDTH->nRanges; i++)
				{
				IntX span = (offset[i + 1] - offset[i]) / sizeof(uFWord);
				for (j = 0; j < span; j++)
					DL(3, (OUTPUTBUFF, "[%04lx]=%hu ", offset[i] + j * sizeof(uFWord),
						   WDTH->width[iWidth++]));
				}
			}
		}
	else
		{
		DL(3, (OUTPUTBUFF, "--- width[offset]={value+}\n"));
		if (WDTH->flags & LONG_OFFSETS)
			{
			Card32 *offset = WDTH->offset;
			for (i = 0; i < WDTH->nRanges; i++)
				{
				IntX span = (offset[i + 1] - offset[i]) / sizeof(uFWord);
				for (j = 0; j < span; j++)
					{
					DL(3, (OUTPUTBUFF, "[%08lx]={", offset[i] + j * sizeof(uFWord)));
					for (k = 0; k < WDTH->nMasters; k++)
						DL(3, (OUTPUTBUFF, "%hu%s", WDTH->width[iWidth++],
							   k == WDTH->nMasters - 1 ? "} " : ","));
					}
				}
			}
		else
			{
			Card16 *offset = WDTH->offset;
			for (i = 0; i < WDTH->nRanges; i++)
				{
				IntX span = (offset[i + 1] - offset[i]) / sizeof(uFWord);
				for (j = 0; j < span; j++)
					{
					DL(3, (OUTPUTBUFF, "[%04lx]={", offset[i] + j * sizeof(uFWord)));
					for (k = 0; k < WDTH->nMasters; k++)
						DL(3, (OUTPUTBUFF, "%hu%s", WDTH->width[iWidth++],
							   k == WDTH->nMasters - 1 ? "} " : ","));
					}
				}
			}
		}
	DL(3, (OUTPUTBUFF, "\n"));
	}

void WDTHFree(void)
	{
    if (!loaded)
		return;

	memFree(WDTH->firstGlyph);
	memFree(WDTH->offset);
	memFree(WDTH->width);
	memFree(WDTH); WDTH = NULL;
	loaded = 0;
	}

