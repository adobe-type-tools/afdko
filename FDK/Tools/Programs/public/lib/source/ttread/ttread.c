/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#include "ttread.h"
#include "dynarr.h"
#include "ctutil.h"
#include "sfntread.h"

#if PLAT_SUN4
#include "sun4/fixstring.h"
#else /* PLAT_SUN4 */
#include <string.h>
#endif  /* PLAT_SUN4 */

#include <setjmp.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <ctype.h>

typedef unsigned short UV;		/* Unicode value */
#define UV_UNDEF	0xffff		/* Undefined Unicode value */
typedef unsigned short GID;		/* Glyph index */
typedef unsigned short STI;		/* String index */
#define STI_UNDEF	0xffff		/* Undefined string index */

/* Round to nearest integer */
#define RND(v)		((float)floor((v) + 0.5))
#define FIX2FLT(x)	((x)/(float)65536.0)
#define ARRAY_LEN(t) 	(sizeof(t)/sizeof((t)[0]))

typedef struct					/* Glyph encoding */
	{
	unsigned short code;
	GID gid;
	} Encoding;

typedef struct					/* Map AGL Unicode value to glyph name */
	{
	char *gname;
	UV uv;
	} AGLMap;

typedef struct		/* Map secondary AGL Unicode value to primary AGL value */
	{
	UV sec;
	UV pri;
	} AGLDbl;

typedef struct /* Coordinate point */
	{
	float x;
	float y;
	} Point;

/* --------------------------- sfnt Definitions  --------------------------- */

typedef long Fixed;
typedef short FWord;
typedef unsigned short uFWord;
typedef unsigned long Offset;
#define F2Dot14_2FLT(f)	((float)((f)/16384.0))

typedef struct					/* bsearch id match record */
	{
	unsigned short platformId;
	unsigned short platspecId;
	unsigned short languageId;
	unsigned short nameId;
	} MatchIds;

/* name and cmap table platform and platform-specific ids */
enum
	{
	/* Platform ids */
	UNI_PLATFORM	= 0,		/* Unicode */
	MAC_PLATFORM	= 1,		/* Macintosh */
	WIN_PLATFORM	= 3,		/* Microsoft */

	/* Unicode platform-specific ids */
	UNI_DEFAULT		= 0,		/* Default semantics */
	UNI_VER_1_1		= 1,		/* Version 1.1 semantics */
	UNI_ISO_10646	= 2,		/* ISO 10646 semantics */
	UNI_VER_2_0		= 3,		/* Version 2.0 semantics */
	
	/* Macintosh platform-specific and language ids */
	MAC_ROMAN		= 0,		/* Roman */
	MAC_ENGLISH		= 0,		/* English language */
	
	/* Windows platform-specific and language ids */
	WIN_SYMBOL		= 0,		/* Undefined index scheme */
	WIN_UGL			= 1,		/* UGL */
	WIN_ENGLISH		= 0x409		/* English (American) language */
	};

typedef struct					/* head table */
	{
	Fixed version;				/* =1.0 */
	Fixed fontRevision;
	unsigned long checkSumAdjustment;
	unsigned long magicNumber;
	unsigned short flags;
	unsigned short unitsPerEm;
	char created[8];
	char modified[8];
	FWord xMin;
	FWord yMin;
	FWord xMax;
	FWord yMax;
	unsigned short macStyle;
	unsigned short lowestRecPPEM;
	short fontDirectionHint;
	short indexToLocFormat;
	short glyphDataFormat;
	} headTbl;

typedef struct					/* maxp table */
	{
	Fixed version;				/* =1.0 */
	unsigned short numGlyphs;
	unsigned short maxPoints;
	unsigned short maxContours;
	unsigned short maxCompositePoints;
	unsigned short maxCompositeContours;
	unsigned short maxZones;
	unsigned short maxTwilightPoints;
	unsigned short maxStorage;
	unsigned short maxFunctionDefs;
	unsigned short maxInstructionDefs;
	unsigned short maxStackElements;
	unsigned short maxSizeOfInstructions;
	unsigned short maxComponentElements;
	unsigned short maxComponentDepth;
	} maxpTbl;

typedef struct					/* hhea table */
	{
	Fixed version;				/* =1.0 */
	FWord ascender;
	FWord descender;
	FWord lineGap;
	uFWord advanceWidthMax;
	FWord minLeftSideBearing;
	FWord minRightSideBearing;
	FWord xMaxExtent;
	short caretSlopeRise;
	short caretSlopeRun;
	short caretOffset;
	short reserved[4];
	short metricDataFormat;
	unsigned short numberOfLongHorMetrics;
	} hheaTbl;

typedef struct					/* name record */
	{
	unsigned short platformId;
	unsigned short platspecId;
	unsigned short languageId;
	unsigned short nameId;
	unsigned short length;
	Offset offset;
	} NameRecord;

typedef struct					/* name table */
	{
	unsigned short format;		/* =0 */
	unsigned short count;
	unsigned short stringOffset;
	dnaDCL(NameRecord, records);
	} nameTbl;

enum							/* Registered name ids */
	{
	name_COPYRIGHT,
	name_FAMILY,
	name_SUBFAMILY,
	name_UNIQUE,
	name_FULL,
	name_VERSION,
	name_FONTNAME,
	name_TRADEMARK
	};

typedef struct					/* glyf table coordinate */
	{
	short x;
	short y;
	short flags;
#define glyf_ONCURVE		(1<<0)
#define glyf_XSHORT			(1<<1)
#define glyf_YSHORT			(1<<2)
#define glyf_REPEAT			(1<<3)
#define glyf_SHORT_X_IS_POS	(1<<4)
#define glyf_NEXT_X_IS_ZERO	(1<<4)
#define glyf_SHORT_Y_IS_POS	(1<<5)
#define glyf_NEXT_Y_IS_ZERO	(1<<5)
	} glyfCoord;

/* Compound glyph component flags */
#define glyf_ARG_1_AND_2_ARE_WORDS		(1<<0)
#define glyf_ARGS_ARE_XY_VALUES			(1<<1)
#define glyf_ROUND_XY_TO_GRID			(1<<2)
#define glyf_WE_HAVE_A_SCALE			(1<<3)
#define glyf_NON_OVERLAPPING			(1<<4)	/* Apparently obsolete */
#define glyf_MORE_COMPONENTS			(1<<5)
#define glyf_WE_HAVE_AN_X_AND_Y_SCALE	(1<<6)
#define glyf_WE_HAVE_A_TWO_BY_TWO		(1<<7)
#define glyf_WE_HAVE_INSTRUCTIONS		(1<<8)
#define glyf_USE_MY_METRICS				(1<<9)

typedef struct					/* glyf table */
	{
	dnaDCL(unsigned short, endPts);
	dnaDCL(glyfCoord, coords);
	long offset;
	} glyfTbl;

typedef struct
	{
	unsigned short platformId;
	unsigned short platspecId;
	unsigned long offset;
	unsigned short format;		/* From subtable */
	unsigned short length;		/* From subtable */
	unsigned short languageId;	/* From subtable; usually named version */
	} EncRec;
#define ENC_SUBTABLE_HDR_SIZE	(3*2)

typedef struct					/* Format 4 subtable segment */
	{
	unsigned short endCode;
	unsigned short startCode;
	short idDelta;
	long idRangeOffset;
	} Segment4;

typedef struct					/* cmap table */
	{
	unsigned short version;
	unsigned short nEncodings;
	dnaDCL(EncRec, encodings);
	dnaDCL(Segment4, segments);	/* Format 4 subtable segments */
	} cmapTbl;
#define ENC_HDR_SIZE	(2+2+2)

typedef struct					/* Format 2.0 data */
	{
	dnaDCL(unsigned short, glyphNameIndex);
	dnaDCL(char *, strings);
	dnaDCL(char, buf);
	} postFmt2;

typedef struct					/* post table */
	{
	Fixed format;				/* =1.0, 2.0, 2.5, 3.0, 4.0 */
	Fixed italicAngle;
	FWord underlinePosition;
	FWord underlineThickness;
	unsigned long isFixedPitch;
	unsigned long minMemType42;
	unsigned long maxMemType42;
	unsigned long minMemType1;
	unsigned long maxMemType1;
	postFmt2 fmt2;				
	} postTbl;

#define POST_HDR_SIZE	(2*4+2*2+5*4)

typedef struct					/* OS/2 table */
	{
	unsigned short version;		/* =0, 1, 2 */
	short xAvgCharWidth;
	unsigned short usWeightClass;
	unsigned short usWidthClass;
	unsigned short fsType;
	short ySubscriptXSize;
	short ySubscriptYSize;
	short ySubscriptXOffset;
	short ySubscriptYOffset;
	short ySuperscriptXSize;
	short ySuperscriptYSize;
	short ySuperscriptXOffset;
	short ySuperscriptYOffset;
	short yStrikeoutSize;
	short yStrikeoutPosition;
	short sFamilyClass;
	char panose[10];
	unsigned long ulUnicodeRange1;
	unsigned long ulUnicodeRange2;
	unsigned long ulUnicodeRange3;
	unsigned long ulUnicodeRange4;
	char achVendId[4];
	unsigned short fsSelection;
	unsigned short usFirstCharIndex;
	unsigned short usLastCharIndex;
	short sTypoAscender;
	short sTypoDescender;
	short sTypoLineGap;
	unsigned short usWinAscent;
	unsigned short usWinDescent;
	unsigned long ulCodePageRange1;	/* Version 1 */
	unsigned long ulCodePageRange2;	/* Version 1 */
	short sXHeight;					/* Version 2 */
	short sCapHeight;				/* Version 2 */
	unsigned short usDefaultChar;	/* Version 2 */
	unsigned short usBreakChar;		/* Version 2 */
	unsigned short usMaxContext;	/* Version 2 */
	} OS_2Tbl;

/* Glyph names for Standard Apple Glyph Ordering */
static char *applestd[258] =
	{
#include "applestd.h"
	};

/* ---------------------------- Library Context ---------------------------- */

typedef struct				/* Glyph data */
	{
	uFWord hAdv;			/* Horizontal advance */
	FWord xMin;				/* Left of bounding box */
	FWord lsb;				/* Left sidebearing */
	abfGlyphInfo info;		/* Client glyph info */
	} Glyph;

struct ttrCtx_
	{
	long flags;				/* Status flags */
#define ENCODING_ASSIGNED	(1<<0)	/* Font has been assigned an encoding */
#define GID_NAMES_ASSIGNED	(1<<1)	/* GID names assigned; resort needed */
	long client_flags;		/* Copy of client flags argument */
	abfTopDict top;
	abfFontDict fdict;
	maxpTbl maxp;			/* maxp table */
	headTbl head;			/* head table */
	hheaTbl hhea;			/* hhea table */
	nameTbl name;			/* name table */
	glyfTbl glyf;			/* glyf table */
	cmapTbl cmap;			/* cmap table */
	postTbl post;			/* post table */
	OS_2Tbl OS_2;			/* OS/2 table */
	dnaDCL(Glyph, glyphs);	/* Glyph data */
	dnaDCL(GID, glyphsByName);	/* Glyphs sorted by name */
	dnaDCL(Encoding, encodings);/* Selected encoding */
	long unnamed;			/* Number of unnamed glyphs */
	struct					/* String data */
		{
		dnaDCL(long, index);
		dnaDCL(char, buf);
		} strings;
	dnaDCL(char, tmp0);		/* Temporary buffer 0 */
	dnaDCL(char, tmp1);		/* Temporary buffer 1 */
	struct					/* Streams */
		{
		void *src;
		void *dbg;
		} stm;
	struct					/* Source stream */
		{
		long offset;		/* Buffer offset */
		char *buf;			/* Buffer data */
		size_t length;		/* Buffer length */
		char *end;			/* Buffer end */
		char *next;			/* Next byte available */
		} src;
	struct					/* Client callbacks */
		{
		ctlMemoryCallbacks mem;
		ctlStreamCallbacks stm;
		} cb;
	struct					/* Contexts */
		{
		dnaCtx dna;			/* dynarr */
		sfrCtx sfr;			/* sfntread */
		} ctx;
	struct					/* Error handling */
		{
		jmp_buf env;
		int code;
		} err;
	};

