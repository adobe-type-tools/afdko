/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <stdlib.h>
#include <limits.h>

#include "hotconv.h"
#include "cmap.h"
#include "map.h"
#include "feat.h"
#include "dynarr.h"

/* ---------------------------- Table Definition --------------------------- */

#define cmap_VERSION        0

/* Common header for formats 0, 2, 4, 6. Access by the GET_* macros below */
typedef struct {
	uint16_t format;
	uint16_t length;
	uint16_t language;
} FormatHdr;

/* Common header for the extended formats: 8, 10, 12. Access by the GET_* macros below */
typedef struct {
	uint16_t format;
	uint16_t reserved;
	uint32_t length;
	uint32_t language;
} FormatHdrExt;

typedef struct {
	uint16_t format;
	uint32_t length;
	uint32_t numUVS;
} FormatHdr14;

#define IS_EXT_FORMAT(f) ((f) == 8 || (f) == 10 || (f) == 12) /* Extended formats */

#define GET_FORMAT(p)   (*((unsigned short *)p))

typedef struct {
	uint16_t format;
	uint16_t length;
	uint16_t language;
	unsigned char glyphId[256];
} Format0;
#define FORMAT0_SIZE (uint16 * 3 + uint8 * 256)

typedef struct {
	unsigned short firstCode;
	unsigned short entryCount;
	short idDelta;
	unsigned short idRangeOffset;
} Segment2;
#define SEGMENT2_SIZE (uint16 * 3 + int16 * 1)

typedef struct {
	unsigned short format;
	unsigned short length;
	unsigned short language;
	unsigned short segmentKeys[256];
	dnaDCL(Segment2, segment);
	dnaDCL(GID, glyphId);
} Format2;
#define FORMAT2_SIZE(segs, glyphs) \
	(uint16 * 3 + uint16 * 256 + SEGMENT2_SIZE * (segs) + uint16 * (glyphs))

typedef struct {
	unsigned short format;
	unsigned short length;
	unsigned short language;
	unsigned short segCountX2;
	unsigned short searchRange;
	unsigned short entrySelector;
	unsigned short rangeShift;
	unsigned short *endCode;
	unsigned short password;
	unsigned short *startCode;
	short *idDelta;
	unsigned short *idRangeOffset;
	dnaDCL(GID, glyphId);
} Format4;
#define FORMAT4_SIZE(segs, glyphs) \
	(uint16 * 8 + (3 * uint16 + int16) * (segs) + uint16 * (glyphs))

typedef struct {
	unsigned short format;
	unsigned short length;
	unsigned short language;
	unsigned short firstCode;
	unsigned short entryCount;
	GID *glyphId;
} Format6;
#define FORMAT6_SIZE(glyphs) (uint16 * 5 + uint16 * (glyphs))

/* Segment for formats 8 and 12: */
typedef struct {
	uint32_t startCharCode;
	uint32_t endCharCode;
	uint32_t startGlyphID;
} Segment8_12;
#define SEGMENT8_12_SIZE (3 * uint32)

typedef struct {
	uint16_t format;
	uint16_t reserved;
	uint32_t length;
	uint32_t language;
	uint32_t nGroups;
	dnaDCL(Segment8_12, segment);
} Format12;

#define FORMAT12_SIZE(nSegs) (2 * uint16 + 3 * uint32 + SEGMENT8_12_SIZE * (nSegs))

typedef struct {
	uint32_t uv;
	uint32_t addtlCnt;
} DefaultUVSEntryRange;

typedef struct {
	uint32_t uv;
	uint16_t glyphID;
} ExtUVSEntry;

typedef struct {
	unsigned long uvs;
	dnaDCL(DefaultUVSEntryRange, dfltUVS);
	dnaDCL(ExtUVSEntry, extUVS);
} UVSRecord;

typedef struct {
	uint16_t format;
	uint32_t length;
	uint32_t numUVS;
} Format14;

#define FORMAT14_SIZE(nUVSRecs) (uint16 + 2 * uint32 + nUVSRecs * (uint24 + 2 * uint32))

typedef struct {
	int16_t id;   /* Internal use. All Encoding's that have the same id will end
	               up pointing to the same subtable in the OTF. Of all the
	               Encoding's with the same id, exactly one will have a
	               non-NULL format field. */
	uint16_t platformId;
	uint16_t scriptId;
	uint32_t offset;
	void *format;
} Encoding;
#define ENCODING_SIZE (uint16 * 2 + uint32 * 1)

typedef struct {
	uint16_t version;
	uint16_t nEncodings;
	dnaDCL(Encoding, encoding);
} cmapTbl;
#define TBL_HDR_SIZE (uint16 * 2)

/* --------------------------- Context Definition ---------------------------*/

/* Format 2 code space ranges */
typedef struct {
	unsigned short lo;
	unsigned short hi;
} Range;

/* New encoding information */
typedef struct {
	unsigned long code;
	GID glyphId;
	unsigned short span;        /* number of mappings in this range */
	unsigned short ordered;
	unsigned short flags;
#define CODE_BREAK   (1 << 0) /* Broke range because of code discontinuity */
#define CODE_1BYTE   (1 << 1) /* Single byte range (for mixed-byte) */
#define CODE_MACCNTL (1 << 2) /* An added Mac control code */
#define CODE_DELETE  (1 << 3) /* Mark for deletion */
} Mapping;

typedef struct {
	unsigned long uv;
	unsigned long uvs;
} DefaultUVEntry;

typedef struct {
	unsigned long uv;
	unsigned long uvs;
	unsigned short gid;
} NonDefaultUVEntry;

struct cmapCtx_ {
	/* Stores the current encoding (cmap subtable) */
	unsigned platformId;
	unsigned scriptId;
	unsigned language;
	unsigned long maxCode;          /* Max code in encoding */
	unsigned maxGlyphId;        /* Max glyphId in the encoding */
	short flags;
#define SEEN_1BYTE  (1 << 0)      /* Single-byte code point seen */
#define SEEN_2BYTE  (1 << 1)      /* Double-byte code point seen */
#define SEEN_4BYTE  (1 << 2)      /* Four-byte code point seen */
#define SEEN_UVS    (1 << 2)      /*Seen Unicode Variation Selector map */
	unsigned long lastUVS;
	dnaDCL(UVSRecord, uvsList);
	dnaDCL(Mapping, mapping);
	dnaDCL(Range, codespace);   /* for mixed-byte encodings' dbl-byte ranges */

	cmapTbl tbl;                /* Stores the entire cmap */
	hotCtx g;
};

/* --------------------------- Standard Functions -------------------------- */

/* Return a cmap context */
void cmapNew(hotCtx g) {
	cmapCtx h = MEM_NEW(g, sizeof(struct cmapCtx_));    /* Allocate context */

	h->tbl.nEncodings = 0;
	h->lastUVS = 0;
	dnaINIT(g->dnaCtx, h->tbl.encoding, 5, 2);
	dnaINIT(g->dnaCtx, h->uvsList, 40, 40);
	dnaINIT(g->dnaCtx, h->mapping, 9000, 2000);
	dnaINIT(g->dnaCtx, h->codespace, 5, 10);

	/* Link contexts */
	h->g = g;
	g->ctx.cmap = h;
}

