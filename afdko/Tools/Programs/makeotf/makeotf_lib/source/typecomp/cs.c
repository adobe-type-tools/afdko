/* @(#)CM_VerSion cs.c atm08 1.2 16245.eco sum= 33693 atm08.002 */
/* @(#)CM_VerSion cs.c atm07 1.2 16164.eco sum= 49053 atm07.012 */
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

#include "cs.h"
#include "recode.h"

#include "txops.h"

#include <stdio.h>
#include <string.h>
#include "dynarr.h"

/* Private dict data */
typedef struct {
	short lenIV;
	csDecrypt decrypt;          /* Charstring decryptor */
	dnaDCL(Charstring, subrs);
} Private;

/* Module context */
struct csCtx_ {
	dnaDCL(Private, private);   /* Private dictionaries */
	csConvProcs procs;          /* Charstring conversion procs */
	int first;                  /* Flags first character in font */
	Font *font;                 /* Current font */
	tcCtx g;                    /* Package context */
};

/* Initialize Private dict data */
static void initPrivate(void *ctx, long count, Private *private) {
	tcCtx g = ctx;
	long i;
	for (i = 0; i < count; i++) {
		dnaINIT(g->ctx.dnaCtx, private->subrs, 500, 200);
		private ++;
	}
	return;
}

/* Module initialization */
void csNew(tcCtx g) {
	csCtx h = MEM_NEW(g, sizeof(struct csCtx_));

	dnaINIT(g->ctx.dnaCtx, h->private, 1, 10);
	h->private.func = initPrivate;

	/* Link contexts */
	h->g = g;
	g->ctx.cs = h;
}

/* Free resources */
void csFree(tcCtx g) {
	csCtx h = g->ctx.cs;
	int i;
	for (i = 0; i < h->private.size; i++) {
		dnaFREE(h->private.array[i].subrs);
	}
	dnaFREE(h->private);
	MEM_FREE(g, g->ctx.cs);
}

/* Initialize for new font */
void csNewFont(tcCtx g, Font *font) {
	csCtx h = g->ctx.cs;
	h->private.cnt = 0;
	h->procs = recodeGetConvProcs(g);
	h->first = 1;
	h->font = font;
}

/* Initialize for new private dict in this font */
void csNewPrivate(tcCtx g, int fd, int lenIV, csDecrypt decrypt) {
	csCtx h = g->ctx.cs;
	Private *private = dnaINDEX(h->private, fd);
	private->lenIV = lenIV;
	private->decrypt = decrypt;
	private->subrs.cnt = 0;
}

/* Decrypt subr according to lenIV and add to buffer */
void csAddSubr(tcCtx g, unsigned length, char *cstr, int fd) {
	csCtx h = g->ctx.cs;
	Private *private = &h->private.array[fd];
	Charstring *subr = dnaNEXT(private->subrs);

#if TC_STATISTICS
	if (g->stats.gather) {
		g->stats.nSubrs++;
		g->stats.subrSize += length;
	}
#endif /* TC_STATISTICS */

	if (private->lenIV != -1) {
		/* Decrypt subr */
		int i;

		/* Check for multiple references to same subr data */
		for (i = 0; i < fd; i++) {
			long j;
			Private *p = &h->private.array[i];
			for (j = 0; j < p->subrs.cnt; j++) {
				if (cstr == p->subrs.array[j].cstr - p->lenIV) {
					/* Match found; just copy matching subr */
					*subr = p->subrs.array[j];
					return;
				}
			}
		}

		recodeDecrypt(length, (unsigned char *)cstr);
		cstr += private->lenIV;
		length -= private->lenIV;
	}

	/* Save subr reference */
	subr->length = length;
	subr->cstr = cstr;
}

/* Decrypt according to lenIV and parse */
void csAddChar(tcCtx g, unsigned length, char *cstr,
               unsigned id, int fd, int encrypted) {
	csCtx h = g->ctx.cs;
	Private *private = &h->private.array[fd];

#if TC_STATISTICS
	if (g->stats.gather) {
		g->stats.nChars++;
		g->stats.charSize += length;
	}
#endif /* TC_STATISTICS */

	if (private->lenIV != -1) {
		/* Decrypt charstring */
		if (encrypted) {
			private->decrypt(length, (unsigned char *)cstr);
		}
		cstr += private->lenIV;
		length -= private->lenIV;
	}

	if (h->first) {
		h->procs.newFont(g, h->font);
		h->first = 0;
	}

	/* Recode and save charstring */
	h->procs.addChar(g, length, cstr, id,
	                 private->subrs.cnt, private->subrs.array, fd);
}