/* ----------------------------- Error Handling ---------------------------- */

/* Write message to debug stream from va_list. */
static void vmessage(ttrCtx h, char *fmt, va_list ap)
	{
	char text[500];

	if (h->stm.dbg == NULL)
		return;	/* Debug stream not available */

	vsprintf(text, fmt, ap);
	(void)h->cb.stm.write(&h->cb.stm, h->stm.dbg, strlen(text), text);
	}

/* Write message to debug stream from varargs. */
static void CTL_CDECL message(ttrCtx h, char *fmt, ...)
	{
	va_list ap;
	va_start(ap, fmt);
	vmessage(h, fmt, ap);
	va_end(ap);
	}

/* Handle fatal error. */
static void CTL_CDECL fatal(ttrCtx h, int err_code, char *fmt, ...)
	{
	if (fmt == NULL)
		/* Write standard error message */
		message(h, "%s", ttrErrStr(err_code));
	else
		{
		/* Write font-specific error message */
		va_list ap;
		va_start(ap, fmt);
		vmessage(h, fmt, ap);
		va_end(ap);
		}
	h->err.code = err_code;
	longjmp(h->err.env, 1);
	}

/* ---------------------- Error Handling dynarr Context -------------------- */

/* Manage memory. */
static void *dna_manage(ctlMemoryCallbacks *cb, void *old, size_t size)
	{
	ttrCtx h = cb->ctx;
	void *ptr = h->cb.mem.manage(&h->cb.mem, old, size);
	if (size > 0 && ptr == NULL)
		fatal(h, ttrErrNoMemory, NULL);
	return ptr;
	}

/* Initialize error handling dynarr context. */
static void dna_init(ttrCtx h)
	{
	ctlMemoryCallbacks cb;
	cb.ctx 		= h;
	cb.manage 	= dna_manage;
	h->ctx.dna		= dnaNew(&cb, DNA_CHECK_ARGS);
	}

/* --------------------------- Context Management -------------------------- */

/* Validate client and create context. */
ttrCtx ttrNew(ctlMemoryCallbacks *mem_cb, ctlStreamCallbacks *stm_cb,
			  CTL_CHECK_ARGS_DCL)
	{
	ttrCtx h;

	/* Check client/library compatibility */
	if (CTL_CHECK_ARGS_TEST(TTR_VERSION))
		return NULL;

	/* Allocate context */
	h = mem_cb->manage(mem_cb, NULL, sizeof(struct ttrCtx_));
	if (h == NULL)
		return NULL;

	/* Safety initialization */
	h->name.records.size = 0;
	h->cmap.encodings.size = 0;
	h->cmap.segments.size = 0;
	h->post.fmt2.glyphNameIndex.size = 0;
	h->post.fmt2.strings.size = 0;
	h->post.fmt2.buf.size = 0;
	h->glyphs.size = 0;
	h->glyphsByName.size = 0;
	h->encodings.size = 0;
	h->strings.index.size = 0;
	h->strings.buf.size = 0;
	h->glyf.endPts.size = 0;
	h->glyf.coords.size = 0;
	h->tmp0.size = 0;
	h->tmp1.size = 0;
	h->stm.dbg = NULL;
	h->ctx.dna = NULL;
	h->ctx.sfr = NULL;

	/* Copy callbacks */
	h->cb.mem = *mem_cb;
	h->cb.stm = *stm_cb;

	if (setjmp(h->err.env))
		goto cleanup;

	/* Initialize service libraries */
	dna_init(h);
	h->ctx.sfr = sfrNew(mem_cb, stm_cb, SFR_CHECK_ARGS);
	if (h->ctx.sfr == NULL)
		fatal(h, ttrErrSfntread, "(sfr) can't init lib");

	dnaINIT(h->ctx.dna, h->name.records, 50, 50);
	dnaINIT(h->ctx.dna, h->cmap.encodings, 10, 10);
	dnaINIT(h->ctx.dna, h->cmap.segments, 30, 60);
	dnaINIT(h->ctx.dna, h->post.fmt2.glyphNameIndex, 256, 768);
	dnaINIT(h->ctx.dna, h->post.fmt2.strings, 50, 200);
	dnaINIT(h->ctx.dna, h->post.fmt2.buf, 300, 1200);
	dnaINIT(h->ctx.dna, h->glyphs, 256, 768);
	dnaINIT(h->ctx.dna, h->glyphsByName, 256, 768);
	dnaINIT(h->ctx.dna, h->encodings, 256, 768);
	dnaINIT(h->ctx.dna, h->strings.index, 250, 500);
	dnaINIT(h->ctx.dna, h->strings.buf, 1000, 2000);
	dnaINIT(h->ctx.dna, h->glyf.endPts, 10, 20);
	dnaINIT(h->ctx.dna, h->glyf.coords, 500, 1000);
	dnaINIT(h->ctx.dna, h->tmp0, 200, 500);
	dnaINIT(h->ctx.dna, h->tmp1, 200, 500);

	/* Open debug stream */
	h->stm.dbg = h->cb.stm.open(&h->cb.stm, TTR_DBG_STREAM_ID, 0);

	return h;

 cleanup:
	/* Initilization failed */
	ttrFree(h);
	return NULL;
	}

/* Free context. */
void ttrFree(ttrCtx h)
	{
	if (h == NULL)
		return;

	dnaFREE(h->name.records);
	dnaFREE(h->cmap.encodings);
	dnaFREE(h->cmap.segments);
	dnaFREE(h->post.fmt2.glyphNameIndex);
	dnaFREE(h->post.fmt2.strings);
	dnaFREE(h->post.fmt2.buf);
	dnaFREE(h->glyphs);
	dnaFREE(h->glyphsByName);
	dnaFREE(h->encodings);
	dnaFREE(h->strings.index);
	dnaFREE(h->strings.buf);
	dnaFREE(h->glyf.endPts);
	dnaFREE(h->glyf.coords);
	dnaFREE(h->tmp0);
	dnaFREE(h->tmp1);

	dnaFree(h->ctx.dna);
	sfrFree(h->ctx.sfr);

	/* Close debug stream */
	if (h->stm.dbg != NULL)
		(void)h->cb.stm.close(&h->cb.stm, h->stm.dbg);

	/* Free library context */
	h->cb.mem.manage(&h->cb.mem, h, 0);
	}

/* ----------------------------- Source Stream ----------------------------- */

/* Fill source buffer. */
static void fillbuf(ttrCtx h, long offset)
	{
	h->src.length = h->cb.stm.read(&h->cb.stm, h->stm.src, &h->src.buf);
	if (h->src.length == 0)
		fatal(h, ttrErrSrcStream, NULL);
	h->src.offset = offset;
	h->src.next = h->src.buf;
	h->src.end = h->src.buf + h->src.length;
	}

/* Refill source buffer. */
static char nextbuf(ttrCtx h)
	{
	/* 64-bit warning fixed by cast here HO */
	fillbuf(h, (long)(h->src.offset + h->src.length));
	return *h->src.next++;
	}

/* Seek to specified offset. */
static void srcSeek(ttrCtx h, long offset)
	{
	long delta = offset - h->src.offset;
	if (delta >= 0 && (size_t)delta < h->src.length)
		/* Offset within current buffer; reposition next byte */
		h->src.next = h->src.buf + delta;
	else 
		{
		/* Offset outside current buffer; seek to offset and fill buffer */
		if (h->cb.stm.seek(&h->cb.stm, h->stm.src, offset))
			fatal(h, ttrErrSrcStream, NULL);
		fillbuf(h, offset);
		}
	}

/* Return absolute byte position in stream. */
#define srcTell(h)	(h->src.offset + (h->src.next - h->src.buf))

/* Read buffer. */
static void srcRead(ttrCtx h, size_t cnt, char *buf)
	{
	size_t left = h->src.end - h->src.next;

	while (left < cnt)
		{
		/* Copy buffer */
		memmove(buf, h->src.next, left);
		buf += left;
		cnt -= left;

		/* Refill buffer */
		/* 64-bit warning fixed by cast here HO */
		fillbuf(h, (long)(h->src.offset + h->src.length));
		left = h->src.length;
		}
		
	memmove(buf, h->src.next, cnt);
	h->src.next += cnt;
	}

/* Read 1-byte unsigned number. */
#define read1(h) \
	((unsigned char)((h->src.next==h->src.end)?nextbuf(h):*h->src.next++))

/* Read 2-byte unsigned number. */
static unsigned short read2(ttrCtx h)
	{
	unsigned short value = (unsigned short)read1(h)<<8;
	return value | (unsigned short)read1(h);
	}

/* Read 2-byte signed number. */
static short sread2(ttrCtx h)
	{
	unsigned short value = (unsigned short)read1(h)<<8;
	value |= (unsigned short)read1(h);
#if SHRT_MAX == 32767
	return (short)value;
#else
	return (short)((value > 32767)? value - 65536: value);
#endif
	}

/* Read 4-byte unsigned number. */
static unsigned long read4(ttrCtx h)
	{
	unsigned long value = (unsigned long)read1(h)<<24;
	value |= (unsigned long)read1(h)<<16;
	value |= (unsigned long)read1(h)<<8;
	return value | (unsigned long)read1(h);
	}

/* Read 4-byte signed number. */
static long sread4(ttrCtx h)
	{
	unsigned long value = (unsigned long)read1(h)<<24;
	value |= (unsigned long)read1(h)<<16;
	value |= (unsigned long)read1(h)<<8;
	value |= (unsigned long)read1(h);
#if LONG_MAX == 2147483647
	return (long)value;
#else
	return (long)((value > 2417483647)? value - 4294967296: value);
#endif
	}

/* ---------------------------- TrueType Reading --------------------------- */

/* Read sfnt header. */
static void sfntReadHdr(ttrCtx h, long origin, int iTTC)
	{
	int i;
	int result;
	ctlTag sfnt_tag;
	long offset = origin;

 readhdr:
	/* Read sfnt header */
	result = sfrBegFont(h->ctx.sfr, h->stm.src, offset, &sfnt_tag);
	if (result)
		fatal(h, ttrErrSfntread, "(sfr) %s", sfrErrStr(result));

	switch (sfnt_tag)
		{
	case sfr_ttcf_tag:
		/* Get i'th TrueType Collection offset */
		for (i = 0; (offset = sfrGetNextTTCOffset(h->ctx.sfr)); i++)
			if (i == iTTC)
				{
				iTTC = 0;
				goto readhdr;
				}
		fatal(h, ttrErrBadCall, NULL);
	case sfr_v1_0_tag:
	case sfr_true_tag:
		if (iTTC != 0)
			fatal(h, ttrErrBadCall, NULL);
		break;
	default:
		fatal(h, ttrErrFontType, NULL);
		}
	}

