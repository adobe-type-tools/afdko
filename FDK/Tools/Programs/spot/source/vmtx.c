/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)vmtx.c	1.7
 * Changed:    5/19/99 18:59:21
 ***********************************************************************/

#include "vmtx.h"
#include "sfnt_vmtx.h"
#include "vhea.h"
#include "maxp.h"
#include "sfnt.h"
#include "head.h"


static vmtxTbl *vmtx = NULL;
static IntX loaded = 0;
static Card16 nLongVerMetrics;
static Card16 nTopSideBearings;
static Card16 nGlyphs;
static Card16 unitsPerEm;

#define RND(x)	((IntX)((x) + 0.5))

void vmtxRead(LongN start, Card32 length)
	{
	IntX i;

	if (loaded)
		return;

	vmtx = (vmtxTbl *)memNew(sizeof(vmtx));

	if (vheaGetNLongVerMetrics(&nLongVerMetrics, vmtx_) ||
		getNGlyphs(&nGlyphs, hmtx_) ||
		headGetUnitsPerEm(&unitsPerEm, hmtx_))
		return;


	if (nLongVerMetrics > 1)
	  {
		/* Read vertical metrics */
		SEEK_ABS(start);

		vmtx->vMetrics = memNew(sizeof(LongVerMetrics) * nGlyphs);
		for (i = 0; i < nLongVerMetrics; i++)
		  {
			IN1(vmtx->vMetrics[i].advanceHeight);
			IN1(vmtx->vMetrics[i].topSideBearing);
		  }
		for (i = nLongVerMetrics; i < nGlyphs; i++)
		  {
			vmtx->vMetrics[i].advanceHeight = vmtx->vMetrics[nLongVerMetrics-1].advanceHeight;
			vmtx->vMetrics[i].topSideBearing = vmtx->vMetrics[nLongVerMetrics-1].topSideBearing;
		  }

		/* Read top side bearings */
		nTopSideBearings = nGlyphs - nLongVerMetrics;
		if (nTopSideBearings > 0)
		  {
			vmtx->topSideBearing = 
			  memNew(sizeof(vmtx->topSideBearing[0]) * nTopSideBearings);
			for (i = 0; i < nTopSideBearings; i++)
			{
			  IN1(vmtx->topSideBearing[i]);
			  vmtx->vMetrics[nLongVerMetrics + i].topSideBearing = vmtx->topSideBearing[i];
			}
		  }
	  }
	else
	  {
		/* Monospace metrics */
		uFWord adv;
		FWord tsb;

		SEEK_ABS(start);
		IN1(adv);
		IN1(tsb);

		vmtx->vMetrics = memNew(sizeof(LongVerMetrics) * nGlyphs);
		for (i = 0; i < nGlyphs; i++)
		  {
			vmtx->vMetrics[i].advanceHeight = adv;
			vmtx->vMetrics[i].topSideBearing = tsb;
		  }
		/* Read top side bearings */
		nTopSideBearings = nGlyphs - nLongVerMetrics;
		if (nTopSideBearings > 0)
		  {
			vmtx->topSideBearing = 
			  memNew(sizeof(vmtx->topSideBearing[0]) * nTopSideBearings);
			for (i = 0; i < nTopSideBearings; i++)
			{
			  IN1(vmtx->topSideBearing[i]);
			  vmtx->vMetrics[nLongVerMetrics + i].topSideBearing = vmtx->topSideBearing[i];
			}
		  }
	  }
	loaded = 1;
	}

static void dumpFormat5_6(IntX level)
	{
	int i;

	fprintf(OUTPUTBUFF,  "--- [name]=top side bearing (%d units/em)\n", (level == 5)? unitsPerEm: 1000);

	initGlyphNames();
	for (i = 0; i < nGlyphs; i++)
		fprintf(OUTPUTBUFF,  "[%s]=%hd ", getGlyphName(i, 0), 
			   (Int16)((level == 6)?
			   RND(vmtx->vMetrics[i].topSideBearing * 1000.0 / unitsPerEm):
			   vmtx->vMetrics[i].topSideBearing));
	
	fprintf(OUTPUTBUFF,  "\n");
	}

static void dumpFormat7_8(IntX level)
	{
	int i;

	fprintf(OUTPUTBUFF,  "--- [name]=advance height,top side bearing (%d units/em)\n", (level == 7)? unitsPerEm: 1000);

	initGlyphNames();
	for (i = 0; i < nGlyphs; i++)
		fprintf(OUTPUTBUFF,  "[%s]=%hu,%hd ", getGlyphName(i, 0), 
			   (Card16)((level == 8)?
			   RND(vmtx->vMetrics[i].advanceHeight * 1000.0 / unitsPerEm):
			   vmtx->vMetrics[i].advanceHeight),
			   (Int16)((level == 8)?
			   RND(vmtx->vMetrics[i].topSideBearing * 1000.0 / unitsPerEm):
			   vmtx->vMetrics[i].topSideBearing));

	fprintf(OUTPUTBUFF,  "\n");
	}

void vmtxDump(IntX level, LongN start)
	{
	IntX i;

	if (!loaded)
		return;

	DL(1, (OUTPUTBUFF, "### [vmtx] (%08lx)\n", start));

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

	DL(2, (OUTPUTBUFF, "--- vMetrics[index]={advanceHeight,topSideBearing}\n"));
	for (i = 0; i < nLongVerMetrics; i++)
		DL(2, (OUTPUTBUFF, "[%d]={%hu,%hd} ", i, vmtx->vMetrics[i].advanceHeight,
			   vmtx->vMetrics[i].topSideBearing));
	DL(2, (OUTPUTBUFF, "\n"));

	if (nTopSideBearings > 0)
		{
		DL(2, (OUTPUTBUFF, "--- topSideBearing[index]=value\n"));
		for (i = 0; i < nTopSideBearings; i++)
			DL(2, (OUTPUTBUFF, "[%d]=%hd ", i, vmtx->topSideBearing[i]));
		DL(2, (OUTPUTBUFF, "\n"));
		}
	}

void vmtxFree(void)
	{
	if (!loaded)
		return;
	
	memFree(vmtx->vMetrics);
	if (nTopSideBearings > 0)
		memFree(vmtx->topSideBearing);
	memFree(vmtx); vmtx = NULL;
	loaded = 0;
	}


IntX vmtxGetMetrics(GlyphId glyphId, FWord *topSideBearing, 
					uFWord *vadvance, Card32 client)
	{
	if (!loaded)
		{
		if (sfntReadTable(vmtx_))
			return 1 /* tableMissing(vmtx_, client) */  ; 
		}

	*topSideBearing = vmtx->vMetrics[glyphId].topSideBearing;
	*vadvance = vmtx->vMetrics[glyphId].advanceHeight;
	
	return 0;
	}

void vmtxUsage(void)
	{
	fprintf(OUTPUTBUFF,  "--- hmtx\n"
		   "=5  Print advance top side bearings by glyph name (font's units/em)\n"
		   "=6  Print advance top side bearings by glyph name (1000 units/em)\n"
		   "=7  Print advance heights and top side bearings by glyph name (font's units/em)\n"
		   "=8  Print advance heights and top side bearings by glyph name (1000 units/em)\n"
		   "Note that the default dump has two sections: a first section which lists\n"
		   "advance height and top side bearing, and a second section which lists only\n"
		   "the top side bearing. The index for the second section is not the glyph ID,\n"
		   "as it starts at 0. To convert an index in the second section to a glyph ID,\n"
		   "you need to add to the index the number of entries in the first section.");
	}
