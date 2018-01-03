/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

#include "cffwrite_sindex.h"

#include "ctutil.h"

#if PLAT_SUN4
#include "sun4/fixstring.h"
#else /* PLAT_SUN4 */
#include <string.h>
#endif /* PLAT_SUN4 */

#include <stdlib.h>

/* Standard strings */
typedef struct      /* Standard string record */
{
	SID sid;
	char *string;
} StdRec;

static const StdRec std2sid[] = /* Map standard string to SID */
{
#include "stdstr0.h"
};
#define STD_STR_CNT ARRAY_LEN(std2sid)

static char *sid2std[] =    /* Map SID to standard string */
{
#include "stdstr1.h"
};

typedef struct      /* Custom string record */
{
	long iString;   /* String buffer index */
	SID sid;
} CustomRec;

/* Module context */
struct sindexCtx_ {
	dnaDCL(CustomRec, custom);      /* Custom string records */
	dnaDCL(char, strings);          /* Custom string buffer */
	dnaDCL(unsigned short, byName); /* Custom strings by name */
	SID nextid;                     /* Next custom id */
	cfwCtx g;                       /* Package context */
};

/* Initialize module. */
void cfwSindexNew(cfwCtx g) {
	sindexCtx h = (sindexCtx)cfwMemNew(g, sizeof(struct sindexCtx_));

	/* Link contexts */
	h->g = g;
	g->ctx.sindex = h;

	dnaINIT(g->ctx.dnaSafe, h->custom, 260, 1000);
	dnaINIT(g->ctx.dnaSafe, h->strings, 1500, 3000);
	dnaINIT(g->ctx.dnaSafe, h->byName, 260, 1000);
	h->nextid = STD_STR_CNT;
}

/* Get module into reusable state. */
void cfwSindexReuse(cfwCtx g) {
	sindexCtx h = g->ctx.sindex;

	h->custom.cnt = 0;
	h->strings.cnt = 0;
	h->byName.cnt = 0;
	h->nextid = STD_STR_CNT;
}

/* Free resources. */
void cfwSindexFree(cfwCtx g) {
	sindexCtx h = g->ctx.sindex;

	if (h == NULL) {
		return;
	}

	dnaFREE(h->custom);
	dnaFREE(h->strings);
	dnaFREE(h->byName);

	cfwMemFree(g, h);
	g->ctx.sindex = NULL;
}

/* Match standard string. */
static int CTL_CDECL matchStdStr(const void *key, const void *value) {
	return strcmp((char *)key, ((StdRec *)value)->string);
}

/* Match non-standard string. */
static int CTL_CDECL matchNonStdStr(const void *key, const void *value,
                                    void *ctx) {
	sindexCtx h = (sindexCtx)ctx;
	return strcmp((char *)key, &h->strings.array
	              [h->custom.array[*(unsigned short *)value].iString]);
}

/* Add string. If standard string return its SID, otherwise if in table return
   existing record index, else add to table and return new record index. If
   string is empty return SRI_UNDEF. */
SRI cfwSindexAddString(cfwCtx g, char *string) {
	sindexCtx h = g->ctx.sindex;
	size_t index;
	StdRec *std;

	if (string == NULL || *string == '\0') {
		return SRI_UNDEF;   /* Reject invalid strings */
	}
	std = (StdRec *)bsearch(string, std2sid, STD_STR_CNT,
	                        sizeof(StdRec), matchStdStr);
	if (std != NULL) {
		/* Standard string; return its SID */
		return std->sid;
	}

	/* Search custom strings */
	if (ctuLookup(string, h->byName.array, h->byName.cnt,
	              sizeof(h->byName.array[0]), matchNonStdStr, &index, h)) {
		/* Match found; return custom record index */
		return h->byName.array[index] + STD_STR_CNT;
	}
	else {
		/* Not found; add to table */
		CustomRec *custom;
        size_t stringLen;
        
		unsigned short *_new = &dnaGROW(h->byName, h->byName.cnt)[index];

		/* Make and fill hole */
		memmove(_new + 1, _new,
		        (h->byName.cnt++ - index) * sizeof(h->byName.array[0]));
		*_new = (unsigned short)h->custom.cnt;

		/* Allocate and fill new record */
		custom = dnaNEXT(h->custom);
		custom->iString = h->strings.cnt;
		custom->sid = SID_UNDEF;

		/* Save string */
		/* 64-bit warning fixed by cast here */
        stringLen = (long)strlen(string) + 1;
		STRCPY_S(dnaEXTEND(h->strings, (long)stringLen), stringLen, string);

		return *_new + STD_STR_CNT;
	}
}

