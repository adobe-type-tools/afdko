/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Compact Font Embedding library.
 */

#include "dynarr.h"
#include "cfembed.h"
#include "t1read.h"
#include "cffread.h"
#include "ttread.h"
#include "cffwrite.h"
#include "sfntread.h"
#include "svgwrite.h"

#if PLAT_SUN4
#include "sun4/fixstring.h"
#else /* PLAT_SUN4 */
#include <string.h>
#endif  /* PLAT_SUN4 */

#include <setjmp.h>
#include <stdlib.h>
#include <stdio.h>

#define ARRAY_LEN(a)	(sizeof(a)/sizeof((a)[0]))
#define ABS(v)			(((v)<0)?-(v):(v))

typedef unsigned short GID;		/* Glyph Index */

enum							/* Source font types */
	{
	SRC_TYPE1,					/* PFA, PFB, POST, GX Typ1, naked CID, GX CID*/
	SRC_CFF,					/* OpenType CFF, naked CFF */
	SRC_TRUETYPE				/* TrueType sfnt */
	};

/* Predefined tags */
#define CFF__tag	CTL_TAG('C','F','F',' ')	/* OTF CFF table */
#define CID__tag	CTL_TAG('C','I','D',' ')	/* sfnt-wrapped CID table */
#define DFLT_tag	CTL_TAG('D','F','L','T')	/* Default GPOS script */
#define GPOS_tag	CTL_TAG('G','P','O','S')	/* OTF GPOS table */
#define POST_tag	CTL_TAG('P','O','S','T')	/* Macintosh POST resource */
#define sfnt_tag	CTL_TAG('s','f','n','t')	/* Macintosh sfnt resource */
#define TYP1_tag	CTL_TAG('T','Y','P','1')	/* GX Type 1 sfnt table */
#define cmap_tag	CTL_TAG('c','m','a','p')	/* OTF cmap table */
#define kern_tag	CTL_TAG('k','e','r','n')	/* OTF kern feature */

/* File signatures */
#define sig_PostScript 	CTL_TAG('%',  '!',0x00,0x00)
#define sig_PFB   	   	CTL_TAG(0x80,0x01,0x00,0x00)
#define sig_CFF		   	CTL_TAG(0x01,0x00,0x00,0x00)
#define sig_MacResource	CTL_TAG(0x00,0x00,0x01,0x00)

/* ------------------------------- cmap Table ------------------------------ */

typedef struct	cmapSeg4_				/* Format 4 segment */
	{
	unsigned short endCode;
	unsigned short startCode;
	short idDelta;
	unsigned short idRangeOffset;
	} cmapSeg4;
#define CMAP_SEG4_SIZE	(4*2)

typedef struct	cmapFmt4_				/* Format 4 cmap subtable */
	{
	unsigned short format;
	unsigned short length;
	unsigned short languageId;	/* Documented as revision */
	unsigned short segCountX2;
	unsigned short searchRange;
	unsigned short entrySelector;
	unsigned short rangeShift;
/*	unsigned short *endCode;	// written here, held in segment */
	unsigned short password;
/*	unsigned short *startCode;	// written here, held in segment */
/*	short *idDelta;				// written here, held in segment */
/*	unsigned short *idRangeOffset;	// written here, held in segment */
	dnaDCL(cmapSeg4, segment);
	dnaDCL(GID, glyphId);
	} cmapFmt4;
#define CMAP_FMT4_SIZE(segs,glyphs) (8*2 + (segs)*CMAP_SEG4_SIZE + (glyphs)*2)

typedef struct cmapGroup12_
	{
	unsigned long startcode;
	unsigned long endcode;
	unsigned long startGID;
	} cmapGroup12;
#define CMAP_GROUP12_SIZE (3*4)
	
typedef struct cmapFmt12_
	{
	unsigned short format; /* 12 */
	unsigned short reserved; /* 0 */
	unsigned long length; /* len of subtable, including the header */
	unsigned long languageId;
	/* unsigned long nGroups;	*/
	dnaDCL(cmapGroup12, group);
	} cmapFmt12;
#define CMAP_FMT12_SIZE(groups) (2*2 + 3*4 + (groups)*CMAP_GROUP12_SIZE)
	

typedef struct cmapEncoding_
	{
	unsigned short platformId;
	unsigned short encodingID; 
	unsigned long offset;
	cmapFmt4 subtable4;
	cmapFmt12 subtable12;
	} cmapEncoding;
#define CMAP_ENC_SIZE (2*2 + 4)

typedef struct
	{
	unsigned short version;
	unsigned short nEncodings;	/* =1 if all unicode values are in BMP, else =2 */
	unsigned long _flags;
#define FILLEDFMT4 0x1
#define FILLEDFMT12 0x2
	cmapEncoding encoding[2];
	} cmapTbl;
#define CMAP_HDR_SIZE (2*2)

/* Code to glyph mapping */
typedef struct
	{
	unsigned long code; /* was: short */
	GID glyphId;
	unsigned short span;		/* # of mappings in this range */
	unsigned short ordered;
	unsigned short flags;
#define CMAP_CODE_BREAK	 (1<<0)	/* Broke range because of code discontinuity */
#define CMAP_GLYPH_BREAK (1<<1) /* broke because of glyphid discontinuity */
	} Mapping;

/* ----------------------------- Coverage Table ---------------------------- */

typedef struct							/* Coverage format 1 */
	{									
	unsigned short CoverageFormat;		/* =1 */
	unsigned short GlyphCount;			
	GID *GlyphArray;					/* [GlyphCount] */
	} CoverageFormat1;
#define COVERAGE1_SIZE(nGlyphs)		(2*2+(nGlyphs)*2)

typedef struct
	{
	GID Start;
	GID End;
	unsigned short StartCoverageIndex;
	} RangeRecord;
#define RANGE_RECORD_SIZE			(3*2)

typedef struct							/* Coverage format 2 */
	{
	unsigned short CoverageFormat;		/* =2 */
	unsigned short RangeCount;
	dnaDCL(RangeRecord, RangeRecord);	/* [RangeCount] */
	} CoverageFormat2;
#define COVERAGE2_SIZE(nRanges)		(2*2+(nRanges)*RANGE_RECORD_SIZE)

/* ------------------------------- GPOS Table ------------------------------ */

/* Pair Adjustment Subtable Definition */

typedef unsigned short Offset;
typedef short ValueRecord;
typedef struct
	{
	GID SecondGlyph;
	ValueRecord Value1;
/*	ValueRecord Value2;					// not used in this implementation */
	} PairValueRecord;
#define PAIR_VALUE_REC_SIZE 		(2*2)

typedef struct
	{
	Offset offset;						/* Offset to this PairSet */
	unsigned short PairValueCount;
	} PairSet;
#define PAIR_SET_SIZE(nRecs, nValues) (2+(nRecs)*PAIR_VALUE_REC_SIZE)

typedef struct
	{
	unsigned short PosFormat;			/* =1 */
	Offset Coverage;
	unsigned short ValueFormat1;
	unsigned short ValueFormat2;
	unsigned short PairSetCount;
	dnaDCL(PairSet, PairSet);			/* [PairSetCount] */
	} PairPosFormat1;
#define PAIR_POS1_SIZE(nPairSets)	(5*2+(nPairSets)*2)

/* ---------------------------- Library context ---------------------------- */

typedef struct					/* Client id to CFF gid remapping */
	{
	char *gname;				/* subset.names[id] (Type 1 only) */
	unsigned short id;			/* Client nid/cid/gid */
	unsigned long uv;			/* Unicode value */
	GID gid;					/* Final CFF glyph index */
	} GlyphMap;

/* Format-specific font data refill callback */
typedef size_t (*FmtSpecRefill)(cefCtx h, char **ptr);

struct cefCtx_
	{
	long flags;					/* Status flags */
#define DO_NEW_TABLES		(1<<0)	/* Call new table functions */
#define CLOSE_DST_STREAM 	(1<<1)	/* Close destination stream */
	long svwFlags;				/* svgwrite library flags */
	cefEmbedSpec *spec;			/* Client's embedding spec */
	struct						/* Streams */
		{
		void *src;
		void *dst;
		ctlStreamCallbacks cb;
		} stm;
	struct						/* Source stream */
		{					
		int type;				/* Font type */
		long origin;			/* Offset to start of font data in stream */
		long offset;			/* Buffer offset */
		size_t length;			/* Buffer length */
		char *buf;				/* Buffer beginning */
		char *end;				/* Buffer end */
		char *next;				/* Next byte available (buf <= next < end) */
		} src;
	struct						/* Font data segment */
		{
		FmtSpecRefill refill;	/* Format-specific callback */
		size_t left;			/* Bytes remaining in segment */
		} seg;
	struct 						/* cmap table data */
		{
		dnaDCL(Mapping, mapping);/* Code/glyph mapping accumulator */
		cmapTbl tbl;			/* cmap table */
		} cmap;
	struct						/* GPOS kern feature data */
		{
		dnaDCL(cefKernPair, pairs);/* Kern pair accumulator */
		dnaDCL(PairValueRecord, pvrecs);/* PairValueRecord accumulator */
		PairPosFormat1 subtbl;	/* PairPos format subtable */
		} GPOS;
	struct						/* Coverage table (used for GPOS) */
		{
		dnaDCL(GID, gids);		/* Covered glyphs */
		short format;			/* Format selector */
		CoverageFormat1 fmt1;	/* Format 1 data */
		CoverageFormat2 fmt2;	/* Format 2 data */
		} coverage;
	dnaDCL(GlyphMap, subset);	/* Working subset specification */
	struct						/* Client callbacks */
		{
		ctlMemoryCallbacks mem;
		ctlStreamCallbacks stm;
		abfGlyphCallbacks glyph;
		cefMapCallback *map;
		} cb;
	abfTopDict *top;
	struct						/* Service library contexts */
		{
		dnaCtx dna;
		t1rCtx t1r;
		cfrCtx cfr;
		ttrCtx ttr;
		cfwCtx cfw;
		sfrCtx sfr;
		sfwCtx sfw;
		} ctx;
	struct						/* Error handling */
		{
		jmp_buf env;
		int code;
		} err;
	};