/* Finish charstring processing for font */
void csEndFont(tcCtx g, unsigned nChars, unsigned short *reorder) {
	csCtx h = g->ctx.cs;
	unsigned i;
	Offset offset;
	char *dst;
	char *dststart;
	CSData *chars = &h->font->chars;
	unsigned long size = h->procs.endFont(g);
	int cid = h->font->flags & FONT_CID;

	/* Allocate to store charstring data */
	chars->nStrings = nChars;
	chars->offset = MEM_NEW(h->g, nChars * sizeof(Offset));
	chars->data = dst =
	        dststart = (g->flags & TC_SMALLMEMORY) ? NULL : MEM_NEW(g, size);

	offset = 0;
	for (i = 0; i < nChars; i++) {
		unsigned length;
		char *src;

		if (cid) {
			src = h->procs.getChar(g, i, h->font->fdIndex[i], &length);
		}
		else {
			src = h->procs.getChar(g, reorder[i], 0, &length);
		}

		if (dst != NULL) {
			/* Only copy charstring data if not TC_SMALLMEMORY */
			memcpy(dst, src, length);
			dst += length;
		}
		chars->offset[i] = offset += length;
	}
}

/* Compute chars size */
long csSizeChars(tcCtx g, Font *font) {
	CSData *chars = &font->chars;
	if (chars->nStrings == 0) {
		return 0;
	}
	else {
		return INDEX_SIZE(chars->nStrings, chars->offset[chars->nStrings - 1]);
	}
}

/* Write charstring data */
void csWriteData(tcCtx g, CSData *data, long dataSize, int offSize) {
	unsigned i;

	/* Write offset array */
	OUTOFF(offSize, 1);
	for (i = 0; i < data->nStrings; i++) {
		OUTOFF(offSize, data->offset[i] + 1);
	}

	/* Write charstring data */
	OUTN(dataSize, data->data);
}

/* Write charstring data from temp file */
static void csWriteDataSmall(tcCtx g, Font *font, CSData *data, int offSize) {
	csCtx h = g->ctx.cs;
	unsigned i;

	/* Write offset array */
	OUTOFF(offSize, 1);
	for (i = 0; i < data->nStrings; i++) {
		OUTOFF(offSize, data->offset[i] + 1);
	}

	/* Write charstring data */
	for (i = 0; i < data->nStrings; i++) {
		h->procs.writeChar(g, font, i);
	}

	/* Close temporary file */
	g->cb.tmpClose(g->cb.ctx);
}

/* Write chars */
void csWriteChars(tcCtx g, Font *font) {
	long dataSize;
	INDEXHdr header;
	CSData *chars = &font->chars;

	if (chars->nStrings == 0) {
		return;
	}

	/* Construct header */
	header.count = chars->nStrings;
	dataSize = chars->offset[header.count - 1];
	header.offSize = OFF_SIZE(dataSize + 1);

	/* Write header */
	OUT2(header.count);
	OUT1(header.offSize);

	if (g->flags & TC_SMALLMEMORY) {
		csWriteDataSmall(g, font, chars, header.offSize);
	}
	else {
		csWriteData(g, chars, dataSize, header.offSize);
	}
}

/* Free charstring data */
void csFreeData(tcCtx g, CSData *data) {
	if (data->nStrings != 0) {
		MEM_FREE(g, data->offset);
		MEM_FREE(g, data->data);
		data->nStrings = 0;
	}
}

/* Free font's charstring and subr data */
void csFreeFont(tcCtx g, Font *font) {
	if (font->flags & FONT_CID) {
		int i;
		for (i = 0; i < font->fdCount; i++) {
			csFreeData(g, &font->fdInfo[i].subrs);
		}
	}
	csFreeData(g, &font->subrs);
	csFreeData(g, &font->chars);
}

/* Set charstring conversion procs */
void csSetConvProcs(tcCtx g, csConvProcs *procs) {
	csCtx h = g->ctx.cs;
	h->procs = *procs;
}

