/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/*
 *	OS/2 and Windows Metrics.
 */

#include "OS_2.h"
#include "map.h"

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h> /* to get getenv */

/* ---------------------------- Table Definition --------------------------- */

#define PANOSE_SIZE     10
#define VEND_ID_SIZE    4

typedef struct {
	unsigned short version;
	short xAvgCharWidth;
	unsigned short usWeightClass;
#define FWEIGHT_NORMAL  400
	unsigned short usWidthClass;
#define FWIDTH_NORMAL   5
	unsigned short fsType;      /* Embedding permissions */
#define EMBED_NONE              (1 << 1)
#define EMBED_PRINT_AND_VIEW    (1 << 2)
#define EMBED_EDITABLE          (1 << 3)
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
#define CLASS_NONE  0
	char panose[PANOSE_SIZE];
#define PANOSE_ANY              0   /* [any] */
#define PANOSE_NO_FIT           1   /* [no fit] */
#define PANOSE0_TEXT            2
#define PANOSE0_SCRIPT          3
#define PANOSE0_DECORATIVE      4
#define PANOSE0_PICTORIAL       5
#define PANOSE1_COVE            2
#define PANOSE1_SANS            11
#define PANOSE3_EXPANDED        5
#define PANOSE3_CONDENSED       6
#define PANOSE3_VERY_EXPANDED   7
#define PANOSE3_VERY_CONDENSED  8
#define PANOSE3_MONOSPACED      9
	uint32_t ulUnicodeRange1;
	uint32_t ulUnicodeRange2;
	uint32_t ulUnicodeRange3;
	uint32_t ulUnicodeRange4;
	char achVendId[VEND_ID_SIZE];
	unsigned short fsSelection;
#define SEL_ITALIC  (1 << 0)
#define SEL_BOLD    (1 << 5)
#define SEL_REGULAR (1 << 6)
#define SEL_RESPECT_STYPO (1 << 7)

	unsigned short usFirstCharIndex;
	unsigned short usLastCharIndex;
	short sTypoAscender;
	short sTypoDescender;
	short sTypoLineGap;
	unsigned short usWinAscent;
	unsigned short usWinDescent;
	uint32_t ulCodePageRange1; /* Version 1 */
	uint32_t ulCodePageRange2; /* Version 1 */
	short sXHeight;                 /* Version 2 */
	short sCapHeight;               /* Version 2 */
	unsigned short usDefaultChar;   /* Version 2 */
	unsigned short usBreakChar;     /* Version 2 */
	unsigned short usMaxContext;    /* Version 2 */
	unsigned short usLowerOpticalPointSize;     /* Version 5 */
	unsigned short usUpperOpticalPointSize; /* Version 5 */
} OS_2Tbl;

/* --------------------------- Context Definition -------------------------- */

/* Set of fields which can or must be set by a call to this module */
enum {
	kUnicodeRanges,
	kCodePageRanges,
	kCharIndexRange,
	kMaxContext,
	kPanose,
	kFSType,
	kWeightClass,
	kWidthClass,

	kFieldCount
};

struct OS_2Ctx_ {
	OS_2Tbl tbl;            /* Table data */
	char seen[kFieldCount]; /* Flags keys seen */
	hotCtx g;               /* Package context */
};

/* --------------------------- Standard Functions -------------------------- */

/* Initialize field seen array. */
static void initFieldSeen(OS_2Ctx h) {
	memset(h->seen, 0, sizeof(h->seen));
}

void OS_2New(hotCtx g) {
	OS_2Ctx h = MEM_NEW(g, sizeof(struct OS_2Ctx_));

	initFieldSeen(h);
	/* Following is not necessarily set by processing of normal defaults. */
	h->tbl.usMaxContext = 0;
	h->tbl.usLowerOpticalPointSize = 0;
	h->tbl.usUpperOpticalPointSize = 0;

	/* Link contexts */
	h->g = g;
	g->ctx.OS_2 = h;
}