/* ----------------------------- Error Handling ---------------------------- */

/* Handle fatal error */
static void fatal(cefCtx h, int err_code)
	{
	h->err.code = err_code;
	longjmp(h->err.env, 1);
	}

/* ----------------------------- Source Stream ----------------------------- */

/* Fill source buffer. */
static void srcFillBuf(cefCtx h, long offset)
	{
	h->src.length = h->cb.stm.read(&h->cb.stm, h->stm.src, &h->src.buf);
	if (h->src.length == 0)
		fatal(h, cefErrSrcStream);
	h->src.offset = offset;
	h->src.next = h->src.buf;
	h->src.end = h->src.buf + h->src.length;
	}

/* Open source and destination streams. */
static void stmsOpen(cefCtx h)
	{
	h->stm.src = h->cb.stm.open(&h->cb.stm, CEF_SRC_STREAM_ID, 0);
	if (h->stm.src == NULL)
		fatal(h, cefErrSrcStream);
	srcFillBuf(h, 0);

	h->stm.dst = h->cb.stm.open(&h->cb.stm, CEF_DST_STREAM_ID, 0);
	if (h->stm.dst == NULL)
		fatal(h, cefErrDstStream);
	}

/* Close source and destination streams. */
static void stmsClose(cefCtx h)
	{
	if (h->cb.stm.close(&h->cb.stm, h->stm.src))
		fatal(h, cefErrSrcStream);
	if (h->cb.stm.close(&h->cb.stm, h->stm.dst))
		fatal(h, cefErrDstStream);
	}

/* Read next sequential source buffer; update offset and return first byte. */
static char srcNextBuf(cefCtx h)
	{
	/* 64-bit warning fixed by cast here */
	srcFillBuf(h, (long)(h->src.offset + h->src.length));
	return *h->src.next++;
	}

/* Seek to specified offset. */
static void srcSeek(cefCtx h, long offset)
	{
	long delta = offset - h->src.offset;
	if (delta >= 0 && (size_t)delta < h->src.length)
		/* Offset within current buffer; reposition next byte */
		h->src.next = h->src.buf + delta;
	else 
		{
		/* Offset outside current buffer; seek to offset and fill buffer */
		if (h->cb.stm.seek(&h->cb.stm, h->stm.src, offset))
			fatal(h, cefErrSrcStream);
		srcFillBuf(h, offset);
		}
	}

/* Read 1-byte unsigned number from source stream. */
#define srcRead1(h) \
	((unsigned char)((h->src.next==h->src.end)?srcNextBuf(h):*h->src.next++))

/* Read 2-byte number from source stream. */
static unsigned short srcRead2(cefCtx h)
	{
	unsigned short value = (unsigned short)srcRead1(h)<<8;
	return value | (unsigned short)srcRead1(h);
	}

/* Read 4-byte number from source stream. */
static unsigned long srcRead4(cefCtx h)
	{
	unsigned long value = (unsigned long)srcRead1(h)<<24;
	value |= (unsigned long)srcRead1(h)<<16;
	value |= (unsigned long)srcRead1(h)<<8;
	return value | (unsigned long)srcRead1(h);
	}

/* --------------------------- Destination Stream -------------------------- */

/* Write 2-byte number. */
static void dstWrite2(cefCtx h, unsigned short value)
	{
	unsigned char buf[2];
	buf[0] = (unsigned char)(value>>8);
	buf[1] = (unsigned char)value;
	if (h->cb.stm.write(&h->cb.stm, h->stm.dst, sizeof(buf), (char *)buf) !=
		sizeof(buf))
		fatal(h, cefErrDstStream);
	}

/* Write 4-byte number. */
static void dstWrite4(cefCtx h, unsigned long value)
	{
	unsigned char buf[4];
	buf[0] = (unsigned char)(value>>24);
	buf[1] = (unsigned char)(value>>16);
	buf[2] = (unsigned char)(value>>8);
	buf[3] = (unsigned char)value;
	if (h->cb.stm.write(&h->cb.stm, h->stm.dst, sizeof(buf), (char *)buf) !=
		sizeof(buf))
		fatal(h, cefErrDstStream);
	}

/* ---------------------------- Stream Callbacks --------------------------- */

/* Open stream. */
static void *stm_open(ctlStreamCallbacks *cb, int id, size_t size)
    {
    cefCtx h = cb->indirect_ctx;
    switch (id)
        {
	case T1R_SRC_STREAM_ID:
	case CFR_SRC_STREAM_ID:
	case TTR_SRC_STREAM_ID:
		/* Hand back already opened source stream */
		return h->stm.src;
	case CFW_DST_STREAM_ID:
	case SFW_DST_STREAM_ID:
		/* Hand back already opened destination stream */
		return h->stm.dst;
	case T1R_TMP_STREAM_ID:
		/* Convert id and pass to client */
		return h->cb.stm.open(&h->cb.stm, CEF_TMP0_STREAM_ID, size);
	case CFW_TMP_STREAM_ID:
		/* Convert id and pass to client */
		return h->cb.stm.open(&h->cb.stm, CEF_TMP1_STREAM_ID, size);
	default:
		/* Ignore debug calls */
		break;
		}
    return NULL;
    }

/* Read from stream. */
static size_t stm_read(ctlStreamCallbacks *cb, void *stream, char **ptr)
    {
	cefCtx h = cb->indirect_ctx;
	if (stream == h->stm.src && h->seg.refill != NULL)
		/* Segmented */
		return h->seg.refill(h, ptr);
	else
		return h->cb.stm.read(&h->cb.stm, stream, ptr);
	}

/* Close stream. */
static int stm_close(ctlStreamCallbacks *cb, void *stream)
    {
    cefCtx h = cb->indirect_ctx;
	if (stream == h->stm.src || stream == h->stm.dst)
		return 0;	/* These streams are closed later */
	return h->cb.stm.close(&h->cb.stm, stream);
    }

/* Initialize stream callbacks. */
static void stmInit(cefCtx h)
	{
	h->stm.cb = h->cb.stm;		/* Copy client stream callbacks */

	/* Patch client callbacks */
	h->stm.cb.indirect_ctx 	= h;	/* Provide cfembed context */
	h->stm.cb.open 			= stm_open;	
	h->stm.cb.close 		= stm_close;
	}

/* ---------------- Platform-Specific Font Wrapper Callbacks --------------- */

/* Return file offset of first POST/sfnt resource data. */
static long POST_sfnt_Seek(cefCtx h, unsigned long *rsrctypefound)
	{
	/* Macintosh resource structures */
	struct
		{
		unsigned long dataOffset;
		unsigned long mapOffset;
		unsigned long dataLength;
		unsigned long mapLength;
		} header;
	enum {HEADER_SIZE = 256};				/* Resource header size */
	struct
		{
		char reserved1[16];
		unsigned long reserved2;
		unsigned short reserved3;
		unsigned short attrs;
		unsigned short typeListOffset;
		unsigned short nameListOffset;
		} map;
	enum {MAP_SKIP_SIZE = 16 + 4 + 2*2};	/* Skip to typeListOffset */
	struct
		{
		unsigned long type;
		unsigned short cnt;
		unsigned short refListOffset;
		} type;
	long typeListCnt;
#if 0
	/* Included for reference only */
	struct
		{
		unsigned short id;
		unsigned short nameOffset;
		char attrs;
		char dataOffset[3];
		unsigned long reserved;
		} refList;
#endif
	enum {REFLIST_SKIP_SIZE = 2*2 + 1};		/* Skip to dataOffset */
	long dataOffset;
	long i;

	/* Read header (4-byte header.dataOffset already read) */
	header.mapOffset = srcRead4(h);

	/* Read map */
	srcSeek(h, header.mapOffset + MAP_SKIP_SIZE);
	map.typeListOffset = srcRead2(h);
	map.nameListOffset = srcRead2(h);

	/* Find POST resource or sfnt resource in type list */
	typeListCnt = srcRead2(h);
	for (i = 0; i <= typeListCnt; i++)
		{
		type.type = srcRead4(h);
		type.cnt = srcRead2(h);
		type.refListOffset = srcRead2(h);
		switch (type.type)
			{
		case sfnt_tag:
		case POST_tag:
			*rsrctypefound = type.type;
			/* Read first resource data offset from reference list */
			srcSeek(h, (header.mapOffset + 
						map.typeListOffset + 
						type.refListOffset + 
						REFLIST_SKIP_SIZE));
			dataOffset = (unsigned long)srcRead1(h)<<16;
			dataOffset |= srcRead1(h)<<8;
			dataOffset |= srcRead1(h);

			/* Return offset */
			return dataOffset + HEADER_SIZE;
			break;
			}
		}

	fatal(h, cefErrNoPOST);

	return 0;	/* Suppress compiler warning */
	}

/* Prepare next segment of font data. */
static size_t nextseg(cefCtx h, char **ptr)
	{
	size_t srcleft = h->src.end - h->src.next;

	if (srcleft == 0)
		{
		/* Refill empty source buffer */
		/* 64-bit warning fixed by cast here */
		srcFillBuf(h, (long)(h->src.offset + h->src.length));
		srcleft = h->src.length;
		}

	*ptr = h->src.next;
	if (srcleft <= h->seg.left)
		{
		/* Return full buffer */
		h->seg.left -= srcleft;
		h->src.next += srcleft;
		}
	else
		{
		/* Return partial buffer */
		srcleft = h->seg.left;
		h->src.next += h->seg.left;
		h->seg.left = 0;
		}

	return srcleft;
	}

