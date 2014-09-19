/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)hmtx.c	1.13
 * Changed:    5/19/99 18:59:19
 ***********************************************************************/

#include "hmtx.h"
#include "sfnt_hmtx.h"
#include "hhea.h"
#include "maxp.h"
#include "sfnt.h"
#include "head.h"

static hmtxTbl *hmtx = NULL;
static IntX loaded = 0;
static Card16 nLongHorMetrics;
static Card16 nLeftSideBearings;
static Card16 nGlyphs;
static Card16 unitsPerEm;

#define RND(x)	((IntX)((x) + 0.5))

void hmtxRead(LongN start, Card32 length)
	{
	IntX i;

	if (loaded)
		return;

	hmtx = (hmtxTbl *)memNew(sizeof(hmtxTbl));

	if (hheaGetNLongHorMetrics(&nLongHorMetrics, hmtx_) ||
		getNGlyphs(&nGlyphs, hmtx_) ||
		headGetUnitsPerEm(&unitsPerEm, hmtx_))
		return;
		
	/* Hack to deal with SWF subsetted font, where hmtx table is NOT subsetted, and has more entries than the CFF table. */
	if (nGlyphs < nLongHorMetrics)
		{
		fprintf(OUTPUTBUFF,  "spot [Warning]: ifont data is inconsistent - there are more nLongHorMetrics in hmtx table than there are glyphs.\n");
		nGlyphs = (Card16)(length - (nLongHorMetrics*sizeof(LongHorMetrics)));
		if (nGlyphs > 0)
			nGlyphs = nLongHorMetrics + nGlyphs/sizeof(hmtx->leftSideBearing[0]);
		else if (nGlyphs == 0)
			nGlyphs = nLongHorMetrics;
		else
			{
			fprintf(OUTPUTBUFF,  "spot [Error]: Badly constructed font - the hmtx table isn't long enough to hold the numbe of nLongHorMetrics specified in the head table.\n");
			nLongHorMetrics = (Card16)(length/sizeof(LongHorMetrics));
			nGlyphs = nLongHorMetrics;
			return;
			}
		}

	if (nLongHorMetrics > 1)
	  {
		/* Read horizontal metrics */
		SEEK_ABS(start);

		hmtx->hMetrics = memNew(sizeof(LongHorMetrics) * nGlyphs /*nLongHorMetrics*/);
		for (i = 0; i < nLongHorMetrics; i++)
		  {
			IN1(hmtx->hMetrics[i].advanceWidth);
			IN1(hmtx->hMetrics[i].lsb);
		  }
		for (i = nLongHorMetrics; i < nGlyphs; i++)
		  {
			hmtx->hMetrics[i].advanceWidth = hmtx->hMetrics[nLongHorMetrics-1].advanceWidth;
			hmtx->hMetrics[i].lsb = hmtx->hMetrics[nLongHorMetrics-1].lsb;
		  }
		
		/* Read left side bearings */
		nLeftSideBearings = nGlyphs - nLongHorMetrics;
		if (nLeftSideBearings > 0)
		  {
			hmtx->leftSideBearing = 
			  memNew(sizeof(hmtx->leftSideBearing[0]) * nLeftSideBearings);
			for (i = 0; i < nLeftSideBearings; i++)
				{
				IN1(hmtx->leftSideBearing[i]);
				hmtx->hMetrics[i+nLongHorMetrics].lsb = hmtx->leftSideBearing[i];
				}
		  }
	  }
	else
	  {
		/* Monospace metrics. Read the value and replicate for the rest */
		uFWord adv;
		FWord lsb;

		SEEK_ABS(start);
		IN1(adv);
		IN1(lsb);

		hmtx->hMetrics = memNew(sizeof(LongHorMetrics) * nGlyphs);
		for (i = 0; i < nGlyphs; i++)
		  {
			hmtx->hMetrics[i].advanceWidth = adv;
			hmtx->hMetrics[i].lsb = lsb;
		  }

		/* Read left side bearings */
		nLeftSideBearings = nGlyphs - nLongHorMetrics;
		if (nLeftSideBearings > 0)
		  {
			hmtx->leftSideBearing = 
			  memNew(sizeof(hmtx->leftSideBearing[0]) * nLeftSideBearings);
			for (i = 0; i < nLeftSideBearings; i++)
				{
				IN1(hmtx->leftSideBearing[i]);
				hmtx->hMetrics[i+nLongHorMetrics].lsb = hmtx->leftSideBearing[i];
				}
		  }


	  }
	loaded = 1;
	}

