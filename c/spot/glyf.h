/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * glyf table support.
 */

#ifndef GLYF_H
#define GLYF_H

#include "spot.h"
#include "spot_global.h"
#include "opt.h"
#include "proof.h"

extern void glyfRead(int32_t offset, uint32_t length);
extern void glyfDump(int level, int32_t offset);
extern void glyfFree(void);
extern void glyfUsage(void);
extern int glyfLoaded(void);
extern ProofContextPtr glyfSynopsisInit(int8_t *title, uint32_t opt_tag);
extern void glyfSynopsisFinish(void);
extern void glyfDrawTile(GlyphId glyphId, int8_t *code);

extern void glyfProofGlyph(GlyphId glyphId, void *ctx);
extern void glyfgetMetrics(GlyphId glyphId,
                           int *origShift,
                           int *lsb, int *rsb, int *hwidth,
                           int *tsb, int *bsb, int *vwidth, int *yorig);

extern opt_Scanner glyfGlyphScan;
extern IdList glyphs;

extern opt_Scanner glyfBBoxScan;
typedef struct
{
    int16_t left;
    int16_t bottom;
    int16_t right;
    int16_t top;
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