/* Refill input buffer from PFB stream. */
static size_t PFBRefill(cefCtx h, char **ptr)
	{
	while (h->seg.left == 0)
		{
		/* New segment; read segment header */
		int escape = srcRead1(h);
		int type = srcRead1(h);
		
		/* Check segment header */
		if (escape != 128 || (type != 1 && type != 2 && type != 3))
			fatal(h, cefErrBadPFB);

		if (type == 3)
			{
			/* EOF */
			*ptr = NULL;
			return 0;
			}
		else
			{
			/* Read segment length (little endian) */
			h->seg.left = srcRead1(h);
			h->seg.left |= srcRead1(h)<<8;
			h->seg.left |= (long)srcRead1(h)<<16;
			h->seg.left |= (long)srcRead1(h)<<24;
			}
		}

	return nextseg(h, ptr);
	}

/* Refill input buffer from POST resource stream. */
static size_t POSTRefill(cefCtx h, char **ptr)
	{
	while (h->seg.left == 0)
		{
		/* New POST resource */
		int type;

		/* Read length and type */
		h->seg.left = srcRead4(h) - 2;

		type = srcRead1(h);
		(void)srcRead1(h);	/* Discard padding byte (not documented) */

		/* Process resource data */
		switch (type)
			{
		case 0:	
			/* Comment; skip data */
			/* 64-bit warning fixed by cast here */
			srcSeek(h, (long)(h->src.offset + h->seg.left));
			h->seg.left = 0;
			break;
		case 1:	/* ASCII */
		case 2:	/* Binary */
			break;
		case 3: /* End-of-file */
		case 5:	/* End-of-data */
			*ptr = NULL;
			return 0;
		case 4:		/* Data in data fork; unsupported */
		default:	/* Unknown POST type */
			fatal(h, cefErrBadPOST);
			}
		}

	return nextseg(h, ptr);
	}

/* Read 4-byte signature and try to match against sfnt. */
static int readsfnt(cefCtx h, long origin)
	{
	enum 
		{
		CID__HDR_SIZE = 4*4 + 3*2,	/* 'CID ' table header size */
		TYP1_HDR_SIZE = 5*4 + 2*2	/* 'TYP1' table header size */
		};
	ctlTag Asfnt_tag;
	sfrTable *table;
	int type = 0;	/* Suppress optimizer warning */

	if (h->ctx.sfr == NULL)
		{
		/* Initialize library */
		h->ctx.sfr = 
			sfrNew(&h->cb.mem, &h->cb.stm, SFR_CHECK_ARGS);
		if (h->ctx.sfr == NULL)
			fatal(h, cefErrSfntreadInit);
		}

	switch (sfrBegFont(h->ctx.sfr, h->stm.src, origin /* was: 0 */, &Asfnt_tag))
		{
	case sfrSuccess:
		switch (Asfnt_tag)
			{
		case sfr_v1_0_tag:
		case sfr_true_tag:
			/* TrueType */
			h->src.origin = origin; /* was: 0 */
			type = SRC_TRUETYPE;
			break;
		case sfr_OTTO_tag:
			/* OTF */
			h->src.origin = origin;  /* was: 0 */
 			type = SRC_CFF;
			break;
		case sfr_typ1_tag:
			/* GX or sfnt-wrapped CID font */
			table = sfrGetTableByTag(h->ctx.sfr, CID__tag);
			if (table != NULL)
				{
				h->src.origin = table->offset + CID__HDR_SIZE;
				type = SRC_TYPE1;
				break;
				}
			else
				{
				table = sfrGetTableByTag(h->ctx.sfr, TYP1_tag);
				if (table != NULL)
					{
					h->src.origin = table->offset + TYP1_HDR_SIZE;
					type = SRC_TYPE1;
					break;
					}
				}
			fatal(h, cefErrBadSfnt);
		case sfr_ttcf_tag:
			fatal(h, cefErrTTCNoSupport);
			}
		break;
	case sfrErrBadSfnt:
		fatal(h, cefErrUnrecFont);
	default:
		fatal(h, cefErrSfntParse);
		}

	if (sfrEndFont(h->ctx.sfr))
		fatal(h, cefErrSfntParse);

	return type;
	}

/* Determine input font type and setup callbacks (assumes start of file). */
static int getFontType(cefCtx h)
	{
	ctlTag sig;
	int type = 0;	/* Suppress optimizer warning */

	/* Initialize segment */
	h->seg.refill = NULL;

	/* Determine font data type */
	sig = (ctlTag)srcRead1(h)<<24;
	sig |= (ctlTag)srcRead1(h)<<16;
	switch (sig)
		{
	case sig_PostScript:
		type = SRC_TYPE1;
		h->src.origin = 0;
		break;
	case sig_PFB:
		type = SRC_TYPE1;
		h->seg.refill = PFBRefill;
		h->src.origin = 0;
		break;
	case sig_CFF:
		type = SRC_CFF;
		h->src.origin = 0;
		break;
	default:
		/* Make 4-byte signature */
		sig |= srcRead1(h)<<8;
		sig |= srcRead1(h);
		switch (sig)
			{
		case sig_MacResource:
			{
			ctlTag whichkind = 0;
			long off = 0;
			
			off = POST_sfnt_Seek(h, &whichkind);
			switch (whichkind)
				{
				case POST_tag:
					type = SRC_TYPE1;
					h->seg.refill = POSTRefill;
					h->src.origin = off;
					break;
				case sfnt_tag:
					type = readsfnt(h, off + 4);
					break;
				}
			}
			break;
		default:
			type = readsfnt(h, 0);
			}
		}

	if (h->seg.refill != NULL)
		{	
		/* Prep source filter */
		h->seg.left = 0;
		h->src.next = h->src.end;
		}
	
	return type;
	}

/* ------------------------ Miscellaneous Functions ------------------------ */

/* Calculate values of binary search table parameters */
static void setSearchParams(unsigned unitSize, long nUnits, 
							unsigned short *searchRange, 
							unsigned short *entrySelector, 
							unsigned short *rangeShift)
	{
	long nextPwr;
	int pwr2;
	int log2;

	nextPwr = 2;
	for (log2 = 0; nextPwr <= nUnits; log2++)
		nextPwr *= 2;
	pwr2 = nextPwr / 2;

	*searchRange = unitSize * pwr2;
	*entrySelector = log2;
	*rangeShift = (unsigned short)(unitSize * (nUnits - pwr2));
	}

/* Match id. */
static int CTL_CDECL matchId(const void *key, const void *value)
	{
	unsigned short a = *(unsigned short *)key;
	unsigned short b = ((GlyphMap *)value)->id;
	if (a < b)
		return -1;
	else if (a > b)
		return 1;
	else
		return 0;
	}

/* Map client id to final gid. */
static GID mapId2GID(cefCtx h, unsigned short id)
	{
	if (id >= CEF_VID_BEGIN)
		return id;	/* Return VID as VGID */
	else
		{
		GlyphMap *map = (GlyphMap *)bsearch(&id, h->subset.array, 
											h->subset.cnt, 
											sizeof(GlyphMap), matchId);
		if (map == NULL)
			fatal(h, cefErrBadSpec);
		return map->gid;
		}
	}

/* ------------------------------- cmap Table ------------------------------ */

/* Safety initialization. */
static void cmapInit(cefCtx h)
	{
	cmapFmt4 *fmt4 = &h->cmap.tbl.encoding[0].subtable4;
	cmapFmt12 *fmt12 = &h->cmap.tbl.encoding[1].subtable12;
	fmt4->segment.size = 0;
	fmt4->glyphId.size = 0;
	fmt12->group.size = 0;
	
	h->cmap.mapping.size = 0;
	}

/* Initialize cmap-specific data. */
static int cmapNew(sfwTableCallbacks *cb)
	{
	cefCtx h = cb->ctx;
	cmapFmt4 *fmt4 = &h->cmap.tbl.encoding[0].subtable4;
	cmapFmt12 *fmt12 = &h->cmap.tbl.encoding[1].subtable12;
	dnaINIT(h->ctx.dna, fmt4->segment, 64, 64);
	dnaINIT(h->ctx.dna, fmt4->glyphId, 128, 128);
	dnaINIT(h->ctx.dna, fmt12->group, 64, 64);
	
	dnaINIT(h->ctx.dna, h->cmap.mapping, 256, 256);
	return 0;
	}

/* Free cmap-specific data. */
static int cmapFree(sfwTableCallbacks *cb)
	{
	cefCtx h = cb->ctx;
	cmapFmt4 *fmt4 = &h->cmap.tbl.encoding[0].subtable4;
	cmapFmt12 *fmt12 = &h->cmap.tbl.encoding[1].subtable12;
	dnaFREE(fmt4->segment);
	dnaFREE(fmt4->glyphId);
	dnaFREE(fmt12->group);
	dnaFREE(h->cmap.mapping);
	return 0;
	}
	
/* Partition mappings optimally for format 4 and 12. Assumes mapping in code order. */
static int partitionRanges4_12(long nMaps, Mapping *mapping, int isforformat4)
	{
	int span;
	int ordered = 0;
	int total = 0;
	int i;
	int nSegments = 0;
	int beyondbmp_4 = 0;

	/* Create ordered partitioning */
	span = 1;
	for (i = 1; i < nMaps; i++)
		{
		int codeBreak = (mapping[i - 1].code + 1 != mapping[i].code);
		int glyphBreak = (mapping[i - 1].glyphId + 1 != mapping[i].glyphId);

		if (((mapping[i - 1].code > 0xFFFF) || (mapping[i].code > 0xFFFF)) && 
				isforformat4)
			{	/* not handled in format4 */
			beyondbmp_4 = 1;
			mapping[i - span].span = span;
			mapping[i].flags |= CMAP_CODE_BREAK;
			span = 1;			
			break; 
			}
		
		if (codeBreak || glyphBreak)
			{
			mapping[i - span].span = span;
			if (codeBreak)
				mapping[i].flags |= CMAP_CODE_BREAK;
			if (glyphBreak)
				mapping[i].flags |= CMAP_GLYPH_BREAK;
			span = 1;
			}
		else
			span++;
		}
	mapping[i - span].span = span;

	/* Coalesce ranges */
	nSegments = 0;
	total = ordered = span = mapping[0].span;
	for (i = span; (i < nMaps) && (span > 0); i += span)
		{		
		span = mapping[i].span;
		if (!(mapping[i].flags & CMAP_CODE_BREAK) && 
			!(mapping[i].flags & CMAP_GLYPH_BREAK) && 
			(ordered + span < 4))
			{
			total += span;
			ordered = 0;
			}
		else
			{
			mapping[i - total].span = total;
			mapping[i - total].ordered = ordered;
			total = ordered = span;
			nSegments++;
			}
		}
	mapping[i - total].span = total;
	mapping[i - total].ordered = ordered;
	nSegments += 1; /* Add last segment */

	return nSegments;
	}