/* Add standard Mac C0 control chars; any client-specified mappings will take
   priority; they'll be detected later in checkDuplicates(). */
static void addMacControlChars(hotCtx g, cmapCtx h) {
	unsigned int i;
	GID spaceGID = mapUV2GID(g, UV_SPACE);
	static unsigned short addCodes[] = {
		/* ---------- Apple's rationale: ---------- */
		9,  /* HORIZONTAL TAB    SimpleText displays this for tab - and we
		                         don't want marking notdefs to be displayed. */
		13, /* CARRIAGE RETURN   SimpleText displays this for carriage return
		                         - and we don't want marking notdefs to be
		                         displayed. */
#if 0
		/* Removed these since the space at NULL was tickling an InDesign/Mac
		   1.0 bug whereby it was identifying the space's GID as the notdef
		   GID. */
		0,  /* NULL              TTY streams: you don't want the NULLs to be
		                         displayed.
		                         "Faux" Unicode: Unicode text in the ASCII
		                         range can be displayed as if it were ASCII */
		8,  /* BACKSPACE         <No rationale> */
		29, /* GROUP SEPARATOR   <No rationale> */
#endif
	};

	if (spaceGID != GID_UNDEF) {
		for (i = 0; i < ARRAY_LEN(addCodes); i++) {
			cmapAddMapping(g, addCodes[i], spaceGID, 1);
			h->mapping.array[h->mapping.cnt - 1].flags |= CODE_MACCNTL;
		}
	}
}

/* Start a new encoding (cmap subtable) */
void cmapBeginEncoding(hotCtx g, unsigned platformId, unsigned scriptId,
                       unsigned language) {
	cmapCtx h = g->ctx.cmap;

	h->platformId = platformId;
	h->scriptId = scriptId;
	h->language = language;
	h->maxCode = 0;
	h->maxGlyphId = 0;
	h->flags = 0;
	h->mapping.cnt = 0;
	h->codespace.cnt = 0;

	if (h->platformId == cmap_MAC && !IS_CID(g)) {
		addMacControlChars(g, h);
	}
}

/* Prints message. Message should not have newline */
static void CDECL cmapMsg(hotCtx g, int msgType, char *fmt, ...) {
	cmapCtx h = g->ctx.cmap;
	va_list ap;
	char msgVar[1024];
	char msg[1024];

	va_start(ap, fmt);
	vsprintf(msgVar, fmt, ap);
	va_end(ap);
	sprintf(msg, "cmap{plat=%u,script=%u,lang=%u}: %s", h->platformId,
	        h->scriptId, h->language, msgVar);
	hotMsg(g, msgType, msg);
}

/* Add a mapping pair to current encoding. Assumes that any notdef/cid layering
   (for CMap encodings) has already been done. Doesn't weed out notdefs;
   faithfully records them if added by this API. codeSize must be 1, 2 or 4; it
   is needed to flag values like 0x0020 as double-byte. */
void cmapAddMapping(hotCtx g, unsigned long code, unsigned glyphId,
                    int codeSize) {
	cmapCtx h = g->ctx.cmap;
	Mapping *mapping = dnaNEXT(h->mapping);

	mapping->code = code;
	mapping->glyphId = glyphId;
	mapping->span = 0;
	mapping->ordered = 0;
	mapping->flags = codeSize == 1 ? CODE_1BYTE : 0;

	if (codeSize == 1) {
		h->flags |= SEEN_1BYTE;
		if (code > 0xFF) {
			cmapMsg(g, hotFATAL, "code <%lx> is not single-byte", code);
		}
	}
	else if (codeSize == 2) {
		h->flags |= SEEN_2BYTE;
		if (code > 0xFFFF) {
			cmapMsg(g, hotFATAL, "code <%lx> is not double-byte", code);
		}
	}
	else if (codeSize == 4) {
		h->flags |= SEEN_4BYTE;
	}
	else {
		hotMsg(g, hotFATAL, "[internal] invalid cmap codeSize");
	}

	if (h->maxCode < code) {
		h->maxCode = code;
	}
	if (h->maxGlyphId < glyphId) {
		h->maxGlyphId = glyphId;
	}
}

/* Add only 2-byte codespace ranges (needs to be called for mixed-byte
   encodings only) */
void cmapAddCodeSpaceRange(hotCtx g, unsigned lo, unsigned hi, int codeSize) {
	cmapCtx h = g->ctx.cmap;
	Range *csr;

	if (codeSize == 1 && (lo > 0xff || hi > 0xff)) {
		cmapMsg(g, hotFATAL, "codespace range <%hx> <%hx> is not single-byte",
		        lo, hi);
	}

#if 0 /* xxx Remove after testing */
	printf("codespace: %hx-%hx (codeSize=%d)\n", lo, hi, codeSize);
#endif

	if (codeSize != 2) {
		return;
	}

	csr = dnaNEXT(h->codespace);
	csr->lo = lo;
	csr->hi = hi;
}

/* Add a UVS entry to current tables. Needs to add the entry to one of the four
   lists: default UVS with BMP UB, default UVS with supplement UV, non default UVS
   with BMP UVm non-default UBS with supplemental UV, Assumes that the
   entries are added in UVS order, so we don;t need to sort the four tables.. */
void cmapAddUVSEntry(hotCtx g, unsigned int uvsFlags, unsigned long uv, unsigned long uvs, GID gid) {
	cmapCtx h = g->ctx.cmap;
	UVSRecord *uvsRec = NULL;

	if (h->lastUVS != uvs) {
		uvsRec = dnaNEXT(h->uvsList);
		dnaINIT(g->dnaCtx, uvsRec->dfltUVS, 1000, 1000);
		dnaINIT(g->dnaCtx, uvsRec->extUVS, 1000, 1000);
		h->lastUVS = uvsRec->uvs = uvs;
	}
	else {
		uvsRec = dnaINDEX(h->uvsList, h->uvsList.cnt - 1);
	}

	if (uvsFlags & UVS_IS_DEFAULT) {
		if (uvsRec->dfltUVS.cnt == 0) {
			DefaultUVSEntryRange *uvd = dnaNEXT(uvsRec->dfltUVS);
			uvd->uv = uv;
			uvd->addtlCnt = 0;
		}
		else {
			DefaultUVSEntryRange *uvd = dnaINDEX(uvsRec->dfltUVS,  uvsRec->dfltUVS.cnt - 1);
			if ((uvd->addtlCnt < 255) && ((uvd->uv  + uvd->addtlCnt + 1) == uv)) {
				uvd->addtlCnt++;
			}
			else {
				DefaultUVSEntryRange *uvd = dnaNEXT(uvsRec->dfltUVS);
				uvd->uv = uv;
				uvd->addtlCnt = 0;
			}
		}
	}
	else {
		ExtUVSEntry *uvExt = dnaNEXT(uvsRec->extUVS);
		uvExt->uv = uv;
		uvExt->glyphID = gid;
	}
}

