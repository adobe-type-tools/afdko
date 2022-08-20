/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * CFF_ table support.
 */

#ifndef CFF__H
#define CFF__H

#include "spot.h"
#include "spot_global.h"
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
} GlyphComplementReportT;

extern void CFF_Read(int32_t offset, uint32_t length);
extern void CFF_Dump(int level, int32_t offset);
extern void CFF_Free_spot(void);
extern int CFF_Loaded(void);
extern void CFF_Usage(void);

extern void CFF_ProofGlyph(GlyphId glyphId, void *ctx);
extern void CFF_getMetrics(GlyphId glyphId,
                           int *origShift,
                           int *lsb, int *rsb, int *hwidth,
                           int *tsb, int *bsb, int *vwidth, int *yorig);
extern int CFF_GetNGlyphs(uint16_t *nGlyphs, uint32_t client);
extern int CFF_GetNMasters(uint16_t *nMasters, uint32_t client);
extern int CFF_InitName(void);
extern int8_t *CFF_GetName(GlyphId glyphId, int *length, int forProofing);

extern int CFF_GetBBox(int16_t *xMin, int16_t *yMin, int16_t *xMax, int16_t *yMax,
                        uint32_t client);
extern int CFF_isCID(void);

extern opt_Scanner CFF_GlyphScan;
extern opt_Scanner CFF_BBoxScan;
extern opt_Scanner CFF_ScaleScan;
extern int CFF_DrawTile(GlyphId glyphId, int8_t *code);
extern ProofContextPtr CFF_SynopsisInit(int8_t *title, uint32_t opt_tag);
extern void CFF_SynopsisFinish(void);
#endif /* CFF__H */