/* Fill format 4 subtable. */
static void cmapFillFmt4(cefCtx h)
	{
	int iStart;
	cmapSeg4 *segment;
	Mapping *mapping = h->cmap.mapping.array;
	long nMaps = h->cmap.mapping.cnt;
	cmapFmt4 *fmt = &h->cmap.tbl.encoding[0].subtable4;
	int nSegments = partitionRanges4_12(nMaps, mapping, 0x1);

	nSegments += 1; /* add sentinel */

	/* Add segments */
	fmt->segment.cnt = 0;
	iStart = 0; 
	while (iStart < nMaps) 
		{
		int j;
		Mapping *startmap = &mapping[iStart];
		int iEnd = iStart + startmap->span - 1;
		long iSegment = fmt->segment.cnt;
		
		if (mapping[iEnd].code > 0xFFFF)
			{
			nSegments -= (nMaps - iStart); /* cannot include beyond this segment */
			break;
			}
			
		if (mapping[iStart].code > 0xFFFF)
			break;	/* cannot use this segment in format4 */
			
		segment = dnaNEXT(fmt->segment);
		segment->endCode = (unsigned short)(mapping[iEnd].code & 0xFFFF);
		segment->startCode = (unsigned short)(startmap->code & 0xFFFF);
		if (startmap->ordered)
			{
			segment->idDelta = (unsigned short)(startmap->glyphId - startmap->code);
			segment->idRangeOffset = 0;
			}
		else
			{
			segment->idDelta = 0;
			segment->idRangeOffset = (unsigned short) ((nSegments - iSegment)*2 + fmt->glyphId.cnt*2);
			for (j = iStart; j <= iEnd; j++)
				*dnaNEXT(fmt->glyphId) = mapping[j].glyphId;
			}

		iStart = iEnd + 1;
		}

	if ((0 == nSegments) || (0 == fmt->segment.cnt))
		return;

	/* Add sentinel segment */
	segment = dnaNEXT(fmt->segment);
	segment->endCode = 0xffff;
	segment->startCode = 0xffff;
	segment->idDelta = 1;
	segment->idRangeOffset = 0;

	nSegments = fmt->segment.cnt;

	fmt->segCountX2 = nSegments*2;
	fmt->password = 0;
	setSearchParams(2, nSegments, 
					&fmt->searchRange,
					&fmt->entrySelector, 
					&fmt->rangeShift);

	fmt->format = 4;
	fmt->languageId = 0;		
	fmt->length = (unsigned short)CMAP_FMT4_SIZE(nSegments, fmt->glyphId.cnt);
	h->cmap.tbl._flags |= FILLEDFMT4;
	}
		
	
/* Fill format 12 subtable. */
static void cmapFillFmt12(cefCtx h)
	{
	Mapping *mapping = h->cmap.mapping.array;
	long nMaps = h->cmap.mapping.cnt;
	cmapFmt12 *fmt = &h->cmap.tbl.encoding[1].subtable12;
	int iStart;
	
	partitionRanges4_12(nMaps, mapping, 0x0);
	
	/* add groups */
	fmt->group.cnt = 0;
	iStart = 0;
	while (iStart < nMaps /* yes: nMaps, not nGroups */)
		{
		cmapGroup12 *group = NULL;
		Mapping *startmap = &mapping[iStart];
		int iEnd = iStart + startmap->span - 1;

		group = dnaNEXT(fmt->group);
		group->startcode = startmap->code;
		group->endcode = mapping[iEnd].code;
		group->startGID = startmap->glyphId;
		
		iStart = iEnd + 1;
		}
				
	fmt->format = 12;
	fmt->reserved = 0;
	fmt->languageId = 0;
	fmt->length = (unsigned short)CMAP_FMT12_SIZE(fmt->group.cnt);
	h->cmap.tbl._flags |= FILLEDFMT12;
	}	
	
	
/* Compare character codes. */
static int CTL_CDECL cmpCodes(const void *first, const void *second)
	{
	const Mapping *a = first;
	const Mapping *b = second;

	if (a->code < b->code)
		return -1;
	else if (a->code > b->code)
		return 1;
	else
		return 0;
	}

/* Fill cmap table. */
static int cmapFill(sfwTableCallbacks *cb, int *dont_write)
	{
	cefCtx h = cb->ctx;
	long i;
	cmapEncoding *encoding;
	unsigned long lastcode;
	unsigned char sawbeyondBMP = 0;

	/* Copy subset spec. to mapping array */
	h->cmap.mapping.cnt = 0;
	for (i = 0; i < h->spec->subset.cnt; i++)
		{
		cefSubsetGlyph *glyph = &h->spec->subset.array[i];
		Mapping *mapping = dnaNEXT(h->cmap.mapping);

		mapping->code = glyph->uv;
		if (glyph->uv > 0xFFFF)
			sawbeyondBMP = 1;
		mapping->glyphId = mapId2GID(h, glyph->id);
		mapping->span = 0;
		mapping->ordered = 0;
		mapping->flags = 0;
		}

	/* Sort mappings by encoding */
	qsort(h->cmap.mapping.array, h->cmap.mapping.cnt, 
		  sizeof(Mapping), cmpCodes);

	/* Check for duplicate encodings */
	lastcode = h->cmap.mapping.array[0].code;
	for (i = 1; i < h->cmap.mapping.cnt; i++)
		{
		unsigned long thiscode = h->cmap.mapping.array[i].code;
		if (lastcode == thiscode)
			fatal(h, cefErrBadSpec);
		lastcode = thiscode;
		}


	h->cmap.tbl._flags = 0;

	/* Fill subtable */
	cmapFillFmt4(h);

	/* Fill header */
	h->cmap.tbl.version = 0;
	h->cmap.tbl.nEncodings = 0;
	if (h->cmap.tbl._flags & FILLEDFMT4)
		h->cmap.tbl.nEncodings++;

	if (sawbeyondBMP)
		{ /* Fill subtable with values > 0xFFFF */
		cmapFillFmt12(h);
		}

	if (h->cmap.tbl._flags & FILLEDFMT12)
		h->cmap.tbl.nEncodings++;
	
	/* Fill encoding records */
	encoding = &h->cmap.tbl.encoding[0];
	encoding->platformId = 0;	/* Unicode */
	encoding->encodingID = 3;	/* Unicode */
	encoding->offset = CMAP_HDR_SIZE + CMAP_ENC_SIZE * h->cmap.tbl.nEncodings;

	encoding = &h->cmap.tbl.encoding[1];
	encoding->platformId = 0;	/* Unicode */
	encoding->encodingID = 4;	/* UTF-32 */
	encoding->offset = 0xFFFF;  /* not yet settable */

	*dont_write = 0;
	return 0;
	}

/* Write format 4 subtable. */
static void cmapWriteFmt4(cefCtx h)
	{
	long i;
	cmapFmt4 *fmt = &h->cmap.tbl.encoding[0].subtable4;

	dstWrite2(h, fmt->format);
	dstWrite2(h, fmt->length);
	dstWrite2(h, fmt->languageId);
	dstWrite2(h, fmt->segCountX2);
	dstWrite2(h, fmt->searchRange);
	dstWrite2(h, fmt->entrySelector);
	dstWrite2(h, fmt->rangeShift);

	for (i = 0; i < fmt->segment.cnt; i++)
		dstWrite2(h, fmt->segment.array[i].endCode);

	dstWrite2(h, fmt->password);

	for (i = 0; i < fmt->segment.cnt; i++)
		dstWrite2(h, fmt->segment.array[i].startCode);

	for (i = 0; i < fmt->segment.cnt; i++)
		dstWrite2(h, fmt->segment.array[i].idDelta);

	for (i = 0; i < fmt->segment.cnt; i++)
		dstWrite2(h, fmt->segment.array[i].idRangeOffset);

	for (i = 0; i < fmt->glyphId.cnt; i++)
		dstWrite2(h, fmt->glyphId.array[i]);
	}

static void cmapWriteFmt12(cefCtx h)
	{
	long i;
	cmapFmt12 *fmt = &h->cmap.tbl.encoding[1].subtable12;

	dstWrite2(h, fmt->format);
	dstWrite2(h, fmt->reserved);
	dstWrite4(h, fmt->length);
	dstWrite4(h, fmt->languageId);
	dstWrite4(h, fmt->group.cnt);
	
	for (i = 0; i < fmt->group.cnt; i++)
		{
		cmapGroup12 *grp = &(fmt->group.array[i]);
		dstWrite4(h, grp->startcode);
		dstWrite4(h, grp->endcode);
		dstWrite4(h, grp->startGID);		
		}
	
	}

