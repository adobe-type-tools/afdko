/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)CFF_.h	1.6
 * Changed:    11/30/98 18:32:50
 ***********************************************************************/

/*
 * CFF_ table support.
 */

#ifndef CFF__H
#define CFF__H

#include "global.h"
#include "opt.h"
#include "proof.h"

typedef struct 
	{
		int reportNumber;
		int numFonts;
		int numGlyphs;
		int synOnly;
		int startGlyph, endGlyph;
		int maxNumGlyphs;
		int byname;
	}GlyphComplementReportT;


extern void CFF_Read(LongN offset, Card32 length);
extern void CFF_Dump(IntX level, LongN offset);
extern void CFF_Free(void);
extern IntX CFF_Loaded(void);
extern void CFF_Usage(void);

extern void CFF_ProofGlyph(GlyphId glyphId, void *ctx);
extern void CFF_getMetrics(GlyphId glyphId, 
						   IntX *origShift, 
						   IntX *lsb, IntX *rsb, IntX *hwidth, 
						   IntX *tsb, IntX *bsb, IntX *vwidth, IntX *yorig);
extern IntX CFF_GetNGlyphs(Card16 *nGlyphs, Card32 client);
extern IntX CFF_GetNMasters(Card16 *nMasters, Card32 client);
extern IntX CFF_InitName(void);
extern Byte8 *CFF_GetName(GlyphId glyphId, IntX *length, IntX forProofing);

extern IntX CFF_GetBBox(Int16 *xMin, Int16 *yMin, Int16 *xMax, Int16 *yMax,
                                                Card32 client);
extern IntX CFF_isCID(void);

extern opt_Scanner CFF_GlyphScan;
extern opt_Scanner CFF_BBoxScan;
extern opt_Scanner CFF_ScaleScan;
extern int CFF_DrawTile(GlyphId glyphId, Byte8 *code);
extern ProofContextPtr CFF_SynopsisInit(Byte8 *title, Card32 opt_tag);
extern void CFF_SynopsisFinish(void);
#endif /* CFF__H */