/* Encode integer and return length */
int csEncInteger(long i, char *t) {
	if (-107 <= i && i <= 107) {
		/* Single byte number */
		t[0] = (char)(i + 139);
		return 1;
	}
	else if (108 <= i && i <= 1131) {
		/* +ve 2-byte number */
		i -= 108;
		t[0] = (unsigned char)((i >> 8) + 247);
		t[1] = (unsigned char)i;
		return 2;
	}
	else if (-1131 <= i && i <= -108) {
		/* -ve 2-byte number */
		i += 108;
		t[0] = (unsigned char)((-i >> 8) + 251);
		t[1] = (unsigned char)-i;
		return 2;
	}
	else if (-32768 <= i && i <= 32767) {
		/* +ve/-ve 3-byte number (shared with dict ops) */
		t[0] = (unsigned char)t2_shortint;
		t[1] = (unsigned char)(i >> 8);
		t[2] = (unsigned char)i;
		return 3;
	}
	else {
		/* +ve/-ve 5-byte number (dict ops only) */
		t[0] = (unsigned char)cff_longint;
		t[1] = (unsigned char)(i >> 24);
		t[2] = (unsigned char)(i >> 16);
		t[3] = (unsigned char)(i >> 8);
		t[4] = (unsigned char)i;
		return 5;
	}
}

/* Encode 16.16 fixed point number and return length */
int csEncFixed(Fixed f, char *t) {
	if ((f & 0x0000ffff) == 0) {
		return csEncInteger(f >> 16, t);  /* No fraction, use integer */
	}
	else {
		/* 5-byte fixed point */
		t[0] = (unsigned char)255;
		t[1] = (unsigned char)(f >> 24);
		t[2] = (unsigned char)(f >> 16);
		t[3] = (unsigned char)(f >> 8);
		t[4] = (unsigned char)f;
		return 5;
	}
}

#if TC_DEBUG

/* Dump charstring for debugging purposes. If called with -ve length function
   will dump until endchar and then return the length of the charstring */