/* Write cmap table. */
static int cmapWrite(sfwTableCallbacks *cb, 
					 ctlStreamCallbacks *stm_cb, void *stm,
					 int *use_checksum, unsigned long *checksum)
	{
	cefCtx h = cb->ctx;
	cmapEncoding *encoding;
	unsigned long fmt4len = 0;
	unsigned long fmt12len = 0;
	cmapFmt4 *fmt4;
	cmapFmt12 *fmt12;
	
	/* Write header */
	dstWrite2(h, h->cmap.tbl.version);
	dstWrite2(h, h->cmap.tbl.nEncodings);

	/* Write encoding records */
	fmt4 = &h->cmap.tbl.encoding[0].subtable4;
	if (h->cmap.tbl._flags & FILLEDFMT4)
		{
		fmt4len = fmt4->length;

		encoding = &h->cmap.tbl.encoding[0];
		dstWrite2(h, encoding->platformId);
		dstWrite2(h, encoding->encodingID);
		dstWrite4(h, encoding->offset);
		}

	if (h->cmap.tbl._flags & FILLEDFMT12)		
		{
		fmt12 = &h->cmap.tbl.encoding[1].subtable12;
		fmt12len = fmt12->length;
		
		encoding = &h->cmap.tbl.encoding[1];
		dstWrite2(h, encoding->platformId);
		dstWrite2(h, encoding->encodingID);
		encoding->offset = CMAP_HDR_SIZE + CMAP_ENC_SIZE * h->cmap.tbl.nEncodings + fmt4len;
		dstWrite4(h, encoding->offset);
		}

	/* Write subtables */
	if (h->cmap.tbl._flags & FILLEDFMT4)
		cmapWriteFmt4(h); 

	if (h->cmap.tbl._flags & FILLEDFMT12)
		cmapWriteFmt12(h);
		
	return 0;
	}

/* Register cmap table. */
static void cmapRegisterTable(cefCtx h)
	{
	sfwTableCallbacks cb;
	cb.ctx 			= h;
	cb.table_tag 	= cmap_tag;
	cb.new_table	= cmapNew;
	cb.fill_table	= cmapFill;
	cb.write_table	= cmapWrite;	
	cb.reuse_table	= NULL;
	cb.free_table 	= cmapFree;
	cb.fill_seq 	= cef_cmap_FILL_SEQ;
	cb.write_seq 	= cef_cmap_WRITE_SEQ;
	if (sfwRegisterTable(h->ctx.sfw, &cb))
		fatal(h, cefErrCantRegister);
	}

/* ------------------------------- CFF Table ------------------------------- */

/* Safety initialization. */
static void CFF_Init(cefCtx h)
	{
	h->ctx.t1r = NULL;
	h->ctx.cfr = NULL;
	h->ctx.ttr = NULL;
	h->ctx.cfw = NULL;
	}

/* Initialize CFF-specific data. */
static int CFF_New(sfwTableCallbacks *cb)
	{
	cefCtx h = cb->ctx;
	h->ctx.cfw = cfwNew(&h->cb.mem, &h->stm.cb, CFW_CHECK_ARGS);
	if (h->ctx.cfw == NULL)
		fatal(h, cefErrCffwriteInit);
	h->cb.glyph.direct_ctx = h->ctx.cfw;
	return 0;
	}

/* Free CFF-specific data. */
static int CFF_Free(sfwTableCallbacks *cb)
	{
	cefCtx h = cb->ctx;
	t1rFree(h->ctx.t1r);
	cfrFree(h->ctx.cfr);
	ttrFree(h->ctx.ttr);
	cfwFree(h->ctx.cfw);
	return 0;
	}

/* Compare glyph names. */
static int CTL_CDECL cmpNames(const void *first, const void *second)
	{
	return strcmp(((GlyphMap *)first)->gname, ((GlyphMap *)second)->gname);
	}

/* Call t1read library to subset and convert Type 1 font data. */
static void t1Parse(cefCtx h)
	{
	long i;

	/* Patch in filtered stream read */
	h->stm.cb.read = stm_read;

	if (h->ctx.t1r == NULL)
		{
		/* Initialize library */
		h->ctx.t1r = t1rNew(&h->cb.mem, &h->stm.cb, T1R_CHECK_ARGS);
		if (h->ctx.t1r == NULL)
			fatal(h, cefErrT1readInit);
		}

	/* Parse font */
	if (t1rBegFont(h->ctx.t1r,
				   T1R_UPDATE_OPS, h->src.origin, &h->top, h->spec->UDV))
		fatal(h, cefErrT1Parse);

	if (h->top->sup.flags & ABF_CID_FONT)
		{
		/* CID font; check name override */
		if (h->spec->newFontName != NULL)
			h->top->cid.CIDFontName.ptr = h->spec->newFontName;

		/* Subset by CID */
		for (i = 0; i < h->subset.cnt; i++)
			if (t1rGetGlyphByCID(h->ctx.t1r, 
								 h->subset.array[i].id, &h->cb.glyph))
				fatal(h, cefErrNoGlyph);
		}
	else
		{
		/* Name-keyed font; check name override */
		if (h->spec->newFontName != NULL)
			h->top->FDArray.array[0].FontName.ptr = h->spec->newFontName;

		/* Validate subsetting method */
		if (h->spec->subset.names == NULL)
			fatal(h, cefErrBadSpec);

		/* Subset by glyph name */
		for (i = 0; i < h->subset.cnt; i++)
			if (t1rGetGlyphByName(h->ctx.t1r, 
								  h->subset.array[i].gname, &h->cb.glyph))
				fatal(h, cefErrNoGlyph);
		}

	/* Restore client stream read */
	h->stm.cb.read = h->cb.stm.read;
	}

/* Call cffread library to subset CFF data. */
static void cffParse(cefCtx h)
	{
	long i;

	if (h->ctx.cfr == NULL)
		{
		/* Initialize library */
		h->ctx.cfr = cfrNew(&h->cb.mem, &h->stm.cb, CFR_CHECK_ARGS);
		if (h->ctx.cfr == NULL)
			fatal(h, cefErrCffreadInit);
		}

	/* Parse font */
	if (cfrBegFont(h->ctx.cfr, CFR_UPDATE_OPS, h->src.origin, 0, &h->top, NULL))
		fatal(h, cefErrCFFParse);

	if (h->top->sup.flags & ABF_CID_FONT)
		{
		/* CID font; check name override */
		if (h->spec->newFontName != NULL)
			h->top->cid.CIDFontName.ptr = h->spec->newFontName;

		/* Subset by CID */
		for (i = 0; i < h->subset.cnt; i++)
			if (cfrGetGlyphByCID(h->ctx.cfr, 
								 h->subset.array[i].id, &h->cb.glyph))
				fatal(h, cefErrNoGlyph);
		}
	else
		{
		/* Name-keyed font; check name override */
		if (h->spec->newFontName != NULL)
			h->top->FDArray.array[0].FontName.ptr = h->spec->newFontName;

		/* Validate subsetting method */
		if (h->spec->subset.names != NULL)
			{
			/* Subset by glyph name */
			for (i = 0; i < h->subset.cnt; i++)
				if (cfrGetGlyphByName(h->ctx.cfr, 
									  h->subset.array[i].gname, &h->cb.glyph))
					fatal(h, cefErrNoGlyph);
			}
		else
			/* Subset by GID */
			for (i = 0; i < h->subset.cnt; i++)
				if (cfrGetGlyphByTag(h->ctx.cfr, 
									 h->subset.array[i].id, &h->cb.glyph))
					fatal(h, cefErrNoGlyph);
		}
	}

/* Call ttread library to subset TrueType data. */
static void ttParse(cefCtx h)
	{
	long i;

	if (h->ctx.ttr == NULL)
		{
		/* Initialize library */
		h->ctx.ttr = ttrNew(&h->cb.mem, &h->stm.cb, TTR_CHECK_ARGS);
		if (h->ctx.ttr == NULL)
			fatal(h, cefErrTtreadInit);
		}

	/* Parse font */
	if (ttrBegFont(h->ctx.ttr, TTR_EXACT_PATH, h->src.origin, 0, &h->top))
		fatal(h, cefErrTTParse);

	/* Check name override */
	if (h->spec->newFontName != NULL)
		h->top->FDArray.array[0].FontName.ptr = h->spec->newFontName;

	/* Validate subsetting method */
	if (h->spec->subset.names != NULL)
		fatal(h, cefErrBadSpec);

	/* Subset by GID */
	for (i = 0; i < h->subset.cnt; i++)
		if (ttrGetGlyphByTag(h->ctx.ttr, h->subset.array[i].id, &h->cb.glyph))
			fatal(h, cefErrNoGlyph);
	}

/* Match glyph name in subset. */
static int CTL_CDECL matchName(const void *key, const void *value)
	{
	return strcmp((char *)key, ((GlyphMap *)value)->gname);
	}

/* GID to glyph mapping callback. */
static void glyphmap(cfwMapCallback *cb, 
					 unsigned short gid, abfGlyphInfo *info)
	{
	cefCtx h = cb->ctx;
	GlyphMap *map;
	if (h->spec->subset.names == NULL)
		{
		/* Match cid or tag to subset id */
		unsigned short key = 
			(info->flags & ABF_CID_FONT)? info->cid: info->tag;
		map = (GlyphMap *)bsearch(&key, h->subset.array, h->subset.cnt,
								  sizeof(GlyphMap), matchId);
		}
	else
		/* Match glyph names */
		map = (GlyphMap *)bsearch(info->gname.ptr, h->subset.array, 
								  h->subset.cnt, sizeof(GlyphMap), matchName);
	if (map == NULL)
		fatal(h, cefErrCantHappen);
	map->gid = gid;

	if (h->cb.map != NULL)
		/* Callback mapping to client */
		h->cb.map->glyphmap(h->cb.map, gid, info);
	}

/* Compare client ids. */
static int CTL_CDECL cmpIds(const void *first, const void *second)
	{
	unsigned short a = ((GlyphMap *)first)->id;
	unsigned short b = ((GlyphMap *)second)->id;
	if (a < b)
		return -1;
	else if (a > b)
		return 1;
	return 0;
	}