/* Read head table. Must be first table reading function to be called. */
static void headRead(ttrCtx h)
	{	
	sfrTable *table = sfrGetTableByTag(h->ctx.sfr, CTL_TAG('h','e','a','d'));
	if (table == NULL)
		fatal(h, ttrErrNoHead, NULL);

	/* Force first read */
	if (h->cb.stm.seek(&h->cb.stm, h->stm.src, table->offset))
		fatal(h, ttrErrSrcStream, NULL);
	fillbuf(h, table->offset);

	/* Read and validate table version */
	h->head.version = read4(h);
	if (h->head.version != sfr_v1_0_tag)
		message(h, "invalid head table version");
	
	/* Read rest of table */
	h->head.fontRevision		= read4(h);
	h->head.checkSumAdjustment	= read4(h);
	h->head.magicNumber			= read4(h);
	h->head.flags 				= read2(h);
	h->head.unitsPerEm 			= read2(h);
	srcRead(h, sizeof(h->head.created), h->head.created);
	srcRead(h, sizeof(h->head.modified), h->head.modified);
	h->head.xMin 				= read2(h);
	h->head.yMin 				= read2(h);
	h->head.xMax 				= read2(h);
	h->head.yMax 				= read2(h);
	h->head.macStyle 			= read2(h);
	h->head.lowestRecPPEM 		= read2(h);
	h->head.fontDirectionHint 	= read2(h);
	h->head.indexToLocFormat 	= read2(h);
	h->head.glyphDataFormat 	= read2(h);
	}

/* Read hhea table. */
static void hheaRead(ttrCtx h)
	{
	sfrTable *table = sfrGetTableByTag(h->ctx.sfr, CTL_TAG('h','h','e','a'));
	if (table == NULL)
		fatal(h, ttrErrNoHhea, NULL);
	srcSeek(h, table->offset);

	/* Read and validate table version */
	h->hhea.version = read4(h);
	if (h->hhea.version != sfr_v1_0_tag)
		message(h, "invalid hhea table version");
	
	/* Read rest of table */
	h->hhea.ascender 				= read2(h);
	h->hhea.descender 				= read2(h);
	h->hhea.lineGap 				= read2(h);
	h->hhea.advanceWidthMax 		= read2(h);
	h->hhea.minLeftSideBearing 		= read2(h);
	h->hhea.minRightSideBearing 	= read2(h);
	h->hhea.xMaxExtent 				= read2(h);
	h->hhea.caretSlopeRise 			= read2(h);
	h->hhea.caretSlopeRun 			= read2(h);
	h->hhea.caretOffset 			= read2(h);
	h->hhea.reserved[0] 			= read2(h);
	h->hhea.reserved[1] 			= read2(h);
	h->hhea.reserved[2] 			= read2(h);
	h->hhea.reserved[3] 			= read2(h);
	h->hhea.metricDataFormat		= read2(h);
	h->hhea.numberOfLongHorMetrics	= read2(h);
	}

/* Read maxp table. */
static void maxpRead(ttrCtx h)
	{
	sfrTable *table = sfrGetTableByTag(h->ctx.sfr, CTL_TAG('m','a','x','p'));
	if (table == NULL)
		fatal(h, ttrErrNoMaxp, NULL);
	srcSeek(h, table->offset);

	/* Read and validate table version */
	h->maxp.version = read4(h);
	if (h->maxp.version != sfr_v1_0_tag)
		message(h, "invalid maxp table version");
	
	/* Read rest of table */
	h->maxp.numGlyphs 				= read2(h);
	h->maxp.maxPoints 				= read2(h);
	h->maxp.maxContours 			= read2(h);
	h->maxp.maxCompositePoints 		= read2(h);
	h->maxp.maxCompositeContours	= read2(h);
	h->maxp.maxZones 				= read2(h);
	h->maxp.maxTwilightPoints 		= read2(h);
	h->maxp.maxStorage 				= read2(h);
	h->maxp.maxFunctionDefs 		= read2(h);
	h->maxp.maxInstructionDefs 		= read2(h);
	h->maxp.maxStackElements 		= read2(h);
	h->maxp.maxSizeOfInstructions	= read2(h);
	h->maxp.maxComponentElements	= read2(h);
	h->maxp.maxComponentDepth		= read2(h);
	}

/* Read loca table. */
static void locaRead(ttrCtx h)
	{
	long i;
	Offset begin = 0;	/* Suppress optimizer warning */
	Offset end = 0;	/* Suppress optimizer warning */
	sfrTable *table = sfrGetTableByTag(h->ctx.sfr, CTL_TAG('l','o','c','a'));
	if (table == NULL)
		fatal(h, ttrErrNoLoca, NULL);
	srcSeek(h, table->offset);

	/* Read first offset */
	switch (h->head.indexToLocFormat)
		{
	case 0:
		begin = 2*read2(h);
		break;
	case 1:
		begin = read4(h);
		break;
	default:
		fatal(h, ttrErrLocaFormat, NULL);
		}

	/* Read offset array (note there are numGlyphs+1 offsets) */
	for (i = 0; i < h->glyphs.cnt; i++)
		{
		end = (h->head.indexToLocFormat == 0)? 2*read2(h): read4(h);
		if (end > begin)
			{
			abfGlyphInfo *info = &h->glyphs.array[i].info;
			info->sup.begin = begin;
			info->sup.end = end;
			}
		begin = end;
		}
	}

/* Read hmtx table. */
static void hmtxRead(ttrCtx h)
	{
	uFWord last;
	Glyph *glyph = NULL;	/* Suppress optimizer warning */
	long i;
	sfrTable *table = sfrGetTableByTag(h->ctx.sfr, CTL_TAG('h','m','t','x'));
	if (table == NULL)
		fatal(h, ttrErrNoHmtx, NULL);
	srcSeek(h, table->offset);

	/* Read long horizontal metrics */
	for (i = 0; i < h->hhea.numberOfLongHorMetrics; i++)
		{
		glyph = &h->glyphs.array[i];
		glyph->hAdv = read2(h);
		glyph->lsb = sread2(h);
		}
	last = glyph->hAdv;

	/* Read left sidebearings */
	for (; i < h->glyphs.cnt; i++)
		{
		glyph = &h->glyphs.array[i];
		glyph->hAdv = last;
		glyph->lsb	= sread2(h);
		}
	}

/* Read name table. */
static void nameRead(ttrCtx h)
	{
	int i;
	sfrTable *table = sfrGetTableByTag(h->ctx.sfr, CTL_TAG('n','a','m','e'));
	if (table == NULL)
		{
		message(h, "name table missing");
		h->name.records.cnt = 0;
		return;
		}
	srcSeek(h, table->offset);

	/* Read and validate table format */
	h->name.format = read2(h);
	if (h->name.format != 0)
		message(h, "invalid name table format");

	/* Read rest of header */
	h->name.count			= read2(h);
	h->name.stringOffset 	= read2(h);

	/* Read name records */
	dnaSET_CNT(h->name.records, h->name.count);
	for (i = 0; i < h->name.records.cnt; i++)
		{
		NameRecord *rec = &h->name.records.array[i];
		rec->platformId	= read2(h);
		rec->platspecId = read2(h);
		rec->languageId = read2(h);
		rec->nameId 	= read2(h);
		rec->length 	= read2(h);
		rec->offset 	= table->offset + h->name.stringOffset + read2(h);
		}
	}

/* Match ids. */
static int CTL_CDECL matchIds(const void *key, const void *value)
	{
	MatchIds *a = (MatchIds *)key;
	NameRecord *b = (NameRecord *)value;
	if (a->platformId < b->platformId)
		return -1;
	else if (a->platformId > b->platformId)
		return 1;
	else if (a->platspecId < b->platspecId)
		return -1;
	else if (a->platspecId > b->platspecId)
		return 1;
	else if (a->languageId < b->languageId)
		return -1;
	else if (a->languageId > b->languageId)
		return 1;
	else if (a->nameId < b->nameId)
		return -1;
	else if (a->nameId > b->nameId)
		return 1;
	else 
		return 0;
	}

/* Return name record match specified ids or NULL if no match. */
static NameRecord *nameFind(ttrCtx h,
							unsigned short platformId,
							unsigned short platspecId,
							unsigned short languageId,
							unsigned short nameId)
	{
	MatchIds match;

	if (h->name.records.cnt == 0)
		return NULL;	/* name table missing */

	/* Initialize match record */
	match.platformId	= platformId;
	match.platspecId	= platspecId;
	match.languageId	= languageId;
	match.nameId		= nameId;
	
	return (NameRecord *)
		bsearch(&match, h->name.records.array, h->name.records.cnt, 
				sizeof(NameRecord), matchIds); 
	}

static int invalidStreamOffset(ttrCtx h, unsigned long offset)
	{
	h->src.length = 0;	/* To force re-read after this test */
	h->cb.stm.seek(&h->cb.stm, h->stm.src, offset);
	return h->cb.stm.read(&h->cb.stm, h->stm.src, &h->src.buf) == 0;
	}

/* Read post table. */
static void postRead(ttrCtx h)
	{
	enum { POST_HEADER_SIZE = 4*7 + 2*2 };
	char *p;
	char *end;
	long i;
	long length;
	long numGlyphs;
	sfrTable *table = sfrGetTableByTag(h->ctx.sfr, CTL_TAG('p','o','s','t'));
	if (table == NULL)
		return;	/* Optional table missing */

	if (invalidStreamOffset(h, table->offset + POST_HEADER_SIZE - 1))
		{
		message(h, "post: header outside stream bounds");
		return;
		}

	srcSeek(h, table->offset);

	h->post.format				= read4(h);
	h->post.italicAngle			= sread4(h);
	h->post.underlinePosition	= sread2(h);
	h->post.underlineThickness	= sread2(h);
	h->post.isFixedPitch		= read4(h);
	h->post.minMemType42		= read4(h);
	h->post.maxMemType42		= read4(h);
	h->post.minMemType1			= read4(h);
	h->post.maxMemType1			= read4(h);

	if (h->post.format != 0x00020000)
		return;	/* Successfully read header */

	if (invalidStreamOffset(h, table->offset + table->length - 1))
		{
		message(h, "post: table truncated");
		goto parseError;
		}

	srcSeek(h, table->offset + POST_HEADER_SIZE);

	/* Parse format 2.0 data */
	numGlyphs = read2(h);
	if (numGlyphs != h->maxp.numGlyphs)
		message(h, "post 2.0: name index size doesn't match numGlyphs");

	/* Validate table length against size of glyphNameIndex */
	length = table->length - (srcTell(h) - table->offset);
	if (length < numGlyphs * 2)
		{
		message(h, "post 2.0: table truncated (table ignored)");
		goto parseError;
		}

	/* Read name index */
	dnaSET_CNT(h->post.fmt2.glyphNameIndex, numGlyphs);
	h->post.fmt2.strings.cnt = 0;
	for (i = 0; i < numGlyphs; i++)
		{
		unsigned short nid = read2(h);
		h->post.fmt2.glyphNameIndex.array[i] = nid;
		if (nid > 32767)
			{
			message(h, "post 2.0: invalid name id (table ignored)");
			goto parseError;
			}
		else if (nid > 257)
			h->post.fmt2.strings.cnt++;
		}
	
	/* Read string data */
	length = table->length - (srcTell(h) -  table->offset);
	dnaSET_CNT(h->post.fmt2.buf, length + 1);
	srcRead(h, length, h->post.fmt2.buf.array);

	/* Build C strings array */
	dnaSET_CNT(h->post.fmt2.strings, h->post.fmt2.strings.cnt);
	p = h->post.fmt2.buf.array;
	end = p + length;
	i = 0;
	for (i = 0; i < h->post.fmt2.strings.cnt; i++)
		{
		length = *(unsigned char *)p;
		*p++ = '\0';
		h->post.fmt2.strings.array[i] = p;
		p += length;
		if (p > end)
			{
			message(h, "post 2.0: invalid strings");
			goto parseError;
			}
		}
	*p = '\0';
	if (p != end)
		message(h, "post 2.0: string data didn't reach end of table");
	
	return;	/* Success */
	
parseError:
	/* We managed to read the header but the rest of the table had an error that
	   prevented reading some (or all) glyph names. We set the the post format
	   to a value that will allow us to use the header values but will prevent
	   us from using any glyph name data which is likely missing or invalid */
	h->post.format = 0x00000001;
	}