/* Make format 0 cmap */
static Format0 *makeFormat0(cmapCtx h) {
	int i;
	Mapping *mapping = h->mapping.array;
	Format0 *fmt = MEM_NEW(h->g, sizeof(Format0));

	for (i = 0; i < 256; i++) {
		fmt->glyphId[i] = 0;
	}
	for (i = 0; i < h->mapping.cnt; i++) {
		fmt->glyphId[mapping[i].code] = (unsigned char)mapping[i].glyphId;
	}

	fmt->format = 0;
	fmt->length = FORMAT0_SIZE;
	fmt->language = h->language;

	return fmt;
}

/* ------- begin format 2 */

/* GlyphId array element initializer */
static void glyphIdInit(void *ctx, long count, GID *element) {
	int i;
	for (i = 0; i < count; i++) {
		*element = 0;
		element++;
	}
	return;
}

/* Sort deleted last; single-byte before double-byte; then by code; then
   MAC_CNTL before client-added */
static int CDECL cmpMixedByteCodes(const void *first, const void *second) {
	const Mapping *a = (Mapping *)first;
	const Mapping *b = (Mapping *)second;

	if ((a->flags & CODE_DELETE) && (b->flags & CODE_DELETE)) {
		return 0;
	}
	else if (a->flags & CODE_DELETE) {
		return 1;
	}
	else if (b->flags & CODE_DELETE) {
		return -1;
	}
	else if ((a->flags & CODE_1BYTE) && !(b->flags & CODE_1BYTE)) {
		return -1;
	}
	else if (!(a->flags & CODE_1BYTE) && (b->flags & CODE_1BYTE)) {
		return 1;
	}
	else if (a->code < b->code) {
		return -1;
	}
	else if (a->code > b->code) {
		return 1;
	}
	else if ((a->flags & CODE_MACCNTL) && !(b->flags & CODE_MACCNTL)) {
		return -1;
	}
	else if (!(a->flags & CODE_MACCNTL) && (b->flags & CODE_MACCNTL)) {
		return 1;
	}
	else {
		return 0;
	}
}

/* Sort deleted last; compare character codes; then MAC_CNTL before client-
   added. */
static int CDECL cmpCodes(const void *first, const void *second) {
	const Mapping *a = first;
	const Mapping *b = second;

	if ((a->flags & CODE_DELETE) && (b->flags & CODE_DELETE)) {
		return 0;
	}
	else if (a->flags & CODE_DELETE) {
		return 1;
	}
	else if (b->flags & CODE_DELETE) {
		return -1;
	}
	else if (a->code < b->code) {
		return -1;
	}
	else if (a->code > b->code) {
		return 1;
	}
	else if ((a->flags & CODE_MACCNTL) && !(b->flags & CODE_MACCNTL)) {
		return -1;
	}
	else if (!(a->flags & CODE_MACCNTL) && (b->flags & CODE_MACCNTL)) {
		return 1;
	}
	else {
		return 0;
	}
}

/* Partition into segments of single byte or equal first byte ranges. Already
   sorted by cmpMixedByteCodes. */
static void partitionSegments(cmapCtx h) {
	long i;
	unsigned first;
	int span;
	Mapping *mapping = h->mapping.array;

	/* Partition single-byte codes */
	i = 0;
	for (;; ) {
		if (i == h->mapping.cnt) {
			cmapMsg(h->g, hotFATAL, "no multi-byte codes in encoding");
		}
		else if (mapping[i].flags & CODE_1BYTE) {
			i++;
		}
		else {
			break;
		}
	}

	if (i == 0) {
		cmapMsg(h->g, hotFATAL, "no single-byte codes in encoding");
	}
	mapping[0].span = (unsigned short)i;

	/* Partition by hi-byte codes */
	span = 1;
	first = mapping[i].code & 0xff00;
	for (i++; i < h->mapping.cnt; i++) {
		unsigned curr = mapping[i].code & 0xff00;
		if (first != curr) {
			mapping[i - span].span = span;
			span = 1;
			first = curr;
		}
		else {
			span++;
		}
	}
	mapping[i - span].span = span;
}

/* Add format 2 segment */
static Segment2 *addSegment2(Format2 *fmt, unsigned short firstCode,
                             unsigned short entryCount, short idDelta,
                             unsigned short idRangeOffset) {
	Segment2 *segment = dnaNEXT(fmt->segment);

	segment->firstCode = firstCode;
	segment->entryCount = entryCount;
	segment->idDelta = idDelta;
	segment->idRangeOffset = idRangeOffset;

	return segment;
}

/* Make first format 2 segment. This handles the single byte encoding */
static void makeFirstSegment2(cmapCtx h, Format2 *fmt) {
	int i;
	Mapping *mapping = h->mapping.array;
	int span = mapping[0].span;
	GID *glyphId = dnaEXTEND(fmt->glyphId, 256);

	(void)addSegment2(fmt, 0, 256, 0, 0);

	/* Construct glyphId array */
	for (i = 0; i < 256; i++) {
		glyphId[i] = 0;
	}
	for (i = 0; i < span; i++) {
		glyphId[mapping[i].code] = mapping[i].glyphId;
	}
}

/* Make format 2 segment. Returns segment index */
static int makeSegment2(cmapCtx h, Format2 *fmt, int start) {
	int i;
	int iSegment;
	Segment2 *segment;
	int minGlyphId; /* Minimum glyphId in segment */
	GID glyphId[256];
	Mapping *mapping = h->mapping.array;
	int span = mapping[start].span;
	int firstCode = mapping[start].code & 0xff;
	int codeRange = (mapping[start + span - 1].code & 0xff) - firstCode + 1;

	/* Find minimum glyph id in segment */
	minGlyphId = mapping[start].glyphId;
	for (i = 1; i < span; i++) {
		if (mapping[start + i].glyphId < minGlyphId) {
			minGlyphId = mapping[start + i].glyphId;
		}
	}

	/* Add new segment */
	iSegment = fmt->segment.cnt;    /* Save new segment index */
	segment =
	    addSegment2(fmt, (unsigned short)firstCode, (unsigned short)codeRange,
	                (short)(minGlyphId - 1), 0);

	/* Construct glyphId array */
	for (i = 0; i < codeRange; i++) {
		glyphId[i] = 0;
	}
	for (i = 0; i < span; i++) {
		glyphId[(mapping[start + i].code & 0xff) - firstCode] =
		    mapping[start + i].glyphId - segment->idDelta;
	}

	/* Look for a glyphId array match */
	for (i = 1; i < iSegment; i++) {
		int j;
		int iGlyphId = fmt->segment.array[i].idRangeOffset;
		GID *existing = &fmt->glyphId.array[iGlyphId];

		for (j = 0; j < codeRange; j++) {
			if (existing[j] != glyphId[j]) {
				goto failed;    /* Match failed, try next segment */
			}
		}
		/* Match succeeded, reference matching range */
		segment->idRangeOffset = iGlyphId;
		return iSegment;

failed:
		;
	}

	/* Match failed, add glyphIds */
	segment->idRangeOffset = (unsigned short)fmt->glyphId.cnt;
	COPY(dnaEXTEND(fmt->glyphId, codeRange), glyphId, codeRange);

	return iSegment;
}