/* Set weight and width classes from FullName */
static void setClasses(hotCtx g, OS_2Ctx h) {
	typedef struct {
		char *string;
		unsigned short value;
	} Match;
	static Match weight[] = {
		{ "thin",            250 },
		{ "extralight",      250 },
		{ "ultralight",      250 },
		{ "light",           300 },   /* Must succeed other lights */
		{ "medium",          500 },
		{ "semibold",        600 },
		{ "demibold",        600 },
		{ "demi",            600 },   /* Must succeed demibold */
		{ "extrabold",       800 },
		{ "ultrabold",       800 },
		{ "extrablack",      950 },   /* added */
		{ "ultrablack",      950 },   /* added */
		{ "ultra",           900 },   /* We need a special hack avoid seeing the width keyword "ultracondensed" as being "ultra". See code below. */
		{ "super",           800 },
		{ "bold",            700 },   /* Must succeed other bolds */
		{ "black",           900 },
		{ "poster",          900 },
		{ "heavy",           850 },  /* changed */
		{ "nord",            900 },
	};
	static Match width[] = {
		{ "ultracondensed",  1 },
		{ "extracompressed", 1 },
		{ "ultracompressed", 1 },
		{ "extracondensed",  2 },
		{ "compressed",      2 },
		{ "semicondensed",   4 },
		{ "compact",         4 },
		{ "narrow",          4 },
		{ "condensed",       3 }, /* Must succeed other condenseds */
		{ "semiexpanded",    6 },
		{ "semiextended",    6 },
		{ "extraexpanded",   8 },
		{ "extraextended",   8 },
		{ "ultraexpanded",   9 },
		{ "ultraextended",   9 },
		{ "expanded",        7 }, /* Must succeed other expandeds */
		{ "extended",        7 }, /* Must succeed other extendeds */
	};
	unsigned int i;
	int j;
	unsigned length;
	char *p;
	char FullName[128];

	if (h->seen[kWeightClass] && h->seen[kWidthClass]) {
		return;
	}

	/* Set defaults */
	if (!h->seen[kWeightClass]) {
		h->tbl.usWeightClass = FWEIGHT_NORMAL;
	}
	if (!h->seen[kWidthClass]) {
		h->tbl.usWidthClass = FWIDTH_NORMAL;
	}

	if (g->font.FullName == SID_UNDEF) {
		return; /* No FullName; assume default if no override */
	}
	/* Make null-terminated downcased copy without spaces */
	p = hotGetString(g, g->font.FullName, &length);
	i = 0;
	j = 0;
	while (j < 127 && i < length) {
		int c = p[i++];
		if (c != ' ') {
			FullName[j++] = isupper(c) ? tolower(c) : c;
		}
	}
	FullName[j] = '\0';

	/* Match weight class */
	if (!h->seen[kWeightClass]) {
		for (i = 0; i < ARRAY_LEN(weight); i++) {
			if (strstr(FullName, weight[i].string) != NULL) {
				/* so that we don't mistake 'ultracondensed' width for 'ultra' weight. */
				if ((0 == strcmp(weight[i].string, "ultra")) &&  (NULL != strstr(FullName, "ultrcondensed"))) {
					continue;
				}
				h->tbl.usWeightClass = weight[i].value;
				break;
			}
		}
	}

	if (h->tbl.usWeightClass < 250) {
		hotMsg(h->g, hotERROR, "OS/2 weight class was set below 250 <%d>; bumping to  250. Use correct feature file override.", h->tbl.usWeightClass);
		h->tbl.usWeightClass = 250;
	}

	/* Match width class */
	if (!h->seen[kWidthClass]) {
		for (i = 0; i < ARRAY_LEN(width); i++) {
			if (strstr(FullName, width[i].string) != NULL) {
				h->tbl.usWidthClass = width[i].value;
				break;
			}
		}
	}
}

/* Set Panose number. We are really using the first 4 Panose bytes to convey
   information that was originally in the PitchAndFamily and CharSet fields of
   PFM or MMM files. The font driver has code that reads these Panose bytes
   and reconstitutes the original values. I'm not proud of this but there
   doesn't seem to be a better way of achieving this within the available sfnt
   tables. */
