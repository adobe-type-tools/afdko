/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)glyf.h	1.5
 * Changed:    11/30/98 18:33:07
 ***********************************************************************/

/*
 * glyf table support.
 */

#ifndef GLYF_H
#define GLYF_H

#include "global.h"
#include "opt.h"
#include "proof.h"

extern void glyfRead(LongN offset, Card32 length);
extern void glyfDump(IntX level, LongN offset);
extern void glyfFree(void);
extern void glyfUsage(void);
extern IntX glyfLoaded(void);
extern ProofContextPtr glyfSynopsisInit(Byte8 *title, Card32 opt_tag);
extern void glyfSynopsisFinish(void);
extern void glyfDrawTile(GlyphId glyphId, Byte8 *code);

extern void glyfProofGlyph(GlyphId glyphId, void *ctx);
extern void glyfgetMetrics(GlyphId glyphId, 
						   IntX *origShift, 
						   IntX *lsb, IntX *rsb, IntX *hwidth, 
						   IntX *tsb, IntX *bsb, IntX *vwidth, IntX *yorig);
					   
extern opt_Scanner glyfGlyphScan;
extern IdList glyphs;

extern opt_Scanner glyfBBoxScan;
typedef struct
	{
	Int16 left;
	Int16 bottom;
	Int16 right;
	Int16 top;
	} TargetBBoxType;
extern TargetBBoxType target;

extern opt_Scanner glyfScaleScan;
typedef struct
	{
	double h;
	double v;
	} ScaleType;
extern ScaleType scale;
#endif /* GLYF_H */