/* Add double-byte codespace ranges */
static void addcodespaceRanges(cmapCtx h, Format2 *fmt) {
	int i;
	int addSegment = 0;

	for (i = 0; i < h->codespace.cnt; i++) {
		unsigned j;
		unsigned endHiByte;
		Range *range = &h->codespace.array[i];

		endHiByte = range->hi >> 8;
		for (j = range->lo >> 8; j <= endHiByte; j++) {
			if (fmt->segmentKeys[j] == 0) {
				fmt->segmentKeys[j] = (unsigned short)fmt->segment.cnt;
				addSegment = 1;
			}
		}
	}

	if (addSegment) {
		/* Add segment that always gives notdef */
		(void)addSegment2(fmt, 0, 0, 0, 0);
	}
}

/* Make format 2 cmap. Already sorted by cmpMixedByteCodes. */
static Format2 *makeFormat2(cmapCtx h) {
	long i;
	Mapping *mapping = h->mapping.array;
	Format2 *fmt = MEM_NEW(h->g, sizeof(Format2));

	dnaINIT(h->g->dnaCtx, fmt->segment, 128, 32);
	dnaINIT(h->g->dnaCtx, fmt->glyphId, 2000, 500);
	fmt->glyphId.func = glyphIdInit;

	partitionSegments(h);

	/* Initialize segment keys */
	for (i = 0; i < 256; i++) {
		fmt->segmentKeys[i] = 0;
	}

	makeFirstSegment2(h, fmt);

	/* Make all remaining the segments */
	for (i = mapping[0].span; i < h->mapping.cnt; i += mapping[i].span) {
		fmt->segmentKeys[mapping[i].code >> 8] = makeSegment2(h, fmt, i);
	}

	addcodespaceRanges(h, fmt);

	/* Adjust segmentKeys values */
	for (i = 0; i < 256; i++) {
		fmt->segmentKeys[i] *= 8;
	}

	/* Adjust idRangeOffset values */
	for (i = 0; i < fmt->segment.cnt; i++) {
		fmt->segment.array[i].idRangeOffset = (unsigned short)(
		        ((fmt->segment.cnt - i - 1) * SEGMENT2_SIZE +
		         uint16 +
		         fmt->segment.array[i].idRangeOffset * uint16));
	}

	fmt->format = 2;
	fmt->language = h->language;
	fmt->length = (unsigned short)FORMAT2_SIZE(fmt->segment.cnt,
	                                           fmt->glyphId.cnt);
	return fmt;
}

/* ------- end format 2 */

static void setOrdered( Mapping *mapping, int codeBreak, int span, int index)
{
	int threshold;
	/* If the ordered segment starts with a code break or ends with a code
	break, then the cost of keeping this as a separate segment is just one new
	segment break (the following segment break). Else, the cost is adding two
	new segment breaks - the one to start this segment, and to start the next
	segment. We mark as NOT ordered those segments that are no larger than the
	threshold.
	 */
	if ( (mapping[index-span].flags & CODE_BREAK) || codeBreak)
		threshold = 4;
	else
		threshold = 8;
	if (span > threshold)
		mapping[index - span].ordered = 1;

}

/* Partition mappings optimally for format 4. Assumes mapping in code order */
static int partitionRanges(cmapCtx h, Mapping *mapping) {
	int span;
	int ordered;
	int lastOrdered;
	int total;
	int i;
	int nSegments;
	
	/* mapping is sorted by charCode, then by glyphID. If there is a break on
	the character code sequence, we always make a new segment. If there is a
	break in the glyph ID sequence, then we may make a segment break, if doing
	so saves more bytes than not doing so. 
	If a segment has more than one glyph and no breaks in its glyphID sequence,
	then it is set as  ordered. Ordered segments are dropped only if keeping
	them costs less than the new segment cost. All other segments are merged
	togther. idRangeOffset for an ordered segment is set to 0, and its glyphID's
	are not added to the glyphId array, as its glyphIDs can be calculated from
	startCode/endCode/idDelta alone.
	*/
	
	/* Create ordered partitioning. */
	span = 1;
	for (i = 1; i < h->mapping.cnt; i++) {
		int codeBreak = (mapping[i - 1].code + 1 != mapping[i].code);
		if (codeBreak || mapping[i - 1].glyphId + 1 != mapping[i].glyphId) {
			mapping[i - span].span = span;
			mapping[i - span].ordered = 0; /* default */
			if (span > 1) /* an ordered segment is a segment with more than one code point with consecutive glyph ID's */
			{
				setOrdered(mapping, codeBreak, span, i);
			}
			if (codeBreak) {
				mapping[i].flags |= CODE_BREAK;
			}
			span = 1;
		}
		else {
			span++;
		}
	}
	mapping[i - span].span = span;
	if (span > 1)
		setOrdered(mapping, 1, span, i - span);
	else
		mapping[i - span].ordered = 0;
	
	/* Coalesce ranges */
	nSegments = 0;
	total  = span = mapping[0].span;
	lastOrdered =  mapping[0].ordered;
	for (i = span; i < h->mapping.cnt; i += span) {
		int codeBreak = mapping[i].flags & CODE_BREAK;
		ordered = mapping[i].ordered;
		span = mapping[i].span;
		/* merge a segement with the previous segment if
		 1) it does not start with a code break, and
		 2) it is not ordered, and the previous segment is not ordered.
		*/
		if ((!codeBreak) && (!ordered) && (!lastOrdered)) {
			mapping[i - total].span += span;
			total = mapping[i - total].span;
			lastOrdered = ordered;
		}
		else {
			lastOrdered = mapping[i].ordered;
			total = span;
			nSegments++;
		}
	}
	mapping[i - total].span = total;
	mapping[i - total].ordered = ordered;
	nSegments += 2; /* Add last segment and sentinel */

	/* Previously, 'ordered' has been used to mean "keep this as a segment, and
	don't merge it with the prior segment or following segment". Now we need to
	set it to really mean 'the glyphID's in this segment are sequential', as
	this field is used to decide how to use idDelta and idRange.
	 */
	for (i = 0; i < h->mapping.cnt; i += span)
		{
			int j;
			int ordered = 1;
			span = mapping[i].span;
			for (j = i+1; j< i+span;j++)
			{
			   if (mapping[j].glyphId-1 != mapping[j-1].glyphId)
			   {
				   ordered = 0;
				   break;
			   }
			}
			mapping[i].ordered = ordered;
	   }

	return nSegments;
}

