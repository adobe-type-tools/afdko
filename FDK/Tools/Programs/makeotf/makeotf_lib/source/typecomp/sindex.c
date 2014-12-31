/* @(#)CM_VerSion string.c atm08 1.2 16245.eco sum= 49937 atm08.002 */
/* @(#)CM_VerSion string.c atm07 1.2 16164.eco sum= 13682 atm07.012 */
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

#include "sindex.h"

#include <stdlib.h>

#if PLAT_SUN4
#include "sun4/fixstring.h"
#else /* PLAT_SUN4 */
#include <string.h>
#endif  /* PLAT_SUN4 */

/* Standard string ids */
typedef struct {
	SID id;
	char *strng;
} StdId;

/* Standard string names */
static StdId str2sid[] = {
#include "stdstr0.h"
};
#define STD_STR_CNT TABLE_LEN(str2sid)

/* Maps string to id */
typedef struct {
	long index; /* String buffer index */
	unsigned short length;
	SID id;
} Map;

/* Module context */
struct sindexCtx_ {
	dnaDCL(Map, maps);      /* String to id map */
	dnaDCL(char, strings);  /* String buffer */
	dnaDCL(char, seen);     /* Flags SIDs already seen in font */
	tcCtx g;                /* Package context */
};

/* Initialize seen element */
static void initSeen(void *ctx, long count, char *seen) {
	long i;
	for (i = 0; i < count; i++) {
		*seen = 0;
		seen++;
	}
	return;
}

/* Initialize module */
void sindexNew(tcCtx g) {
	sindexCtx h = MEM_NEW(g, sizeof(struct sindexCtx_));

	dnaINIT(g->ctx.dnaCtx, h->maps, 260, 1000);
	dnaINIT(g->ctx.dnaCtx, h->strings, 1500, 3000);
	dnaINIT(g->ctx.dnaCtx, h->seen, 450, 1000);
	h->seen.func = initSeen;

	/* Link contexts */
	h->g = g;
	g->ctx.sindex = h;
}

/* Free resources */
void sindexFree(tcCtx g) {
	sindexCtx h = g->ctx.sindex;
	dnaFREE(h->maps);
	dnaFREE(h->strings);
	dnaFREE(h->seen);
	MEM_FREE(g, g->ctx.sindex);
}

/* Match string */
static int CDECL matchString(const void *key, const void *value) {
	const String *strng = key;
	return tc_strncmp(strng->data, strng->length, ((StdId *)value)->strng);
}

/* Search standard strings and return id if found else SID_UNDEF. */
static SID getStdId(sindexCtx h, char *strng, unsigned length) {
	StdId *pStd;
	String key;

	key.length = length;
	key.data = strng;

	pStd = (StdId *)bsearch(&key, str2sid, STD_STR_CNT,
	                        sizeof(StdId), matchString);

	return (pStd != NULL) ? pStd->id : SID_UNDEF;
}

/* Lookup string in index and return its index. Returns 1 if found else 0.
   Found or insertion position returned via index parameter. */
static int lookup(sindexCtx h, unsigned length, char *strng, long *index) {
	long lo = 0;
	long hi = h->maps.cnt - 1;

	while (lo <= hi) {
		long mid = (lo + hi) / 2;
		Map *map = &h->maps.array[mid];
		if (length > map->length) {
			lo = mid + 1;
		}
		else if (length < map->length) {
			hi = mid - 1;
		}
		else {
			/* String lengths equal, compare strings */
			int cmp = memcmp(strng, &h->strings.array[map->index], length);
			if (cmp > 0) {
				lo = mid + 1;
			}
			else if (cmp < 0) {
				hi = mid - 1;
			}
			else {
				*index = mid;
				return 1;       /* Found */
			}
		}
	}
	*index = lo;
	return 0;   /* Not found */
}

/* Lookup string and return its id. If doesn't exist it's added to table */
SID sindexGetId(tcCtx g, unsigned length, char *strng) {
	sindexCtx h = g->ctx.sindex;
	long index;
	SID id = getStdId(h, strng, length);
	if (id != SID_UNDEF) {
		return id;
	}

	/* Search custom strings */
	if (!lookup(h, length, strng, &index)) {
		/* Add string */
		Map *new = &dnaGROW(h->maps, h->maps.cnt)[index];

		/* Make hole */
		COPY(new + 1, new, h->maps.cnt - index);

		/* Fill record */
		new->id = (SID)(h->maps.cnt++ + STD_STR_CNT);
		new->length = length;
		new->index = h->strings.cnt;
		memcpy(dnaEXTEND(h->strings, (long)length), strng, length);
		return new->id;
	}
	return h->maps.array[index].id;
}