/* Fill CFF table. */
static int CFF_Fill(sfwTableCallbacks *cb, int *dont_write)
	{
	cefCtx h = cb->ctx;
	cfwMapCallback map_cb;

	/* Initialize callback */
	map_cb.ctx = h;
	map_cb.glyphmap = glyphmap;

	/* Prepare to add CFF data */
	if (cfwBegSet(h->ctx.cfw, CFW_EMBED_OPT) ||
		cfwBegFont(h->ctx.cfw, &map_cb, 0))
		fatal(h, cefErrCffwriteFont);

	switch (h->src.type)
		{
	case SRC_TYPE1:
		t1Parse(h);
		break;
	case SRC_CFF:
		cffParse(h);
		break;
	case SRC_TRUETYPE:
		ttParse(h);
		break;
		}

	if (h->spec->flags & CEF_FORCE_LANG_1)
		{
		/* Force LanguageGroup 1 in all Private DICTs */
		long i;
		for (i = 0; i < h->top->FDArray.cnt; i++)
			h->top->FDArray.array[i].Private.LanguageGroup = 1;
		}

	if (h->spec->flags & CEF_FORCE_IDENTITY_ROS)
 		{
 		h->top->cid.Registry.ptr = "Adobe";
 		h->top->cid.Ordering.ptr = "Identity";
 		h->top->cid.Supplement = 0;
 		}

	/* Write CFF data */
	if (cfwEndFont(h->ctx.cfw, h->top))
		fatal(h, cefErrCffwriteFont);

	/* End parse */
	switch (h->src.type)
		{
	case SRC_TYPE1:
		if (t1rEndFont(h->ctx.t1r))
			fatal(h, cefErrT1Parse);
		break;
	case SRC_CFF:
		if (cfrEndFont(h->ctx.cfr))
			fatal(h, cefErrCFFParse);
		break;
	case SRC_TRUETYPE:
		if (ttrEndFont(h->ctx.ttr))
			fatal(h, cefErrTTParse);
		break;
		}

	if (h->spec->subset.names != NULL)
		/* Re-sort subset by id */
		qsort(h->subset.array, h->subset.cnt, sizeof(GlyphMap), cmpIds);

	*dont_write = 0;
	return 0;
	}

/* Write CFF table. */
static int CFF_Write(sfwTableCallbacks *cb,
					 ctlStreamCallbacks *stm_cb, void *stm,
 					 int *use_checksum, unsigned long *checksum)

	{
	cefCtx h = cb->ctx;

	/* Write CFF data */
	if (cfwEndSet(h->ctx.cfw))
		fatal(h, cefErrCffwriteFont);

	return 0;
	}

/* Register CFF table. */
static void CFF_RegisterTable(cefCtx h)
	{
	sfwTableCallbacks cb;
	cb.ctx 			= h;
	cb.table_tag 	= CFF__tag;
	cb.new_table	= CFF_New;
	cb.fill_table 	= CFF_Fill;
	cb.write_table 	= CFF_Write;	
	cb.reuse_table 	= NULL;
	cb.free_table	= CFF_Free;
	cb.fill_seq 	= cef_CFF__FILL_SEQ;
	cb.write_seq 	= cef_CFF__WRITE_SEQ;
	if (sfwRegisterTable(h->ctx.sfw, &cb))
		fatal(h, cefErrCantRegister);
	}

/* ----------------------------- Coverage Table ---------------------------- */

/* Safety initialization. */
static void coverageSafe(cefCtx h)
	{
	h->coverage.gids.size = 0;
	h->coverage.fmt2.RangeRecord.size = 0;
	}

/* Initialize converage-specific data. */
static void coverageNew(cefCtx h)
	{
	dnaINIT(h->ctx.dna, h->coverage.gids, 100, 200);
	dnaINIT(h->ctx.dna, h->coverage.fmt2.RangeRecord, 50, 100);	
	}

/* Free converage-specific data. */
static void coverageFree(cefCtx h)
	{
	dnaFREE(h->coverage.gids);
	dnaFREE(h->coverage.fmt2.RangeRecord);
	}

/* Write format 1 coverage table. */
static void writeCoverage1(cefCtx h)
	{
	int i;
	CoverageFormat1 *fmt = &h->coverage.fmt1;

	dstWrite2(h, fmt->CoverageFormat);
	dstWrite2(h, fmt->GlyphCount);
	for (i = 0; i < fmt->GlyphCount; i++)
		dstWrite2(h, fmt->GlyphArray[i]);
	}

/* Write format 2 coverage table. */
static void writeCoverage2(cefCtx h)
	{
	int i;
	CoverageFormat2 *fmt = &h->coverage.fmt2;

	dstWrite2(h, fmt->CoverageFormat);
	dstWrite2(h, fmt->RangeCount);
	for (i = 0; i < fmt->RangeCount; i++)
		{
		RangeRecord *rec = &fmt->RangeRecord.array[i];
		dstWrite2(h, rec->Start);
		dstWrite2(h, rec->End);
		dstWrite2(h, rec->StartCoverageIndex);
		}
	}

/* Write coverage table. */
static void writeCoverage(cefCtx h)
	{
	switch (h->coverage.format)
		{
	case 1:
		writeCoverage1(h);
		break;
	case 2:
		writeCoverage2(h);
		break;
		}
	}

/* Fill coverage format 1 table. */
static void fillCoverage1(cefCtx h, long nGlyphs, GID *glyphs)
	{
	CoverageFormat1 *fmt = &h->coverage.fmt1;

	fmt->CoverageFormat = 1;
	fmt->GlyphCount = (unsigned short)nGlyphs;
	fmt->GlyphArray = glyphs;
	}

/* Fill coverage format 2 table. */
static void fillCoverage2(cefCtx h, int nRanges, long nGlyphs, GID *glyphs)
	{
	int i;
	int iRange;
	int iStart;
	CoverageFormat2 *fmt = &h->coverage.fmt2;

	fmt->CoverageFormat = 2;
	fmt->RangeCount = nRanges;
	dnaSET_CNT(fmt->RangeRecord, nRanges);

	iRange = 0;
	iStart = 0;
	for (i = 1; i <= nGlyphs; i++)
		if (i == nGlyphs || glyphs[i - 1] != glyphs[i] - 1)
			{
			RangeRecord *rec = &fmt->RangeRecord.array[iRange++];
					
			rec->Start = glyphs[iStart];
			rec->End = glyphs[i - 1];
			rec->StartCoverageIndex = iStart;

			iStart = i;
			}
	}

/* Fill coverage table. */
static unsigned short fillCoverage(cefCtx h)
	{
	int i;
	GID first;
	int nRanges;
	unsigned size1;
	unsigned size2;
	long nGlyphs;
	GID *glyphs;
	
	/* Make list of unique first glyphs */
	h->coverage.gids.cnt = 0;
	first = h->GPOS.pairs.array[0].first;
	for (i = 1; i < h->GPOS.pairs.cnt; i++)
		if (h->GPOS.pairs.array[i].first != first)
			{
			*dnaNEXT(h->coverage.gids) = first;
			first = h->GPOS.pairs.array[i].first;
			}
	*dnaNEXT(h->coverage.gids) = first;

	/* Count ranges (assumes glyphs sorted by increasing GID) */
	nGlyphs = h->coverage.gids.cnt;
	glyphs = h->coverage.gids.array;
	nRanges = 1;
	for (i = 1; i < nGlyphs; i++)
		if (glyphs[i - 1] != glyphs[i] - 1)
			nRanges++;

	/* Choose format with smallest representation */
	size1 = COVERAGE1_SIZE(nGlyphs);
	size2 = COVERAGE2_SIZE(nRanges);
	if (size1 < size2)
		{
		h->coverage.format = 1;
		fillCoverage1(h, nGlyphs, glyphs);
		}
	else
		{
		h->coverage.format = 2;
		fillCoverage2(h, nRanges, nGlyphs, glyphs);
		}

	return (unsigned short)h->coverage.gids.cnt;
	}

/* ------------------------------- GPOS Table ------------------------------ */

/* Safety initialization. */
static void GPOSInit(cefCtx h)
	{
	h->GPOS.pairs.size = 0;
	h->GPOS.subtbl.PairSet.size = 0;
	h->GPOS.pvrecs.size = 0;
	coverageSafe(h);
	}

/* Initialize GPOS-specific data. */
static int GPOSNew(sfwTableCallbacks *cb)
	{
	cefCtx h = cb->ctx;
	dnaINIT(h->ctx.dna, h->GPOS.pairs, 1000, 2000);
	dnaINIT(h->ctx.dna, h->GPOS.subtbl.PairSet, 100, 200);
	dnaINIT(h->ctx.dna, h->GPOS.pvrecs, 1000, 2000);
	coverageNew(h);
	return 0;
	}

/* Free GPOS-specific data. */
static int GPOSFree(sfwTableCallbacks *cb)
	{
	cefCtx h = cb->ctx;
	dnaFREE(h->GPOS.pairs);
	dnaFREE(h->GPOS.subtbl.PairSet);
	dnaFREE(h->GPOS.pvrecs);
	coverageFree(h);
	return 0;
	}

/* Compare kern pairs. */
static int CTL_CDECL cmpKernPairs(const void *first, const void *second)
	{
	const cefKernPair *a = first;
	const cefKernPair *b = second;
	if (a->first < b->first)				/* Compare first glyphs */
		return -1;
	else if (a->first > b->first)
		return 1;
	else if (a->second < b->second)			/* Compare second glyphs */
		return -1;
	else if (a->second > b->second)
		return 1;
	else if (ABS(a->value) < ABS(b->value))	/* Smallest absolute value first */
		return -1;
	else if (ABS(a->value) > ABS(b->value))
		return 1;
	else if (a->value < b->value)	/* Same absolute value; negative first */
		return -1;
	else if (a->value > b->value)
		return 1;
	else
		return 0;
	}