/* Get glyph name from format 2.0 post table. */
static char *post2GetName(ttrCtx h, GID gid)
	{
	if (gid >= h->post.fmt2.glyphNameIndex.cnt)
		return NULL;	/* Out of bounds; .notdef */
	else
		{
		long nid = h->post.fmt2.glyphNameIndex.array[gid];
		if (nid == 0)
			return NULL;	/* .notdef */
		else if (nid < 258)
			return applestd[nid];
		else if (nid - 258 >= h->post.fmt2.strings.cnt)
			return NULL;	/* Out of bounds; .notdef */
		else
			return h->post.fmt2.strings.array[nid - 258];
		}
	}

/* Read OS/2 table. */
static void OS_2Read(ttrCtx h)
	{
	sfrTable *table = sfrGetTableByTag(h->ctx.sfr, CTL_TAG('O','S','/','2'));
	if (table == NULL)
		return;	/* Optional table missing */
	srcSeek(h, table->offset);
	
	h->OS_2.version 			= read2(h);
	h->OS_2.xAvgCharWidth 		= sread2(h);
	h->OS_2.usWeightClass 		= read2(h);
	h->OS_2.usWidthClass 		= read2(h);
	h->OS_2.fsType 				= read2(h);
	h->OS_2.ySubscriptXSize 	= sread2(h);
	h->OS_2.ySubscriptYSize 	= sread2(h);
	h->OS_2.ySubscriptXOffset 	= sread2(h);
	h->OS_2.ySubscriptYOffset 	= sread2(h);
	h->OS_2.ySuperscriptXSize 	= sread2(h);
	h->OS_2.ySuperscriptYSize 	= sread2(h);
	h->OS_2.ySuperscriptXOffset = sread2(h);
	h->OS_2.ySuperscriptYOffset = sread2(h);
	h->OS_2.yStrikeoutSize 		= sread2(h);
	h->OS_2.yStrikeoutPosition 	= sread2(h);
	h->OS_2.sFamilyClass 		= sread2(h);
	srcRead(h, sizeof(h->OS_2.panose), h->OS_2.panose);
	h->OS_2.ulUnicodeRange1 	= read4(h);
	h->OS_2.ulUnicodeRange2 	= read4(h);
	h->OS_2.ulUnicodeRange3 	= read4(h);
	h->OS_2.ulUnicodeRange4 	= read4(h);
	srcRead(h, sizeof(h->OS_2.achVendId), h->OS_2.achVendId);
	h->OS_2.fsSelection 		= read2(h);
	h->OS_2.usFirstCharIndex 	= read2(h);
	h->OS_2.usLastCharIndex 	= read2(h);
	h->OS_2.sTypoAscender 		= sread2(h);
	h->OS_2.sTypoDescender 		= sread2(h);
	h->OS_2.sTypoLineGap 		= sread2(h);
	h->OS_2.usWinAscent 		= read2(h);
	h->OS_2.usWinDescent 		= read2(h);
	if (h->OS_2.version < 1)
		return;
	h->OS_2.ulCodePageRange1	= read4(h);
	h->OS_2.ulCodePageRange2	= read4(h);
	if (h->OS_2.version < 2)
		return;
	h->OS_2.sXHeight			= sread2(h);
	h->OS_2.sCapHeight			= sread2(h);
	h->OS_2.usDefaultChar		= read2(h);
	h->OS_2.usBreakChar			= read2(h);
	h->OS_2.usMaxContext		= read2(h);
	}

/* Read cmap table. */
static void cmapRead(ttrCtx h)
	{
	int i;
	EncRec *enc;
	sfrTable *table = sfrGetTableByTag(h->ctx.sfr, CTL_TAG('c','m','a','p'));
	if (table == NULL)
		{
		message(h, "cmap table missing");
		h->cmap.encodings.cnt = 0;
		return;
		}
	srcSeek(h, table->offset);

	/* Read and validate table format */
	h->cmap.version = read2(h);
	if (h->cmap.version != 0)
		message(h, "invalid cmap table version");

	/* Read rest of header */
	h->cmap.nEncodings = read2(h);

	/* Read cmap records */
	dnaSET_CNT(h->cmap.encodings, h->cmap.nEncodings);
	for (i = 0; i < h->cmap.encodings.cnt; i++)
		{
		enc = &h->cmap.encodings.array[i];
		enc->platformId	= read2(h);
		enc->platspecId = read2(h);
		enc->offset 	= read4(h) + table->offset;
		}

	/* Add subtable information */
	for (i = 0; i < h->cmap.encodings.cnt; i++)
		{
		enc = &h->cmap.encodings.array[i];
		srcSeek(h, enc->offset);
		enc->format		= read2(h);
		enc->length		= read2(h);
		enc->languageId = read2(h);
		}
	}

/* Add encoding to glyph. */
static void encAdd(ttrCtx h, GID gid, unsigned short code)
	{
	if (gid >= h->glyphs.cnt)
		message(h, "encoding for nonexistent glyph (ignored)");
	else
		{
		Encoding *enc = dnaNEXT(h->encodings);
		enc->code = code;
		enc->gid = gid;
		}
	}

/* Read format 0 encoding subtable. */
static void cmapReadFmt0(ttrCtx h)
	{
	unsigned short code;
	unsigned char gids[256];
	srcRead(h, 256, (char *)gids);
	for (code = 0; code < 256; code++)
		encAdd(h, gids[code], code);
	}

/* Read format 4 encoding subtable. */
static void cmapReadFmt4(ttrCtx h)
	{
	long offset;
	long i;
    long nSegments = read2(h)/2;

	/* Skip binary search fields */
	(void)read2(h);		/* searchRange */
	(void)read2(h);		/* entrySelector */
	(void)read2(h);		/* rangeShift */

	/* Read segment arrays */
	dnaSET_CNT(h->cmap.segments, nSegments);
	for (i = 0; i < nSegments; i++)
		h->cmap.segments.array[i].endCode = read2(h);
	(void)read2(h);		/* password */
	for (i = 0; i < nSegments; i++)
		h->cmap.segments.array[i].startCode = read2(h);
	for (i = 0; i < nSegments; i++)
		h->cmap.segments.array[i].idDelta = sread2(h);
	offset = srcTell(h);
	for (i = 0; i < nSegments; i++)
		{
		unsigned short idRangeOffset = read2(h);
		if (idRangeOffset == 0xffff)
			{
			idRangeOffset = 0;	/* Fix Fontographer bug */
			message(h, "cmap: invalid idRangeOffset in segment[%d] (fixed)", i);
			}
		h->cmap.segments.array[i].idRangeOffset = 
			(idRangeOffset == 0)? 0: offset + idRangeOffset;
		offset += 2;
		}

	/* Process segments */
	for (i = 0; i < nSegments; i++)
		{
		unsigned short gid;
		unsigned long code;
		Segment4 *seg = &h->cmap.segments.array[i];

		if (seg->idRangeOffset == 0)
			{
			gid = seg->idDelta + seg->startCode;
			for (code = seg->startCode; code <= seg->endCode; code++)
				{
				if (code == 0xffff)
					break;
				else if (gid != 0)
					encAdd(h, gid, (unsigned short)code);
				gid++;
				}
			}
		else
			{
			srcSeek(h, seg->idRangeOffset);
			for (code = seg->startCode; code <= seg->endCode; code++)
				{
				gid = read2(h);
				if (code != 0xffff && gid != 0)
					encAdd(h, gid, (unsigned short)code);
				}
			}
		}
	}

/* Read format 6 encoding subtable. */
static void cmapReadFmt6(ttrCtx h)
	{
	unsigned short code;
	unsigned short firstCode = read2(h);
	unsigned short endCode = firstCode + read2(h);
	for (code = firstCode; code < endCode; code++)
		encAdd(h, read2(h), code);
	}

/* Compare encodings. */
static int CTL_CDECL cmpEncs(const void *first, const void *second)
	{
	unsigned short a = ((Encoding *)first)->code;
	unsigned short b = ((Encoding *)second)->code;
	if (a < b)
		return -1;
	else if (a > b)
		return 1;
	else
		return 0;
	}

/* Decode selected encoding from cmap table. Return 1 if cmap found else 0. */
static int cmapDecode(ttrCtx h, 
					  unsigned short platformId,
					  unsigned short platspecId,
					  unsigned short languageId)
	{
	long i;
	h->encodings.cnt = 0;
	for (i = 0; i < h->cmap.encodings.cnt; i++)
		{
		EncRec *enc = &h->cmap.encodings.array[i];
		if (enc->platformId == platformId &&
			enc->platspecId == platspecId &&
			enc->languageId == languageId)
			{
			srcSeek(h, enc->offset + ENC_SUBTABLE_HDR_SIZE);
			switch (enc->format)
				{
			case 0:
				cmapReadFmt0(h);
				goto found;
			case 4:			 
				cmapReadFmt4(h);
				goto found;
			case 6:			 
				cmapReadFmt6(h);
				goto found;
				}
			}
		}
	return 0;	/* No match found */

	/* Match found; sort encodings by code */
 found:
	qsort(h->encodings.array, h->encodings.cnt, sizeof(Encoding), cmpEncs);
	return 1;
	}
			
/* Add string to pool. */
static STI addString(ttrCtx h, int length, char *string)
	{
	STI result = (STI)h->strings.index.cnt;
	*dnaNEXT(h->strings.index) = h->strings.buf.cnt;
	memcpy(dnaEXTEND(h->strings.buf, length + 1), string, length);
	h->strings.buf.array[h->strings.buf.cnt - 1] = '\0';
	return result;
	}

/* Get string from pool. */
static char *getString(ttrCtx h, STI sti)
	{
	return &h->strings.buf.array[h->strings.index.array[sti]];
	}

/* Ensure space to end of tmp1 string. */
static void addTmp1Space(ttrCtx h)
	{
	if (h->tmp1.cnt == 0 || isspace(h->tmp1.array[h->tmp1.cnt - 1]))
		return;
	*dnaNEXT(h->tmp1) = ' ';
	}

