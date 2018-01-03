/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/*
 *	Font header table.
 */

#include "head.h"

#ifdef _WIN32
#include <time.h>
#else
#include <sys/time.h>
#endif

/* ---------------------------- Table Definition --------------------------- */

#define DATE_TIME_SIZE  8
typedef char longDateTime[DATE_TIME_SIZE];

typedef struct {
	Fixed version;
	Fixed fontRevision;
	unsigned long checkSumAdjustment;
	unsigned long magicNumber;
	unsigned short flags;
#define head_BASELINE_0     (1 << 0)
#define head_LSB_0          (1 << 1)
#define head_CONVERTED_FONT (1 << 12)
	unsigned short unitsPerEm;
	longDateTime created;
	longDateTime modified;
	FWord xMin;
	FWord yMin;
	FWord xMax;
	FWord yMax;
	unsigned short macStyle;
	unsigned short lowestRecPPEM;
#define head_LOWEST_PPEM 3
	short fontDirectionHint;
#define head_L2R_PLUS_NEUTRALS  2   /* xxx should I support other values? */
	short indexToLocFormat;
	short glyphDataFormat;
} headTbl;

/* --------------------------- Context Definition -------------------------- */
struct headCtx_ {
	headTbl tbl;    /* Table data */
	hotCtx g;       /* Package context */
};

/* --------------------------- Standard Functions -------------------------- */

void headNew(hotCtx g) {
	headCtx h = MEM_NEW(g, sizeof(struct headCtx_));

	/* Link contexts */
	h->g = g;
	g->ctx.head = h;
}

/* Save date as number of seconds since 1 Jan 1904. Assuming 32-bit long
   this is valid for dates between 1 Jan 1904 00:00:00 and 6 Mar 2040 06:28:15.
   (32-bit overflow occurs on the next second.) */
static void saveDate(longDateTime ldt,
                     int year, int month, int day,
                     int hour, int min, int sec) {
	unsigned long elapsed;

	if (month < 3) {
		/* Jan and Feb become months 13 and 14 of the previous year */
		month += 12;
		year--;
	}

	/* Calculate elapsed seconds since 1 Jan 1904 00:00:00 */
	elapsed = (((year - 1904L) * 365 +          /* Add years (as days) */
	            (year - 1900) / 4 +             /* Add leap year days */
	            (306 * (month + 1)) / 10 - 64 + /* Add month days */
	            day) *                          /* Add days */
	           24 * 60 * 60 +                   /* Convert days to secs */
	           hour * 60 * 60 +                 /* Add hours (as secs) */
	           min * 60 +                       /* Add minutes (as secs) */
	           sec);                            /* Add seconds */

	/* Set value */
	ldt[0] = ldt[1] = ldt[2] = ldt[3] = 0;
	ldt[4] = (char)(elapsed >> 24);
	ldt[5] = (char)(elapsed >> 16);
	ldt[6] = (char)(elapsed >> 8);
	ldt[7] = (char)elapsed;
}

/* Save Unix time as longDateTime */
static void saveTime(longDateTime ldt, struct tm *tm) {
	saveDate(ldt,
	         tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
	         tm->tm_hour, tm->tm_min, tm->tm_sec);
}

int headFill(hotCtx g) {
	headCtx h = g->ctx.head;

	h->tbl.version              = VERSION(1, 0);
	h->tbl.fontRevision         = g->font.version.otf;
	h->tbl.checkSumAdjustment   = 0;
	h->tbl.magicNumber          = 0x5F0F3CF5;
	h->tbl.flags                = head_BASELINE_0 | head_LSB_0;
	h->tbl.unitsPerEm           = g->font.unitsPerEm;
	saveTime(h->tbl.created, &g->time);
	saveTime(h->tbl.modified, &g->time);
	h->tbl.xMin                 = g->font.bbox.left;
	h->tbl.yMin                 = g->font.bbox.bottom;
	h->tbl.xMax                 = g->font.bbox.right;
	h->tbl.yMax                 = g->font.bbox.top;
	h->tbl.macStyle             = g->font.flags & 0x3;
	h->tbl.lowestRecPPEM        = head_LOWEST_PPEM;
	h->tbl.fontDirectionHint    = head_L2R_PLUS_NEUTRALS;
	h->tbl.indexToLocFormat     = 0;
	h->tbl.glyphDataFormat      = 0;

	return 1;
}

void headWrite(hotCtx g) {
	headCtx h = g->ctx.head;

	/* Perform checksum adjustment */
	OUT4(h->tbl.version);
	OUT4(h->tbl.fontRevision);
	OUT4(h->tbl.checkSumAdjustment);
	OUT4(h->tbl.magicNumber);
	OUT2(h->tbl.flags);
	OUT2(h->tbl.unitsPerEm);
	OUTN(DATE_TIME_SIZE, h->tbl.created);
	OUTN(DATE_TIME_SIZE, h->tbl.modified);
	OUT2(h->tbl.xMin);
	OUT2(h->tbl.yMin);
	OUT2(h->tbl.xMax);
	OUT2(h->tbl.yMax);
	OUT2(h->tbl.macStyle);
	OUT2(h->tbl.lowestRecPPEM);
	OUT2(h->tbl.fontDirectionHint);
	OUT2(h->tbl.indexToLocFormat);
	OUT2(h->tbl.glyphDataFormat);
}

void headReuse(hotCtx g) {
}

void headFree(hotCtx g) {
	MEM_FREE(g, g->ctx.head);
}