static void setPanose(hotCtx g, OS_2Ctx h) {
	/* Initialize */
	memset(h->tbl.panose, PANOSE_ANY, PANOSE_SIZE);

	/* Set family kind [0] and serif style [1] */
	switch (g->font.win.Family) {
		case HOT_ROMAN:
			h->tbl.panose[0] = PANOSE0_TEXT;
			h->tbl.panose[1] = PANOSE1_COVE;
			break;

		case HOT_SWISS:
			h->tbl.panose[0] = PANOSE0_TEXT;
			h->tbl.panose[1] = PANOSE1_SANS;
			break;

		case HOT_MODERN:
			h->tbl.panose[0] = PANOSE_NO_FIT;
			break;

		case HOT_SCRIPT:
			h->tbl.panose[0] = PANOSE0_SCRIPT;
			break;

		case HOT_DECORATIVE:
			/* xxx does Symbol font really need to be special? */
			h->tbl.panose[0] = strcmp(g->font.FontName.array, "Symbol") == 0 ?
			    PANOSE0_PICTORIAL : PANOSE0_DECORATIVE;
			break;

		case HOT_DONTCARE:
			break;
	}

	/* Set Weight */
	h->tbl.panose[2] = h->tbl.usWeightClass / 100 + 1;

	/* Set Proportion */
	if (g->font.flags & FI_FIXED_PITCH) {
		h->tbl.panose[3] = PANOSE3_MONOSPACED;
	}
	else {
		switch (h->tbl.usWidthClass) {
			case 1:
			case 2:
				h->tbl.panose[3] = PANOSE3_VERY_CONDENSED;
				break;

			case 3:
			case 4:
				h->tbl.panose[3] = PANOSE3_CONDENSED;
				break;

			case 6:
			case 7:
				h->tbl.panose[3] = PANOSE3_EXPANDED;
				break;

			case 8:
			case 9:
				h->tbl.panose[3] = PANOSE3_VERY_EXPANDED;
				break;
		}
	}
}

/* Fail if fields haven't been set */
static void failIfNotSeen(OS_2Ctx h, int enumField, char *desc) {
	if (!h->seen[enumField]) {
		hotMsg(h->g, hotFATAL, "[internal] OS/2.%s not set", desc);
	}
}