/* Ensure no space at end of tmp1 string. */
static void deleteTmp1Space(ttrCtx h)
	{
	while (h->tmp1.cnt > 0 && isspace(h->tmp1.array[h->tmp1.cnt - 1]))
		h->tmp1.cnt--;
	}

/* Get specified name from name table and insert in string index. */
static STI nameGet(ttrCtx h, unsigned short nameId)
	{
	unsigned char *p;
	int i;
	NameRecord *rec;

	rec = nameFind(h, WIN_PLATFORM, WIN_UGL, WIN_ENGLISH, nameId);
	if (rec == NULL)
		rec = nameFind(h, WIN_PLATFORM, WIN_SYMBOL, WIN_ENGLISH, nameId);
	if (rec != NULL && rec->length > 1)
		{
		/* Unicode; read string */
		dnaSET_CNT(h->tmp0, rec->length);
		srcSeek(h, rec->offset);
		srcRead(h, rec->length, h->tmp0.array);

		/* Trim high byte and handle copyright, registered, and trademark */
		p = (unsigned char *)h->tmp0.array;
		h->tmp1.cnt = 0;
		for (i = 0; i < rec->length - 1; i += 2)
			{
			unsigned short hi = *p++;
			unsigned short lo = *p++;
			switch (hi<<8|lo)
				{
			case 0x00A9:
				addTmp1Space(h);
				memcpy(dnaEXTEND(h->tmp1, 3), "(C)", 3);
				break;
			case 0x00AE:
				/* Registered */
				deleteTmp1Space(h);
				memcpy(dnaEXTEND(h->tmp1, 3), "(R)", 3);
				break;
			case 0x2122:
				/* Trademark */
				deleteTmp1Space(h);
				memcpy(dnaEXTEND(h->tmp1, 4), "(TM)", 4);
				break;
			default:
				*dnaNEXT(h->tmp1) = (hi == 0)? lo: '?';
				break;
				}
			}
		}
	else
		{
		rec = nameFind(h, MAC_PLATFORM, MAC_ROMAN, MAC_ENGLISH, nameId);
		if (rec != NULL && rec->length > 0)
			{
			/* Macintosh Roman; read string */
			dnaSET_CNT(h->tmp0, rec->length);
			srcSeek(h, rec->offset);
			srcRead(h, rec->length, h->tmp0.array);

			/* Trim high bit and handle copyright, registered, and trademark */
			p = (unsigned char *)h->tmp0.array;
			h->tmp1.cnt = 0;
			for (i = 0; i < rec->length; i++)
				{
				unsigned short c = *p++;
				switch (c)
					{
				case 0xA9:
					addTmp1Space(h);
					memcpy(dnaEXTEND(h->tmp1, 3), "(C)", 3);
					break;
				case 0xA8:
					/* Registered */
					deleteTmp1Space(h);
					memcpy(dnaEXTEND(h->tmp1, 3), "(R)", 3);
					break;
				case 0xAA:
					/* Trademark */
					deleteTmp1Space(h);
					memcpy(dnaEXTEND(h->tmp1, 4), "(TM)", 4);
					break;
				default:
					*dnaNEXT(h->tmp1) = (c & 0x80)? '?': c;
					break;
					}
				}
			}
		else
			return STI_UNDEF;
		}

	return addString(h, h->tmp1.cnt, h->tmp1.array);
	}

/* Report absfont error message to debug stream. */
static void report_error(abfErrCallbacks *cb, int err_code, int iFD)
	{
	ttrCtx h = cb->ctx;
	if (iFD == -1)
		message(h, "%s (ignored)", abfErrStr(err_code));
	else
		message(h, "%s FD[%d] (ignored)", abfErrStr(err_code), iFD);
	}

/* Return 1 if glyph is unnamed else 0. */
static int unnamedGlyph(ttrCtx h, GID gid)
	{
	abfString *gname = &h->glyphs.array[gid].info.gname;
	return gname->impl == ABF_UNSET_INT && gname->ptr == ABF_UNSET_PTR;
	}

/* Match glyph name. */
static int CTL_CDECL matchName(const void *key, const void *value, void *ctx)
	{
	ttrCtx h = ctx;
	abfString *gname = &h->glyphs.array[*(GID *)value].info.gname;
	return strcmp((char *)key, (gname->ptr != ABF_UNSET_PTR)? 
				  gname->ptr: getString(h, (unsigned short)gname->impl));
	}

/* Test if glyph name exists. Return 1 if glyph name exists else 0. */
static int existsGlyphName(ttrCtx h, char *gname, size_t *index)
	{
	return ctuLookup(gname, h->glyphsByName.array, h->glyphsByName.cnt,
					 sizeof(h->glyphsByName.array[0]), matchName, index, h);
	}

/* Add GID to sorted list at specified index. */
static void addSortedGID(ttrCtx h, GID gid, size_t index)
	{
	GID *new = &h->glyphsByName.array[index];
	memmove(new + 1, new, (h->glyphsByName.cnt++ - index)*
			sizeof(h->glyphsByName.array[0]));
	*new = gid;
	}

/* Create and then add unique glyph name. Trial names are created by 
   adding: ., .2, .3, etc. to the glyph name until it is unique. */
/* Performance fix: append gid before a unique number suffix otherwise
   the code may spend O(N^2*log(N)) time if we start out with
   glyph names all being the same. */
static void addUniqueGlyphName(ttrCtx h, GID gid, char *gname)
	{
	char *buf;
	int matches;
	size_t index;

	/* Allocate buffer */
	/* 64-bit warning fixed by cast here HO */
	dnaSET_CNT(h->tmp1, (long)(strlen(gname) + 7*2));
	buf = h->tmp1.array;

	sprintf(buf, "%s.%d", gname, gid);

	matches = 1;
	while (existsGlyphName(h, buf, &index))
		sprintf(buf, "%s.%d.%d", gname, gid, matches++);

	/* Name unique; add to glyph */
	/* 64-bit warning fixed by cast here HO */
	h->glyphs.array[gid].info.gname.impl = addString(h, (int)strlen(buf), buf);

	addSortedGID(h, gid, index);
	}

/* Assign glyph name from string constant. */
static void assignGlyphNameRef(ttrCtx h, GID gid, char *ref)
	{
	size_t index;

	h->unnamed--;
	if (existsGlyphName(h, ref, &index))
		addUniqueGlyphName(h, gid, ref);
	else
		{
		addSortedGID(h, gid, index);
		h->glyphs.array[gid].info.gname.ptr = ref;
		}
	}

/* Assign glyph name from volatile string. */
static void assignGlyphNameCopy(ttrCtx h, GID gid, int length, char *string)
	{
	size_t index;
	char *gname = dnaGROW(h->tmp0, length);
	sprintf(gname, "%.*s", length, string);

	h->unnamed--;
	if (existsGlyphName(h, gname, &index))
		addUniqueGlyphName(h, gid, gname);
	else
		{
		addSortedGID(h, gid, index);
		h->glyphs.array[gid].info.gname.impl = addString(h, length, string);
		}	
	}

/* Assign uni<CODE> name from Unicode value. */
static void assignUnicodeName(ttrCtx h, GID gid, UV uv)
	{
	char buf[8];

	if (uv == 0xFFFF ||
		uv == 0xFFFE ||
		(0xD800 <= uv && uv <= 0xDFFF))
		return;	/* Reject to non-Unicode and surrogate values */

	sprintf(buf, "uni%04hX", uv);
	assignGlyphNameCopy(h, gid, 7, buf);
	}

/* Assign names from format 2.0 post table. */
static void assignPost2Names(ttrCtx h)
	{
	long i;

	if (h->unnamed == 0)
		return;

	for (i = 0; i < h->glyphs.cnt; i++)
		if (unnamedGlyph(h, (unsigned short)i))
			{
			char *gname = post2GetName(h, (unsigned short)i);
			if (gname != NULL)
				{
				if (gname[0] == '\0')
					message(h, "gid[%ld]: empty name", i);
				else
					{
					assignGlyphNameRef(h, (unsigned short)i, gname);
					if (h->unnamed == 0)
						break;
					}
				}
			}
	}

/* Assign glyph names from format 1.0 post table. */
static void assignPost1Names(ttrCtx h)
	{
	long i;
	for (i = 0; i < h->glyphs.cnt; i++)
		assignGlyphNameRef(h, (GID)i, applestd[i]);
	h->unnamed = 0;
	}

/* Match Unicode Value. */
static int CTL_CDECL matchUV(const void *key, const void *value)
	{
	unsigned short a = *(unsigned short *)key;
	unsigned short b = ((AGLMap *)value)->uv;
	if (a < b)
		return -1;
	else if (a > b)
		return 1;
	else
		return 0;
	}

/* Match Secondary Unicode Value in AGL double-mapped record. */
static int CTL_CDECL matchSec(const void *key, const void *value)
	{
	unsigned short a = *(unsigned short *)key;
	unsigned short b = ((AGLDbl *)value)->sec;
	if (a < b)
		return -1;
	else if (a > b)
		return 1;
	else
		return 0;
	}

/* Match code in font encoding. */
static int CTL_CDECL matchCode(const void *key, const void *value)
	{
	unsigned short a = *(unsigned short *)key;
	unsigned short b = ((Encoding *)value)->code;
	if (a < b)
		return -1;
	else if (a > b)
		return 1;
	else
		return 0;
	}

/* Name glyphs using AGL. */
static void assignAGLNames(ttrCtx h)
	{
	static const AGLMap agl[] =
		{
#include "uv2agl.h"
		};
	static const AGLDbl dbl[] =
		{
#include "dblmapuv.h"
		};
	long i;

	if (h->unnamed == 0)
		return;

	for (i = 0; i < h->encodings.cnt; i++)
		{
		Encoding *enc = &h->encodings.array[i];
		if (unnamedGlyph(h, enc->gid))
			{
			AGLMap *aglmap = (AGLMap *)
				bsearch(&enc->code, agl, ARRAY_LEN(agl), 
						sizeof(AGLMap), matchUV);
			if (aglmap != NULL)
				{
				/* UV in AGL */
				if (strchr(aglmap->gname, '%') != NULL)
					{
					/* Secondary UV of a double-mapping */
					AGLDbl *dblmap = (AGLDbl *)
						bsearch(&enc->code, dbl, ARRAY_LEN(dbl),
								sizeof(AGLDbl), matchSec);
					Encoding *pri = (Encoding *)
						bsearch(&dblmap->pri, h->encodings.array, 
								h->encodings.cnt, sizeof(Encoding), matchCode);
					if (pri != NULL && pri->gid == enc->gid)
						/* Same GID is encodied at the primary UV */
						assignGlyphNameRef(h, enc->gid, aglmap->gname);
					else
						assignUnicodeName(h, enc->gid, enc->code);
					}
				else
					assignGlyphNameRef(h, enc->gid, aglmap->gname);

				if (h->unnamed == 0)
					return;
				}
			}
		}

	for (i = 0; i < h->encodings.cnt; i++)
		{
		Encoding *enc = &h->encodings.array[i];
		if (unnamedGlyph(h, enc->gid))
			{
			assignUnicodeName(h, enc->gid, enc->code);
			if (h->unnamed == 0)
				break;
			}
		}
	}