/* Fill format 1 pair positioning subtable. */
static void fillPairPos1(cefCtx h)
	{
	int i;
	int j;
	int iFirst;
	GID first;
	Offset offset;
	PairPosFormat1 *fmt = &h->GPOS.subtbl;

	/* Fill format 1 header */
	fmt->PosFormat		= 1;
	fmt->ValueFormat1 	= 1<<2;	/* ValueXAdvance */
	fmt->ValueFormat2 	= 0;
	fmt->PairSetCount 	= fillCoverage(h);

	/* Allocate/size pair set array */
	dnaSET_CNT(fmt->PairSet, fmt->PairSetCount);

	/* Fill pair sets */
	j = 0;
	iFirst = 0;
	offset = PAIR_POS1_SIZE(fmt->PairSetCount);
	first = h->GPOS.pairs.array[0].first;
	for (i = 1; i <= h->GPOS.pairs.cnt; i++)
		if (i == h->GPOS.pairs.cnt || 
			h->GPOS.pairs.array[i].first != h->GPOS.pairs.array[iFirst].first)
			{
			int k;
			PairValueRecord *dst;
			cefKernPair *src = &h->GPOS.pairs.array[iFirst];
			PairSet *ps = &fmt->PairSet.array[j++];

			/* Fill PairSet */
			ps->offset = offset;
			ps->PairValueCount = i - iFirst;

			dst = dnaEXTEND(h->GPOS.pvrecs, ps->PairValueCount);
			for (k = 0; k < ps->PairValueCount; k++)
				{
				dst->SecondGlyph = src->second;
				dst->Value1 = src->value;
				src++;
				dst++;
				}

			offset += PAIR_SET_SIZE(ps->PairValueCount, 1);
			iFirst = i;
			}

	/* Fill coverage table offset */
	fmt->Coverage = offset;
	}


/* Fill GPOS table. */
static int GPOSFill(sfwTableCallbacks *cb, int *dont_write)
	{
	cefCtx h = cb->ctx;
	long i;
	cefKernPair *last;

	if (h->spec->kern.cnt == 0)
		{
		/* Nothing to do */
		*dont_write = 1;
		return 0;
		}

	/* Copy pairs to local accumulator */
	dnaSET_CNT(h->GPOS.pairs, h->spec->kern.cnt);
	for (i = 0; i < h->GPOS.pairs.cnt; i++)
		{
		cefKernPair *src = &h->spec->kern.array[i];
		cefKernPair *dst = &h->GPOS.pairs.array[i];
		dst->first = mapId2GID(h, src->first);
		dst->second = mapId2GID(h, src->second);
		dst->value = src->value;
		}

	/* Sort pairs */
	qsort(h->GPOS.pairs.array, h->GPOS.pairs.cnt, sizeof(cefKernPair),
		  cmpKernPairs);
	
	/* Remove duplicates. A pair is considered duplicate if the first and
	   second glyphs are identical. If the value associated with identical
	   pairs is the same either one can be discarded. If the value is different
	   the pair with the smallest absolute value is retained. The rational is
	   that one of the duplicates is mostly likely correct and the other is
	   wrong but we have no way of knowing which. Therefore, we choose the
	   smallest adjustment value because it will be the least obtrusive even if
	   it is wrong choice. */

	last = &h->GPOS.pairs.array[0];
	for (i = 1; i < h->GPOS.pairs.cnt; i++)
		{
		cefKernPair *curr = &h->GPOS.pairs.array[i];
		if (curr->first != last->first || curr->second != last->second)
			{
			if (++last != curr)
				*last = *curr;
			}
		}
	/* 64-bit warning fixed by cast here */
	h->GPOS.pairs.cnt = (long)(last - h->GPOS.pairs.array + 1);

	fillPairPos1(h);

	*dont_write = 0;
	return 0;
	}

/* Write format 1 pair positioning table */
static void writePairPos1(cefCtx h)
	{
	int i;
	PairValueRecord *pvrec;
	PairPosFormat1 *fmt = &h->GPOS.subtbl;

	/* Write header */
	dstWrite2(h, fmt->PosFormat);
	dstWrite2(h, fmt->Coverage);
	dstWrite2(h, fmt->ValueFormat1);
	dstWrite2(h, fmt->ValueFormat2);
	dstWrite2(h, fmt->PairSetCount);

	/* Write pair sets */
	for (i = 0; i < fmt->PairSetCount; i++)
		dstWrite2(h, fmt->PairSet.array[i].offset);

	pvrec = h->GPOS.pvrecs.array;
	for (i = 0; i < fmt->PairSetCount; i++)
		{
		int j;
		PairSet *ps = &fmt->PairSet.array[i];

		/* Write pair value records */
		dstWrite2(h, ps->PairValueCount);
		for (j = 0; j < ps->PairValueCount; j++)
			{
			dstWrite2(h, pvrec->SecondGlyph);
			dstWrite2(h, pvrec->Value1);
			pvrec++;
			}
		}
	}

/* Write GPOS table. */
static int GPOSWrite(sfwTableCallbacks *cb, 
					 ctlStreamCallbacks *stm_cb, void *stm,
					 int *use_checksum, unsigned long *checksum)
	{
	cefCtx h = cb->ctx;

	/* An offset field specifies a byte offset of data relative to some format
	   component, normally the beginning of the record it belongs to. In the
	   following comments, the notation <component> indicates an offset field
	   is relative to <component>. Offsets, both absolute and relative, within
	   the GPOS table are shown in parentheses. */

	/* Write GPOS header */
	dstWrite4(h, 0x00010000);		/* Version */
	dstWrite2(h, 0x000a);			/* ScriptList <Header> */
	dstWrite2(h, 0x001e);			/* FeatureList <Header> */
	dstWrite2(h, 0x002c);			/* LookupList <Header> */

	/* Write ScriptList (000a) */
	dstWrite2(h, 0x0001);			/* ScriptCount */

	/* Write ScriptRecord */
	dstWrite4(h, DFLT_tag);			/* ScriptTag */
	dstWrite2(h, 0x0008);			/* Script <ScriptList> */

	/* Write Script (0008) */
	dstWrite2(h, 0x0004);			/* DefaultLangSys <Script> */
	dstWrite2(h, 0x0000);			/* LangSys */
								
	/* Write LangSys (0004) */	
	dstWrite2(h, 0x0000);			/* LookupOrder */
	dstWrite2(h, 0xffff);			/* ReqFeatureIndex */
	dstWrite2(h, 0x0001);			/* FeatureCount */
	dstWrite2(h, 0x0000);			/* FeatureIndex */

	/* Write FeatureList (001e) */
	dstWrite2(h, 0x0001);			/* FeatureCount */

	/* Write FeatureRecord */
	dstWrite4(h, kern_tag);			/* FeatureTag */
	dstWrite2(h, 0x0008);			/* Feature <FeatureList> */

	/* Write FeatureTable (0008) */
	dstWrite2(h, 0x0000);			/* FeatureParam */
	dstWrite2(h, 0x0001);			/* LookupCount */
	dstWrite2(h, 0x0000);			/* LookupListIndex[0] */

	/* Write LookupList (002c) */
	dstWrite2(h, 0x0001);			/* LookupCount */
	dstWrite2(h, 0x0004);			/* Lookup[0] <LookupList> */

	/* Write Lookup (0004) */
	dstWrite2(h, 0x0002);			/* LookupType (GPOS KernPair) */
	dstWrite2(h, 0x0000);			/* LookupFlag */
	dstWrite2(h, 0x0001);			/* SubTableCount */
	dstWrite2(h, 0x0008);			/* SubTable[0] <Lookup> */

	writePairPos1(h);
	writeCoverage(h);

	return 0;
	}

/* Register GPOS table. */
static void GPOSRegisterTable(cefCtx h)
	{
	sfwTableCallbacks cb;
	cb.ctx 			= h;
	cb.table_tag 	= GPOS_tag;
	cb.new_table	= GPOSNew;
	cb.fill_table 	= GPOSFill;
	cb.write_table 	= GPOSWrite;	
	cb.reuse_table 	= NULL;
	cb.free_table	= GPOSFree;
	cb.fill_seq 	= cef_GPOS_FILL_SEQ;
	cb.write_seq 	= cef_GPOS_WRITE_SEQ;
	if (sfwRegisterTable(h->ctx.sfw, &cb))
		fatal(h, cefErrCantRegister);
	}

/* ----------------------------- Font Embedding ---------------------------- */

/* This hook determines the Unicode value for the glyph, which is required for
   SVG. */
static int svg_GlyphBeg(abfGlyphCallbacks *cb, abfGlyphInfo *info)
	{
	cefCtx h = cb->indirect_ctx;

	GlyphMap *map;
	if (h->spec->subset.names == NULL)
		{
		/* Match cid or tag to subset id */
		unsigned short key = 
			(info->flags & ABF_CID_FONT)? info->cid: info->tag;
		map = (GlyphMap *)bsearch(&key, h->subset.array, h->subset.cnt,
								  sizeof(GlyphMap), matchId);
		}
	else
		/* Match glyph names */
		map = (GlyphMap *)bsearch(info->gname.ptr, h->subset.array, 
								  h->subset.cnt, sizeof(GlyphMap), matchName);
	if (map == NULL)
		fatal(h, cefErrCantHappen);

	info->flags |= ABF_GLYPH_UNICODE;
	info->encoding.code = map->uv;

	return svwGlyphCallbacks.beg(cb, info);
	}