/* Make format 4 segment */
static void makeSegment4(hotCtx g, Format4 *fmt, int nSegments, int segment,
                         Mapping *mapping, int start) {
	int i;
	int end = start + mapping[start].span - 1;

	fmt->endCode[segment] = (unsigned short)mapping[end].code;
	fmt->startCode[segment] = (unsigned short)mapping[start].code;

	if (mapping[start].ordered) {
		fmt->idDelta[segment] = mapping[start].glyphId - (unsigned short)mapping[start].code;
		fmt->idRangeOffset[segment] = 0;
	}
	else {
		fmt->idDelta[segment] = 0;
		fmt->idRangeOffset[segment] = (unsigned short)
		    (uint16 * (nSegments - segment) + uint16 * fmt->glyphId.cnt);
		for (i = start; i <= end; i++) {
			*dnaNEXT(fmt->glyphId) = mapping[i].glyphId;
		}
	/* Future improvement: when you are using idRangeOffset, you do not have to
	set idDelta to 0. This could be used to combine sequential segments that are
	not ordered.
	 */
	}
}

/* Make format 4 cmap */
static void doRemovePUAs(hotCtx g, cmapCtx h, int isMixedByte) {
	long i;
	int nPUAs = 0;
	Mapping *mapping = h->mapping.array;

	for (i = 1; i < h->mapping.cnt; i++) {
		if ((mapping[i].code >= 0xE000L)  &&
		    (mapping[i].code <= 0xF8FFL)) {
			nPUAs++;
			mapping[i].flags |= CODE_DELETE;
		}
	}

	/* Remove PUA's */
	if (nPUAs > 0) {
		qsort(mapping, h->mapping.cnt, sizeof(Mapping),
		      isMixedByte ? cmpMixedByteCodes : cmpCodes);
		h->mapping.cnt -= nPUAs;
	}
}

static Format4 *makeFormat4(cmapCtx h, unsigned long *length) {
	hotCtx g = h->g;
	int i;
	int nSegments = 0;
	int segment;
	int notOK = 1;
	int truncateCmap = 0;
	long tempLength;

	Mapping *mapping = h->mapping.array;
	Format4 *fmt = MEM_NEW(h->g, sizeof(Format4));
	dnaINIT(h->g->dnaCtx, fmt->glyphId, 256, 64);

	if (g->convertFlags & HOT_STUB_CMAP4)
		truncateCmap = 1;

	while (notOK) {
		fmt->glyphId.cnt = 0;
		if (h->mapping.cnt == 0)
		{
			Mapping *stub_mapping = dnaNEXT(h->mapping);
			
			stub_mapping->code = 0;
			stub_mapping->glyphId = 0;
			stub_mapping->span = 1;
			stub_mapping->ordered = 1;
			stub_mapping->flags = 0;
			nSegments = 1;
			mapping = h->mapping.array;
		}
		nSegments = partitionRanges(h, mapping);

		if (truncateCmap) {
			nSegments = 2;
		}
		fmt->endCode = MEM_NEW(h->g, sizeof(fmt->endCode[0]) * nSegments);
		fmt->startCode = MEM_NEW(h->g, sizeof(fmt->startCode[0]) * nSegments);
		fmt->idDelta = MEM_NEW(h->g, sizeof(fmt->idDelta[0]) * nSegments);
		fmt->idRangeOffset =
		    MEM_NEW(h->g, sizeof(fmt->idRangeOffset[0]) * nSegments);

		segment = 0;
		for (i = 0; i < h->mapping.cnt; i += mapping[i].span) {
			if (truncateCmap && (segment >= (nSegments - 1))) {
				break;
			}
			makeSegment4(g, fmt, nSegments, segment++, mapping, i);
		}

		/* Add sentinel */
		fmt->endCode[segment] = 0xffff;
		fmt->startCode[segment] = 0xffff;
		fmt->idDelta[segment] = 1;
		fmt->idRangeOffset[segment] = 0;

		fmt->segCountX2 = nSegments * 2;
		fmt->password = 0;
		hotCalcSearchParams(uint16, nSegments, &fmt->searchRange,
		                    &fmt->entrySelector, &fmt->rangeShift);
		/*	hotCalcSearchParams(sizeof(fmt->endCode[0]), nSegments, &fmt->searchRange,
		                        &fmt->entrySelector, &fmt->rangeShift);
		 */

		fmt->format = 4;
		fmt->language = h->language;
		tempLength = FORMAT4_SIZE(nSegments, fmt->glyphId.cnt);
		if (tempLength < 0x10000L) {
			notOK = 0;
			if (truncateCmap) {
				cmapMsg(g, hotWARNING, "cmap format 4 subtable was truncated to 2 segments, due to overflow with non-truncated version");
			}
		}
		else {
			if (truncateCmap) {
				cmapMsg(g, hotFATAL, "Length overflow in cmap format 4 subtable, despite truncating cmap to 2 segments.");
			}
			else {
				truncateCmap = 1;
			}
		}
	} /* end while */

	*length = fmt->length = (unsigned short)tempLength;

	return fmt;
}

/* Make format 6 cmap */
static Format6 *makeFormat6(cmapCtx h, unsigned long *length) {
	int i;
	Mapping *mapping = h->mapping.array;
	Format6 *fmt = MEM_NEW(h->g, sizeof(Format6));

	fmt->firstCode = (unsigned short)mapping[0].code;
	fmt->entryCount = (unsigned short)(mapping[h->mapping.cnt - 1].code - mapping[0].code + 1);
	fmt->glyphId = MEM_NEW(h->g, fmt->entryCount * sizeof(fmt->glyphId[0]));
	for (i = 0; i < fmt->entryCount; i++) {
		fmt->glyphId[i] = 0;
	}
	for (i = 0; i < h->mapping.cnt; i++) {
		fmt->glyphId[mapping[i].code - mapping[0].code] = mapping[i].glyphId;
	}

	fmt->format = 6;
	fmt->language = h->language;
	*length = fmt->length = FORMAT6_SIZE(fmt->entryCount);

	return fmt;
}

/* Make format 12 cmap */
static Format12 *makeFormat12(cmapCtx h, unsigned long *length) {
	long i;
	Mapping *mapping = h->mapping.array;
	long iSegBeg = 0;   /* Index of beginning of segment */
	Format12 *fmt = MEM_NEW(h->g, sizeof(Format12));

	dnaINIT(h->g->dnaCtx, fmt->segment, 40, 80);    /* xxx dynamic numbers? */

	for (i = 1; i <= h->mapping.cnt; i++) {
		if (i == h->mapping.cnt
		    || mapping[i].code != mapping[i - 1].code + 1
		    || mapping[i].glyphId != mapping[i - 1].glyphId + 1) {
			/* Segment starts at [iSegBeg] and ends at [i-1]. Create it: */
			Segment8_12 *seg   = dnaNEXT(fmt->segment);

			seg->startCharCode = mapping[iSegBeg].code;
			seg->endCharCode   = mapping[i - 1].code;
			seg->startGlyphID  = mapping[iSegBeg].glyphId;

			iSegBeg = i;
		}
	}

	fmt->format = 12;
	fmt->reserved = 0;
	fmt->length = FORMAT12_SIZE(fmt->segment.cnt);
	fmt->language = h->language;
	fmt->nGroups = fmt->segment.cnt;

	if (length) {
		*length = fmt->length;
	}

	return fmt;
}