/* Assign encoding to glyphs. */
static void assignEncoding(ttrCtx h, int unicode)
	{
	long i;
	for (i = 0; i < h->encodings.cnt; i++)
		{
		Encoding *enc = &h->encodings.array[i];
		abfGlyphInfo *info = &h->glyphs.array[enc->gid].info;
		if (info->encoding.code == ABF_GLYPH_UNENC)
			{
			if (unicode)
				{
				info->flags |= ABF_GLYPH_UNICODE;
				info->encoding.code = enc->code;
				}
			else
				info->encoding.code = enc->code&0xff;
			}
		}
	}

/* Assign names from Mac Standard Roman encoding. */
static void assignMacRomanNames(ttrCtx h)
	{
	static UV macrmn[256] =
		{
#include "macromn0.h"
		};
	long i;

	if (h->unnamed == 0)
		return;

	/* Convert to Unicode equivalent */
	for (i = 0; i < h->encodings.cnt; i++)
		{
		Encoding *enc = &h->encodings.array[i];
		enc->code = macrmn[enc->code];
		}
	assignAGLNames(h);
	}

/* Calculate worst-case size for all g<GID> names. */
static long GIDNameSize(ttrCtx h)
	{
	long digits;
	long size = h->unnamed*2;	/* "g" + "\0" */
	long power = 10;
	long gid = h->glyphs.cnt - h->unnamed;
	for (digits = 1; gid < h->glyphs.cnt; digits++)
		{
		if (power > h->glyphs.cnt)
			power = h->glyphs.cnt;
		if (gid < power)
			{
			size += (power - gid)*digits;
			gid = power;
			}
		power *= 10;
		}
	return size;
	}

/* Assign g<GID> names to unassigned glyphs. There is a remote possibility that
   the source font specified glyph name(s) in the post table that matched the
   g<GID> pattern and that this function will generate a duplicate glyph name
   that could make ttrGetGlyphByName() malfunction. This, however, seems a
   remote possibility since glyph names following the g<GID> convention are
   likely to be assigned to the glyph with the corresponding gid. Also, most
   users of this library will most likely be accessing glyphs by something
   other than a glyph name. I choose to keep the code simple rather than
   address this unlikely situation. */
static void assignGIDNames(ttrCtx h)
	{
	long gid;

	if (h->unnamed == 0)
		return;

	/* Allocate the memory we need to avoid reallocation */
	(void)dnaGROW(h->strings.index, h->strings.index.cnt + h->unnamed - 1);
	(void)dnaGROW(h->strings.buf, h->strings.buf.cnt + GIDNameSize(h) - 1);

	for (gid = 0; gid < h->glyphs.cnt; gid++)
		if (unnamedGlyph(h, (unsigned short)gid))
			{
			char buf[10];
			sprintf(buf, "g%ld", gid);
			
			h->glyphs.array[gid].info.gname.impl =
				/* 64-bit warning fixed by cast here HO */
				addString(h, (int)strlen(buf), buf);
			h->glyphsByName.array[h->glyphsByName.cnt++] = (GID)gid;

			if (--h->unnamed == 0)
				break;
			}

	h->flags |= GID_NAMES_ASSIGNED;
	}

/* Add names and encoding to glyphs. */
static void assignNamesAndEncoding(ttrCtx h)
	{
	/* Allocate sorted GID list */
	(void)dnaGROW(h->glyphsByName, h->glyphs.cnt - 1);
	h->glyphsByName.cnt = 0;

	/* Initially all glyphs are unnamed */
	h->unnamed = h->glyphs.cnt;

	if (h->post.format == 0x00010000 && h->glyphs.cnt == 258)
		assignPost1Names(h);
	else
		/* Assign ".notdef" to GID 0 */
		assignGlyphNameRef(h, 0, ".notdef");

	if (h->post.format == 0x00020000)
		assignPost2Names(h);

	if (cmapDecode(h, UNI_PLATFORM, UNI_VER_2_0, 0) ||
		cmapDecode(h, WIN_PLATFORM, WIN_UGL, 0) ||
		cmapDecode(h, UNI_PLATFORM, UNI_ISO_10646, 0) ||
		cmapDecode(h, UNI_PLATFORM, UNI_VER_1_1, 0) ||
		cmapDecode(h, UNI_PLATFORM, UNI_DEFAULT, 0))
		{
		assignEncoding(h, 1);
		assignAGLNames(h);
		}

	if (cmapDecode(h, WIN_PLATFORM, WIN_SYMBOL, 0))
		assignEncoding(h, 0);
	else if (cmapDecode(h, MAC_PLATFORM, MAC_ROMAN, 0))
		{
		assignEncoding(h, 0);
		assignMacRomanNames(h);
		}

	assignGIDNames(h);
	}

/* Add string pointer from string index. */
static void addStrPtr(ttrCtx h, abfString *str)
	{
	if (str->impl != STI_UNDEF)
		str->ptr = getString(h, (STI)str->impl);
	}

/* Compare glyph names. */
static int CTL_CDECL cmpGNames(const void *first, const void *second, 
							   void *ctx)
	{
	ttrCtx h = ctx;
	return strcmp(h->glyphs.array[*(unsigned short *)first].info.gname.ptr,
				  h->glyphs.array[*(unsigned short *)second].info.gname.ptr);
	}

static STI getFontVersion(ttrCtx h)
{

    float fontRevision;
    char buf[16];
    fontRevision = ((float)(h->head.fontRevision)/65536L);
    sprintf(buf, "%.3f", fontRevision);
    return addString(h, (int)strlen(buf), buf);
}

/* Fill out client data. */
static void fillClientData(ttrCtx h)
	{
	long i;
	abfTopDict *top = &h->top;
	abfFontDict *font = &top->FDArray.array[0];

	/* Top dict */
	top->version.impl 	= getFontVersion(h);
	top->Notice.impl 	= nameGet(h, name_TRADEMARK);
	top->Copyright.impl = nameGet(h, name_COPYRIGHT);
	top->FullName.impl 	= nameGet(h, name_FULL);
	top->FamilyName.impl= nameGet(h, name_FAMILY);
	top->Weight.impl 	= nameGet(h, name_SUBFAMILY);

	if (h->post.format != 0)
		{
		top->isFixedPitch		= h->post.isFixedPitch;
		top->ItalicAngle		= FIX2FLT(h->post.italicAngle);
		top->UnderlinePosition  = (float)(h->post.underlinePosition - floor(h->post.underlineThickness * 0.5));
		top->UnderlineThickness = h->post.underlineThickness;
		}
	else
		{
		/* Scale default 1000 unit values */
		float unitscale = h->head.unitsPerEm/1000.0f;
		h->top.UnderlinePosition 	*= unitscale;
		h->top.UnderlineThickness	*= unitscale;
		}

	top->FontBBox[0] = h->head.xMin;
	top->FontBBox[1] = h->head.yMin;
	top->FontBBox[2] = h->head.xMax;
	top->FontBBox[3] = h->head.yMax;

	if (h->OS_2.version != 0xffff)
		top->FSType = h->OS_2.fsType;

	/* Font Dict */
	font->FontName.impl = nameGet(h, name_FONTNAME);
	if (font->FontName.impl == STI_UNDEF)
		{
		char *unknown = "unknown";
		if (h->name.records.cnt != 0)
			message(h, "name: FontName missing");
		/* 64-bit warning fixed by cast here HO */
		font->FontName.impl = addString(h, (int)strlen(unknown), unknown);
		}

	font->FontMatrix.cnt 		= 6;
	font->FontMatrix.array[0]	= 1.0f/h->head.unitsPerEm;
	font->FontMatrix.array[1]	= 0.0;
	font->FontMatrix.array[2]	= 0.0;
	font->FontMatrix.array[3]	= 1.0f/h->head.unitsPerEm;
	font->FontMatrix.array[4]	= 0.0;
	font->FontMatrix.array[5]	= 0.0;

	/* Supplementary data */
	top->sup.srcFontType = abfSrcFontTypeTrueType;
	top->sup.UnitsPerEm = h->head.unitsPerEm;
	top->sup.nGlyphs = h->glyphs.cnt;

	/* Validate dictionaries */
	if (h->stm.dbg == NULL)
		abfCheckAllDicts(NULL, &h->top);
	else
		{
		abfErrCallbacks cb;
		cb.ctx = h;
		cb.report_error = report_error;
		abfCheckAllDicts(&cb, &h->top);
		}

	assignNamesAndEncoding(h);

	/* Add strings to top dict */
	addStrPtr(h, &top->version);
	addStrPtr(h, &top->Notice);
	addStrPtr(h, &top->Copyright);
	addStrPtr(h, &top->FullName);
	addStrPtr(h, &top->FamilyName);
	addStrPtr(h, &top->Weight);

	/* Add FontName string to font dict */
	addStrPtr(h, &font->FontName);

	/* Add glyph name strings */
	for (i = 0; i < h->glyphs.cnt; i++)
		{
		abfString *gname = &h->glyphs.array[i].info.gname;
		if (gname->ptr == ABF_UNSET_PTR)
			addStrPtr(h, gname);
		}

	if (h->flags & GID_NAMES_ASSIGNED)
		/* Sort the glyph names into order */
		ctuQSort(h->glyphsByName.array, h->glyphsByName.cnt,
				 sizeof(unsigned short), cmpGNames, h);
	}

/* Read TrueType font. */
int ttrBegFont(ttrCtx h, long flags, long origin, int iTTC, abfTopDict **top)
	{
	sfrTable *table;
	long i;

	/* Set error handler */
	if (setjmp(h->err.env))
		return h->err.code;
	
	/* Initialize */
	h->flags = 0;
	h->client_flags = flags;
	h->post.format = 0;
	h->OS_2.version = 0xffff;
	h->strings.index.cnt = 0;
	h->strings.buf.cnt = 0;

	h->top.FDArray.cnt = 1;
	h->top.FDArray.array = &h->fdict;
	abfInitAllDicts(&h->top);

	/* Open source stream */
	h->stm.src = h->cb.stm.open(&h->cb.stm, TTR_SRC_STREAM_ID, 0);
	if (h->stm.src == NULL)
		fatal(h, ttrErrSrcStream, NULL);

	/* Read sfnt support tables */
	sfntReadHdr(h, origin, iTTC);
	headRead(h);
	hheaRead(h);
	maxpRead(h);
	nameRead(h);
	cmapRead(h);
	postRead(h);
	OS_2Read(h);

	/* Initialize glyph array */
	dnaSET_CNT(h->glyphs, h->maxp.numGlyphs);
	for (i = 0; i < h->glyphs.cnt; i++)
		{
		abfGlyphInfo *info = &h->glyphs.array[i].info;
		abfInitGlyphInfo(info);
		info->tag = (unsigned short)i;
		}

	/* Read auxiliary glyph info */
	hmtxRead(h);
	locaRead(h);
	table = sfrGetTableByTag(h->ctx.sfr, CTL_TAG('g','l','y','f'));
	if (table == NULL)
		return ttrErrNoGlyph;
	h->glyf.offset = table->offset;

	/* Fill out and report client data */
	fillClientData(h);
	*top = &h->top;

	return ttrSuccess;
	}