/* Lookup glyph name string (that is supposed to be unique) and return its id
   if not seen already else return SID_UNDEF. */
SID sindexGetGlyphNameId(tcCtx g, unsigned length, char *strng) {
	sindexCtx h = g->ctx.sindex;
	SID id = sindexGetId(g, length, strng);
	char *seen = dnaMAX(h->seen, id);
	if (*seen) {
		/* Flag this glyph as already seen */
		return SID_UNDEF;
	}
	*seen = 1;  /* Mark SID as seen for subsequent test */
	return (SID)id;
}

/* Return 1 if glyph name id has been seen else 0. */
int sindexSeenGlyphNameId(tcCtx g, SID sid) {
	sindexCtx h = g->ctx.sindex;
	return (sid <= h->seen.cnt) ? h->seen.array[sid] : 0;
}

/* Lookup string and return its id. If doesn't exist return SID_UNDEF. */
SID sindexCheckId(tcCtx g, unsigned length, char *strng) {
	sindexCtx h = g->ctx.sindex;
	long index;
	SID id = getStdId(h, strng, length);
	if (id != SID_UNDEF) {
		return id;
	}

	/* Search custom strings */
	if (lookup(h, length, strng, &index)) {
		return h->maps.array[index].id;
	}

	return SID_UNDEF;
}

/* Get string from id. Only used for diagnostics so linear search is OK */
char *sindexGetString(tcCtx g, SID id) {
	static char *sid2str[] = {
#include "stdstr1.h"
	};
	sindexCtx h = g->ctx.sindex;
	long i;

	if (id < STD_STR_CNT) {
		return sid2str[id]; /* Return standard string */
	}
	/* Search custom strings */
	for (i = 0; i < h->maps.cnt; i++) {
		Map *map = &h->maps.array[i];
		if (map->id == id) {
			static char strng[101];
			int length = (map->length > 100) ? 100 : map->length;
			memcpy(strng, &h->strings.array[map->index], length);
			strng[length] = '\0';
			return strng;
		}
	}
	tcFatal(g, "can't get string for id [%d]", id);

	return NULL;    /* Suppress compiler warning */
}

/* Calculate table size */
long sindexSize(tcCtx g) {
	sindexCtx h = g->ctx.sindex;
	return INDEX_SIZE(h->maps.cnt, h->strings.cnt);
}

/* Compare string ids */
static int CDECL cmpStringIds(const void *first, const void *second) {
	SID a = ((Map *)first)->id;
	SID b = ((Map *)second)->id;
	if (a < b) {
		return -1;
	}
	else if (a > b) {
		return 1;
	}
	else {
		return 0;
	}
}

/* Initialize for new font. */
void sindexFontInit(tcCtx g) {
	sindexCtx h = g->ctx.sindex;
	if (h->seen.size != 0) {
		memset(h->seen.array, 0, h->seen.cnt + 1);
	}
	h->seen.cnt = 0;
}

/* Get module into reusable state */
static void reuseInit(tcCtx g, sindexCtx h) {
	h->maps.cnt = 0;
	h->strings.cnt = 0;
}

/* Write string table */
void sindexWrite(tcCtx g) {
	sindexCtx h = g->ctx.sindex;
	long i;
	Offset offset;
	INDEXHdr header;

	/* Construct header */
	header.count = (unsigned short)h->maps.cnt;
	header.offSize = INDEX_OFF_SIZE(h->strings.cnt);

	OUT2(header.count);

	if (header.count == 0) {
		return; /* Empty table just has zero count */
	}
	OUT1(header.offSize);

	/* Sort table into string id order */
	qsort(h->maps.array, h->maps.cnt, sizeof(Map), cmpStringIds);

	/* Write string offset array */
	offset = 1;
	OUTOFF(header.offSize, offset);
	for (i = 0; i < h->maps.cnt; i++) {
		offset += h->maps.array[i].length;
		OUTOFF(header.offSize, offset);
	}

	/* Write string data */
	OUTN(h->strings.cnt, h->strings.array);

	reuseInit(g, h);
}