/* Make embedding font. */
int cefMakeEmbeddingFont(cefCtx h, cefEmbedSpec *spec, cefMapCallback *map)
	{	
	long i;
	long j;
	long namecnt;
	unsigned short notdef;

	if (spec->URL != NULL)
		return cefErrNotImpl;

	if ((spec->flags & CEF_FORCE_LANG_1) && (spec->flags & CEF_WRITE_SVG))
		return cefErrBadSpec;

	if (spec->subset.cnt <= 0 || spec->kern.cnt < 0)
		return cefErrBadSpec;

	/* Set error handler */
	if (setjmp(h->err.env))
		return h->err.code;

	namecnt = 0;	/* Suppress optimizer warning */

	/* Make copy of client's subset id list and insert .notdef */
	h->subset.cnt = 0;
	dnaSET_CNT(h->subset, spec->subset.cnt + 1);
	if (spec->subset.names != NULL)
		{
		/* Subset glyph names specified; count them and find .notdef */
		int seen_notdef = 0;
		for (namecnt = 0;; namecnt++)
			{
			char *gname = spec->subset.names[namecnt];
			if (gname == NULL)
				break;
			else if (!seen_notdef && strcmp(gname, ".notdef") == 0)
				{
				h->subset.array[0].id = (unsigned short)namecnt;
				h->subset.array[0].uv = 0xFFFF;  /* Signifies .notdef */
				seen_notdef = 1;	/* Ignore redefinitions! */
				}
			}
		if (!seen_notdef)
			return cefErrBadSpec;
		}
	else
		{
		h->subset.array[0].id = 0;
		h->subset.array[0].uv = 0xFFFF;  /* Signifies .notdef */
		}

	/* Save the .notdef id */
	notdef = h->subset.array[0].id;

	for (i = 0; i < spec->subset.cnt; i++)
		{
		h->subset.array[i + 1].id = spec->subset.array[i].id;
		h->subset.array[i + 1].uv = spec->subset.array[i].uv;
		}

	/* Sort id list (including vids) and remove duplicates */
	qsort(h->subset.array, h->subset.cnt, sizeof(GlyphMap), cmpIds);
	j = 0; /* target index */
	for (i = 1; i < h->subset.cnt; i++) /* source index */
		{
		if (h->subset.array[i].id >= CEF_VID_BEGIN)
			break;	/* Exclude VIDs from subset */
		else if (h->subset.array[i].id == h->subset.array[j].id) /* Duplicate ids */
			{
			if (h->subset.array[j].id != notdef) /* Target id is not .notdef */
				{
				if (h->subset.array[i].uv < h->subset.array[j].uv) /* Current target value is not the minimum for this id */
					h->subset.array[j].uv = h->subset.array[i].uv; /* Keep the minimum */
				}
			else /* The .notdef id is a special case */
				h->subset.array[j].uv = 0xFFFF; /* Must have the value signifying .notdef */
			}
		else /* Source and target have different ids */
			{
			/* Copy source to target to collapse any sequence of duplicate ids dealt with above */
			++j;
			h->subset.array[j].id = h->subset.array[i].id;
			h->subset.array[j].uv = h->subset.array[i].uv;
			}
		}
	h->subset.cnt = j + 1; /* Total count is now number of target values written */

	if (spec->subset.names != NULL)
		{
		/* Validate glyph/name indexes */
		if (h->subset.array[h->subset.cnt - 1].id > namecnt)
			fatal(h, cefErrBadSpec);

		/* Add glyph names to subset */
		for (i = 0; i < h->subset.cnt; i++)
			{
			GlyphMap *map = &h->subset.array[i];
			map->gname = spec->subset.names[map->id];
			}

		/* Sort subset by glyph name */
		qsort(h->subset.array, h->subset.cnt, sizeof(GlyphMap), cmpNames);
		}

	h->spec = spec;
	h->cb.map = map;

	stmsOpen(h);
	h->src.type = getFontType(h);

	if (spec->flags & CEF_WRITE_SVG) /* Write SVG font */
		{
		abfGlyphCallbacks cb = h->cb.glyph;

		svwCtx hSvw = svwNew(&h->cb.mem, &h->cb.stm, SVW_CHECK_ARGS);
		if (NULL == hSvw)
			fatal(h, cefErrSvgwriteInit);


		h->cb.glyph = svwGlyphCallbacks;
		h->cb.glyph.beg = svg_GlyphBeg;
		h->cb.glyph.indirect_ctx = h;
		h->cb.glyph.direct_ctx = hSvw;

		if (svwBegFont(hSvw, h->svwFlags))
			fatal(h, cefErrCffwriteFont);

		switch (h->src.type)
			{
			case SRC_TYPE1:
				t1Parse(h);
				break;
			case SRC_CFF:
				cffParse(h);
				break;
			case SRC_TRUETYPE:
				ttParse(h);
				break;
			}

		/* Write the SVG font */
		if (svwEndFont(hSvw, h->top))
			fatal(h, cefErrCffwriteFont);

		/* End parse */
		switch (h->src.type)
			{
			case SRC_TYPE1:
				if (t1rEndFont(h->ctx.t1r))
					fatal(h, cefErrT1Parse);
				break;
			case SRC_CFF:
				if (cfrEndFont(h->ctx.cfr))
					fatal(h, cefErrCFFParse);
				break;
			case SRC_TRUETYPE:
				if (ttrEndFont(h->ctx.ttr))
					fatal(h, cefErrTTParse);
				break;
			}

		svwFree(hSvw);
		h->cb.glyph = cb;
		}
	else  /* Write CEF font */
		{
		if (h->flags & DO_NEW_TABLES)
			{
			if (sfwNewTables(h->ctx.sfw))
				fatal(h, cefErrSfntwrite);
			h->flags &= ~DO_NEW_TABLES;
			}

		if (sfwFillTables(h->ctx.sfw) ||
			sfwWriteTables(h->ctx.sfw, NULL, sfr_OTTO_tag))
			fatal(h, cefErrSfntwrite);
		}

	stmsClose(h);

	return cefSuccess;
	}

/* -------------------------- Safe dynarr Callbacks ------------------------ */

/* Manage memory. */
static void *dna_manage(ctlMemoryCallbacks *cb, void *old, size_t size)
	{
	cefCtx h = cb->ctx;
	void *ptr = h->cb.mem.manage(&h->cb.mem, old, size);
	if (size > 0 && ptr == NULL)
		fatal(h, cefErrNoMemory);
	return ptr;
	}

/* Initialize error handling dynarr context. */
static void dna_init(cefCtx h)
	{
	ctlMemoryCallbacks cb;
	cb.ctx 		= h;
	cb.manage 	= dna_manage;
	h->ctx.dna		= dnaNew(&cb, DNA_CHECK_ARGS);
	}

/* --------------------------- Context Management -------------------------- */

/* Validate client and create context. */
cefCtx cefNew(ctlMemoryCallbacks *mem_cb, ctlStreamCallbacks *stm_cb,
			  CTL_CHECK_ARGS_DCL)
	{
	cefCtx h;

	/* Check client/library compatibility */
	if (CTL_CHECK_ARGS_TEST(CEF_VERSION))
		return NULL;

	/* Allocate context */
	h = mem_cb->manage(mem_cb, NULL, sizeof(struct cefCtx_));
	if (h == NULL)
		return NULL;

	/* Safety initialization */
	cmapInit(h);
	CFF_Init(h);
	GPOSInit(h);
	h->stm.src = NULL;
	h->stm.dst = NULL;
	h->subset.size = 0;
	h->ctx.dna = NULL;

	/* Copy callbacks */
	h->cb.mem = *mem_cb;
	h->cb.stm = *stm_cb;

	/* Set error handler */
	if (setjmp(h->err.env))
		goto cleanup;

	/* Initialize stream patches */
	stmInit(h);

	/* Initialize service libraries */
	dna_init(h);
	h->ctx.sfr = sfrNew(mem_cb, &h->stm.cb, SFR_CHECK_ARGS);
	if (h->ctx.sfr == NULL)
		fatal(h, ttrErrSfntread);
	h->ctx.sfw = sfwNew(mem_cb, &h->stm.cb, SFW_CHECK_ARGS);
	if (h->ctx.sfw == NULL)
		fatal(h, cefErrSfntwrite);

	/* Initialize context */
	h->flags = DO_NEW_TABLES;
	h->svwFlags = 0;
	dnaINIT(h->ctx.dna, h->subset, 256, 128);
	h->cb.glyph = cfwGlyphCallbacks;

	/* Register tables */
	cmapRegisterTable(h);
	CFF_RegisterTable(h);
	GPOSRegisterTable(h);

	return h;

 cleanup:
	/* Initilization failed */
	cefFree(h);
	return NULL;
	}

/* Pass svgwrite flags through cfembed context. */
int cefSetSvwFlags(cefCtx h, long flags)
	{
	h->svwFlags = flags;
	return cefSuccess;
	}

/* Register client table. */
int cefRegisterTable(cefCtx h, sfwTableCallbacks *tbl_cb)
	{
	if (sfwRegisterTable(h->ctx.sfw, tbl_cb))
		fatal(h, cefErrCantRegister);
	return cefSuccess;
	}

/* Free context. */
void cefFree(cefCtx h)
	{
	if (h == NULL)
		return;

	dnaFREE(h->subset);

	/* Free sfnt tables */
	if (h->ctx.sfw != NULL)
		(void)sfwFreeTables(h->ctx.sfw);

	dnaFree(h->ctx.dna);
	sfrFree(h->ctx.sfr);
	sfwFree(h->ctx.sfw);

	/* Free context */
	(void)h->cb.mem.manage(&h->cb.mem, h, 0);
	}

/* Get version numbers of libraries. */
void cefGetVersion(ctlVersionCallbacks *cb)
	{
	if (cb->called & 1<<CEF_LIB_ID)
		return;	/* Already enumerated */

	/* Support libraries */
	cfrGetVersion(cb);
	cfwGetVersion(cb);
	dnaGetVersion(cb);
	sfrGetVersion(cb);
	sfwGetVersion(cb);
	t1rGetVersion(cb);
	ttrGetVersion(cb);

	/* This library */
	cb->getversion(cb, CEF_VERSION, "cfembed");

	/* Record this call */
	cb->called |= 1<<CEF_LIB_ID;
	}

/* Map error code to error string. */
char *cefErrStr(int err_code)
	{
	static char *errstrs[] =
		{
#undef CTL_DCL_ERR
#define CTL_DCL_ERR(name,string)	string,
#include "ceferr.h"
		};
	return (err_code < 0 || err_code >= (int)ARRAY_LEN(errstrs))?
		"unknown error": errstrs[err_code];
	}

#ifdef CEF_DEBUG

static void dbkern(cefCtx h)
	{
	long i;
	printf("--- kerning [nGlyphs=%ld]\n", h->subset.cnt);
	printf("--- pairs[index]={first,second,value}\n");
	for (i = 0; i < h->GPOS.pairs.cnt; i++)
		{
		cefKernPair *pair = &h->GPOS.pairs.array[i];
		printf("[%ld]={%hu,%hu,%hd} ", 
			   i, pair->first, pair->second, pair->value);
		}
	printf("\n");
	}

/* This function just serves to suppress annoying "defined but not used"
   compiler messages when debugging is enabled. */
static void CTL_CDECL dbuse(int arg, ...)
	{
	dbuse(0, dbuse, dbkern);
	}

#endif /* CEF_DEBUG */