/* Read simple glyph (header already read). */
static void glyfReadSimple(ttrCtx h, GID gid, int nContours, int iStart)
	{
	long i;
	long where;
	short x;
	short y;
	unsigned short *endPts;
	int nPoints;
	glyfCoord *coords;

	/* Read countour end point indices */
	endPts = dnaEXTEND(h->glyf.endPts, nContours);
	for (i = 0; i < nContours; i++)
		endPts[i] = read2(h) + iStart;

	/* Skip instructions */
	where = srcTell(h);
	srcSeek(h, where + 2 + read2(h));

	/* Read flags */
	nPoints = endPts[nContours - 1] + 1 - iStart;
	if (nPoints > h->maxp.maxPoints)
		fatal(h, ttrErrTooManyPoints,
			"gid[%hu]: max points exceeded (%d > max %d)", gid, nPoints, h->maxp.maxPoints);
	coords = dnaEXTEND(h->glyf.coords, nPoints);
	i = 0;
	while (i < nPoints)
		{
		char flags = read1(h);
		coords[i++].flags = flags;
		if (flags & glyf_REPEAT)
			{
			unsigned cnt = read1(h);
			while (cnt--)
				coords[i++].flags = flags;
			}
		}

	/* Read x-coords */
	x = 0;
	for (i = 0; i < nPoints; i++)
		{
		int flags = coords[i].flags;
		if (flags & glyf_XSHORT)
			x += (flags & glyf_SHORT_X_IS_POS)? read1(h): -read1(h);
		else if (!(flags & glyf_NEXT_X_IS_ZERO))
			x += read2(h);
		coords[i].x = x;
		}

	/* Read y-coords */
	y = 0;
	for (i = 0; i < nPoints; i++)
		{
		int flags = coords[i].flags;
		if (flags & glyf_YSHORT)
			y += (flags & glyf_SHORT_Y_IS_POS)? read1(h): -read1(h);
		else if (!(flags & glyf_NEXT_Y_IS_ZERO))
			y += read2(h);
		coords[i].y = y;
		}	
	}

/* Read glyph header (discarding glyph bounding box). */
static short glyfReadHdr(ttrCtx h, GID gid)
	{
	short nContours;
	Glyph *glyph = &h->glyphs.array[gid];
	
	srcSeek(h, h->glyf.offset + glyph->info.sup.begin);
	nContours = sread2(h);
	glyph->xMin = sread2(h);	/* xMin */
	(void)read2(h);				/* yMin */
	(void)read2(h);				/* xMax */
	(void)read2(h);				/* yMax */

	if (nContours > h->maxp.maxContours)
		fatal(h, ttrErrTooManyContours,
			"gid[%hu]: max contours exceeded (%d > max %d)", gid, nContours, h->maxp.maxContours);

	return nContours;
	}

/* Read compound glyph. Header already read. */
static void glyfReadCompound(ttrCtx h, GID gid, GID *mtx_gid, int depth)
	{
	unsigned short flags;	/* Control flags */
	for (;;)
		{
		Offset saveoff;			
		int i;
		long iStart;
		short nContours;	
		int translate;		
		int transform;		
		float matrix[4];	/* Transformation matrix */
		short x = 0;		/* x-translation */
		short y = 0;		/* y-translation */
		unsigned p1 = 0;	/* Compound matching point */
		unsigned p2 = 0;	/* Component matching point */
		GID component;		/* Component glyph index */

		flags 		= read2(h);
		component 	= read2(h);

        iStart = h->glyf.coords.cnt; //compound glyphs can be nested; iStart is not necessarily 0 here.
            
		/* Validate component's glyph index */
		if (component >= h->glyphs.cnt)
  			fatal(h, ttrErrNoComponent, 
				  "gid[%hu]: component %hu not in font", gid, component);

		translate = flags & glyf_ARGS_ARE_XY_VALUES;
		if (flags & glyf_ARG_1_AND_2_ARE_WORDS)
			{
			/* Short word args */
			if (translate)
				{
				/* Position component by explict offsets */
				x = sread2(h);
				y = sread2(h);
				}
			else
				{
				/* Position component by point matching */
				p1 = read2(h);
				p2 = read2(h);
				}
			}
		else
			{
			/* Byte args */
			if (translate)
				{
				x = (signed char)read1(h);
				y = (signed char)read1(h);
				}
			else
				{
				p1 = read1(h);
				p2 = read1(h);
				}
			}

		if (flags & glyf_WE_HAVE_A_SCALE)
			{
			matrix[0] = F2Dot14_2FLT(sread2(h));
			matrix[1] = 0.0;
			matrix[2] = 0.0;
			matrix[3] = matrix[0];
			transform = 1;
			}
		else if (flags & glyf_WE_HAVE_AN_X_AND_Y_SCALE)
			{
			matrix[0] = F2Dot14_2FLT(sread2(h));
			matrix[1] = 0.0;
			matrix[2] = 0.0;
			matrix[3] = F2Dot14_2FLT(sread2(h));
			transform = 1;
			}
		else if (flags & glyf_WE_HAVE_A_TWO_BY_TWO)
			{
			matrix[0] = F2Dot14_2FLT(sread2(h));
			matrix[1] = F2Dot14_2FLT(sread2(h));
			matrix[2] = F2Dot14_2FLT(sread2(h));
			matrix[3] = F2Dot14_2FLT(sread2(h));	
			transform = 1;
			}
		else 
			transform = 0;

		if (flags & glyf_USE_MY_METRICS)
			*mtx_gid = component;

		/* Save current offset */
		saveoff = srcTell(h);

		/* Read component glyph */
		if (h->glyphs.array[component].info.sup.begin == ABF_UNSET_INT)
			break;	/* Missing component glyph! */

		nContours = glyfReadHdr(h, component);
		if (nContours >= 0)
			{
			iStart = h->glyf.coords.cnt;
			glyfReadSimple(h, gid, nContours, iStart);
			}
		else
			{
			if (depth == h->maxp.maxComponentDepth)
				message(h, "gid[%hu]: max component depth exceeded (ignored)",
				        gid);
			glyfReadCompound(h, gid, mtx_gid, ++depth);
			}

		if (!translate)
			{
			/* Validate point indexes */
			if ((long)p1 >= iStart || (long)p2 + iStart >= h->glyf.coords.cnt)
				fatal(h, ttrErrNoPoints, "gid[%hu]: invalid compound points");

			/* Convert matched points to a translation */
			x = h->glyf.coords.array[p1].x - 
				h->glyf.coords.array[p2 + iStart].x;
			y = h->glyf.coords.array[p1].y - 
				h->glyf.coords.array[p2 + iStart].y;
			}

		if (transform)
			/* Apply transformation to component */
			for (i = iStart; i < h->glyf.coords.cnt; i++)
				{
				glyfCoord *coord = &h->glyf.coords.array[i];
				short X = coord->x;
				short Y = coord->y;
				coord->x = (short)RND(matrix[0]*X + matrix[2]*Y + x);
				coord->y = (short)RND(matrix[1]*X + matrix[3]*Y + y);
				}
		else if (x != 0 || y != 0)
			/* Apply translation to component */
			for (i = iStart; i < h->glyf.coords.cnt; i++)
				{
				glyfCoord *coord = &h->glyf.coords.array[i];
				coord->x += x;	
				coord->y += y;
				}
		if (!(flags & glyf_MORE_COMPONENTS))
			break;

		/* Restore offset for next component */
		srcSeek(h, saveoff);
		}
	}

/* Convert path using mathematically exact conversion of quadratic curve
   segments to cubic curve segments and send it via callbacks. */
static void callbackExactPath(ttrCtx h, GID gid, abfGlyphCallbacks *glyph_cb)
	{
	int i;
	int iBeg = 0;
	for (i = 0; i < h->glyf.endPts.cnt; i++)
		{
		int iEnd = h->glyf.endPts.array[i];
		if (iBeg == iEnd)
			;	/* Skip single point contour */
		else if (iEnd >= h->glyf.coords.cnt || iBeg > iEnd)
			fatal(h, ttrErrBadGlyphData, "gid[%hu]: invalid glyph data", gid);
		else
			{
			float x1 = 0;		/* Suppress optimizer warning */
			float y1 = 0;		/* Suppress optimizer warning */
			glyfCoord *p0;
			glyfCoord *p1;
			glyfCoord *beg = &h->glyf.coords.array[iBeg];
			glyfCoord *end = &h->glyf.coords.array[iEnd];
			glyfCoord *first = beg;

			/* Find on-curve starting point */
			do
				if (first->flags & glyf_ONCURVE)
					{
					glyph_cb->move(glyph_cb, first->x, first->y);
					goto path;
					}
			while (first++ != end);

			/* No on-curve points found; start at mid-point */
			glyph_cb->move(glyph_cb, 
						   RND((end->x + beg->x)/2.0f),
						   RND((end->y + beg->y)/2.0f));

			/* Compute start of next bezier */
			x1 = (end->x + 5*beg->x)/6.0f;
			y1 = (end->y + 5*beg->y)/6.0f;

			first = beg;

		path:
			p0 = p1 = first;
			do
				{
				/* Advance to next point */
				if (p1++ == end)
					p1 = beg;

				if (p0->flags & glyf_ONCURVE)
					{
					if (p1->flags & glyf_ONCURVE)
						{
						/* [on on] */
						if (p1 != first)
							glyph_cb->line(glyph_cb, p1->x, p1->y);
						}
					else
						{
						/* [on off] compute start of next bezier */
						x1 = (p0->x + 2*p1->x)/3.0f;
						y1 = (p0->y + 2*p1->y)/3.0f;
						}
					}
				else if (p1->flags & glyf_ONCURVE)
					/* [off on] */
					glyph_cb->curve(glyph_cb, 
									RND(x1), RND(y1),
									RND((2*p0->x + p1->x)/3.0f),
									RND((2*p0->y + p1->y)/3.0f), 
									p1->x, p1->y);
				else
					{
					/* [off off] */
					glyph_cb->curve(glyph_cb,
									RND(x1), RND(y1),
									RND((5*p0->x + p1->x)/6.0f), 
									RND((5*p0->y + p1->y)/6.0f), 
									RND((p0->x + p1->x)/2.0f),
									RND((p0->y + p1->y)/2.0f));

					if (p1 != first)
						{
						x1 = (p0->x + 5*p1->x)/6.0f;
						y1 = (p0->y + 5*p1->y)/6.0f;
						}
					}

				p0 = p1;
				}
			while (p1 != first);
			}
		iBeg = iEnd + 1;	/* Advance to next contour */
		}
	}

/* Callback first curve. */
static void callbackCurve(Point *p, abfGlyphCallbacks *glyph_cb)
	{
	glyph_cb->curve(glyph_cb, 
					RND((p[0].x + 2*p[1].x)/3),
					RND((p[0].y + 2*p[1].y)/3),
					RND((2*p[1].x + p[2].x)/3),
					RND((2*p[1].y + p[2].y)/3),
					RND(p[2].x), RND(p[2].y));
	}

/* Test if curve pair should be combined. If true, callback combined curve else
   callback first curve of pair. Return 1 if curves combined else 0. */
