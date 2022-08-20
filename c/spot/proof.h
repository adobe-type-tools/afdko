/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0.
   This license is available at: http://opensource.org/licenses/Apache-2.0. */

#ifndef PROOF_H
#define PROOF_H

#include "spot.h"
#include "spot_global.h"

typedef struct _ProofContext *ProofContextPtr;

typedef enum _proofOutputType {
    proofPS = 1
} proofOutputType;

typedef struct _proofOptions {
    /* the following are in Character-space coordinates */
    int16_t vorigin;
    uint16_t voriginflags;
    int16_t baseline;
    uint16_t baselineflags;
    int16_t vbaseline;
    uint16_t vbaselineflags;
    int16_t lsb;
    uint16_t lsbflags;
    int16_t rsb;
    uint16_t rsbflags;
    int16_t tsb;
    uint16_t tsbflags;
    int16_t bsb;
    uint16_t bsbflags;
    int16_t vwidth;
    uint16_t vwidthflags;
    int16_t neworigin;
    uint16_t neworiginflags;
    int16_t newvorigin;
    uint16_t newvoriginflags;
    int16_t newbaseline;
    uint16_t newbaselineflags;
    int16_t newvbaseline;
    uint16_t newvbaselineflags;
    int16_t newlsb;
    uint16_t newlsbflags;
    int16_t newrsb;
    uint16_t newrsbflags;
    int16_t newwidth;
    uint16_t newwidthflags;
    int16_t newtsb;
    uint16_t newtsbflags;
    int16_t newbsb;
    uint16_t newbsbflags;
    int16_t newvwidth;
    uint16_t newvwidthflags;
} proofOptions;

extern void proofClearOptions(proofOptions *options);

extern ProofContextPtr proofInitContext(proofOutputType where,
                                        uint32_t leftposit, uint32_t rightposit,
                                        uint32_t topposit, uint32_t botposit,
                                        int8_t *pageTitle,
                                        double glyphSizepts,
                                        double thinspacepts,
                                        double unitsPerEM,
                                        int noFrills,
                                        int isPatt,
                                        int8_t *PSFilenameorPATTorNULL);
extern int proofPageCount(ProofContextPtr ctx);
extern int8_t *proofFileName(ProofContextPtr ctx);
extern void proofDestroyContext(ProofContextPtr *ctx);

/* Annotation flags */
#define ANNOT_NOSHOW        0x00
#define ANNOT_SHOWIT        0x01
#define ANNOT_NOLINE        0x02
#define ANNOT_DASHEDLINE    0x04
#define ANNOT_BOLD          0x08
#define ANNOT_EMPHASIS      0x10
#define ANNOT_ATBOTTOM      0x20
#define ANNOT_ATBOTTOMDOWN1 0x40
#define ANNOT_ATBOTTOMDOWN2 0x80
#define ANNOT_ATRIGHT       0x100
#define ANNOT_ATRIGHTDOWN1  0x200
#define ANNOT_ATRIGHTDOWN2  0x400
#define ANNOT_ABSVAL        0x800
#define ANNOT_RELVAL        0x1000

/* Glyph Adornment flags */
#define ADORN_WIDTHMARKS 0x2000
#define ADORN_BBOXMARKS 0x4000

extern void proofMessage(ProofContextPtr ctx, int8_t *str);

extern void proofDrawGlyph(ProofContextPtr c,
                           GlyphId glyphId, uint16_t glyphflags,
                           int8_t *glyphname, uint16_t glyphnameflags,
                           int8_t *altlabel, uint16_t altlabelflags,
                           int16_t originDx, int16_t originDy,
                           int16_t origin, uint16_t originflags,
                           int16_t width, uint16_t widthflags,
                           proofOptions *options, int16_t yOrigKanji, char *message);

extern void proofSymbol(ProofContextPtr c, int symbol);
/* Symbols: */
#define PROOF_PLUS      0x2B
#define PROOF_YIELDS    0xAE
#define PROOF_DBLYIELDS 0xDE
#define PROOF_COMMA     0x2C
#define PROOF_COLON     0x3A
#define PROOF_LPAREN    0x28
#define PROOF_RPAREN    0x29
#define PROOF_PRIME     0xA2
#define PROOF_NOTELEM   0xCF

extern void proofThinspace(ProofContextPtr c, int count);
extern void proofNewline(ProofContextPtr c);
extern void proofNewPage(ProofContextPtr ctx);
extern void proofOnlyNewPage(ProofContextPtr ctx);
extern void proofCheckAdvance(ProofContextPtr c, int16_t wx);

extern void proofGlyphMT(ProofContextPtr c, double x, double y);
extern void proofGlyphLT(ProofContextPtr c, double x, double y);
extern void proofGlyphCT(ProofContextPtr c, double x1, double y1, double x2, double y2, double x3, double y3);
extern void proofGlyphClosePath(ProofContextPtr c);

extern void proofPSOUT(ProofContextPtr c, const int8_t *cmd);

#define STDPAGE_MARGIN  36
#define STDPAGE_WIDTH   612
#define STDPAGE_HEIGHT  792
#define STDPAGE_LEFT    STDPAGE_MARGIN
#define STDPAGE_RIGHT   (STDPAGE_WIDTH - STDPAGE_MARGIN)
#define STDPAGE_TOP     STDPAGE_HEIGHT - STDPAGE_MARGIN
#define STDPAGE_BOTTOM  STDPAGE_MARGIN
#define STDPAGE_GLYPH_PTSIZE  24.0

#define kProofBufferLen 1024

extern void proofSetPolicy(int policyNum, int value);
extern void proofResetPolicies(void);
extern opt_Scanner proofPolicyScan;
extern double proofCurrentGlyphSize(void);
extern int proofIsVerticalMode(void);
extern void proofSetGlyphSize(double);
extern void proofDrawTile(GlyphId glyphId, int8_t *code);
extern ProofContextPtr proofSynopsisInit(int8_t *title, uint32_t opt_tag);
extern void proofSynopsisFinish(void);
extern void proofSetVerticalMode(void);
extern void proofUnSetVerticalMode(void);
extern int proofIsAltKanjiKern(void);
#endif /* PROOF_H */