/* Make format 14 cmap */
static Format14 *makeFormat14(cmapCtx h) {
	long i;
	Format14 *fmt = MEM_NEW(h->g, sizeof(Format14));

	i = 0;

	fmt->format = 14;
	fmt->length = (uint16 + 2 * uint32); /* size of header*/
	fmt->length += h->uvsList.cnt * (uint24 + 2 * uint32); /* size of Variation Selector list */
	for (i = 0; i < h->uvsList.cnt; i++) {
		UVSRecord *uvsRec = &h->uvsList.array[i];
		if (uvsRec->dfltUVS.cnt > 0) {
			fmt->length += uint32 + uvsRec->dfltUVS.cnt * (uint24 + uint8); /* size of UV range list for this UVS record */
		}
		if (uvsRec->extUVS.cnt > 0) {
			fmt->length += uint32 + uvsRec->extUVS.cnt * (uint24 + uint16); /* size of UV range list for this UVS record */
		}
	}

	fmt->numUVS = h->uvsList.cnt;
	return fmt;
}

/* Free format 0 cmap */
static void freeFormat0(hotCtx g, Format0 *fmt) {
	MEM_FREE(g, fmt);
}

/* Free format 2 cmap */
static void freeFormat2(hotCtx g, Format2 *fmt) {
	dnaFREE(fmt->segment);
	dnaFREE(fmt->glyphId);
	MEM_FREE(g, fmt);
}

/* Free format 4 cmap */
static void freeFormat4(hotCtx g, Format4 *fmt) {
	MEM_FREE(g, fmt->endCode);
	MEM_FREE(g, fmt->startCode);
	MEM_FREE(g, fmt->idDelta);
	MEM_FREE(g, fmt->idRangeOffset);
	dnaFREE(fmt->glyphId);
	MEM_FREE(g, fmt);
}

/* Free format 6 cmap */
static void freeFormat6(hotCtx g, Format6 *fmt) {
	MEM_FREE(g, fmt->glyphId);
	MEM_FREE(g, fmt);
}

/* Free format 12 cmap */
static void freeFormat12(hotCtx g, Format12 *fmt) {
	dnaFREE(fmt->segment);
	MEM_FREE(g, fmt);
}

/* Free format 14 cmap */
static void freeFormat14(hotCtx g, Format14 *fmt) {
	cmapCtx h = g->ctx.cmap;
	long i = 0;
	while (i < h->uvsList.cnt) {
		UVSRecord *uvsRec = &h->uvsList.array[i];
		dnaFREE(uvsRec->dfltUVS);
		dnaFREE(uvsRec->extUVS);
		i++;
	}
	MEM_FREE(g, fmt);
}

/* Check for duplicate codes. Client-specified Mac C0 control mappings take
   priority to the ones added by this module. */
static void checkDuplicates(hotCtx g, cmapCtx h, int isMixedByte) {
	long i;
	int nDupes = 0;
	Mapping *mapping = h->mapping.array;

	for (i = 1; i < h->mapping.cnt; i++) {
		if (mapping[i - 1].code == mapping[i].code &&
		    (mapping[i - 1].flags & CODE_1BYTE) ==
		    (mapping[i].flags & CODE_1BYTE)) {
			if (mapping[i - 1].flags & CODE_MACCNTL) {
				nDupes++;
				mapping[i - 1].flags |= CODE_DELETE;
			}
			else {
				featGlyphDump(g, mapping[i - 1].glyphId, ',', 0);
				featGlyphDump(g, mapping[i].glyphId, 0, 0);
				cmapMsg(g, hotFATAL, "multiple glyphs (%s) mapped to code <%hX>",
				        g->note.array, mapping[i].code);
			}
		}
	}

	/* Remove duplicates */
	if (nDupes > 0) {
		qsort(mapping, h->mapping.cnt, sizeof(Mapping),
		      isMixedByte ? cmpMixedByteCodes : cmpCodes);
		h->mapping.cnt -= nDupes;
	}
}

/* Finish an encoding (adding special Mac C0 controls, if necessary), select
   its format, make the subtable. If no mappings were added, it returns 0 (no
   Encoding will be created); otherwise, 1. */
int cmapEndEncoding(hotCtx g) {
	cmapCtx h = g->ctx.cmap;
	unsigned long format4Size;
	Encoding *encoding;
	int isMixedByte;

	if (h->mapping.cnt == 0) {
		cmapMsg(g, hotWARNING, "empty encoding");

		if (h->platformId == cmap_MAC) {
			/* Mac cmap is required; the space wasn't present, so: */
			cmapAddMapping(g, 0, GID_NOTDEF, 1);
		}
 		else if (!((IS_CID(g)) && (h->platformId == cmap_MS) && ( h->scriptId == cmap_MS_UGL))){
            /* Ms cmap is required; the space wasn't present, so: */
			return 0;
		}
	}

	encoding = dnaNEXT(h->tbl.encoding);

	/* Assign id's incrementally */
	encoding->id = (h->tbl.encoding.cnt == 1) ? 0 : (encoding - 1)->id + 1;

	encoding->platformId = h->platformId;
	encoding->scriptId = h->scriptId;

	isMixedByte = h->flags & SEEN_1BYTE && h->flags & SEEN_2BYTE;
	qsort(h->mapping.array, h->mapping.cnt, sizeof(Mapping),
	      isMixedByte ? cmpMixedByteCodes : cmpCodes);
	checkDuplicates(g, h, isMixedByte);

#if 0 /* xxx Remove after testing */
	if (encoding->platformId == 1) {
		long i;
		for (i = 0; i < h->mapping.cnt; i++) {
			Mapping *map = &h->mapping.array[i];
			if (map->flags & CODE_1BYTE) {
				printf("[%02hx]=%hu\n", map->code, map->glyphId);
			}
			else {
				printf("[%04hx]=%hu\n", map->code, map->glyphId);
			}
		}
	}
#endif

	/* Choose most suitable format from 0, 2, 4, 6, or 12 */
	if (isMixedByte) {
		if (h->platformId != cmap_MAC) {
			cmapMsg(g, hotWARNING, "platform %hd cmap is mixed-byte",
			        h->platformId);
		}
		encoding->format = makeFormat2(h);
	}
	else if (h->platformId == cmap_MS) {
		/* Microsoft platform always has format restrictions! */

		if (h->flags & SEEN_1BYTE) {
			cmapMsg(g, hotWARNING, "single-byte codes in MS platform");
		}


		if (h->flags & SEEN_4BYTE) {
			if (h->flags & SEEN_2BYTE) {
				cmapMsg(g, hotWARNING, "2-byte codes in 4-byte MS encoding");
			}

			encoding->format = makeFormat12(h, NULL);
		}
		else {
			encoding->format = makeFormat4(h, &format4Size);
		}
	}
	else if (h->maxCode > 0xFFFF) {
		cmapMsg(g, hotFATAL, "[internal] maxCode > 0xFFFF");
	}
	else {
		/* Choose between formats 0, 4 and 6, if possible and allowed.
		   o ATM/NT 4.1 and NT 5 restrictions: custom cmap must be format 0 or
		     6, but due to a bug in the NT 5 driver's format 6-reading code,
		     choose 0 whenever possible, even if 6 would be smaller in size.
		   o InDesign 1.0 CoolType restrictions: supports only format 0 or 6
		     Mac cmap. */

		int format4Selected = 0;
		unsigned long format4Size = ULONG_MAX; /* Denotes fmt 4 not allowed */
		Format4 *format4 =
		    (h->platformId == cmap_MAC || h->platformId == cmap_CUSTOM)
		    ? NULL : makeFormat4(h, &format4Size);
		unsigned long format6Size =
		    FORMAT6_SIZE(h->mapping.array[h->mapping.cnt - 1].code -
		                 h->mapping.array[0].code + 1);

		if (h->maxCode < 256 && h->maxGlyphId < 256 /* ie if fmt 0 possible */
		    && (h->platformId == cmap_CUSTOM ||
		        (FORMAT0_SIZE <= format4Size && FORMAT0_SIZE <= format6Size))) {
			encoding->format = makeFormat0(h);
		}
		else if (format4Size < format6Size) {
#if HOT_DEBUG
			cmapMsg(g, hotNOTE, "format 4 cmap created");
#endif
			format4Selected = 1;
			encoding->format = format4;
		}
		else {
			encoding->format = makeFormat6(h, &format6Size);

			/* A client may want to intercept this note if it wants to warn
			   customers that the font won't work in NT 5. */
			if (h->platformId == cmap_CUSTOM) {
				cmapMsg(g, hotNOTE, "format 6 custom cmap created");
			}
		}
	}

	return 1;
}