static int combinePair(Point *p, abfGlyphCallbacks *glyph_cb)
	{
	float a = p[3].y - p[1].y;
	float b = p[1].x - p[3].x;
	if ((a != 0 || p[1].y != p[2].y) && (b != 0 || p[1].x != p[2].x))
		{
		/* Not a vertical or horizontal join... */
		float absq = a*a + b*b;
		if (absq != 0)
			{
			float sr = a*(p[2].x - p[1].x) + b*(p[2].y - p[1].y);
			if ((sr*sr)/absq < 1)
				{
				/* ...that is straight... */
				if ((a*(p[0].x - p[1].x) + b*(p[0].y - p[1].y) < 0) ==
					(a*(p[4].x - p[1].x) + b*(p[4].y - p[1].y) < 0))
					{
					/* ...and without inflexion... */
					float d0 = (p[2].x - p[0].x)*(p[2].x - p[0].x) + 
						(p[2].y - p[0].y)*(p[2].y - p[0].y);
					float d1 = (p[4].x - p[2].x)*(p[4].x - p[2].x) + 
						(p[4].y - p[2].y)*(p[4].y - p[2].y);
					if (d0 <= 3*d1 && d1 <= 3*d0)
						{
						/* ...and small segment length ratio; combine curve */
						glyph_cb->curve(glyph_cb,
										RND((4*p[1].x - p[0].x)/3),
										RND((4*p[1].y - p[0].y)/3),
										RND((4*p[3].x - p[4].x)/3),
										RND((4*p[3].y - p[4].y)/3),
										RND(p[4].x), RND(p[4].y));
						p[0] = p[4];
						return 1;
						}
					}
				}
			}
		}

	/* Callback first curve then replace it by second curve */
	callbackCurve(p, glyph_cb);
	p[0] = p[2];
	p[1] = p[3];
	p[2] = p[4];

	return 0;
	}

/* Convert path using an approximate conversion of quadratic curve segments to
   cubic curve segments and send it via callbacks. 

   Points are either on or off the curve and are accumulated in an array until
   there is enough context to make a decision about how to convert the point
   sequence. This is implemented using a state machine as follows:

   state	sequence		points
   			0=off,1=on		accumulated
   0		1				0
   1		1 0				0-1
   2		1 0 0			0-3 (p[2] is mid-point of p[1] and p[3])
   3		1 0 1			0-2
   4		1 0 1 0			0-3

   One curve is described by points 0-2 and another by point 2-4. Point 5 is a
   temporary. 

   States 2 and 4 are complicated by the fact that a test must be performed to
   decide if the 2 curves decribed by the point data can be combined into a
   single curve or must retained as 2 curves. */
static void callbackApproxPath(ttrCtx h, GID gid, abfGlyphCallbacks *glyph_cb)
	{
/* Save current point at index n */
#define SAVE_PT(n) \
	p[n].x = q->x; \
	p[n].y = q->y
/* Save mid-point at index n from points at indexes a and b */
#define MID_PT(n, a, b) \
	p[n].x = (p[a].x + p[b].x)/2; \
	p[n].y = (p[a].y + p[b].y)/2

	int i;
	int iBeg = 0;
	for (i = 0; i < h->glyf.endPts.cnt; i++)
		{
		int iEnd = h->glyf.endPts.array[i];
		if (iBeg == iEnd)
			;	/* Skip single point contour */
		else if (iEnd >= h->glyf.coords.cnt || iBeg > iEnd)
			fatal(h, ttrErrBadGlyphData, "gid[%hu]: invalid glyph data", gid);
		else
			{
			Point p[6];			/* Points: 0,2,4-on, 1,3-off, 5-tmp */
			glyfCoord *q;		/* Current point */
			glyfCoord *beg = &h->glyf.coords.array[iBeg];
			glyfCoord *end = &h->glyf.coords.array[iEnd];
			int cnt = iEnd - iBeg + 1;
			int state = 0;

			/* Save initial on-curve point */
			if (beg->flags & glyf_ONCURVE)
				{
				q = beg;
				SAVE_PT(0);
				}
			else if (end->flags & glyf_ONCURVE)
				{
				q = end;
				SAVE_PT(0);
				}
			else
				{
				/* Start at mid-point */
				q = end;
				cnt++;
				p[0].x = (end->x + beg->x)/2.0f;
				p[0].y = (end->y + beg->y)/2.0f;
				}

			/* Callback initial move */
			glyph_cb->move(glyph_cb, RND(p[0].x), RND(p[0].y));

			while (cnt--)
				{
				/* Advance to next point */
				q = (q == end)? beg: q + 1;

				if (q->flags & glyf_ONCURVE)
					/* On-curve */
					switch (state)
						{
					case 0:
						if (cnt > 0)
							{
							glyph_cb->line(glyph_cb, q->x, q->y);
							SAVE_PT(0);
							/* stay in state 0 */
							}
						break;
					case 1:
						SAVE_PT(2);
						state = 3;
						break;
					case 2:
						SAVE_PT(4);
						state = combinePair(p, glyph_cb)? 0: 3;
						break;
					case 3:
						callbackCurve(p, glyph_cb);
						if (cnt > 0)
							{
							glyph_cb->line(glyph_cb, q->x, q->y);
							SAVE_PT(0);
							}
						state = 0;
						break;
					case 4:
						SAVE_PT(4);
						state = combinePair(p, glyph_cb)? 0: 3;
						break;
						}
				else
					/* Off-curve */
					switch (state)
						{
					case 0:
						SAVE_PT(1);
						state = 1;
						break;
					case 1:
						SAVE_PT(3);
						MID_PT(2, 1, 3);
						state = 2;
						break;
					case 2:
						SAVE_PT(5);
						MID_PT(4, 3, 5);
						if (combinePair(p, glyph_cb))
							{
							p[1] = p[5];
							state = 1;
							}
						else
							{
							p[3] = p[5];
							state = 4;
							}
						break;
					case 3:
						SAVE_PT(3);
						state = 4;
						break;
					case 4:
						SAVE_PT(5);
						MID_PT(4, 3, 5);
						if (combinePair(p, glyph_cb))
							{
							p[1] = p[5];
							state = 1;
							}
						else
							{
							p[3] = p[5];
							state = 2;
							}
						break;
						}
				}

			/* Finish up */
			switch (state)
				{
			case 2:
				SAVE_PT(3);
				MID_PT(2, 1, 3);
				/* Fall through */
			case 3:
			case 4:
				callbackCurve(p, glyph_cb);
				break;
				}
			}
		iBeg = iEnd + 1;	/* Advance to next contour */
		}

#undef SAVE_PT
#undef MID_PT	
	}

/* Read glyph outline. */
static void readGlyph(ttrCtx h, 
					  unsigned short gid, abfGlyphCallbacks *glyph_cb)
	{
	int result;
	long nContours;
	Glyph *glyph = &h->glyphs.array[gid];

	/* Begin glyph and mark it as seen */
	result = glyph_cb->beg(glyph_cb, &glyph->info);
	glyph->info.flags |= ABF_GLYPH_SEEN;

	/* Check result */
	switch (result)
		{
	case ABF_CONT_RET:
		break;
	case ABF_WIDTH_RET:
		glyph_cb->width(glyph_cb, glyph->hAdv);
		return;
	case ABF_SKIP_RET:
		return;
	case ABF_QUIT_RET:
		fatal(h, ttrErrCstrQuit, NULL);
	case ABF_FAIL_RET:
		fatal(h, ttrErrCstrFail, NULL);
		}

	glyph_cb->width(glyph_cb, glyph->hAdv);
	if (glyph->info.sup.begin != ABF_UNSET_INT && 
		(nContours = glyfReadHdr(h, gid)) != 0)
		{
		short xshift;
		GID mtx_gid = gid;

		/* Callback path */
		h->glyf.endPts.cnt = 0;
		h->glyf.coords.cnt = 0;

		if (nContours >= 0)
			glyfReadSimple(h, gid, nContours, 0);
		else
			glyfReadCompound(h, gid, &mtx_gid, 0);

		xshift = h->glyphs.array[mtx_gid].lsb - h->glyphs.array[mtx_gid].xMin;
		if (xshift != 0)
			{
			/* Shift outline to match lsb in hmtx table */
			long i;
			for (i = 0; i < h->glyf.coords.cnt; i++)
				h->glyf.coords.array[i].x += xshift;
			}

		if (h->client_flags & TTR_BOTH_PATHS)
			{
			callbackExactPath(h, gid, glyph_cb);
			callbackApproxPath(h, gid, glyph_cb);
			}
		else if ((h->client_flags & TTR_EXACT_PATH) || 
				 h->top.sup.UnitsPerEm < 1000)
			callbackExactPath(h, gid, glyph_cb);
		else
			callbackApproxPath(h, gid, glyph_cb);
		}

	/* End glyph */
	glyph_cb->end(glyph_cb);
	}

/* Read all glyphs in font. */
int ttrIterateGlyphs(ttrCtx h, abfGlyphCallbacks *glyph_cb)
					 
	{
	long gid;

	/* Set error handler */
	if (setjmp(h->err.env))
		return h->err.code;
	
	for (gid = 0; gid < h->glyphs.cnt; gid++)
		readGlyph(h, (unsigned short)gid, glyph_cb);

	return ttrSuccess;
	}

/* Get glyph from font by its tag. */
int ttrGetGlyphByTag(ttrCtx h, 
					 unsigned short tag, abfGlyphCallbacks *glyph_cb)
	{
	if (tag >= h->glyphs.cnt)
		return ttrErrNoGlyph;

	/* Set error handler */
	if (setjmp(h->err.env))
		return h->err.code;
	
	readGlyph(h, tag, glyph_cb);

	return ttrSuccess;
	}

/* Get glyph from font by its name. */
int ttrGetGlyphByName(ttrCtx h, char *gname, abfGlyphCallbacks *glyph_cb)
	{
	size_t index;

	if (!existsGlyphName(h, gname, &index))
		return ttrErrNoGlyph;

	/* Set error handler */
	if (setjmp(h->err.env))
		return h->err.code;
	
	readGlyph(h, h->glyphsByName.array[index], glyph_cb);

	return ttrSuccess;
	}

/* Clear record of glyphs seen by client. */
int ttrResetGlyphs(ttrCtx h)
	{
	long i;
	
	for (i = 0; i < h->glyphs.cnt; i++)
		{
		Glyph *glyph = &h->glyphs.array[i];
		glyph->info.flags &= ~ABF_GLYPH_SEEN;
		}

	return ttrSuccess;
	}

/* Finish reading font. */
int ttrEndFont(ttrCtx h)
	{
	int result = sfrEndFont(h->ctx.sfr);
	if (result)
		{
		message(h, "(sfr) %s", sfrErrStr(result));
		return ttrErrSfntread;
		}

	/* Close source stream */
	if (h->cb.stm.close(&h->cb.stm, h->stm.src) == -1)
		return ttrErrSrcStream;

	return ttrSuccess;
	}

/* Get version numbers of libraries. */
void ttrGetVersion(ctlVersionCallbacks *cb)
	{
	if (cb->called & 1<<TTR_LIB_ID)
		return;	/* Already enumerated */

	/* Support libraries */
	abfGetVersion(cb);
	ctuGetVersion(cb);
	dnaGetVersion(cb);
	sfrGetVersion(cb);

	/* This library */
	cb->getversion(cb, TTR_VERSION, "ttread");

	/* Record this call */
	cb->called |= 1<<TTR_LIB_ID;
	}

/* Map error code to error string. */
char *ttrErrStr(int err_code)
	{
	static char *errstrs[] =
		{
#undef CTL_DCL_ERR
#define CTL_DCL_ERR(name,string)	string,
#include "ttrerr.h"
		};
	return (err_code < 0 || err_code >= (int)ARRAY_LEN(errstrs))?
		"unknown error": errstrs[err_code];
	}