unsigned csDump(long length, unsigned char *cstr, int nMasters, int t1) {
	static char *opname[32] = {
		/*  0 */ "reserved0",
		/*  1 */ "hstem",
		/*  2 */ "compose",
		/*  3 */ "vstem",
		/*  4 */ "vmoveto",
		/*  5 */ "rlineto",
		/*  6 */ "hlineto",
		/*  7 */ "vlineto",
		/*  8 */ "rrcurveto",
		/*  9 */ "closepath",
		/* 10 */ "callsubr",
		/* 11 */ "return",
		/* 12 */ "escape",
		/* 13 */ "hsbw",
		/* 14 */ "endchar",
		/* 15 */ "moveto",
		/* 16 */ "blend",
		/* 17 */ "curveto",
		/* 18 */ "hstemhm",
		/* 19 */ "hintmask",
		/* 20 */ "cntrmask",
		/* 21 */ "rmoveto",
		/* 22 */ "hmoveto",
		/* 23 */ "vstemhm",
		/* 24 */ "rcurveline",
		/* 25 */ "rlinecurve",
		/* 26 */ "vvcurveto",
		/* 27 */ "hhcurveto",
		/* 28 */ "shortint",
		/* 29 */ "callgsubr",
		/* 30 */ "vhcurveto",
		/* 31 */ "hvcurveto",
	};
	static char *escopname[] = {
		/*  0 */ "dotsection",
		/*  1 */ "vstem3",
		/*  2 */ "hstem3",
		/*  3 */ "and",
		/*  4 */ "or",
		/*  5 */ "not",
		/*  6 */ "seac",
		/*  7 */ "sbw",
		/*  8 */ "store",
		/*  9 */ "abs",
		/* 10 */ "add",
		/* 11 */ "sub",
		/* 12 */ "div",
		/* 13 */ "load",
		/* 14 */ "neg",
		/* 15 */ "eq",
		/* 16 */ "callother",
		/* 17 */ "pop",
		/* 18 */ "drop",
		/* 19 */ "setvw",
		/* 20 */ "put",
		/* 21 */ "get",
		/* 22 */ "ifelse",
		/* 23 */ "random",
		/* 24 */ "mul",
		/* 25 */ "div2",
		/* 26 */ "sqrt",
		/* 27 */ "dup",
		/* 28 */ "exch",
		/* 29 */ "index",
		/* 30 */ "roll",
		/* 31 */ "reservedESC31",
		/* 32 */ "reservedESC32",
		/* 33 */ "setcurrentpt",
		/* 34 */ "hflex",
		/* 35 */ "flex",
		/* 36 */ "hflex1",
		/* 37 */ "flex1",
		/* 38 */ "cntron",
	};
	int single = 0; /* Suppress optimizer warning */
	int stems = 0;
	int args = 0;
	long i = 0;

	while (i < length || length < 0) {
		int op = cstr[i];
		switch (op) {
			case tx_endchar:
				printf("%s ", opname[op]);
				if (length < 0) {
					return i + 1;
				}
				i++;
				break;

			case tx_reserved0:
			case t1_compose:
			case tx_vmoveto:
			case tx_rlineto:
			case tx_hlineto:
			case tx_vlineto:
			case tx_rrcurveto:
			case t1_closepath:
			case tx_callsubr:
			case tx_return:
			case t1_hsbw:
			case t1_moveto:
			case t1_curveto:
			case tx_rmoveto:
			case tx_hmoveto:
			case t2_rcurveline:
			case t2_rlinecurve:
			case t2_vvcurveto:
			case t2_hhcurveto:
			case t2_callgsubr:
			case tx_vhcurveto:
			case tx_hvcurveto:
				printf("%s ", opname[op]);
				args = 0;
				i++;
				break;

			case tx_hstem:
			case tx_vstem:
			case t2_hstemhm:
			case t2_vstemhm:
				printf("%s ", opname[op]);
				stems += args / 2;
				args = 0;
				i++;
				break;

			case t2_hintmask:
			case t2_cntrmask: {
				int bytes;
				if (args > 0) {
					stems += args / 2;
				}
				bytes = (stems + 7) / 8;
				printf("%s[", opname[op]);
				i++;
				while (bytes--) {
					printf("%02x", cstr[i++]);
				}
				printf("] ");
				args = 0;
				break;
			}

			case tx_escape: {
				/* Process escaped operator */
				unsigned int escop = cstr[i + 1];
				if (escop > TABLE_LEN(escopname) - 1) {
					printf("? ");
				}
				else {
					printf("%s ", escopname[escop]);
				}
				args = 0;
				i += 2;
				break;
			}

			case t2_blend:
				printf("%s ", opname[op]);
				args -= single * (nMasters - 1) + 1;
				i++;
				break;

			case t2_shortint:
				printf("%d ", cstr[i + 1] << 8 | cstr[i + 2]);
				args++;
				i += 3;
				break;

			case 247:
			case 248:
			case 249:
			case 250:
				/* +ve 2 byte number */
				printf("%d ", 108 + 256 * (cstr[i] - 247) + cstr[i + 1]);
				args++;
				i += 2;
				break;

			case 251:
			case 252:
			case 253:
			case 254:
				/* -ve 2 byte number */
				printf("%d ", -108 - 256 * (cstr[i] - 251) - cstr[i + 1]);
				i += 2;
				args++;
				break;

			case 255: {
				/* 5 byte number */
				long value = (long)cstr[i + 1] << 24 | (long)cstr[i + 2] << 16 |
				    (long)cstr[i + 3] << 8 | (long)cstr[i + 4];
				if (t1) {
					printf("%ld ", value);
				}
				else {
					printf("%.4g ", value / 65536.0);
				}
				args++;
				i += 5;
				break;
			}

			default:
				/* 1 byte number */
				single = cstr[i] - 139;
				printf("%d ", single);
				args++;
				i++;
				break;
		}
	}
	printf("\n");
	return length;
}

void csDumpSubrs(tcCtx g, Font *font) {
	long i;
	long bias;
	long offset;

	printf("--- subrs (count=%hu)\n", font->subrs.nStrings);
	if (font->subrs.nStrings == 0) {
		printf("NONE\n");
		return;
	}

	if (font->subrs.nStrings < 1240) {
		bias = 107;
	}
	else if (font->subrs.nStrings < 33900) {
		bias = 1131;
	}
	else {
		bias = 32768;
	}
	printf("bias=%ld\n", bias);

	offset = 0;
	for (i = 0; i < font->subrs.nStrings; i++) {
		long nextoff = font->subrs.offset[i];
		printf("[%ld]= ", i);
		csDump(nextoff - offset, (unsigned char *)&font->subrs.data[offset],
		       g->nMasters, 0);
		offset = nextoff;
	}
}

#endif /* TC_DEBUG */