void cmapEndUVSEncoding(hotCtx g) {
	cmapCtx h = g->ctx.cmap;
	Encoding *encoding;

	encoding = dnaNEXT(h->tbl.encoding);

	/* Assign id's incrementally */
	encoding->id = (h->tbl.encoding.cnt == 1) ? 0 : (encoding - 1)->id + 1;

	encoding->platformId = h->platformId;
	encoding->scriptId = h->scriptId;
	encoding->format = makeFormat14(h);
}

/* Create an encoding which points to the same subtable as the last Encoding
   added by cmapBeginEncoding()..cmapEndEncoding(). Note: it returns silently
   if no Encoding was previously added. */
void cmapPointToPreviousEncoding(hotCtx g, unsigned platform,
                                 unsigned platformSpecific) {
	cmapCtx h = g->ctx.cmap;
	Encoding *newEncoding;

	if (h->tbl.encoding.cnt == 0) {
		return;
	}

	newEncoding = dnaNEXT(h->tbl.encoding);
	newEncoding->id = (newEncoding - 1)->id;
	newEncoding->platformId = platform;
	newEncoding->scriptId = platformSpecific;
	newEncoding->format = NULL; /* Indicates a "link" */
}

/* Compare encodings */
static int CDECL cmpEncodings(const void *first, const void *second) {
	const Encoding *a = first;
	const Encoding *b = second;
	if (a->platformId < b->platformId) {
		return -1;
	}
	else if (a->platformId > b->platformId) {
		return 1;
	}
	else if (a->scriptId < b->scriptId) {
		return -1;
	}
	else if (a->scriptId > b->scriptId) {
		return 1;
	}
	else {
		unsigned long lang1;
		unsigned long lang2;

		if (a->format == NULL || b->format == NULL) {
			return 0;
		}

		switch (GET_FORMAT((a->format))) {
			case 14: {
				lang1 = 0;
				break;
			}

			case 8:
			case 10:
			case 12: {
				lang1 = ((FormatHdrExt *)(a->format))->language;
				break;
			}

			default:
				lang1 = ((FormatHdr *)(a->format))->language;
		}
		switch (GET_FORMAT((b->format))) {
			case 14: {
				lang2 = 0;
				break;
			}

			case 8:
			case 10:
			case 12: {
				lang2 = ((FormatHdrExt *)(b->format))->language;
				break;
			}

			default:
				lang2 = ((FormatHdr *)(b->format))->language;
		}

		if (lang1 < lang2) {
			return -1;
		}
		else if (lang1 > lang2) {
			return 1;
		}
		return 0;
	}
}

/* Check for Macintosh platform */
static int checkMacPlatform(cmapCtx h) {
	int i;

	for (i = 0; i < h->tbl.encoding.cnt; i++) {
		if (h->tbl.encoding.array[i].platformId == cmap_MAC) {
			return 1;
		}
	}
	return 0;
}

/* Fill all data structures in preparation for writing */
int cmapFill(hotCtx g) {
	cmapCtx h = g->ctx.cmap;
	int i;
	Encoding *encoding = h->tbl.encoding.array;
	unsigned long offset;

	if (h->tbl.encoding.cnt == 0) {
		hotMsg(h->g, hotFATAL, "no cmap table specified");
	}

	if (!checkMacPlatform(h)) {
		hotMsg(h->g, hotWARNING, "no Mac cmap specified");
	}

	h->tbl.version = cmap_VERSION;

	h->tbl.nEncodings = (unsigned short)h->tbl.encoding.cnt;
	offset = TBL_HDR_SIZE + ENCODING_SIZE * h->tbl.nEncodings;

	/* Sort encodings and fill offsets */
	qsort(encoding, h->tbl.nEncodings, sizeof(Encoding), cmpEncodings);
	for (i = 0; i < h->tbl.nEncodings; i++) {
		if (encoding[i].format != NULL) {
			unsigned short formatID;
			formatID  = GET_FORMAT(encoding[i].format);
			encoding[i].offset = offset;
			switch (formatID) {
				case 14: {
					offset += ((FormatHdr14 *)(encoding[i].format))->length;
					break;
				}

				case 8:
				case 10:
				case 12: {
					offset += ((FormatHdrExt *)(encoding[i].format))->length;
					break;
				}

				default:
					offset += ((FormatHdr *)(encoding[i].format))->length;
			}
		}
	}

	/* Second pass to fill in offsets for links */
	for (i = 0; i < h->tbl.nEncodings; i++) {
		if (encoding[i].format == NULL) {
			int j;
			for (j = 0; j < h->tbl.nEncodings; j++) {
				if (encoding[j].id == encoding[i].id &&
				    encoding[j].format != NULL) {
					encoding[i].offset = encoding[j].offset;
					break;
				}
			}
		}
	}

	return 1;
}

/* Write format 0 cmap */
static void writeFormat0(cmapCtx h, Format0 *fmt) {
	int i;

	OUT2(fmt->format);
	OUT2(fmt->length);
	OUT2(fmt->language);

	for (i = 0; i < 256; i++) {
		OUT1(fmt->glyphId[i]);
	}
}

/* Write format 2 cmap */
static void writeFormat2(cmapCtx h, Format2 *fmt) {
	int i;

	OUT2(fmt->format);
	OUT2(fmt->length);
	OUT2(fmt->language);

	for (i = 0; i < 256; i++) {
		OUT2(fmt->segmentKeys[i]);
	}

	for (i = 0; i < fmt->segment.cnt; i++) {
		Segment2 *segment = &fmt->segment.array[i];

		OUT2(segment->firstCode);
		OUT2(segment->entryCount);
		OUT2(segment->idDelta);
		OUT2(segment->idRangeOffset);
	}

	for (i = 0; i < fmt->glyphId.cnt; i++) {
		OUT2(fmt->glyphId.array[i]);
	}
}