int OS_2Fill(hotCtx g) {
	OS_2Ctx h = g->ctx.OS_2;
	FontInfo_ *font = &g->font;

	failIfNotSeen(h, kUnicodeRanges, "UnicodeRanges");
	failIfNotSeen(h, kCodePageRanges, "CodePageRanges");
	failIfNotSeen(h, kCharIndexRange, "CharIndexRange");

	h->tbl.xAvgCharWidth        = font->win.AvgWidth;
	setClasses(g, h);
	if (!h->seen[kFSType]) {
		char *fsType = getenv(kFSTypeEnviron);
		if (fsType != NULL) {
			h->tbl.fsType = atoi(fsType);
		}
		else {
			h->tbl.fsType = EMBED_PRINT_AND_VIEW;
		}
	}
	h->tbl.ySubscriptXSize      = font->win.SubscriptXSize;
	h->tbl.ySubscriptYSize      = font->win.SubscriptYSize;
	h->tbl.ySubscriptXOffset    = font->win.SubscriptXOffset;
	h->tbl.ySubscriptYOffset    = font->win.SubscriptYOffset;

	h->tbl.ySuperscriptXSize    = font->win.SuperscriptXSize;
	h->tbl.ySuperscriptYSize    = font->win.SuperscriptYSize;
	h->tbl.ySuperscriptXOffset  = font->win.SuperscriptXOffset;
	h->tbl.ySuperscriptYOffset  = font->win.SuperscriptYOffset;

	h->tbl.yStrikeoutSize       = font->win.StrikeOutSize;
	h->tbl.yStrikeoutPosition   = font->win.StrikeOutPosition;

	h->tbl.sFamilyClass = CLASS_NONE;   /* No classification */
	if (!h->seen[kPanose]) {
		setPanose(g, h);
	}
	strncpy(h->tbl.achVendId, font->vendId, VEND_ID_SIZE);

	h->tbl.fsSelection = 0;
	if (font->flags & HOT_BOLD) {
		h->tbl.fsSelection |= SEL_BOLD;
	}
	if (font->flags & HOT_ITALIC) {
		h->tbl.fsSelection |= SEL_ITALIC;
	}
	if (h->tbl.fsSelection == 0 && h->tbl.usWeightClass == FWEIGHT_NORMAL) {
		h->tbl.fsSelection |= SEL_REGULAR;
	}

	if (g->font.fsSelectionMask_on >= 0) {
		h->tbl.fsSelection |= g->font.fsSelectionMask_on;
	}

	if (g->font.fsSelectionMask_off >= 0) {
		h->tbl.fsSelection &= ~g->font.fsSelectionMask_off;
	}

	if (h->tbl.usLowerOpticalPointSize > 0) {
		h->tbl.version = g->font.os2Version = 5;
	}
	else if (h->tbl.fsSelection >= 0x80) {
		/* If any of the OS/2 v 4 fsSelection bits are on.*/
		h->tbl.version = g->font.os2Version = 4;
	}
    else if ((h->tbl.ulUnicodeRange3 >= (uint32_t)(1<<29)) || (h->tbl.ulUnicodeRange4 != 0))
    {
         h->tbl.version = g->font.os2Version = 4;
    }
	else if (g->font.os2Version != 0) {
		h->tbl.version              = g->font.os2Version;
	}
	else {
		h->tbl.version              = 3;
	}

	h->tbl.sTypoAscender = font->TypoAscender;
	h->tbl.sTypoDescender = font->TypoDescender;
	h->tbl.sTypoLineGap = font->TypoLineGap;

	h->tbl.usWinAscent = font->win.ascent;
	h->tbl.usWinDescent = font->win.descent;

	/* Version 2 fields */
	h->tbl.sXHeight         = font->win.XHeight;
	h->tbl.sCapHeight       = font->win.CapHeight;
	h->tbl.usDefaultChar    = font->win.DefaultChar;
	h->tbl.usBreakChar      = font->win.BreakChar;

	return 1;
}

void OS_2Write(hotCtx g) {
	OS_2Ctx h = g->ctx.OS_2;
	OUT2(h->tbl.version);
	OUT2(h->tbl.xAvgCharWidth);
	OUT2(h->tbl.usWeightClass);
	OUT2(h->tbl.usWidthClass);
	OUT2(h->tbl.fsType);
	OUT2(h->tbl.ySubscriptXSize);
	OUT2(h->tbl.ySubscriptYSize);
	OUT2(h->tbl.ySubscriptXOffset);
	OUT2(h->tbl.ySubscriptYOffset);
	OUT2(h->tbl.ySuperscriptXSize);
	OUT2(h->tbl.ySuperscriptYSize);
	OUT2(h->tbl.ySuperscriptXOffset);
	OUT2(h->tbl.ySuperscriptYOffset);
	OUT2(h->tbl.yStrikeoutSize);
	OUT2(h->tbl.yStrikeoutPosition);
	OUT2(h->tbl.sFamilyClass);
	OUTN(PANOSE_SIZE, h->tbl.panose);
	OUT4(h->tbl.ulUnicodeRange1);
	OUT4(h->tbl.ulUnicodeRange2);
	OUT4(h->tbl.ulUnicodeRange3);
	OUT4(h->tbl.ulUnicodeRange4);
	OUTN(VEND_ID_SIZE, h->tbl.achVendId);
	OUT2(h->tbl.fsSelection);
	OUT2(h->tbl.usFirstCharIndex);
	OUT2(h->tbl.usLastCharIndex);
	OUT2(h->tbl.sTypoAscender);
	OUT2(h->tbl.sTypoDescender);
	OUT2(h->tbl.sTypoLineGap);
	OUT2(h->tbl.usWinAscent);
	OUT2(h->tbl.usWinDescent);
	OUT4(h->tbl.ulCodePageRange1);
	OUT4(h->tbl.ulCodePageRange2);
	OUT2(h->tbl.sXHeight);
	OUT2(h->tbl.sCapHeight);
	OUT2(h->tbl.usDefaultChar);
	OUT2(h->tbl.usBreakChar);
	OUT2(h->tbl.usMaxContext);
	if (h->tbl.version > 4) {
		OUT2(h->tbl.usLowerOpticalPointSize);
		OUT2(h->tbl.usUpperOpticalPointSize);
	}
}