static void dumpFormat5_6(IntX level)
	{
	int i;
	
	fprintf(OUTPUTBUFF,  "--- [name]=width (%d units/em)\n", (level == 5)? unitsPerEm: 1000);

	initGlyphNames();
	for (i = 0; i < nGlyphs; i++)
		fprintf(OUTPUTBUFF,  "[%s]=%hd ", getGlyphName(i, 0), 
			   (Int16)(
                       (level == 6)?
                       (RND(hmtx->hMetrics[i].advanceWidth * 1000.0 / unitsPerEm)):
                       (hmtx->hMetrics[i].advanceWidth)
                       )
                );
	
	fprintf(OUTPUTBUFF,  "\n");
	}

static void dumpFormat7_8(IntX level)
	{
	int i;

	fprintf(OUTPUTBUFF,  "--- [name]=advance width,left side bearing (%d units/em)\n", (level == 7)? unitsPerEm: 1000);

	initGlyphNames();
	for (i = 0; i < nGlyphs; i++)
		fprintf(OUTPUTBUFF,  "[%s]=%hu,%hd ", getGlyphName(i, 0), 
			   (Card16)((level == 8)?
			   RND(hmtx->hMetrics[i].advanceWidth * 1000.0 / unitsPerEm):
			   hmtx->hMetrics[i].advanceWidth),
			   (Int16)((level == 8)?
			   RND(hmtx->hMetrics[i].lsb * 1000.0 / unitsPerEm):
			   hmtx->hMetrics[i].lsb));

	fprintf(OUTPUTBUFF,  "\n");
	}

void hmtxDump(IntX level, LongN start)
	{
	IntX i;

	if (!loaded)
		return;

	DL(1, (OUTPUTBUFF, "### [hmtx] (%08lx)\n", start));

	switch (level)
		{
	case 7:
	case 8:
		dumpFormat7_8(level);
		return;
	case 5:
	case 6:
		dumpFormat5_6(level);
		return;
	default:
		break;
		}

	DL(2, (OUTPUTBUFF, "--- hMetrics[index]={advanceWidth,lsb}\n"));
	for (i = 0; i < nLongHorMetrics; i++)
		DL(2, (OUTPUTBUFF, "[%d]={%hu,%hd} ", i, 
			   hmtx->hMetrics[i].advanceWidth, hmtx->hMetrics[i].lsb));
	DL(2, (OUTPUTBUFF, "\n"));

	if (nLeftSideBearings > 0)
		{
		DL(2, (OUTPUTBUFF, "--- leftSideBearing[index]=value\n"));
		for (i = 0; i < nLeftSideBearings; i++)
			DL(2, (OUTPUTBUFF, "[%d]=%hd ", i, hmtx->leftSideBearing[i]));
		DL(2, (OUTPUTBUFF, "\n"));
		}
	}

void hmtxFree(void)
	{
	if (!loaded)
		return;
	
	memFree(hmtx->hMetrics);
	if (nLeftSideBearings > 0)
		memFree(hmtx->leftSideBearing);

	memFree(hmtx); hmtx = NULL;
	loaded = 0;
	}

IntX hmtxGetMetrics(GlyphId glyphId, FWord *lsb, uFWord *width, Card32 client)
	{
	if (!loaded)
		{
		if (sfntReadTable(hmtx_))
			return tableMissing(hmtx_, client);
		}

		*lsb = hmtx->hMetrics[glyphId].lsb;
		*width = hmtx->hMetrics[glyphId].advanceWidth;

	return 0;
	}

void hmtxUsage(void)
	{
	fprintf(OUTPUTBUFF,  "--- hmtx\n"
		   "=5  Print advance widths by glyph name (font's units/em)\n"
		   "=6  Print advance widths by glyph name (1000 units/em)\n"
		   "=7  Print advance widths and left side bearings by glyph name (font's units/em)\n"
		   "=8  Print advance widths and left side bearings by glyph name (1000 units/em)\n"
			"Note that the default dump has two sections: a first section which lists\n"
		   "advance width and left side bearing, and a second section which lists only\n"
		   "the left side bearing. The index for the second section is not the glyph ID,\n"
		   "as it starts at 0. To convert an index in the second section to a glyph ID,\n"
		   "you need to add to the index the number of entries in the first section.\n");

	}