/* Get string from SRI. */
char *cfwSindexGetString(cfwCtx g, SRI index) {
	sindexCtx h = g->ctx.sindex;
	if (index < STD_STR_CNT) {
		return sid2std[index];
	}
	else {
		return &h->strings.array[h->custom.array[index - STD_STR_CNT].iString];
	}
}

/* Assign the next custom SID to the specified custom string. */
SID cfwSindexAssignSID(cfwCtx g, SRI index) {
	sindexCtx h = g->ctx.sindex;
	if (index < STD_STR_CNT) {
		return index;
	}
	else {
		CustomRec *custom = &h->custom.array[index - STD_STR_CNT];
		if (custom->sid == SID_UNDEF) {
			custom->sid = h->nextid++;
		}
		return custom->sid;
	}
}

/* Return table size. */
long cfwSindexSize(cfwCtx g) {
	sindexCtx h = g->ctx.sindex;
	long i;
	long cnt = 0;
	long size = 0;
	for (i = 0; i < h->custom.cnt; i++) {
		CustomRec *custom = &h->custom.array[i];
		if (custom->sid != SID_UNDEF) {
			cnt++;
			/* 64-bit warning fixed by cast here */
			size += (long)strlen(&h->strings.array[custom->iString]);
		}
	}
	return INDEX_SIZE(cnt, size);
}

/* Compare custom string ids. */
static int CTL_CDECL cmpCustomIds(const void *first, const void *second) {
	SID a = ((CustomRec *)first)->sid;
	SID b = ((CustomRec *)second)->sid;
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

/* Write string table. */
void cfwSindexWrite(cfwCtx g) {
	sindexCtx h = g->ctx.sindex;
	long i;
	Offset offset;
	INDEX index;

	/* Sort custom records into SID order */
	qsort(h->custom.array, h->custom.cnt, sizeof(CustomRec), cmpCustomIds);

	/* Remove unused strings */
	index.datasize = 0;
	for (i = 0; i < h->custom.cnt; i++) {
		CustomRec *custom = &h->custom.array[i];
		if (custom->sid == SID_UNDEF) {
			break;
		}
		else {
			/* 64-bit warning fixed by cast here */
			index.datasize += (long)strlen(&h->strings.array[custom->iString]);
		}
	}
	h->custom.cnt = i;

	/* Construct header */
	index.count = (unsigned short)h->custom.cnt;
	index.offSize = INDEX_OFF_SIZE(index.datasize);

	cfwWrite2(g, (unsigned short)index.count);

	if (index.count == 0) {
		return; /* Empty table just has zero count */
	}
	cfwWrite1(g, index.offSize);

	/* Write string offset array */
	offset = 1;
	cfwWriteN(g, index.offSize, offset);
	for (i = 0; i < h->custom.cnt; i++) {
		/* 64-bit warning fixed by cast here */
		offset += (Offset)strlen(&h->strings.array[h->custom.array[i].iString]);
		cfwWriteN(g, index.offSize, offset);
	}

	/* Write string data */
	for (i = 0; i < h->custom.cnt; i++) {
		char *string = &h->strings.array[h->custom.array[i].iString];
		cfwWrite(g, strlen(string), string);
	}
}