/* Write format 4 cmap */
static void writeFormat4(cmapCtx h, Format4 *fmt) {
	int i;
	int nSegments = fmt->segCountX2 / 2;
	GID *glyphId = fmt->glyphId.array;

	OUT2(fmt->format);
	OUT2(fmt->length);
	OUT2(fmt->language);
	OUT2(fmt->segCountX2);
	OUT2(fmt->searchRange);
	OUT2(fmt->entrySelector);
	OUT2(fmt->rangeShift);

	for (i = 0; i < nSegments; i++) {
		OUT2(fmt->endCode[i]);
	}

	OUT2(fmt->password);

	for (i = 0; i < nSegments; i++) {
		OUT2(fmt->startCode[i]);
	}

	for (i = 0; i < nSegments; i++) {
		OUT2(fmt->idDelta[i]);
	}

	for (i = 0; i < nSegments; i++) {
		OUT2(fmt->idRangeOffset[i]);
	}

	for (i = 0; i < fmt->glyphId.cnt; i++) {
		OUT2(glyphId[i]);
	}
}

/* Write format 6 cmap */
static void writeFormat6(cmapCtx h, Format6 *fmt) {
	int i;

	OUT2(fmt->format);
	OUT2(fmt->length);
	OUT2(fmt->language);
	OUT2(fmt->firstCode);
	OUT2(fmt->entryCount);

	for (i = 0; i < fmt->entryCount; i++) {
		OUT2(fmt->glyphId[i]);
	}
}

/* Write format 12 cmap */
static void writeFormat12(cmapCtx h, Format12 *fmt) {
	long i;

	OUT2(fmt->format);
	OUT2(fmt->reserved);
	OUT4(fmt->length);
	OUT4(fmt->language);
	OUT4(fmt->nGroups);

	for (i = 0; i < fmt->segment.cnt; i++) {
		Segment8_12 *seg = &fmt->segment.array[i];

		OUT4(seg->startCharCode);
		OUT4(seg->endCharCode);
		OUT4(seg->startGlyphID);
	}
}

/* Write format 12 cmap */
static void writeFormat14(cmapCtx h, Format14 *fmt) {
	long i;
	hotCtx g = h->g;
	unsigned long numDefaultUVSEntries = 0;
	unsigned long numExtUVSEntries = 0;
	unsigned long offset = uint16 + 2 * uint32 + h->uvsList.cnt * (uint24 + 2 * uint32); /* length of table to end of UVS records.*/

	OUT2(fmt->format);
	OUT4(fmt->length);
	OUT4(fmt->numUVS);
	for (i = 0; i < h->uvsList.cnt; i++) {
		UVSRecord *uvsRec = dnaINDEX(h->uvsList, i);

		OUT3(uvsRec->uvs);
		if (uvsRec->dfltUVS.cnt == 0) {
			OUT4(0);
		}
		else {
			OUT4(offset);
			numDefaultUVSEntries += uvsRec->dfltUVS.cnt;
			offset += uint32 + (uvsRec->dfltUVS.cnt * (uint24 + uint8));
		}

		if (uvsRec->extUVS.cnt == 0) {
			OUT4(0);
		}
		else {
			OUT4(offset);
			numExtUVSEntries += uvsRec->extUVS.cnt;
			offset += uint32 + (uvsRec->extUVS.cnt * (uint24 + uint16));
		}
	}

	/* Now write out the tables. */
	for (i = 0; i < h->uvsList.cnt; i++) {
		UVSRecord *uvsRec = dnaINDEX(h->uvsList, i);

		if (uvsRec->dfltUVS.cnt > 0) {
			int j;
			OUT4(uvsRec->dfltUVS.cnt);
			for (j = 0; j < uvsRec->dfltUVS.cnt; j++) {
				DefaultUVSEntryRange *dfltUVSER = dnaINDEX(uvsRec->dfltUVS, j);
				OUT3(dfltUVSER->uv);
				OUT1(dfltUVSER->addtlCnt);
				numDefaultUVSEntries += dfltUVSER->addtlCnt;
			}
		}

		if (uvsRec->extUVS.cnt > 0) {
			int j;
			OUT4(uvsRec->extUVS.cnt);
			for (j = 0; j < uvsRec->extUVS.cnt; j++) {
				ExtUVSEntry *exttUVSER = dnaINDEX(uvsRec->extUVS, j);
				OUT3(exttUVSER->uv);
				OUT2(exttUVSER->glyphID);
			}
		}
	}
	cmapMsg(g, hotNOTE, "Number of default Unicode Variation Sequence values %d", numDefaultUVSEntries);
	cmapMsg(g, hotNOTE, "Number of non-default UVS values %d", numExtUVSEntries);
}

void cmapWrite(hotCtx g) {
	cmapCtx h = g->ctx.cmap;
	int i;

	OUT2(h->tbl.version);
	OUT2(h->tbl.nEncodings);

	for (i = 0; i < h->tbl.nEncodings; i++) {
		Encoding *encoding = &h->tbl.encoding.array[i];

		OUT2(encoding->platformId);
		OUT2(encoding->scriptId);
		OUT4(encoding->offset);
	}

	for (i = 0; i < h->tbl.nEncodings; i++) {
		void *format = h->tbl.encoding.array[i].format;

		if (format == NULL) {
			continue;
		}

		switch (GET_FORMAT(format)) {
			case 0:
				writeFormat0(h, format);
				break;

			case 2:
				writeFormat2(h, format);
				break;

			case 4:
				writeFormat4(h, format);
				break;

			case 6:
				writeFormat6(h, format);
				break;

			case 12:
				writeFormat12(h, format);
				break;

			case 14:
				writeFormat14(h, format);
				break;
		}
	}
}

void cmapReuse(hotCtx g) {
	cmapCtx h = g->ctx.cmap;
	int i;

	for (i = 0; i < h->tbl.nEncodings; i++) {
		void *format = h->tbl.encoding.array[i].format;

		if (format == NULL) {
			continue;
		}

		switch (GET_FORMAT(format)) {
			case 0:
				freeFormat0(g, format);
				break;

			case 2:
				freeFormat2(g, format);
				break;

			case 4:
				freeFormat4(g, format);
				break;

			case 6:
				freeFormat6(g, format);
				break;

			case 12:
				freeFormat12(g, format);
				break;

			case 14:
				freeFormat14(g, format);
				break;
		}
	}
	h->tbl.nEncodings = 0;
	h->tbl.encoding.cnt = 0;
	h->lastUVS = 0;
	h->uvsList.cnt = 0;
	h->mapping.cnt = 0;
	h->codespace.cnt = 0;
}

/* Free memory for this context */
void cmapFree(hotCtx g) {
	cmapCtx h = g->ctx.cmap;
	cmapReuse(g); /* frees all the encodings in use */
	dnaFREE(h->tbl.encoding);
	dnaFREE(h->uvsList);
	dnaFREE(h->mapping);
	dnaFREE(h->codespace);
	MEM_FREE(g, h);
}