void OS_2Reuse(hotCtx g) {
	OS_2Ctx h = g->ctx.OS_2;

	initFieldSeen(h);
	h->tbl.usMaxContext = 0;
	h->tbl.usLowerOpticalPointSize = 0;
	h->tbl.usUpperOpticalPointSize = 0;
}

void OS_2Free(hotCtx g) {
	MEM_FREE(g, g->ctx.OS_2);
}

/* ------------------------ Supplementary Functions ------------------------ */

void OS_2SetUnicodeRanges(hotCtx        g,
                          uint32_t ulUnicodeRange1,
                          uint32_t ulUnicodeRange2,
                          uint32_t ulUnicodeRange3,
                          uint32_t ulUnicodeRange4) {
	OS_2Ctx h = g->ctx.OS_2;
	h->tbl.ulUnicodeRange1 = ulUnicodeRange1;
	h->tbl.ulUnicodeRange2 = ulUnicodeRange2;
	h->tbl.ulUnicodeRange3 = ulUnicodeRange3;
	h->tbl.ulUnicodeRange4 = ulUnicodeRange4;
	h->seen[kUnicodeRanges] = 1;
}

void OS_2SetCodePageRanges(hotCtx        g,
                           uint32_t ulCodePageRange1,
                           uint32_t ulCodePageRange2) {
	OS_2Ctx h = g->ctx.OS_2;
	h->tbl.ulCodePageRange1 = ulCodePageRange1;
	h->tbl.ulCodePageRange2 = ulCodePageRange2;
	h->seen[kCodePageRanges] = 1;
}

void OS_2SetCharIndexRange(hotCtx         g,
                           unsigned short usFirstCharIndex,
                           unsigned short usLastCharIndex) {
	OS_2Ctx h = g->ctx.OS_2;
	h->tbl.usFirstCharIndex = usFirstCharIndex;
	h->tbl.usLastCharIndex = usLastCharIndex;
	h->seen[kCharIndexRange] = 1;
}

/* May be called multiple times */
void OS_2SetMaxContext(hotCtx g, unsigned maxContext) {
	OS_2Ctx h = g->ctx.OS_2;
	h->tbl.usMaxContext = MAX(h->tbl.usMaxContext, maxContext);
	if (!h->seen[kMaxContext]) {
		h->seen[kMaxContext] = 1;
	}
}

void OS_2LowerOpticalPointSize(hotCtx g, unsigned short opSize) {
	OS_2Ctx h = g->ctx.OS_2;
	h->tbl.usLowerOpticalPointSize = opSize;
}

void OS_2UpperOpticalPointSize(hotCtx g, unsigned short opSize) {
	OS_2Ctx h = g->ctx.OS_2;
	h->tbl.usUpperOpticalPointSize = opSize;
}

void OS_2SetPanose(hotCtx g, char *panose) {
	OS_2Ctx h = g->ctx.OS_2;
	memcpy(h->tbl.panose, panose, PANOSE_SIZE);
	h->seen[kPanose] = 1;
}

void OS_2SetFSType(hotCtx g, unsigned short fsType) {
	OS_2Ctx h = g->ctx.OS_2;
	h->tbl.fsType = fsType;
	h->seen[kFSType] = 1;
}

void OS_2SetWeightClass(hotCtx g, unsigned short weightClass) {
	OS_2Ctx h = g->ctx.OS_2;
	h->tbl.usWeightClass = weightClass;
	h->seen[kWeightClass] = 1;
}

void OS_2SetWidthClass(hotCtx g, unsigned short widthClass) {
	OS_2Ctx h = g->ctx.OS_2;
	h->tbl.usWidthClass = widthClass;
	h->seen[kWidthClass] = 1;
}