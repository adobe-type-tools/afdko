/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)vmtx.c	1.7
 * Changed:    5/19/99 18:59:21
 ***********************************************************************/

#include "VORG.h"
#include "sfnt_VORG.h"
#include "sfnt.h"
#include "head.h"
#include "proof.h"
static VORGTbl *VORG = NULL;
static IntX loaded = 0;
extern char VORGfound;


void VORGRead(LongN start, Card32 length)
	{
	IntX i;

	if (loaded)
		return;

	VORG = (VORGTbl *)memNew(sizeof(VORGTbl));

	/* Read vertical metrics */
	SEEK_ABS(start);
	IN1(VORG->major);
	IN1(VORG->minor);
	
	IN1(VORG->defaultVertOriginY);
	IN1(VORG->numVertOriginYMetrics);
	if (VORG->numVertOriginYMetrics>0)
	{
		VORG->vertMetrics=(vertOriginYMetric *) memNew(VORG->numVertOriginYMetrics*sizeof(vertOriginYMetric));
		for (i = 0; i < VORG->numVertOriginYMetrics; i++)
		  {
			IN1(VORG->vertMetrics[i].glyphIndex);
			IN1(VORG->vertMetrics[i].vertOriginY);
		  }
	}else VORG->vertMetrics=NULL;
	loaded = 1;
	}
	

static void proofVORGGlyph(ProofContextPtr proofctx, GlyphId glyphId, Int16 originDy, IntX proofElementCounter, IntX level)
		{
		IntX origShift, lsb, rsb, width, tsb, bsb, vwidth, yorig;
		proofOptions options;
		Byte8 *name = getGlyphName(glyphId, 0);
		proofClearOptions(&options);


		getMetrics(glyphId, &origShift, &lsb, &rsb, &width, &tsb, &bsb, &vwidth, &yorig);
		options.vorigin = yorig;
		options.voriginflags = ANNOT_DASHEDLINE|ANNOT_ATBOTTOM;
		options.newvorigin = 0;
		options.newvoriginflags = ANNOT_DASHEDLINE|ANNOT_ATBOTTOMDOWN2;

		proofCheckAdvance(proofctx, 1000 + 2*(abs(vwidth)));
		proofDrawGlyph(proofctx, 
						   glyphId, ANNOT_SHOWIT|ADORN_BBOXMARKS|ADORN_WIDTHMARKS, /* glyphId,glyphflags */
						   name, ANNOT_SHOWIT|ANNOT_ATRIGHT,   /* glyphname,glyphnameflags */
						   NULL, 0, /* altlabel,altlabelflags */
						   0, 0, /* originDx,originDy */
						   originDy,ANNOT_ATRIGHTDOWN2|ANNOT_BOLD, /* origin, originflags */
						   vwidth, ANNOT_NOLINE|ANNOT_ATRIGHTDOWN1, /* width,widthflags */
						   &options, yorig,
						   ""
						   );
		}

void VORGDump(IntX level, LongN start)
	{
	IntX i;
	initGlyphNames();
	if (level == 8)
		{
		ProofContextPtr proofctx = NULL;
		Card16 unitsPerEm = 0;
		headGetUnitsPerEm(&unitsPerEm, VORG_);
		proofSetVerticalMode();
		proofctx = proofInitContext(proofPS, STDPAGE_LEFT, STDPAGE_RIGHT,
									STDPAGE_TOP, STDPAGE_BOTTOM,
									"VORG: name@GID, width and vertical origin.",
									proofCurrentGlyphSize(), 
									0.0, unitsPerEm, 
									0, 1, NULL);
			for (i = 0; i < VORG->numVertOriginYMetrics; i++)
			{
			vertOriginYMetric *vmtx = &VORG->vertMetrics[i];
			proofVORGGlyph(proofctx, vmtx->glyphIndex, vmtx->vertOriginY, i, level);
			}
		proofUnSetVerticalMode();
		proofSynopsisFinish();
		if (proofctx)
		  proofDestroyContext(&proofctx);
 		}
	else
		{
		DL(4, (OUTPUTBUFF, "### [VORG] (%08lx)\n", start));
		DL(4, (OUTPUTBUFF,"majorVersion            = %d\n", VORG->major));
		DL(4, (OUTPUTBUFF,"minorVersion            = %d\n", VORG->minor));
		DL(4, (OUTPUTBUFF, "defaultVertOriginY     = %d\n", VORG->defaultVertOriginY));
		DL(4, (OUTPUTBUFF, "numVertOriginYMetrics  = %d\n", VORG->numVertOriginYMetrics));
		
		if (level ==4)
			DL(4, (OUTPUTBUFF, "--- glyphname @glyphID = vertOriginY\n"));
		else
			DL(2, (OUTPUTBUFF, "--- vertOriginYMetrics[index]=(glyphIndex,vertOriginY)\n"));
		
		for (i = 0; i < VORG->numVertOriginYMetrics; i++)
			{
			Card16 gid = VORG->vertMetrics[i].glyphIndex;
			if (level == 4)
				{
				Byte8 *name = getGlyphName(gid, 0);
				DL(4, (OUTPUTBUFF, "%s @%d = %d \n",name, gid,
				   VORG->vertMetrics[i].vertOriginY));
				}
			else
				{
				DL(2, (OUTPUTBUFF, "[%d]=(%d,%d) \n", i, gid,
				   VORG->vertMetrics[i].vertOriginY));
				}
			}
		DL(2, (OUTPUTBUFF, "\n"));
		}
	}

void VORGFree(void)
	{
	if (!loaded)
		return;
	
	memFree(VORG->vertMetrics);
	memFree(VORG); VORG = NULL;
	loaded = 0;
	}


IntX VORGGetVertOriginY(GlyphId glyphId, Int16 *vertOriginY, Card32 client)
	{
	IntX i;
	
	VORGfound=0;
	if (!loaded)
		{
		if (sfntReadTable(VORG_))
			return 1 /* tableMissing(vmtx_, client) */  ; 
		}

	for (i=0; i<VORG->numVertOriginYMetrics && VORG->vertMetrics[i].glyphIndex<glyphId; i++)
		;
	
	if (i>=VORG->numVertOriginYMetrics)
		*vertOriginY=VORG->defaultVertOriginY;
	else if (VORG->vertMetrics[i].glyphIndex==glyphId){
		*vertOriginY=VORG->vertMetrics[i].vertOriginY;
		VORGfound=1;
	}
	else
		*vertOriginY=VORG->defaultVertOriginY;
	return 0;
	}

void VORGUsage(void)
	{
	fprintf(OUTPUTBUFF,  "--- VORG\n"
		   "=4  Print glyph name and id\n"
		   "=8  Proof glyph in Kanji em-box, with widht and Y origin annotations.\n"
		   );
	}
