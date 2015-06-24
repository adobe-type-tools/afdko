/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************
 * SCCS Id:    @(#)proof.h	1.12
 * Changed:    12/14/98 16:47:38
 ***********************************************************************/


#ifndef PROOF_H
#define PROOF_H

#include "global.h"

typedef struct _ProofContext *ProofContextPtr;

typedef enum _proofOutputType {
  proofPS = 1
} proofOutputType;


typedef struct _proofOptions 
{
  /* the following are in Character-space coords */
  Int16 vorigin;        Card16 voriginflags;
  Int16 baseline;       Card16 baselineflags;
  Int16 vbaseline;      Card16 vbaselineflags;
  Int16 lsb;            Card16 lsbflags;
  Int16 rsb;            Card16 rsbflags;
  Int16 tsb;            Card16 tsbflags;
  Int16 bsb;            Card16 bsbflags;
  Int16 vwidth;         Card16 vwidthflags;
  Int16 neworigin;      Card16 neworiginflags;
  Int16 newvorigin;     Card16 newvoriginflags;
  Int16 newbaseline;    Card16 newbaselineflags;
  Int16 newvbaseline;   Card16 newvbaselineflags;
  Int16 newlsb;         Card16 newlsbflags;
  Int16 newrsb;         Card16 newrsbflags;
  Int16 newwidth;       Card16 newwidthflags;
  Int16 newtsb;         Card16 newtsbflags;
  Int16 newbsb;         Card16 newbsbflags;
  Int16 newvwidth;      Card16 newvwidthflags;
} proofOptions;

extern void proofClearOptions(proofOptions *options);

extern ProofContextPtr proofInitContext(proofOutputType where,
										Card32 leftposit, Card32 rightposit,
										Card32 topposit,  Card32 botposit,
										Byte8 *pageTitle,
										double glyphSizepts,
										double thinspacepts,
										double unitsPerEM,
										IntX noFrills,
										IntX isPatt,
										Byte8 *PSFilenameorPATTorNULL);
extern IntX proofPageCount(ProofContextPtr ctx);
extern Byte8 *proofFileName(ProofContextPtr ctx);
extern void proofDestroyContext(ProofContextPtr *ctx);

/* Annotation flags */
#define ANNOT_NOSHOW 		0x00
#define ANNOT_SHOWIT		0x01
#define ANNOT_NOLINE        0x02
#define ANNOT_DASHEDLINE	0x04
#define ANNOT_BOLD			0x08
#define ANNOT_EMPHASIS      0x10
#define ANNOT_ATBOTTOM		0x20
#define ANNOT_ATBOTTOMDOWN1	0x40
#define ANNOT_ATBOTTOMDOWN2	0x80
#define ANNOT_ATRIGHT		0x100
#define ANNOT_ATRIGHTDOWN1	0x200
#define ANNOT_ATRIGHTDOWN2	0x400
#define ANNOT_ABSVAL		0x800
#define ANNOT_RELVAL		0x1000

/* Glyph Adornment flags */
#define ADORN_WIDTHMARKS	0x2000
#define ADORN_BBOXMARKS		0x4000

extern void proofMessage(ProofContextPtr ctx, Byte8 * str);

extern void proofDrawGlyph(ProofContextPtr c,	
						   GlyphId glyphId,		Card16 glyphflags,
						   Byte8 *glyphname,	Card16 glyphnameflags,
						   Byte8 *altlabel,		Card16 altlabelflags,
						   Int16 originDx,      Int16 originDy,
						   Int16 origin,        Card16 originflags,
						   Int16 width,         Card16 widthflags,
						   proofOptions *options, Int16 yOrigKanji, char * message
						   );	


extern void proofSymbol(ProofContextPtr c, IntX symbol);
/* Symbols: */
#define PROOF_PLUS 		0x2B
#define PROOF_YIELDS	0xAE
#define PROOF_DBLYIELDS 0xDE
#define PROOF_COMMA		0x2C
#define PROOF_COLON		0x3A
#define PROOF_LPAREN	0x28
#define PROOF_RPAREN	0x29
#define PROOF_PRIME		0xA2
#define PROOF_NOTELEM   0xCF

extern void proofThinspace(ProofContextPtr c, IntX count);
extern void proofNewline(ProofContextPtr c);
extern void proofNewPage(ProofContextPtr ctx);
extern void proofOnlyNewPage(ProofContextPtr ctx);
extern void proofCheckAdvance(ProofContextPtr c, Int16 wx);

extern void proofGlyphMT(ProofContextPtr c, double x, double y);
extern void proofGlyphLT(ProofContextPtr c, double x, double y);
extern void proofGlyphCT(ProofContextPtr c, double x1, double y1, double x2, double y2, double x3, double y3);
extern void proofGlyphClosePath(ProofContextPtr c);

extern void proofPSOUT(ProofContextPtr c, const Byte8 *cmd);

#define STDPAGE_MARGIN	36
#define STDPAGE_WIDTH 	612
#define STDPAGE_HEIGHT	792
#define STDPAGE_LEFT  STDPAGE_MARGIN
#define STDPAGE_RIGHT (STDPAGE_WIDTH - STDPAGE_MARGIN)
#define STDPAGE_TOP   STDPAGE_HEIGHT - STDPAGE_MARGIN
#define STDPAGE_BOTTOM STDPAGE_MARGIN
#define STDPAGE_GLYPH_PTSIZE  24.0

#define kProofBufferLen 1024


extern void proofSetPolicy(IntX policyNum, IntX value);
extern void proofResetPolicies(void);
extern opt_Scanner proofPolicyScan;
extern double proofCurrentGlyphSize(void);
extern IntX proofIsVerticalMode(void);
extern void proofSetGlyphSize(double);
extern void proofDrawTile(GlyphId glyphId, Byte8 *code);
extern ProofContextPtr proofSynopsisInit(Byte8 *title, Card32 opt_tag);
extern void proofSynopsisFinish(void);
extern void proofSetVerticalMode(void);
extern void proofUnSetVerticalMode(void);
extern IntX proofIsAltKanjiKern(void);
#endif /* PROOF_H */
