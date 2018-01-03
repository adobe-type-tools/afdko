/* @(#)CM_VerSion parse.c atm08 1.2 16245.eco sum= 22139 atm08.002 */
/* @(#)CM_VerSion parse.c atm07 1.2 16164.eco sum= 13305 atm07.012 */
/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */


/*
 * PostScript font file parser.
 *
 * Handles single master, multiple master, synthetic, and CID fonts.
 */

#include "parse.h"
#include "encoding.h"
#include "sindex.h"
#include "charset.h"
#include "t13.h"
#include "recode.h"

#include "txops.h"
#include "pstoken.h"

#include <stdarg.h>
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <math.h>

#if PLAT_SUN4
#include "sun4/fixstring.h"
#else /* PLAT_SUN4 */
#include <string.h>
#endif  /* PLAT_SUN4 */

#define DEFAULT_LENIV 4

/* Subr data */
typedef struct {
	long index;                 /* Charstring index */
	unsigned short length;      /* Subr length */
} Subr;

/* Char data */
typedef struct {
	long index;                 /* Charstring index */
	unsigned short length;      /* Charstring length */
	unsigned short id;          /* Char identifier (SID or CID) */
	short code;                 /* Encoding */
	unsigned short reorder;     /* Charstring reordering index; the *original* order. */
	long aliasOrder;            /* order in the glyph alias file */
	FDIndex fdIndex;            /* FDArray index */
	char encrypted;             /* Charstring encrypted (unprocessed) */
} Char;

/* Encoding */
typedef struct {
	short std;                  /* Flags standard encoding */
	char *custom[256];          /* Built-in encoded glyph names */
	dnaDCL(char, data);         /* Built-in glyph name strings */
} Encoding;

/* Standard encoded glyphs */
static unsigned char stdcodes[] = {
#include "stdenc0.h"
};

/* The following provides an index for all PostScript keys that TC cares
   about. Additionally, the first two index groups are used to create the
   Top/FD and Private/PD dicts in the order presented (low index first). */
enum {
	/* Top/FD dict ops */
	iWeightVector,          /* MM (Must be first) */
	TOP_DICT_FIRST =
	    iWeightVector,
	iRegistry,              /* CID (Must be first) */
	iChameleon,             /* Chameleon (mist be first) */
	iversion,
	iNotice,
	iCopyright,
	iFullName,
	iFamilyName,
	iWeight,
	iisFixedPitch,
	iItalicAngle,
	iUnderlinePosition,
	iUnderlineThickness,
	iFontName,
	iFontBBox,
	iPaintType,
	iFontMatrix,
	iUniqueID,
	iStrokeWidth,
	iXUID,
	iBlendAxisTypes,        /* MM */
	iCIDFontVersion,        /* CID */
	iCIDFontRevision,       /* CID */
	iCIDCount,              /* CID */
	iUIDBase,               /* CID */
	iBaseFontName,          /* Streamer */
	iBaseFontBlend,         /* Streamer */
	TOP_DICT_LAST =
	    iBaseFontBlend,

	/* Private/PD dict ops */
	iBlueValues,
	PRIVATE_DICT_FIRST =
	    iBlueValues,
	iOtherBlues,            /* Must follow BlueValues */
	iFamilyBlues,
	iFamilyOtherBlues,      /* Must follow FamilyBlues */
	iBlueScale,
	iBlueShift,
	iBlueFuzz,
	iStdHW,
	iStdVW,
	iStemSnapH,
	iStemSnapV,
	iForceBold,
	iForceBoldThreshold,
	iRndStemUp,             /* Must precede LanguageGroup (not a DICT op) */
	iLanguageGroup,
	iExpansionFactor,
	iinitialRandomSeed,
	PRIVATE_DICT_LAST =
	    iinitialRandomSeed,

	/* Key indexes */
	iEncoding,
	iFontType,
	iPrivate,
	iCharStrings,
	ilenIV,
	iErode,
	iSubrs,
	iBlendDesignPositions,  /* MM */
	iBlendDesignMap,        /* MM */
	iNDV,                   /* MM */
	iCDV,                   /* MM */
	ilenBuildCharArray,     /* MM */
	iCIDInit,               /* CID */
	iCIDFontName,           /* CID */
	iCIDFontType,           /* CID */
	iOrdering,              /* CID */
	iSupplement,            /* CID */
	iCIDMapOffset,          /* CID */
	iFDBytes,               /* CID */
	iGDBytes,               /* CID */
	iFDArray,               /* CID */
	iRunInt,                /* CID */
	iSubrMapOffset,         /* CID */
	iSDBytes,               /* CID */
	iSubrCount,             /* CID */
	iignore,                /* Ignored key */
	KEY_COUNT               /* Key count */
};

/* The following provides a mapping from a PostScript dict key (literal) to one
   of the indexes defined above.

   The philosophy adopted when parsing the Private dict is that unknown keys
   should cause the conversion to fail since TC can't determine the intent
   of the unknown additions and would most likely break the font in some way.

   For this scheme to work all known Private dictionary keys must be listed
   below even if value is irrelevant to the conversion, hence these keys are
   recognized and their value is ignored (iignore).

   FontLab misspells lenIV as LenIV but since it's only used to specify the
   default value of 4 it doesn't affect printers or ATM which simply ignore
   unrecognized tokens. For compatibility a similar strategy is adopted by TC
   but in this case the key must be recognized, for the reasons stated above,
   and then its value is ignored. */

/* Dict key name (literal) to index mapping */
typedef struct {
	char *name;
	short index;
} DictKeyMap;

/* NOTE: These keys must be in sort order */
static DictKeyMap keyMap[] = {
	{ "$mmff_origfindfont",      iignore },
	{ "-|",                      iignore },
	{ "BaseFontBlend",           iBaseFontBlend },
	{ "BaseFontName",            iBaseFontName },
	{ "Blend",                   iignore },
	{ "BlendAxisTypes",          iBlendAxisTypes },
	{ "BlendDesignMap",          iBlendDesignMap },
	{ "BlendDesignPositions",    iBlendDesignPositions },
	{ "BlueFuzz",                iBlueFuzz },
	{ "BlueScale",               iBlueScale },
	{ "BlueShift",               iBlueShift },
	{ "BlueValues",              iBlueValues },
	{ "BuildCharArray",          iignore },
	{ "CDV",                     iCDV },
	{ "CIDCount",                iCIDCount },
	{ "CIDFontName",             iCIDFontName },
	{ "CIDFontRevision",         iCIDFontRevision },
	{ "CIDFontType",             iCIDFontType },
	{ "CIDFontVersion",          iCIDFontVersion },
	{ "CIDInit",                 iCIDInit },
	{ "CIDMapOffset",            iCIDMapOffset },
	{ "Chameleon",               iChameleon },
	{ "CharStrings",             iCharStrings },
	{ "ConvertDesignVector",     iignore },
	{ "Copyright",               iCopyright },
	{ "Encoding",                iEncoding },
	{ "Erode",                   iErode },
	{ "ExpansionFactor",         iExpansionFactor },
	{ "FDArray",                 iFDArray },
	{ "FDBytes",                 iFDBytes },
	{ "FamilyBlues",             iFamilyBlues },
	{ "FamilyName",              iFamilyName },
	{ "FamilyOtherBlues",        iFamilyOtherBlues },
	{ "FontBBox",                iFontBBox },
	{ "FontMatrix",              iFontMatrix },
	{ "FontName",                iFontName },
	{ "FontType",                iFontType },
	{ "ForceBold",               iignore }, /* As of 5/20/2010, Adobe is supressing ForceBold operators in CFF fonts. */
	{ "ForceBoldThreshold",      iignore }, /* As of 5/20/2010, Adobe is supressing ForceBold operators in CFF fonts. */
	{ "FullName",                iFullName },
	{ "GDBytes",                 iGDBytes },
	{ "ICSdict",                 iignore },               /* JensonMM */
	{ "ICSsetup",                iignore },               /* JensonMM */
	{ "ItalicAngle",             iItalicAngle },
	{ "LanguageGroup",           iLanguageGroup },
	{ "LenIV",                   iignore },   /* FontLab! (see comment above) */
	{ "MinFeature",              iignore },
	{ "ND",                      iignore },
	{ "NDV",                     iNDV },
	{ "NP",                      iignore },
	{ "NormalizeDesignVector",   iignore },
	{ "Notice",                  iNotice },
	{ "Ordering",                iOrdering },
	{ "OtherBlues",              iOtherBlues },
	{ "OtherSubrs",              iignore },
	{ "PaintType",               iPaintType },
	{ "Post-N&C",                iignore },               /* JensonMM */
	{ "Pre-N&C",                 iignore },               /* JensonMM */
	{ "Private",                 iPrivate },
	{ "RD",                      iignore },
	{ "Registry",                iRegistry },
	{ "RndStemUp",               iRndStemUp },
	{ "RunInt",                  iRunInt },
	{ "SDBytes",                 iSDBytes },
	{ "SharedFontDirectory",     iignore },
	{ "Source",                  iignore },
	{ "StdHW",                   iStdHW },
	{ "StdVW",                   iStdVW },
	{ "StemSnapH",               iStemSnapH },
	{ "StemSnapV",               iStemSnapV },
	{ "StrokeWidth",             iStrokeWidth },
	{ "SubrCount",               iSubrCount },
	{ "SubrMapOffset",           iSubrMapOffset },
	{ "Subrs",                   iSubrs },
	{ "Supplement",              iSupplement },
	{ "UIDBase",                 iUIDBase },
	{ "UnderlinePosition",       iUnderlinePosition },
	{ "UnderlineThickness",      iUnderlineThickness },
	{ "UniqueID",                iUniqueID },
	{ "Weight",                  iWeight },
	{ "WeightVector",            iWeightVector },
	{ "XUID",                    iXUID },
	{ "findfont",                iignore },
	{ "globaldict",              iignore },               /* JensonMM */
	{ "initialRandomSeed",       iinitialRandomSeed },
	{ "internaldict",            iignore },
	{ "interpcharstring",        iignore },               /* JensonMM */
	{ "isFixedPitch",            iisFixedPitch },
	{ "lenBuildCharArray",       ilenBuildCharArray },
	{ "lenIV",                   ilenIV },
	{ "makeblendedfont",         iignore },
	{ "password",                iignore },
	{ "setshared",               iignore },
	{ "shareddict",              iignore },
	{ "version",                 iversion },
	{ "|",                       iignore },
	{ "|-",                      iignore },
};

/* Parsed keys and values */
typedef struct {
	void (*save)(parseCtx h, DICT *dict, int iKey); /* Save-value function */
	char *dflt;             /* Default value */
	psToken value;          /* PostScript token corresponding to key */
	short flags;            /* Control flags */
#define KEY_SEEN    (1 << 0)  /* Key seen in font */
#define KEY_NOEMBED (1 << 1)  /* Don't save key in embeded font (TC_EMBED set) */
	short op;               /* CFF operator associated with key */
} Key;

/* Clear, set, and test if key seen */
#define CLR_KEY_SEEN(i) h->keys[i].flags &= ~KEY_SEEN
#define SET_KEY_SEEN(i) h->keys[i].flags |= KEY_SEEN
#define SEEN_KEY(i) (h->keys[i].flags & KEY_SEEN)

/* Initialize normal dict keys */
#define IKEY(i, s, o, d) \
	h->keys[i].save = (s); \
	h->keys[i].op = (o); \
	h->keys[i].dflt = (d); \
	h->keys[i].flags = 0

/* Initialize no-embed dict keys (when TC_EMBED is set keys won't get saved) */
#define NKEY(i, s, o, d) \
	h->keys[i].save = (s); \
	h->keys[i].op = (o); \
	h->keys[i].dflt = (d); \
	h->keys[i].flags = KEY_NOEMBED

/* Module context */
struct parseCtx_ {
	psCtx ps;                   /* PostScript parser context */
	psBuf buf;                  /* PostScript input buffer */
	psCallbacks psCB;           /* PostScript parser callbacks */
	int lenIV;                  /* lenIV value */
	csDecrypt decrypt;          /* Charstring decryption proc */
	int firstNotice;            /* Flags first copyright notice */
	double FontMatrix[6];       /* FontMatrix */
	unsigned isFixedPitch;      /* isFixedPitch: is font monospaced. */
	Encoding encoding;          /* Encoding */
	Encoding synthenc;          /* Synthetic font's encoding */
	struct {                     /* --- Token(s) seen */
		int end;                /* Seen Charstrings dict end operator */
	}
	seen;
	dnaDCL(Subr, subrs);        /* Subrs */
	dnaDCL(Char, chars);        /* Selected glyphs */
	Key keys[KEY_COUNT];        /* Literals */
	struct {                    /* --- Reordered char mappings */
		dnaDCL(unsigned char, encoding);
		dnaDCL(SID, charset);
		dnaDCL(CodeMap, supplement);
		dnaDCL(unsigned short, index);
	}
	reorder;
	struct {                    /* --- Subsetting data */
		tcSubset *spec;         /* Subset specification */
		unsigned nGlyphs;       /* if !=0 then subset font */
		char *glyph[257];       /* Subset glyph names (256+ 1 for .notdef) */
	}
	subset;
	struct {                    /* --- Potential seac component glyphs */
		int inorder;            /* Flags components in search order */
		dnaDCL(Char, chars);
		long stdindex[256];     /* Component char indexes sorted by std code */
#define COMP_CHAR   0x80000000  /* Flags component in component.chars array */
	}
	component;
	struct {                     /* --- CIDFont values */
		long MapOffset;
		unsigned FDBytes;
		unsigned GDBytes;
		unsigned Count;
		int fd;                 /* FDArray index (= -1 initially) */
		long bytesRead;         /* bytes read into the binary section */
	}
	cid;
	Font *font;                 /* Current font */
	tcCtx g;                    /* Package context */
};

/* Forward declaration */
static void doLiteral(parseCtx h, psToken *literal);
static void dictInit(parseCtx h);
static void saveSynthetic(parseCtx h);
static void saveDicts(parseCtx h);
static void doOperator(parseCtx h, psToken *operator, psToken *strng,
                       psToken *integr);
static void cidInit(parseCtx h);
static void initFDArray(parseCtx h);
static void cidReadSubrs(parseCtx h, int fd);
static void cidReadChars(parseCtx h);
static void cidAddChars(parseCtx h);
static void cidSaveFD(parseCtx h);
static long getOffset(parseCtx h, char *buf, int size);

/* Report current source id if not done so already */
static void reportId(tcCtx g, int type) {
	char text[513];

	if (g->status & TC_MESSAGE || g->cb.psId == NULL) {
		return;
	}
    if (g->flags & TC_VERBOSE)
    {
        sprintf(text, "--- Source font: %s", g->cb.psId(g->cb.ctx));
        g->cb.message(g->cb.ctx, type, text);
    }

	g->status |= TC_MESSAGE;
}

/* Print parser warning error message with input id appended */
void CDECL parseWarning(tcCtx g, char *fmt, ...) {
	if ((g->cb.message != NULL) && (g->flags & TC_DO_WARNINGS)) {
		va_list ap;
		char text[513];

		reportId(g, tcWARNING);

		/* Format and report message */
		va_start(ap, fmt);
		vsprintf(text, fmt, ap);
		g->cb.message(g->cb.ctx, tcWARNING, text);
		va_end(ap);
	}
}

/* Print parser warning error message with input id appended */
void CDECL parseNewGlyphReport(tcCtx g, char *fmt, ...) {
	if (g->cb.message != NULL) {
		va_list ap;
		char text[513];

		reportId(g, tcNOTE);

		/* Format and report message */
		va_start(ap, fmt);
		vsprintf(text, fmt, ap);
		g->cb.message(g->cb.ctx, tcNOTE, text);
		va_end(ap);
	}
}

/* Print parser fatal error message with input id appended and quit */
void CDECL parseFatal(tcCtx g, char *fmt, ...) {
	void *ctx = g->cb.ctx;  /* Client context */

	if (g->cb.message != NULL) {
		/* Format message */
		va_list ap;
		char text[513];

		reportId(g, tcFATAL);

		/* Format and report message */
		va_start(ap, fmt);
		vsprintf(text, fmt, ap);
		g->cb.message(ctx, tcFATAL, text);
		va_end(ap);
	}
	g->cb.fatal(g->cb.ctx);
}

/* Return current font */
Font *parseGetFont(tcCtx g) {
	return g->ctx.parse->font;
}

/* Compare glyph names */
static int CDECL cmpGlyphNames(const void *first, const void *second) {
	return strcmp(*(char **)first, *(char **)second);
}

/* Read encoding array. Just save the reference to encoded glyph names */
static void readEncoding(parseCtx h) {
	/* Check for StandardEncoding */
	psToken *dupToken;
	h->encoding.std =
	    psMatchToken(h->ps, psGetToken(h->ps), PS_OPERATOR, "StandardEncoding");
	if (!h->encoding.std) {
		int i;
		char *p;
		int length;
		long index[256]; /* Glyph name index, b31-8 buf. index, b7-0 length */

		/* Initialize index */
		for (i = 0; i < 256; i++) {
			index[i] = 0;
		}

		/* Skip to first entry and remember offset */
		do {
			dupToken = psGetToken(h->ps);
		}
		while (!(psMatchValue(h->ps, dupToken, "dup") || psMatchValue(h->ps, dupToken, "def")));

		if (psMatchValue(h->ps, dupToken, "dup")) {
			/* Parse encoding */
			length = 0;
			do {
				int code = 0;   /* Suppress optimizer warning */
				psToken codet;
				psToken namet;
				psToken *put;

				codet = *psGetToken(h->ps);
				namet = *psGetToken(h->ps);
				put = psGetToken(h->ps);

				/* Validate entry */
				if (codet.type != PS_INTEGER ||
				    namet.type != PS_LITERAL ||
				    !psMatchToken(h->ps, put, PS_OPERATOR, "put") ||
				    (code = psConvInteger(h->ps, &codet)) < 0 || code > 255) {
					parseFatal(h->g, "bad encoding");
				}

				/* Save glyph index and length */
				index[code] = (namet.index + 1) << 8 | (namet.length - 1);
				length += namet.length;
			}
			while (psMatchToken(h->ps, psGetToken(h->ps), PS_OPERATOR, "dup"));
		}

		/* Allocate name buffer */
		h->encoding.data.cnt = 0;
		p = dnaGROW(h->encoding.data, length);

		/* Build name array */
		for (i = 0; i < 256; i++) {
			if (index[i] == 0) {
				h->encoding.custom[i] = NULL;
			}
			else {
				h->encoding.custom[i] = p;
				length = index[i] & 0xff;
				COPY(p, &h->buf.array[index[i] >> 8], length);
				p[length] = '\0';
				p += length + 1;
			}
		}
	}
}

/* Read Subrs */
static void readSubrs(parseCtx h) {
	tcCtx g = h->g;
	int i;
	psToken *cnt = psGetToken(h->ps);

	if (cnt->type != PS_INTEGER) {
		parseFatal(g, "bad /Subr count");
	}
	h->subrs.cnt = psConvInteger(h->ps, cnt);

	(void)psFindToken(h->ps, PS_OPERATOR, "array");
	for (i = 0; i < h->subrs.cnt; i++) {
		Subr *subr;
		int iSubr;
		psToken num;
		psToken len;

		(void)psFindToken(h->ps, PS_OPERATOR, "dup");
		num = *psGetToken(h->ps);
		len = *psGetToken(h->ps);

		/* Validate entry */
		if (num.type != PS_INTEGER ||
		    len.type != PS_INTEGER ||
		    psGetToken(h->ps)->type != PS_OPERATOR) {
			parseFatal(g, "bad Subr");
		}

		iSubr = psConvInteger(h->ps, &num);
		if (iSubr < 0 || iSubr >= h->subrs.cnt) {
			parseFatal(g, "subr index out of range");
		}

		/* Read charstring data */
		subr = dnaINDEX(h->subrs, iSubr);
		subr->length = (unsigned short)psConvInteger(h->ps, &len);
		subr->index = h->buf.cnt;
		psReadBinary(h->ps, subr->length);
	}
}

/* Match exec key */
static int CDECL matchGlyphName(const void *key, const void *value) {
	const String *strng = key;
	return tc_strncmp(strng->data, strng->length, *(char **)value);
}

/* Read CharStrings dictionary */
static void readChars(parseCtx h) {
	tcCtx g = h->g;
	int isFirstChar = 1;
	int seenNotdef = 0;

	(void)psFindToken(h->ps, PS_OPERATOR, "begin");

	for (;; ) {
		/* Read CharString */
		Char *new;
		SID sid;
		String name;
		unsigned binlen;
		psToken namet;
		psToken len;
		psToken RD;
		psToken ND;

		namet = *psGetToken(h->ps);

		if (psMatchToken(h->ps, &namet, PS_OPERATOR, "end")) {
			/* Finished parse */
			h->seen.end = 1;
			if (h->font->flags & FONT_CHAMELEON) {
				h->chars.cnt = 0;
			}

			/* Font didn't have a notdef - add one! */
			if (!seenNotdef) {
				g->flags |= TC_FORCE_NOTDEF;
			}

			return;
		}

		len = *psGetToken(h->ps);
		RD = *psGetToken(h->ps);

		/* Validate entry */
		if (namet.type != PS_LITERAL ||
		    len.type != PS_INTEGER ||
		    RD.type != PS_OPERATOR) {
			parseFatal(g, "bad CharString format");
		}

		binlen = psConvInteger(h->ps, &len);
		name.data = psConvLiteral(h->ps, &namet, &name.length);
		if (name.length == 0) {
			parseWarning(g, "null charstring name (discarded)");
		}

#if TC_EURO_SUPPORT
		else if ((g->flags & TC_FORCE_NOTDEF) && (tc_strncmp(name.data, name.length, ".notdef") == 0)) {
			parseWarning(g, ".notdef in source font supressed.");
			seenNotdef = 1;
		}
#endif /* TC_EURO_SUPPORT */

		else if (h->subset.nGlyphs == 0 ||
		         bsearch(&name, h->subset.glyph, h->subset.nGlyphs,
		                 sizeof(char *), matchGlyphName) != NULL) {
			/* Whole font conversion or selected subset glyph */
			long int order = -1;
			unsigned int skipGlyph = 0;

			if ((g->flags & TC_RENAME) && (g->cb.getAliasAndOrder != NULL)) {
				char *newName = NULL;
				char oldName[64];

				strncpy(oldName, name.data, name.length);
				oldName[name.length] = 0;

				g->cb.getAliasAndOrder(g->cb.ctx, oldName, &newName, &order);
				if (newName == NULL) {
					if (g->flags & TC_SUBSET) {
						skipGlyph = 1;
					}
					else {
						sid = sindexGetGlyphNameId(g, name.length, name.data);
					}
				}
				else {
					sid = sindexGetGlyphNameId(g, strlen(newName), newName);
				}
			}
			else {
				sid = sindexGetGlyphNameId(g, name.length, name.data);
			}

			if (skipGlyph == 1) {
				parseWarning(g, "Skipping glyph not in GOADB file: <%.*s>", name.length, name.data);
			}

			else if (sid == SID_UNDEF) {
				parseWarning(g, "duplicate charstring <%.*s> (discarded)",
				             name.length, name.data);
			}
			else {
				/* on first time through, h->chars has alrady been initialized to 1, in parseFont
				   I don't change this, as it is important for CID fonts, where .notdef is forced
				   into the first slot */
				if (isFirstChar) {
					isFirstChar = 0;
					new = &h->chars.array[0];
				}
				else {
					new = dnaNEXT(h->chars);
				}

				if (sid == SID_NOTDEF) {
					seenNotdef = 1;
				}

				new->index = h->buf.cnt;
				new->length = binlen;
				new->code = -1;
				new->aliasOrder = order;
				new->id = sid;
				new->encrypted = 1;

				if (g->flags & TC_NOOLDOPS && sid < TABLE_LEN(stdcodes)) {
					/* Save standard-encoded char index */
					int code = stdcodes[sid];
					if (code != -1) {
						h->component.stdindex[code] = h->chars.cnt - 1;
					}
				}
			}     /* end if-else sid == SID_UNDEF */
		}
		else {
			/* Unselected glyph in subset; check if seac component */
			char *bname;
			sid = sindexCheckId(g, name.length, name.data);
			if (sid != SID_UNDEF &&
			    sid < TABLE_LEN(stdcodes) &&
			    (h->encoding.std ||
			     ((bname = h->encoding.custom[stdcodes[sid]]) != NULL &&
			          tc_strncmp(name.data, name.length, bname) == 0))) {
				int code = stdcodes[sid];
				if (code != -1) {
					/* Standard encoded glyph save as possible component */
					new = dnaNEXT(h->component.chars);
					new->index = h->buf.cnt;
					new->length = binlen;
					new->code = code;
					new->id = sid;
					new->encrypted = 1;

					/* Save standard-encoded char index */
					h->component.stdindex[code] =
					    COMP_CHAR | (h->component.chars.cnt - 1);
				}
			}
		}

		psReadBinary(h->ps, binlen);        /* Read charstring data */

		ND = *psGetToken(h->ps);
		if (ND.type != PS_OPERATOR) {
			parseFatal(g, "bad CharString <%.*s>", name.length, name.data);
		}
	}     /* end for all chars loop */
}

/* Lookup SID in list and return its index. Returns 1 if found else 0.
   Found or insertion position returned via index parameter. */
static int lookup(SID sid, int cnt, CodeMap *maps, int *index) {
	int lo = 0;
	int hi = cnt - 1;

	while (lo <= hi) {
		int mid = (lo + hi) / 2;
		if (sid < maps[mid].sid) {
			hi = mid - 1;
		}
		else if (sid > maps[mid].sid) {
			lo = mid + 1;
		}
		else {
			*index = mid;
			return 1;           /* Found */
		}
	}
	*index = lo;
	return 0;   /* Not found */
}

/* Match code mapping */
static int CDECL matchCodeMap(const void *key, const void *value) {
	SID a = *(SID *)key;
	SID b = ((CodeMap *)value)->sid;
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

/* Save encoding for a Chameleon or Synthetic font */
static void saveEncoding(parseCtx h, Encoding *encoding) {
	if (encoding->std) {
		/* Set standard encoding */
		h->font->iEncoding = h->font->synthetic.iEncoding = 0;
	}
	else {
		/* Construct custom encoding */
		int i;
		int nMaps;
		int unencoded;
		SID custom[256];
		CodeMap maps[256];  /* Code/SID mapping */

		/* Construct a binary-searchable code mapping (in SID order) */
		nMaps = 0;
		h->reorder.supplement.cnt = 0;
		for (i = 0; i < 256; i++) {
			SID sid;
			char *name = encoding->custom[i];

			if (name == NULL ||
			    (sid = sindexCheckId(h->g, strlen(name), name)) == SID_UNDEF) {
				/* Unencoded or encoding for non-existent glyph */
				custom[i] = SID_NOTDEF;
			}
			else {
				int index;
				custom[i] = sid;
				if (lookup((SID)sid, nMaps, maps, &index)) {
					/* Already in list; add to supplement */
					CodeMap *supplement = dnaNEXT(h->reorder.supplement);
					supplement->code = i;
					supplement->sid = sid;
				}
				else {
					/* Insert in list */
					CodeMap *map = &maps[index];
					COPY(map + 1, map, nMaps - index);
					map->code = i;
					map->sid = sid;
					nMaps++;
				}
			}
		}

		/* Build encoding array in charset order */
		unencoded = 0;
		h->reorder.encoding.cnt = 0;
		for (i = 0; i < h->reorder.charset.cnt; i++) {
			CodeMap *map =
			    (CodeMap *)bsearch(&h->reorder.charset.array[i], maps,
			                       nMaps, sizeof(CodeMap), matchCodeMap);
			if (map == NULL) {
				unencoded = 1;
			}
			else if (unencoded) {
				/* Map the remainder as supplementary encodings */
				CodeMap *supplement = dnaNEXT(h->reorder.supplement);
				supplement->code = map->code;
				supplement->sid = map->sid;
			}
			else {
				*dnaNEXT(h->reorder.encoding) = map->code;
			}
		}

		/* Add encoding */
		i = (h->reorder.supplement.cnt > 0) ? -1 : encodingCheckPredef(custom);
		h->font->synthetic.iEncoding = (i != -1) ? i :
		    encodingAdd(h->g,
		                h->reorder.encoding.cnt, h->reorder.encoding.array,
		                h->reorder.supplement.cnt, h->reorder.supplement.array);
	}
}

/* Read Chameleon data. The descriptor is represernted in the font dict of a
   Type 1 wrapped font as: /Chameleon N RD ~N~binary~bytes~ def */
static void readChameleon(parseCtx h) {
	static char *name[] = {
#include "isocenm.h"
	};
	tcCtx g = h->g;
	long binlen;
	psToken len;
	psToken RD;
	long index;
	unsigned int i;

	h->font->flags |= FONT_CHAMELEON;

	/* Get the size of chameleon private data */
	len = *psGetToken(h->ps);

	/* Skip over the RD */
	RD = *psGetToken(h->ps);
	if (len.type != PS_INTEGER || RD.type != PS_OPERATOR) {
		parseFatal(g, "bad Chameleon format");
	}

	/* Read data */
	binlen = psConvInteger(h->ps, &len);
	index = h->buf.cnt;
	psReadBinary(h->ps, binlen);

	h->font->chameleon.data = MEM_NEW(g, binlen);
	h->font->size.Private = binlen;
	memcpy(h->font->chameleon.data, &h->buf.array[index], binlen);

	/* Build and save charset */
	h->reorder.charset.cnt = 0;
	for (i = 0; i < TABLE_LEN(name); i++) {
		*dnaNEXT(h->reorder.charset) = sindexGetId(g, strlen(name[i]), name[i]);
	}

	h->font->iCharset =
	    charsetAdd(g, h->reorder.charset.cnt, h->reorder.charset.array, 0);
	saveEncoding(h, &h->encoding);

	/* Save Chameleon op code */
	h->font->chameleon.nGlyphs = TABLE_LEN(name) + 1;
	dictSaveInt(&h->font->dict, h->font->chameleon.nGlyphs);
	DICTSAVEOP(h->font->dict, cff_Chameleon);
}

/* Compare char sids */
static int CDECL cmpSIDs(const void *first, const void *second) {
	SID a = ((Char *)first)->id;
	SID b = ((Char *)second)->id;
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

/* Match SID */
static int CDECL matchSID(const void *key, const void *value) {
	SID a = *(SID *)key;
	SID b = ((Char *)value)->id;
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

/* Add seac component glyph. Return 1 on error else 0 */
int parseAddComponent(tcCtx g, int code) {
	parseCtx h = g->ctx.parse;
	SID sid = encodingGetStd()[code];

	if (sid == 0 || sid >= TABLE_LEN(stdcodes)) {
		return 1;               /* Non-standard encoding */
	}
	else {
		Char *src;

		if (!h->component.inorder) {
			/* Sort components for search */
			qsort(h->component.chars.array, h->component.chars.cnt,
			      sizeof(Char), cmpSIDs);
			h->component.inorder = 1;
		}
		src = (Char *)bsearch(&sid, h->component.chars.array,
		                      h->component.chars.cnt, sizeof(Char), matchSID);
		if (src == NULL) {
			return 0;           /* Glyph already in subset */
		}
		if (src->length != 0) {
			/* Add to char list */
			*dnaNEXT(h->chars) = *src;
			src->length = 0;    /* Avoid subsequent addition */
		}
	}
	return 0;
}

/* Get seac component glyph charstring. Return 1 on error else 0. */
int parseGetComponent(tcCtx g, int code,
                      unsigned *length, unsigned char **cstr) {
	parseCtx h = g->ctx.parse;
	Char *chr;
	long iChar;

	if (code < 0 || code > 255) {
		return 1;   /* Invalid code */
	}
	iChar = h->component.stdindex[code & 0xff];
	if (iChar == 0) {
		return 1;   /* No standard char at this code point */
	}
	chr = (iChar & COMP_CHAR) ?
	    &h->component.chars.array[iChar & ~COMP_CHAR] : &h->chars.array[iChar];

	*length = chr->length;
	*cstr = (unsigned char *)&h->buf.array[chr->index];

	if (h->lenIV != -1) {
		if (chr->encrypted) {
			/* Decrypt charstring if not already done so */
			h->decrypt(*length, *cstr);
			chr->encrypted = 0;
		}
		*cstr += h->lenIV;
		*length -= h->lenIV;
	}
	return 0;
}

/* Compare chars */
static int CDECL cmpChars(const void *first, const void *second) {
	const Char *a = first;
	const Char *b = second;
#if 1
	if (a->aliasOrder != -1 && b->aliasOrder != -1) {
		return a->aliasOrder - b->aliasOrder;       /* Both have an order from the glyph alias file: sort by aliasOrder */
	}
	else if (a->aliasOrder != -1 || b->aliasOrder != -1) {
		return b->aliasOrder - a->aliasOrder + 1;   /* Mixed: aliasOrder preceeds encoding */
	}
	else
#endif
	if (a->code != -1 && b->code != -1) {
		return a->code - b->code;       /* Both encoded: sort by code */
	}
	else if (a->code == -1 && b->code == -1) {
		return a->id - b->id;           /* Both unencoded: sort by SID */
	}
	else {
		return b->code - a->code + 1;   /* Mixed: encoded preceeds unencoded */
	}
}

/* Reorder chars by encoding and add encoding and charset indices to font */
static void reorderChars(parseCtx h) {
	tcCtx g = h->g;
	int i;
	SID custom[256];    /* Custom encoding */

	h->reorder.supplement.cnt = 0;
	h->reorder.encoding.cnt = 0;
	h->reorder.charset.cnt = 0;
	h->reorder.index.cnt = 0;

	/*
	   We need to sort by SID to make sure that the added .notdef gets moved to array[0].
	 */
	qsort(&h->chars.array[0], h->chars.cnt, sizeof(Char), cmpSIDs);


	if (h->encoding.std) {
		/* Add StandardEncoding */
		for (i = 1; i < h->chars.cnt - 1; i++) {
			Char *chr = &h->chars.array[i];
			if (chr->id < TABLE_LEN(stdcodes)) {
				chr->code = stdcodes[chr->id];
			}
		}
	}
	else {
		/* Add custom encoding */
		qsort(&h->chars.array[1], h->chars.cnt - 1, sizeof(Char), cmpSIDs);

		for (i = 0; i < 256; i++) {
			SID sid;
			Char *chr;
			char *name = h->encoding.custom[i];

			if (name == NULL ||
			    (sid = sindexCheckId(h->g, strlen(name), name)) == SID_UNDEF) {
				/* Unencoded or encoding for non-existent glyph */
				custom[i] = SID_NOTDEF;
				continue;
			}
			else {
				custom[i] = sid;
			}

			/* Lookup glyph in char list */
			chr = (Char *)bsearch(&sid, &h->chars.array[0], h->chars.cnt,
			                      sizeof(Char), matchSID);
			if (chr != NULL) {
				if (chr->code != -1) {
					/* Subsequent encoding for glyph */
					CodeMap *supplement = dnaNEXT(h->reorder.supplement);
					supplement->code = i;
					supplement->sid = sid;
				}
				else {
					chr->code = i; /* Add encoding */
				}
			}
		}
	}

	/* Sort chars by encoding then SID for unencoded glyphs */
	qsort(&h->chars.array[1], h->chars.cnt - 1, sizeof(Char), cmpChars);

	/* Build encoding, charset, and index arrays (excluding .notdef) */

	/*
	   We need to include notdef in h->reorder.index, so that in csEndFont, procs.GetChar will be called.
	   else the ordering in the final font gets messed up.
	 */
	{
		Char *chr = &h->chars.array[0];
		*dnaNEXT(h->reorder.index) = chr->reorder;
	}

	for (i = 1; i < h->chars.cnt; i++) {
		Char *chr = &h->chars.array[i];
		if (chr->code != -1) {
			*dnaNEXT(h->reorder.encoding) = (char)chr->code;
		}
		*dnaNEXT(h->reorder.charset) = chr->id;
		*dnaNEXT(h->reorder.index) = chr->reorder;
	}

	if (h->encoding.std) {
		h->font->iEncoding = 0; /* Standard encoding */
	}
	else {
		i = (h->reorder.supplement.cnt > 0) ? -1 : encodingCheckPredef(custom);
		h->font->iEncoding = (i != -1) ? i :
		    encodingAdd(h->g,
		                h->reorder.encoding.cnt, h->reorder.encoding.array,
		                h->reorder.supplement.cnt, h->reorder.supplement.array);
	}
	h->font->iCharset =
	    charsetAdd(h->g, h->reorder.charset.cnt, h->reorder.charset.array, 0);
	csEndFont(h->g, h->reorder.index.cnt, h->reorder.index.array);

	if (g->cb.glyphMap != NULL) {
		/* Pass gid mapping to client */
		if (h->font->flags & FONT_CID) {
			for (i = 0; i < h->chars.cnt; i++) {
				g->cb.glyphMap(g->cb.ctx,
				               (unsigned short)h->chars.cnt, (unsigned short)i,
				               NULL, h->chars.array[i].id);
			}
		}
		else {
			for (i = 0; i < h->chars.cnt; i++) {
				g->cb.glyphMap(g->cb.ctx,
				               (unsigned short)h->chars.cnt, (unsigned short)i,
				               sindexGetString(g, h->chars.array[i].id), 0);
			}
		}
	}
}

/* Return key name from index. Only used for diagnostics so linear search OK */
static char *keyName(int iKey) {
	unsigned int i;
	for (i = 0; i < TABLE_LEN(keyMap); i++) {
		if (keyMap[i].index == iKey) {
			break;
		}
	}
	return keyMap[i].name;
}

/* Report bad dict value */
static void badKeyValue(parseCtx h, int iKey) {
	parseFatal(h->g, "/%s bad value", keyName(iKey));
}

/* Parse number array */
static int parseArray(parseCtx h, int iKey, int max, double *array) {
	int cnt;
	Key *key = &h->keys[iKey];
	psToken *token = &key->value;
	char *p = psGetValue(h->ps, token);
	char *end = p + token->length;

	if (token->type != PS_ARRAY && token->type != PS_PROCEDURE) {
		badKeyValue(h, iKey);   /* Expecting array object */
	}
	cnt = 0;
	while ((p += strspn(p, " []{}\n")) < end) {
		if (cnt == max) {
			parseWarning(h->g, "/%s array too big (truncated)", keyName(iKey));
			break;
		}
		else {
			array[cnt++] = strtod(p, &p);
		}
	}

	return cnt;
}

/* Get the StdVW value from the 16th token in the Erode proc */
static long getStdVWFromErodeProc(parseCtx h) {
	if (SEEN_KEY(iErode)) {
		/* StdVW not found (old font) so use 16th erode proc token! */
		psToken *token = &h->keys[iErode].value;
		if (token->length != 0) {
			int i = 0;
			char *p = psGetValue(h->ps, token) + 1;

			for (;; ) {
				/* Skip whitespace */
				while (isspace(*p)) {
					p++;
				}

				if (*p == '}') {
					return -1;  /* Badly formed erode proc */
				}
				if (++i == 16) {
					/* Return 16th token */
					return strtol(p, NULL, 0);
				}

				/* Skip token */
				while (!isspace(*p)) {
					p++;
				}
			}
		}
	}
	return -1;
}

#if TC_EURO_SUPPORT

/* Get ItalicAngle if present in font else return 0. */
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
double parseGetItalicAngle(tcCtx g) {
	parseCtx h = g->ctx.parse;
	if (SEEN_KEY(iItalicAngle)) {
		psToken *token = &h->keys[iItalicAngle].value;
		if (token->type == PS_INTEGER) {
			return psConvInteger(h->ps, token);
		}
		else if (token->type == PS_REAL) {
			return psConvReal(h->ps, token);
		}
	}
	return 0.0;
}

/* Add Euro character to font. */
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
static int addEuro(parseCtx h, int id) {
	tcCtx g = h->g;
	Char *chr;
	const int kEuro_gid = 2; /* gid of Euro in Euro MM fill-in font. */

	if (h->subset.spec != NULL ||
	    h->font->flags & (FONT_CID | FONT_CHAMELEON) ||
	    sindexSeenGlyphNameId(g, sindexCheckId(g, 4, "Euro")) ||
	    sindexSeenGlyphNameId(g, sindexCheckId(g, 7, "uni20AC")) ||
	    !(sindexSeenGlyphNameId(g, SID_ZERO) ||
	      sindexSeenGlyphNameId(g, SID_ZEROOLDSTYLE)) ||
	    !(sindexSeenGlyphNameId(g, SID_DOLLAR) ||
	      sindexSeenGlyphNameId(g, SID_STERLING) ||
	      sindexSeenGlyphNameId(g, SID_DOLLAROLDSTYLE))) {
		return id;  /* Font doesn't match criteria for euro addition */
	}
	/* Add new character record for euro */
	chr = dnaNEXT(h->chars);
	chr->index = 0;
	chr->length = 0;
	chr->id = sindexGetGlyphNameId(g, 4 /* length of "Euro" */, "Euro");
	chr->code = -1;
	chr->fdIndex = 0;
	chr->reorder = id;
	chr->encrypted = 0;

	parseWarning(h->g, "Adding glyph %s, gid %d.",  "Euro",  chr->id);
	{
		unsigned noslant = 0;
		unsigned adjust_overshoot = 1;
		recodeAddNewGlyph(g, chr->id, kEuroMMIndex, kEuro_gid, noslant, adjust_overshoot,  "Euro");
	}
	return ++id;
}

/* Add the .notdef glyph to the font */
static int addNotDefGlyph(parseCtx h, int char_reorder_index) {
	tcCtx g = h->g;
	Char *chr;
	unsigned gl_id = 0; /* glyph index in FillinMM font */

	/* Add new character record */
	chr = dnaNEXT(h->chars);
	chr->index = 0;
	chr->length = 0;
	chr->id = SID_NOTDEF;
	chr->code = -1;
	chr->fdIndex = 0;
	chr->reorder = char_reorder_index; /* glyph index in h->chars.array */
	chr->encrypted = 0;


	parseNewGlyphReport(h->g, "Adding glyph %s, SID %d.", ".notdef",  chr->id);

	{
		unsigned noslant = 1;
		unsigned adjust_overshoot = 0;
		recodeAddNewGlyph(g, chr->id, kFillinMMIndex, gl_id, noslant, adjust_overshoot, ".notdef");
	}

	return ++char_reorder_index;
}

/* Add the synthesized glyphs to the font */
static int addStdGlyph(parseCtx h, int id, unsigned gl_id, char **charListptr) {
	tcCtx g = h->g;
	Char *chr;
	char *char_name;
	unsigned str_length;
	int i;
	int seenGlyph = 0;


	/* don't synthesize the glyph if:
	   (1) it is a chameleon cid or or subset font
	   (2)	the glyph has already been seen in the font proper.

	   but add it anyway if we adding the .notdef, and (g->flags & TC_FORCE_NOTDEF). */
	if (h->subset.spec != NULL ||
	    h->font->flags & (FONT_CID | FONT_CHAMELEON)) {
		return id;      /* Font doesn't match criteria for euro addition */
	}


	/* save  first (preferred) glyph name in list */
	char_name = *charListptr;
	str_length = strlen(char_name);

	for (i = 0; i < kMaxAlternateNames; i++) {
		char *char_name2 = charListptr[i];
		unsigned str_length2;

		if (char_name2 == NULL) {
			break;
		}

		str_length2 = strlen(char_name2);

		if (sindexSeenGlyphNameId(g, sindexCheckId(g, str_length2, char_name2))) {
			seenGlyph = 1;
			break;
		}
	}

	if (seenGlyph && (!((g->flags & TC_FORCE_NOTDEF) && (id == 0)))) {
		return id;  /* don't need to synthesize glyph - already seen. */
	}
	/* Add new character record */
	chr = dnaNEXT(h->chars);
	chr->index = 0;
	chr->length = 0;
	chr->id = sindexGetGlyphNameId(g, str_length, char_name);
	chr->code = -1;
	chr->fdIndex = 0;
	chr->reorder = id;
	chr->encrypted = 0;


	parseNewGlyphReport(h->g, "Adding glyph %s, gid %d.", char_name,  chr->id);

	{
		unsigned noslant = 1; /* default: do not slant glyph */
		unsigned adjust_overshoot = 0; /* default: do not shift glyph vertically */

		char *slantnames[] = {
			"at", "backslash", "fraction", "numbersign", "partialdiff",                   /* added on 11/12/2001 */
			/*"paragraph", removed on 11/12/2001 */
			"dagger", "daggerdbl", "section", "afii61289",
			"perthousand", "quotesingle", "quotedbl", "onequarter",
			"onehalf", "threequarters", "Delta", "PI",  "pi", "Omega", 0
		};

		char *adjust_overshoot_names[] = {
			"estimated", "afii61289", "partialdiff", 0
		};

		for (i = 0; slantnames[i] != 0; i++) {
			if (strcmp(char_name, slantnames[i]) == 0) {
				noslant = 0;
				break;
			}
		}

		for (i = 0; adjust_overshoot_names[i] != 0; i++) {
			if (strcmp(char_name, adjust_overshoot_names[i]) == 0) {
				adjust_overshoot = 1;
				break;
			}
		}

		recodeAddNewGlyph(g, chr->id, kFillinMMIndex, gl_id, noslant, adjust_overshoot, char_name);
	}

	return ++id;
}

#endif /* TC_EURO_SUPPORT */



/* Add encoding, charset, subrs, and chars */
static void addFont(parseCtx h) {
	tcCtx g = h->g;
	int i, char_reorder_index;


	/* Check .notdef was read */
	/* Note: if g->flags & TC_FORCE_NOTDEF, then char index i is NOT .notdef */
	if (h->chars.array[0].length == 0) {
		parseFatal(g, "missing .notdef charstring in source font");
	}

	/* Add subrs */
	csNewFont(g, h->font);
	csNewPrivate(g, 0, h->lenIV, h->decrypt);
	for (i = 0; i < h->subrs.cnt; i++) {
		Subr *subr = &h->subrs.array[i];
		csAddSubr(g, subr->length, &h->buf.array[subr->index], 0);
	}


	/* Add chars from source font*/
	for (i = 0; i < h->chars.cnt; i++) {
		Char *chr = &h->chars.array[i];
		csAddChar(g, chr->length, &h->buf.array[chr->index],
		          chr->id, 0, chr->encrypted);

		chr->reorder = i;
		chr->encrypted = 0;
	}

#if TC_EURO_SUPPORT
	char_reorder_index = i; /* this is the next-glyph index for adding glyphs.
	                            Needs to be one more than the number of glypsh added to date.*/

/* add chars from FillInMM and Euro built-in fonts */
/* ---------------------------------------------------------------------- */
/* ---------------------------------------------------------------------- */
	if (g->flags & (TC_ADDEURO | TC_FORCE_NOTDEF)) {
		int nStdVW;
		double StdVW[TX_MAX_MASTERS];
		int j;

		/*
		   We will always add the .notdef glyph if TC_FORCE_NOTDEF is set.
		   If the font has a zero, we will consider adding a Euro, and the other synthetic glyhs.
		   For the Euro, there are additional tests inside addEuro() to see if we will do it.
		   For the other synthetic glyphs, wejsut check if lowercase x is present.
		 */

		/* do the set up of adding synthetic glyphs */
		if (sindexSeenGlyphNameId(g, SID_ZERO) || (g->flags & TC_FORCE_NOTDEF)) {
			int font__serif_selector = 0; /* default == use heuristics. */

			if (g->flags & TC_IS_SERIF) {
				font__serif_selector = 1;
			}
			if (g->flags & TC_IS_SANSSERIF) {
				font__serif_selector = 2;
			}


			/* get StdVW for InitStaticFontData */
			/* Determine StdVW */
			if (SEEN_KEY(iStdVW)) {
				nStdVW = parseArray(h, iStdVW, TX_MAX_MASTERS, StdVW);
			}
			else {
				StdVW[0] = getStdVWFromErodeProc(h);
				nStdVW = 1;
			}

			/* If incomplete StdVW array for MM fill missing elements with -1 */
			for (j = nStdVW; j < g->nMasters; j++) {
				StdVW[j] = -1.0;
			}

			InitStaticFontData(g, font__serif_selector, StdVW, h->FontMatrix, h->isFixedPitch);
		}
	}


	/* add the marking .notdef. */
	if (g->flags & TC_FORCE_NOTDEF) {
		char_reorder_index = addNotDefGlyph(h, char_reorder_index);     /* Assumes notdef is gid 0 in FillinMM font. */
	}

	/* add the Euro and the other synthetic glyphs */
	if (g->flags & TC_ADDEURO) {
		if (sindexSeenGlyphNameId(g, SID_ZERO)) {
			/* add Euro  to anything with a zero. */
			char_reorder_index = addEuro(h, char_reorder_index);

			/* add std glyphs to non-PI fonts, e.g fonts with 0 and lowercase x. */
			if (sindexSeenGlyphNameId(g, SID_LOWERCASE_X)) {
				unsigned gl_id = 1; /* skip .notdef, gl_id 0. Either we don't add it at all, or we did it above. */
				char **charListptr = NULL; /* pointer to an array of string pointers. Last one is NULL. */
				                           /* List of glyph names which correspond to the synethetic glyph */

				/*
				   addStdGlyph may or may not increment char_reorder_index, depending on whether the glyph needed to be added.
				   getNextStdGlyph returns 0 on the call after the last glyphs in
				   the list has been processed, else it returns 1.
				 */
				while (getNextStdGlyph(gl_id, &charListptr)) {
					char_reorder_index = addStdGlyph(h, char_reorder_index, gl_id, charListptr);
					gl_id++;
				}
			}     /* end if (sindexSeenGlyphNameId(g, SID_LOWERCASE_X)) */
		}     /* if (sindexSeenGlyphNameId(g, SID_ZERO)) */
	}     /* end if (g->flags & TC_ADDEURO) */

#endif /* TC_EURO_SUPPORT */

	reorderChars(h);
}

/* Parse PostScript font */
void parseFont(tcCtx g, Font *font) {
	parseCtx h = g->ctx.parse;
	psToken strng;  /* Is type arg when StartData operator read */
	psToken integr; /* Is count arg when StartData operator read */

	/* Initialize */
	g->status = 0;
	g->status |= TC_SUBSET;
	h->font = font;
	h->lenIV = DEFAULT_LENIV;
	h->decrypt = recodeDecrypt;
	h->buf.cnt = 0;
	h->subrs.cnt = 0;
	h->component.chars.cnt = 0;
	h->component.inorder = 0;
	h->font->synthetic.oblique_term = 0.0;

	if (g->flags & TC_NOOLDOPS) {
		/* Initialize standard-encoded char index */
		int i;
		for (i = 0; i < 256; i++) {
			h->component.stdindex[i] = 0;
		}
	}
	g->nMasters = 1;
	strng.type = integr.type = PS_EOI;

	h->chars.cnt = 0;

	/* Initialize first char entry data */
	dnaNEXT(h->chars)->length = 0; /* see notdef special case in readChars() */

	dictInit(h);
	sindexFontInit(g);

	/* Parse font */
	h->ps = psNew(&h->psCB);
	h->seen.end = 0;
	do {
		psToken *token = psGetToken(h->ps);
		switch (token->type) {
			case PS_LITERAL:
				doLiteral(h, token);
				break;

			case PS_OPERATOR:
				doOperator(h, token, &strng, &integr);
				break;

			case PS_STRING:
				strng = *token;
				break;

			case PS_INTEGER:
				integr = *token;
				break;

			case PS_EOI:
				parseFatal(g, "premature EOF");
		}
	}
	while (!h->seen.end);

	/* Finish processing font */
	if (h->font->flags & FONT_CID) {
		/* CIDFont */
		int fd;

		/* Read subrs for each FD */
		for (fd = 0; fd < h->font->fdCount; fd++) {
			cidReadSubrs(h, fd);
		}

		cidReadChars(h);
		cidAddChars(h);
	}
	else {
		/* Remove small memory mode for non-CID fonts */
		g->flags &= ~TC_SMALLMEMORY;

		if (h->font->flags & FONT_CHAMELEON) {
			saveDicts(h);
		}
		else {
			/* Type 1 font */
			saveDicts(h);
			addFont(h);
		}
	}

	/*
	   If the font is obliqued, then we have already obliqued the charstrings, and it is no longer
	   synthetic.
	 */
	if (h->font->flags & FONT_SYNTHETIC) {
		if (h->font->synthetic.oblique_term == 0.0) {
			saveSynthetic(h);
		}
		else {
			h->font->flags &= ~FONT_SYNTHETIC;
		}
	}


#if TC_STATISTICS
	if (g->cb.psSize != NULL) {
		g->stats.fontSize += g->cb.psSize(g->cb.ctx);
	}
#endif /* TC_STATISTICS */

	psFree(h->ps);
	h->ps = NULL;
}

/* --- Dictionary parsing --- */

/* Match key name */
static int CDECL matchKey(const void *key, const void *value) {
	const String *strng = key;
	return tc_strncmp(strng->data, strng->length, ((DictKeyMap *)value)->name);
}

/* Read FontName */
static void readFontName(parseCtx h) {
	unsigned length;
	char *FontName;
	psToken *value = psGetToken(h->ps);

	if (value->type != PS_LITERAL) {
		badKeyValue(h, iFontName);
	}

	FontName = psConvLiteral(h->ps, value, &length);
	h->font->FontName = tc_dupstrn(h->g, FontName, length);
}

/* Read integer-valued dict key */
static long readIntKey(parseCtx h, int iKey) {
	psToken *token = psGetToken(h->ps);
	if (token->type != PS_INTEGER) {
		badKeyValue(h, iKey);
	}
	return psConvInteger(h->ps, token);
}

/* Process literal */
static void doLiteral(parseCtx h, psToken *literal) {
	tcCtx g = h->g;
	long type;
	String key;
	DictKeyMap *map;

	key.data = psConvLiteral(h->ps, literal, &key.length);

	/* Lookup literal in dict key map */
	map = (DictKeyMap *)
	    bsearch(&key, keyMap, TABLE_LEN(keyMap), sizeof(DictKeyMap), matchKey);

	/* Check for unknown dict key */
	if (map == NULL) {
		if (SEEN_KEY(iPrivate) && !(h->font->flags & FONT_SYNTHETIC)) {
			parseFatal(g, "unknown Private dict key /%.*s",
			           key.length, key.data);
		}
		return;
	}

	if (h->font->flags & FONT_SYNTHETIC) {
		/* Synthetic font; re-read Encoding, Subrs, and read CharStrings */
		switch (map->index) {
			case iEncoding:
				readEncoding(h);
				break;

			case iSubrs:
				readSubrs(h);
				break;

			case iCharStrings:
				readChars(h);
				break;

			case iChameleon:
				readChameleon(h);
				break;
		}
	}
	else {
		/* For CIDFonts check for key redefinition which signals new FD */
		if (h->font->flags & FONT_CID && h->cid.fd != -1 &&
		    map->index != iignore && SEEN_KEY(map->index)) {
			cidSaveFD(h);
		}

		/* Literal either processed now or value saved for later */
		switch (map->index) {
			case iFontName:
			case iCIDFontName:
				if (h->font->FontName == NULL) {
					/* Add top-level FontName */
					readFontName(h);
					return;
				}
				else if (h->font->flags & FONT_CID) {
					/* Save value for later */
					h->keys[map->index].value = *psGetToken(h->ps);
				}
				else {
					parseFatal(g, "duplicate FontName");
				}
				break;

			case iEncoding:
				readEncoding(h);
				break;

			case iSubrs:
				readSubrs(h);
				break;

			case iCharStrings:
				readChars(h);
				break;

			case iChameleon:
				readChameleon(h);
				break;

			case ilenIV:
				h->lenIV = readIntKey(h, ilenIV);
				break;

			case iFontType:
				type = readIntKey(h, iFontType);
				if (type != 1) {
					parseFatal(g, "FontType %d unsupported", type);
				}
				break;

			case iCIDInit:
				cidInit(h);
				break;

			case iCIDFontType:
				type = readIntKey(h, iCIDFontType);
				if (type != 0) {
					parseFatal(g, "CIDFontType %d unsupported", type);
				}
				break;

			case iFDArray:
				initFDArray(h);
				break;

			case iRunInt:
				if (t13CheckConv(g, h->ps, &h->decrypt)) {
					badKeyValue(h, iRunInt);
				}
				break;

			case iPrivate:
			case iignore:
				break;

			case iFamilyBlues:
				if (SEEN_KEY(iFamilyBlues) && !SEEN_KEY(iWeightVector)) {
					/* Ignore second FamilyBlues in some bad single master fonts */
					parseWarning(g, "duplicate /FamilyBlues (ignored)");
					break;
				}

			/* Fall through */
			default:
				/* Save value token */
				h->keys[map->index].value = *psGetToken(h->ps);
				break;
		}
		SET_KEY_SEEN(map->index);   /* Note key was seen */
	}
}

/* Check BDM for consistency */
static int checkBDM(parseCtx h, int nBDM, double *BDM, int nAxes) {
	tcCtx g = h->g;
	int i;
	int iAxis;

	if (nBDM & 1 || nBDM < nAxes * 4) {
		badKeyValue(h, iBlendDesignMap);
	}

	i = 0;
	iAxis = 0;
	while (i < nBDM) {
		double U0 = BDM[i++];
		double N0 = BDM[i++];
		double U1 = BDM[i++];
		double N1 = BDM[i++];

		for (;; ) {
			if (U0 >= U1 || N0 >= N1 ||
			    N0 < 0 || N0 > 1 || N1 < 0 || N1 > 1) {
				/* Coordinates must be increasing and norm. in range 0-1 */
				parseFatal(g, "invalid axis[%d] map", iAxis);
			}
			else if (N1 == 1.0) {
				/* Last map in axis */
				iAxis++;
				break;
			}
			else {
				double delta;
				double U2 = BDM[i++];
				double N2 = BDM[i++];

				if (N2 - N0 < 0.001) {
					/* Prevent division by very small quantity */
					parseFatal(g, "invalid axis[%d] map", iAxis);
				}

				delta = (N2 - N0) * (U1 - U0) / (U2 - U0) + N0 - N1;
				if (delta < 0) {
					delta = -delta;
				}

				if (delta < 0.001) {
					/* Discard useless map */
					parseWarning(g, "useless axis map[%g %g] (discarded)",
					             U1, N1);
					memmove(&BDM[i - 4], &BDM[i - 2],
					        sizeof(double) * (nBDM - i + 2));
					nBDM -= 2;
				}
				else {
					if (delta < .02) {
						parseWarning(g, "near-miss axis map[%g %g]", U1, N1);
					}
					U0 = U1;
					N0 = N1;
				}
				U1 = U2;
				N1 = N2;
			}
		}
	}
	if (iAxis != nAxes) {
		badKeyValue(h, iBlendDesignMap);
	}

	return nBDM;    /* Return new length of BDM */
}

/* Calculate UDV for default instance (specified by WeightVector) */
static void calcUDV(parseCtx h, double *BDM, double *BDP,
                    int nAxes, double *UDV, double *WV) {
	int i = 0;
	int j;
	double U0 = BDM[i++];
	double N0 = BDM[i++];

	for (j = 0; j < nAxes; j++) {
		int k;
		int l = j;
		double norm = 0;    /* Per-axis normalized coord accumulator */

		/* Add normalized coord contribution for each master */
		for (k = 0; k < h->g->nMasters; k++) {
			norm += BDP[l] * WV[k];
			l += nAxes;
		}

		/* Find matching axis map and interpolate */
		for (;; ) {
			double U1 = BDM[i++];
			double N1 = BDM[i++];

			if (N0 <= norm && norm <= N1) {
				UDV[j] = (int)((U1 - U0) * (norm - N0) / (N1 - N0) + U0 + 0.5);
			}

			if (N1 == 1.0) {
				break;  /* End of axis */
			}
			U0 = U1;
			N0 = N1;
		}
	}
}

/* Get integer value from dict key */
static long getKeyInt(parseCtx h, int iKey, int required) {
	psToken *token = &h->keys[iKey].value;

	if (!SEEN_KEY(iKey)) {
		if (required) {
			parseFatal(h->g, "/%s missing", keyName(iKey));
		}
		return 0;
	}
	if (token->type != PS_INTEGER) {
		badKeyValue(h, iKey);
	}

	return psConvInteger(h->ps, token);
}

/* Get vector conversion subr. Return 0 on success else 1 */
static int getConvSubr(parseCtx h, int iKey, ConvSubr *subr) {
	subr->iSubr =
	    (unsigned short)(SEEN_KEY(iKey) ? getKeyInt(h, iKey, 0) : 65535);
	if (subr->iSubr != 65535) {
		Subr *data;

		if (subr->iSubr >= h->subrs.cnt) {
			parseFatal(h->g, "subr index out of range");
		}

		data = &h->subrs.array[subr->iSubr];
		subr->data.cstr = &h->buf.array[data->index];
		subr->data.length = data->length;

		return 0;
	}
	else {
		subr->data.length = 0;
		return 1;
	}
}

/* Process and save several MM Top dict operators. */
static void saveMM(parseCtx h, DICT *dict, int iKey) {
	tcCtx g = h->g;
	ConvSubr NDV;
	ConvSubr CDV;
	int i;
	int j;
	int k;
	int cnt;
	int nAxes;
	int order[TX_MAX_MASTERS];
	double WV[TX_MAX_MASTERS];                  /* WeightVector */
	double BDP[TX_MAX_MASTERS * T1_MAX_AXES];   /* BlendDesignPostions */
	int nBDM;                                   /* BDM element count */
	double BDM[T1_MAX_AXES * TX_MAX_AXIS_MAPS * 2]; /* BlendDesignMap */
	double UDV[T1_MAX_AXES];                    /* User Design Vector */

#if 0
	/* xxx Abort if MM seen */
	parseFatal(g, "multiple master Type 1 not supported");
#endif

	/* Parse WeightVector */
	g->nMasters = parseArray(h, iKey, TX_MAX_MASTERS, WV);
	if (g->nMasters == 0) {
		badKeyValue(h, iKey);
	}

	/* Parse BlendDesignPositions */
	if (!SEEN_KEY(iBlendDesignPositions)) {
		parseFatal(g, "/BlendDesignPositions missing");
	}
	cnt = parseArray(h, iBlendDesignPositions, TABLE_LEN(BDP), BDP);
	if (cnt == 0) {
		badKeyValue(h, iBlendDesignPositions);
	}

	/* Check consistency of WeightVector and BlendDesignPositions arrays */
	nAxes = cnt / g->nMasters;
	if (nAxes > T1_MAX_AXES || nAxes * g->nMasters != cnt) {
		parseFatal(g, "inconsitent WeightVector/BlendDesignPositions");
	}

	/* Parse and check BlendDesignMap */
	if (!SEEN_KEY(iBlendDesignMap)) {
		parseFatal(g, "/BlendDesignMap missing");
	}
	nBDM = parseArray(h, iBlendDesignMap, TABLE_LEN(BDM), BDM);
	nBDM = checkBDM(h, nBDM, BDM, nAxes);

	calcUDV(h, BDM, BDP, nAxes, UDV, WV);

	for (i = 0; i < g->nMasters; i++) {
		order[i] = i;
	}

	(void)getConvSubr(h, iNDV, &NDV);
	if (getConvSubr(h, iCDV, &CDV)) {
		/* Check master coord values and build and check master reorder list */
		int check[TX_MAX_MASTERS];

		k = 0;
		for (i = 0; i < g->nMasters; i++) {
			order[i] = 0;
			for (j = 0; j < nAxes; j++) {
				double d = BDP[k++];
				long n = (long)d;

				if (n != d || (n != 0 && n != 1)) {
					/* Non-integer master coordinates must have CDV support */
					badKeyValue(h, iBlendDesignPositions);
				}

				/* Each master's position is computed by considering its axis
				   coordinates as a binary number with first axis as lsb. This
				   yields a Adobe's master ordering */
				order[i] |= n << j;
			}
		}

		/* Check every master represented */
		memset(check, 0, g->nMasters);
		for (i = 0; i < g->nMasters; i++) {
			check[order[i]] = 1;
		}
		for (i = 0; i < g->nMasters; i++) {
			if (!check[i]) {
				badKeyValue(h, iBlendDesignPositions);
			}
		}

		/* Check nMasters for power of 2 */
		if (g->nMasters != "\002\004\010\020"[nAxes - 1]) {
			parseFatal(g, "bad nMasters %d", g->nMasters);
		}
	}

	/* Save vector conversion subrs */
	recodeSaveConvSubrs(g, h->font, nAxes, BDM, order, h->lenIV, &NDV, &CDV);

	/* Store: nMasters UDV lenBCA NDV CDV MultipleMaster */
	dictSaveNumber(dict, g->nMasters);
	for (i = 0; i < nAxes; i++) {
		dictSaveNumber(dict, UDV[i]);
	}
	dictSaveNumber(dict, SEEN_KEY(ilenBuildCharArray) ?
	               getKeyInt(h, ilenBuildCharArray, 0) : 32);
	dictSaveNumber(dict, NDV.sid);
	dictSaveNumber(dict, CDV.sid);
	DICTSAVEOP(*dict, cff_MultipleMaster);
}

/* Convert token to SID */
static SID getKeySID(parseCtx h, int iKey) {
	unsigned length;
	char *strng;
	psToken *token = &h->keys[iKey].value;

	if (!SEEN_KEY(iKey)) {
		parseFatal(h->g, "/%s missing", keyName(iKey));
	}
	if (token->type != PS_STRING) {
		badKeyValue(h, iKey);
	}

	strng = psConvString(h->ps, token, &length);
	return sindexGetId(h->g, length, strng);
}

/* Save Registry-Ordering-Supplement with one opcode. */
static void saveROS(parseCtx h, DICT *dict, int iKey) {
	dictSaveInt(dict, getKeySID(h, iRegistry));
	dictSaveInt(dict, getKeySID(h, iOrdering));
	dictSaveInt(dict, getKeyInt(h, iSupplement, 1));
	DICTSAVEOP(*dict, cff_ROS);
}

/* Save string or literal value */
static void saveString(parseCtx h, DICT *dict, int iKey) {
	unsigned length;
	char *strng = NULL; /* Suppress optimizer warning */
	Key *key = &h->keys[iKey];
	psToken *token = &key->value;

	if (key->dflt != NULL && psMatchValue(h->ps, token, key->dflt)) {
		return;     /* Matched default */
	}
	if (token->type == PS_STRING) {
		strng = psConvString(h->ps, token, &length);
	}
	else if (token->type == PS_LITERAL) {
		strng = psConvLiteral(h->ps, token, &length);
	}
	else {
		badKeyValue(h, iKey);
	}

	dictSaveInt(dict, sindexGetId(h->g, length, strng));
	DICTSAVEOP(*dict, key->op);
}

/* Save copyright/trademark notice */
static void saveNotice(parseCtx h, DICT *dict, int iKey) {
	unsigned length;
	char *strng;
	Key *key = &h->keys[iKey];
	psToken *token = &key->value;

	if (token->type != PS_STRING) {
		badKeyValue(h, iKey);
	}

	strng = psConvString(h->ps, token, &length);

	if (!(h->g->flags & TC_NONOTICE)) {
		dictSaveInt(dict, sindexGetId(h->g, length, strng));
		DICTSAVEOP(*dict, cff_Notice);
	}
	if (h->g->flags & TC_SHOWNOTICE) {
		if (h->firstNotice) {
			printf("--- Original font copyright notices:\n");
			h->firstNotice = 0;
		}
		printf("%.*s\n", (int)length, strng);
	}
}

/* Save copyright/trademark notice */
static void saveCopyright(parseCtx h, DICT *dict, int iKey) {
	unsigned length;
	char *strng;
	Key *key = &h->keys[iKey];
	psToken *token = &key->value;

	if (token->type != PS_STRING) {
		badKeyValue(h, iKey);
	}

	strng = psConvString(h->ps, token, &length);

	if (!(h->g->flags & TC_NONOTICE)) {
		dictSaveInt(dict, sindexGetId(h->g, length, strng));
		DICTSAVEOP(*dict, cff_Copyright);
	}
	if (h->g->flags & TC_SHOWNOTICE) {
		if (h->firstNotice) {
			printf("--- Original font copyright notices:\n");
			h->firstNotice = 0;
		}
		printf("%.*s\n", (int)length, strng);
	}
}

/* Save boolean value */
static void saveBoolean(parseCtx h, DICT *dict, int iKey) {
	Key *key = &h->keys[iKey];
	psToken *token = &key->value;

	if (key->dflt != NULL && psMatchValue(h->ps, token, key->dflt)) {
		return;     /* Matched default */
	}
	if (psMatchValue(h->ps, token, "true")) {
		dictSaveInt(dict, 1);
	}
	else if (psMatchValue(h->ps, token, "false")) {
		dictSaveInt(dict, 0);
	}
	else {
		badKeyValue(h, iKey);
	}

	DICTSAVEOP(*dict, key->op);
}

/* Store blend array */
static void storeBlendArray(parseCtx h, DICT *dict, int iKey,
                            int cnt, double *array, int delta) {
	int i;
	int j;
	int nMasters = h->g->nMasters;
	int maxSpan = ((T2_MAX_OP_STACK - 1) / nMasters) * nMasters;

	if (cnt % nMasters != 0) {
		badKeyValue(h, iKey);
	}

	if (delta) {
		/* Compute deltas */
		for (i = cnt - 1; i > nMasters; i -= nMasters) {
			for (j = 0; j < nMasters; j++) {
				array[i - j] -= array[i - j - nMasters];
			}
		}
	}

	/* Save interpreted data */
	DICTSAVEOP(*dict, cff_T2);
	for (i = 0; i < cnt; i += maxSpan) {
		int k;
		int l;

		/* Calculate last blend index */
		j = (cnt - i <= maxSpan) ? cnt : maxSpan + i;

		/* Save initial values */
		for (k = i; k < j; k += nMasters) {
			dictSaveT2Number(dict, array[k]);
		}

		/* Save delta values */
		for (k = i; k < j; k += nMasters) {
			for (l = 1; l < nMasters; l++) {
				dictSaveT2Number(dict, array[k + l] - array[k]);
			}
		}

		/* Save blend op */
		dictSaveT2Number(dict, (j - i) / nMasters);
		DICTSAVEOP(*dict, t2_blend);
	}
	DICTSAVEOP(*dict, tx_endchar);

	DICTSAVEOP(*dict, h->keys[iKey].op);
}

static void skipValue(parseCtx h, DICT *dict, int iKey) {
	return;
}

/* Save number value */
static void saveNumber(parseCtx h, DICT *dict, int iKey) {
	Key *key = &h->keys[iKey];
	psToken *token = &key->value;

	if (token->type == PS_INTEGER) {
		long i = psConvInteger(h->ps, token);
		if (key->dflt != NULL && i == strtol(key->dflt, NULL, 0)) {
			return; /* Matched default */
		}
		dictSaveInt(dict, i);
	}
	else if (token->type == PS_REAL) {
		double d = psConvReal(h->ps, token);

		/* Exact match when ascii representations the same */
		if (key->dflt != NULL && d == strtod(key->dflt, NULL)) {
			return; /* Matched default */
		}
		dictSaveNumber(dict, d);
	}
	else {
		badKeyValue(h, iKey);
	}

	DICTSAVEOP(*dict, key->op);
}

/* Warn of empty array value */
static void warnEmptyArray(parseCtx h, int iKey) {
	parseWarning(h->g, "empty /%s array (ignored)", keyName(iKey));
}

/* Save single blend value */
static void saveBlend(parseCtx h, DICT *dict, int iKey) {
	if (h->keys[iKey].value.type == PS_ARRAY) {
		double array[TX_MAX_MASTERS];
		int cnt = parseArray(h, iKey, TX_MAX_MASTERS, array);
		if (cnt == 0) {
			warnEmptyArray(h, iKey);
		}
		else {
			storeBlendArray(h, dict, iKey, cnt, array, 0);
		}
	}
	else {
		saveNumber(h, dict, iKey);
	}
}

/* Store array values and op in dict */
static void storeArray(parseCtx h, DICT *dict, int op, int cnt, double *array) {
	int i;
	for (i = 0; i < cnt; i++) {
		dictSaveNumber(dict, array[i]);
	}
	DICTSAVEOP(*dict, op);
}

/* Save simple array */
static void saveArray(parseCtx h, DICT *dict, int iKey) {
	double array[4 * TX_MAX_MASTERS]; /* Worst case size is MM FontBBox */
	int cnt = parseArray(h, iKey, TABLE_LEN(array), array);
	if (cnt == 0) {
		switch (iKey) {
			case iFontBBox:
				badKeyValue(h, iKey);
				break;

			default:
				warnEmptyArray(h, iKey);
				break;
		}
	}
	else {
		storeArray(h, dict, h->keys[iKey].op, cnt, array);
	}
}

/* Check for embedded blend array */
static int checkForBlend(parseCtx h, int iKey) {
	psToken *token = &h->keys[iKey].value;
	char *p = psGetValue(h->ps, token);

	/* Terminate value */
	p[token->length] = '\0';

	return strchr(&p[1], '[') != NULL || strchr(&p[1], '{') != NULL;
}

/* Save FontBBox types */
static void saveFontBBox(parseCtx h, DICT *dict, int iKey) {
	if (h->g->nMasters == 1 || !checkForBlend(h, iKey)) {
		saveArray(h, dict, iKey);
	}
	else {
		double array[4 * TX_MAX_MASTERS];
		int cnt = parseArray(h, iKey, TABLE_LEN(array), array);
		if (cnt == 0) {
			badKeyValue(h, iKey);
		}
		else {
			storeBlendArray(h, dict, iKey, cnt, array, 0);
		}
	}
}

/* Save non-default font matrix */
static void saveMatrix(parseCtx h, DICT *dict) {
	static double dflt[6] = {
		0.001, 0, 0, 0.001, 0, 0
	};
	int i;

	for (i = 0; i < 6; i++) {
		if (h->FontMatrix[i] != dflt[i]) {
			int j;
			for (j = 0; j < 6; j++) {
				dictSaveNumber(dict, h->FontMatrix[j]);
			}
			DICTSAVEOP(*dict, cff_FontMatrix);
			break;
		}
	}
}

/* Parse and save font matrix */
static void parseMatrix(parseCtx h, DICT *dict, int iKey) {
	if (parseArray(h, iKey, 6, h->FontMatrix) != 6) {
		badKeyValue(h, iKey);
	}
	if (h->FontMatrix[2] != 0.0) {
		if ((h->FontMatrix[0] == (double)0.001) && (h->FontMatrix[1] == (double)0.000) && (h->FontMatrix[3] == 0.001)) {
			h->font->synthetic.oblique_term = h->FontMatrix[2] / h->FontMatrix[0];
			h->FontMatrix[2] = 0.0;
		}
		else {
			tcCtx g = h->g;
			parseWarning(g, "FontMatrix is obliqued, but other are terms non-zero. Will not flatten it.");
		}
	}
	saveMatrix(h, dict);
}

/* Save UniqueID for non-subsetted fonts only */
static void saveUniqueID(parseCtx h, DICT *dict, int iKey) {
	if (h->subset.spec == NULL) {
		saveNumber(h, dict, iKey);
	}
}

/* Save isFixedPitch, for use in adding synthetic glyphs */
static void saveIsFixedPitch(parseCtx h, DICT *dict, int iKey) {
	Key *key = &h->keys[iKey];
	psToken *token = &key->value;

	h->isFixedPitch = 0;

	if (key->dflt != NULL && psMatchValue(h->ps, token, key->dflt)) {
		return;     /* Matched default */
	}
	if (psMatchValue(h->ps, token, "true")) {
		dictSaveInt(dict, 1);
		h->isFixedPitch = 1;
	}
	else if (psMatchValue(h->ps, token, "false")) {
		dictSaveInt(dict, 0);
	}
	else {
		badKeyValue(h, iKey);
	}

	DICTSAVEOP(*dict, key->op);
}

/* Save axis types */
static void saveAxisTypes(parseCtx h, DICT *dict, int iKey) {
	Key *key = &h->keys[iKey];
	psToken *token = &key->value;
	char *p = psGetValue(h->ps, token);
	char *end = p + token->length;

	if (token->type != PS_ARRAY) {
		badKeyValue(h, iKey);
	}

	while ((p += strspn(p, " [/]")) < end) {
		char *axis = p;
		p += strcspn(p, " /]");
		dictSaveInt(dict, sindexGetId(h->g, p - axis, axis));
	}
	DICTSAVEOP(*dict, cff_BlendAxisTypes);
}

/* Save UIDBase for non-subsetted fonts only  */
static void saveUIDBase(parseCtx h, DICT *dict, int iKey) {
	if (h->subset.spec == NULL) {
		saveNumber(h, dict, iKey);
	}
}

/* Save XUID for non-subsetted fonts only  */
static void saveXUID(parseCtx h, DICT *dict, int iKey) {
	if (h->subset.spec == NULL) {
		saveArray(h, dict, iKey);
	}
}

/* Save delta-encoded array */
static void saveDelta(parseCtx h, DICT *dict, int iKey) {
	double array[TX_MAX_BLUE_VALUES * TX_MAX_MASTERS];
	int cnt = parseArray(h, iKey, TX_MAX_BLUE_VALUES * h->g->nMasters, array);

	if (cnt == 0) {
		switch (iKey) {
			case iBlueValues:
				break; /* Permitted by Black Book */

			default:
				warnEmptyArray(h, iKey);
				break;
		}
	}
	else if (h->g->nMasters == 1 || !checkForBlend(h, iKey)) {
		int i;

		/* Compute deltas */
		for (i = cnt - 1; i > 0; i--) {
			array[i] -= array[i - 1];
		}

		storeArray(h, dict, h->keys[iKey].op, cnt, array);
	}
	else {
		storeBlendArray(h, dict, iKey, cnt, array, 1);
	}
}

/* If we are flattening a syntheticly obliqued font, it becomes an italic font,
   and the stem snapV width must be saved as an empty array. */
static void saveStemSnapV(parseCtx h, DICT *dict, int iKey) {
	if (h->font->synthetic.oblique_term == 0.0) {
		saveDelta(h, dict, iKey);
	}
}

/* If we are flattening a syntheticly obliqued font, it becomes an italic font,
   then the stem stdVW width must be set to (original StdVW * cos(italic-angle) */
static void saveStdVW(parseCtx h, DICT *dict, int iKey) {
	if (h->font->synthetic.oblique_term == 0.0) {
		saveDelta(h, dict, iKey);
	}
	else {
		/* variant copy of saveDelta() */
		double array[TX_MAX_BLUE_VALUES * TX_MAX_MASTERS];
		int cnt = parseArray(h, iKey, TX_MAX_BLUE_VALUES * h->g->nMasters, array);
		int i;

		/* fix array values to account for obliqueing */
		for (i = 0; i < cnt; i++) {
			double width, angleRadians = atan(h->font->synthetic.oblique_term);
			width = floor(array[i] * cos(angleRadians));
			array[i] = width;
		}

		if (cnt == 0) {
			switch (iKey) {
				case iBlueValues:
					break; /* Permitted by Black Book */

				default:
					warnEmptyArray(h, iKey);
					break;
			}
		}
		else if (h->g->nMasters == 1 || !checkForBlend(h, iKey)) {
			int i;

			/* Compute deltas */
			for (i = cnt - 1; i > 0; i--) {
				array[i] -= array[i - 1];
			}

			storeArray(h, dict, h->keys[iKey].op, cnt, array);
		}
		else {
			storeBlendArray(h, dict, iKey, cnt, array, 1);
		}
	}
}

/* Save ForceBold */
static void saveForceBold(parseCtx h, DICT *dict, int iKey) {
	Key *key = &h->keys[iKey];
	psToken *token = &key->value;
	char *p = psGetValue(h->ps, token);

	/* As of 5/20/2010. we are diabling the ForceBold keyword. */
	return;

	if (token->type == PS_ARRAY || token->type == PS_PROCEDURE) {
		/* Array of values */
		double array[TX_MAX_MASTERS];
		int cnt = 0;
		char *end = p + token->length;

		while ((p += strspn(p, " []{}")) < end) {
			char *bool = p;

			p += strcspn(p, " ]}");
			if (strncmp(bool, "false", 5) == 0) {
				array[cnt++] = 0;
			}
			else if (strncmp(bool, "true", 4) == 0) {
				array[cnt++] = 1;
			}
			else {
				badKeyValue(h, iKey);
			}
		}
		storeBlendArray(h, dict, iKey, cnt, array, 0);
	}
	else {
		saveBoolean(h, dict, iKey);
	}
}

/* RndStemUp with any value is saved as LanguageGroup with a value 1 (which
   enables global coloring) */
static void saveRndStemUp(parseCtx h, DICT *dict, int iKey) {
	dictSaveInt(dict, 1);
	DICTSAVEOP(*dict, cff_LanguageGroup);
}

/* Save LanguageGroup if not already handled by RndStemUp */
static void saveLanguageGroup(parseCtx h, DICT *dict, int iKey) {
	if (SEEN_KEY(iRndStemUp)) {
		return; /* Already been saved */
	}
	saveNumber(h, dict, iKey);
}

/* Initialize */
void parseNew(tcCtx g) {
	parseCtx h = MEM_NEW(g, sizeof(struct parseCtx_));

	h->ps = NULL;

	dnaINIT(g->ctx.dnaCtx, h->buf, 35000, 50000);
	dnaINIT(g->ctx.dnaCtx, h->chars, 250, 50);
	dnaINIT(g->ctx.dnaCtx, h->subrs, 500, 200);
	dnaINIT(g->ctx.dnaCtx, h->component.chars, TABLE_LEN(stdcodes), 1);
	dnaINIT(g->ctx.dnaCtx, h->encoding.data, 1500, 500);
	dnaINIT(g->ctx.dnaCtx, h->synthenc.data, 1500, 500);
	dnaINIT(g->ctx.dnaCtx, h->reorder.supplement, 2, 1);
	dnaINIT(g->ctx.dnaCtx, h->reorder.encoding, 256, 64);
	dnaINIT(g->ctx.dnaCtx, h->reorder.charset, 256, 64);
	dnaINIT(g->ctx.dnaCtx, h->reorder.index, 256, 64);

	h->firstNotice = 1;

	/* Initialize font dict keys. The chameleon key is saved in	readChameleon()
	   and only appears here so that it may be initialized to be inactive. */
	IKEY(iWeightVector,     saveMM,         cff_MultipleMaster,     NULL);
	IKEY(iRegistry,         saveROS,        cff_ROS,                NULL);
	IKEY(iChameleon,        NULL,           cff_Chameleon,          NULL);
	NKEY(iversion,          saveString,     cff_version,            NULL);
	IKEY(iNotice,           saveNotice,     cff_Notice,             NULL);
	IKEY(iCopyright,        saveCopyright,  cff_Copyright,          NULL);
	NKEY(iFullName,         saveString,     cff_FullName,           NULL);
	NKEY(iFamilyName,       saveString,     cff_FamilyName,         NULL);
	IKEY(iWeight,           saveString,     cff_Weight,             NULL);
	NKEY(iisFixedPitch,     saveIsFixedPitch, cff_isFixedPitch,     "false");
	IKEY(iItalicAngle,      saveBlend,      cff_ItalicAngle,        "0");
	NKEY(iUnderlinePosition, saveBlend,      cff_UnderlinePosition,  "-100");
	NKEY(iUnderlineThickness, saveBlend,     cff_UnderlineThickness, "50");
	IKEY(iBaseFontName,     saveString,     cff_BaseFontName,       NULL);
	IKEY(iBaseFontBlend,    saveDelta,      cff_BaseFontBlend,      NULL);
	IKEY(iFontName,         saveString,     cff_FontName,           NULL);
	IKEY(iFontBBox,         saveFontBBox,   cff_FontBBox,           NULL);
	IKEY(iPaintType,        saveNumber,     cff_PaintType,          "0");
	IKEY(iFontMatrix,       parseMatrix,    cff_FontMatrix,         NULL);
	IKEY(iUniqueID,         saveUniqueID,   cff_UniqueID,           NULL);
	IKEY(iStrokeWidth,      saveBlend,      cff_StrokeWidth,        "0");
	IKEY(iBlendAxisTypes,   saveAxisTypes,  cff_BlendAxisTypes,     NULL);
	IKEY(iCIDFontVersion,   saveNumber,     cff_CIDFontVersion,     "0");
	IKEY(iCIDFontRevision,  saveNumber,     cff_CIDFontRevision,    "0");
	IKEY(iCIDCount,         saveNumber,     cff_CIDCount,           "8720");
	IKEY(iUIDBase,          saveUIDBase,    cff_UIDBase,            NULL);
	IKEY(iXUID,             saveXUID,       cff_XUID,               NULL);

	/* Initialize Private dict keys */
	IKEY(iBlueValues,       saveDelta,      cff_BlueValues,         NULL);
	IKEY(iOtherBlues,       saveDelta,      cff_OtherBlues,         NULL);
	IKEY(iFamilyBlues,      saveDelta,      cff_FamilyBlues,        NULL);
	IKEY(iFamilyOtherBlues, saveDelta,      cff_FamilyOtherBlues,   NULL);
	IKEY(iBlueScale,        saveBlend,      cff_BlueScale,         "0.039625");
	IKEY(iBlueShift,        saveBlend,      cff_BlueShift,          "7");
	IKEY(iBlueFuzz,         saveBlend,      cff_BlueFuzz,           "1");
	IKEY(iStdHW,            saveDelta,      cff_StdHW,              NULL);
	IKEY(iStdVW,            saveStdVW,      cff_StdVW,              NULL);
	IKEY(iStemSnapH,        saveDelta,      cff_StemSnapH,          NULL);
	IKEY(iStemSnapV,        saveStemSnapV,  cff_StemSnapV,          NULL);
	IKEY(iForceBold,        skipValue,   cff_ForceBold,          NULL);
	IKEY(iForceBoldThreshold, skipValue,    cff_ForceBoldThreshold, NULL);
	IKEY(iRndStemUp,        saveRndStemUp,  cff_LanguageGroup,      NULL);
	IKEY(iLanguageGroup,    saveLanguageGroup, cff_LanguageGroup,    "0");
	IKEY(iExpansionFactor,  saveNumber,     cff_ExpansionFactor,    "0.06");
	IKEY(iinitialRandomSeed, saveNumber,     cff_initialRandomSeed,  "0");

	/* Initialize PostScript callbacks */
	h->psCB.ctx         = g->cb.ctx;
	h->psCB.fatal       = g->cb.fatal;
	h->psCB.malloc      = g->cb.malloc;
	h->psCB.free        = g->cb.free;
	h->psCB.psId        = g->cb.psId;
	h->psCB.psRefill    = g->cb.psRefill;
	h->psCB.buf         = &h->buf;
	h->psCB.message     = g->cb.message;

	/* Link contexts */
	h->g = g;
	g->ctx.parse = h;
}

/* Free resources */
void parseFree(tcCtx g) {
	parseCtx h = g->ctx.parse;

	if (h->ps) {
		psFree(h->ps);
	}

	dnaFREE(h->buf);
	dnaFREE(h->encoding.data);
	dnaFREE(h->synthenc.data);
	dnaFREE(h->reorder.supplement);
	dnaFREE(h->reorder.encoding);
	dnaFREE(h->reorder.charset);
	dnaFREE(h->reorder.index);
	dnaFREE(h->subrs);
	dnaFREE(h->chars);
	dnaFREE(h->component.chars);
	MEM_FREE(g, h);
}

/* Dict initialize */
static void dictInit(parseCtx h) {
	int i;
	for (i = 0; i < KEY_COUNT; i++) {
		CLR_KEY_SEEN(i);
	}
}

/* Save key if key seen and has save function and embedable (if embedding) */
static void saveDictKey(parseCtx h, DICT *dict, int iKey) {
	Key *key = &h->keys[iKey];
	if ((key->flags & KEY_SEEN) && key->save != NULL &&
	    (!(h->g->flags & TC_EMBED) || !(key->flags & KEY_NOEMBED))) {
		(*key->save)(h, dict, iKey);
	}
}

/* Save keys into font or Private dict */
static void saveDict(parseCtx h, DICT *dict, int iFirst, int iLast) {
	int i;
	for (i = iFirst; i <= iLast; i++) {
		saveDictKey(h, dict, i);
	}
}

/* Make a synthetic font dict for use if the base font is available */
static void saveSynthetic(parseCtx h) {
	saveDictKey(h, &h->font->synthetic.dict, iFullName);
	saveDictKey(h, &h->font->synthetic.dict, iItalicAngle);
	saveMatrix(h, &h->font->synthetic.dict);
	saveEncoding(h, &h->synthenc);
}

/* Return 1 if key matched its default else return 0 */
static int defaultNumber(parseCtx h, int iKey) {
	Key *key = &h->keys[iKey];
	psToken *token = &key->value;

	if (token->type == PS_INTEGER) {
		long i = psConvInteger(h->ps, token);
		if (key->dflt != NULL && i == strtol(key->dflt, NULL, 0)) {
			return 1; /* Matched default */
		}
	}
	else if (token->type == PS_REAL) {
		double d = psConvReal(h->ps, token);

		/* Exact match when ascii representations the same */
		if (key->dflt != NULL && d == strtod(key->dflt, NULL)) {
			return 1; /* Matched default */
		}
	}
	return 0;   /* Some other token */
}

/* Save dictionary data for all dictionaries */
static void saveDicts(parseCtx h) {
	saveDict(h, &h->font->dict, TOP_DICT_FIRST, TOP_DICT_LAST);
	saveDict(h, &h->font->Private, PRIVATE_DICT_FIRST, PRIVATE_DICT_LAST);

	if (!SEEN_KEY(iFontBBox)) {
		/* FontBBox not found; may have been incorrectly specified after
		   the CharStrings dictionary in contravention to the Black Book. Since
		   this is a required key, insert the valid but useless [0 0 0 0]. */
		parseWarning(h->g, "missing /FontBBox (default inserted)");
		dictSaveInt(&h->font->dict, 0);
		dictSaveInt(&h->font->dict, 0);
		dictSaveInt(&h->font->dict, 0);
		dictSaveInt(&h->font->dict, 0);
		DICTSAVEOP(h->font->dict, cff_FontBBox);
	}

	if (!SEEN_KEY(iStdVW)) {
		/* StdVW not found (old font) so use 16th erode proc token! */
		long StdVW = getStdVWFromErodeProc(h);
		if (StdVW != -1) {
			dictSaveInt(&h->font->Private, StdVW);
			DICTSAVEOP(h->font->Private, cff_StdVW);
		}
	}
	/* BuildFont incorrectly specifies the ItalicAngle for MM oblique fonts as
	   0. This code checks for this condition and fixes the fonts known to have
	   this problem. */
	if (SEEN_KEY(iWeightVector) && defaultNumber(h, iItalicAngle)) {
		/* MM font and ItalicAngle == 0 */
		static struct {
			char *FontName;
			double ItalicAngle;
		}
		fonts[] = {
			{ "ITCAvantGardeMM-Oblique", -10.5 },
			{ "TektonMM-Oblique",        -10.0 },
		};
		unsigned int i;
		for (i = 0; i < TABLE_LEN(fonts); i++) {
			if (strcmp(h->font->FontName, fonts[i].FontName) == 0) {
				dictSaveNumber(&h->font->dict, fonts[i].ItalicAngle);
				DICTSAVEOP(h->font->dict, cff_ItalicAngle);
				break;
			}
		}
	}
}

/* Process operator */
static void doOperator(parseCtx h,
                       psToken *operator, psToken *strng, psToken *integr) {
	tcCtx g = h->g;
	if (psMatchValue(h->ps, operator, "currentfile")) {
		operator = psGetToken(h->ps);
		if (psMatchValue(h->ps, operator, "eexec")) {
			psSetDecrypt(h->ps);    /* Prepare for eexec */
		}
		else if (psMatchValue(h->ps, operator, "closefile")) {
			psSetPlain(h->ps);      /* Prepare for plaintext */
		}
	}
	else if (SEEN_KEY(iPrivate) &&
	         psMatchValue(h->ps, operator, "FontDirectory")) {
		/* Synthetic font */
		unsigned length;
		char *baseName;
		psToken *token = psGetToken(h->ps);

		if (token->type != PS_LITERAL) {
			parseFatal(g, "expecting synthetic base name\n");
		}

		baseName = psConvLiteral(h->ps, token, &length);
		h->font->synthetic.baseName = tc_dupstrn(g, baseName, length);
		dnaINIT(h->g->ctx.dnaCtx, h->font->synthetic.dict, 50, 50);
		h->font->flags |= FONT_SYNTHETIC;

		/* Save synthetic font's encoding */
		if (h->encoding.std) {
			h->synthenc.std = 1;
		}
		else {
			/* Save synthetic encoding (by swapping) */
			Encoding tmp = h->encoding;
			h->encoding = h->synthenc;
			h->synthenc = tmp;
		}
		h->subrs.cnt = 0;           /* Re-read Subrs in base font */
	}
	else if ((h->font->flags & FONT_CID) &&
	         psMatchValue(h->ps, operator, "StartData")) {
		cidSaveFD(h);       /* Save last FD */

		if (h->cid.fd != h->font->fdCount) {
			parseFatal(g, "bad FDCount");
		}

		h->buf.cnt = 0;         /* Fill buffer with data from beginning */

		/* Read binary data */
		if (strng->type != PS_STRING || integr->type != PS_INTEGER) {
			parseFatal(g, "bad StartData format");
		}
		else if (psMatchValue(h->ps, strng, "(Binary)")) {
			if (g->flags & TC_SMALLMEMORY) {
				/* Find the offset of the first charstring. */
				long offset;

				/* This trick is based on the assumption that the CIDMap is at
				   the beginning of the binary section and that it is followed
				   by the SubrMap and SubrData. */
				if (h->cid.MapOffset != 0) {
					parseFatal(g, "expecting CIDMapOffset == 0\n");
				}

				h->cid.bytesRead = h->cid.FDBytes + h->cid.GDBytes;
				psReadBinary(h->ps, h->cid.bytesRead);
				offset = getOffset(h, &h->buf.array[h->cid.FDBytes],
				                   h->cid.GDBytes);

				/* Read rest of map and subrs */
				psReadBinary(h->ps, offset - h->cid.bytesRead);
				h->cid.bytesRead = offset;

				/* Force allocation for maximum length charstring. */
				/* xxx The CID parser would benefit from using the da for the
				   PostScript part at the beginning of the file and then doing
				   the reading locally without using the pstoken library. This
				   would make the code more efficient and more readable. */
				(void)dnaGROW(h->buf, h->buf.cnt + 65535 - 1);
			}
			else {
				psReadBinary(h->ps, psConvInteger(h->ps, integr));
			}
		}
		else if (psMatchValue(h->ps, strng, "(Hex)")) {
			parseFatal(g, "unimplemented StartData type");
		}
		else {
			parseFatal(g, "bad StartData type");
		}

		h->seen.end = 1;            /* Finished reading input */
	}
}

/* --- CID support --- */

/* Initialize CIDFont */
static void cidInit(parseCtx h) {
	if (!(h->g->flags & TC_SMALLMEMORY) && h->buf.size < 3500000) {
		/* Resize da's for CID fonts */
		dnaFREE(h->buf);
		dnaFREE(h->chars);
		dnaINIT(h->g->ctx.dnaCtx, h->buf, 3500000, 1000000);
		dnaINIT(h->g->ctx.dnaCtx, h->chars, 9000, 1000);
	}
	h->font->flags |= FONT_CID;
	h->font->iEncoding = 0; /* Standard; nothing will be written to dict */
	h->cid.fd = -1;
}

/* Initialize dicts and decryption for new FD */
static void FDInit(parseCtx h, int fd) {
	dictInit(h);
	h->lenIV = DEFAULT_LENIV;
	h->decrypt = recodeDecrypt;
	h->cid.fd = fd;
}

/* Initialize FDArray */
static void initFDArray(parseCtx h) {
	tcCtx g = h->g;
	int i;

	/* Save top-level dict */
	saveDict(h, &h->font->dict, TOP_DICT_FIRST, TOP_DICT_LAST);

	h->font->fdCount = (short)readIntKey(h, iFDArray);
	if (h->font->fdCount <= 0 || h->font->fdCount > 256) {
		tcFatal(g, "FDArray out-of-range");
	}

	/* Initialize FDArray */
	h->font->fdInfo = MEM_NEW(g, h->font->fdCount * sizeof(FDInfo));
	for (i = 0; i < h->font->fdCount; i++) {
		FDInfo *fdi = &h->font->fdInfo[i];

		dnaINIT(h->g->ctx.dnaCtx, fdi->FD, 50, 50);
		dnaINIT(h->g->ctx.dnaCtx, fdi->Private, 100, 50);

		/* In case of defective font */
		fdi->SubrCount = 0;
		fdi->seenChar = 0;

		fdi->subrs.nStrings = 0;
	}
	csNewFont(g, h->font);

	/* Save values used for accessing binary data section */
	h->cid.MapOffset = getKeyInt(h, iCIDMapOffset, 1);
	h->cid.FDBytes = getKeyInt(h, iFDBytes, 1);
	h->cid.GDBytes = getKeyInt(h, iGDBytes, 1);
	h->cid.Count = getKeyInt(h, iCIDCount, 1);

	if (h->cid.FDBytes != 1 && h->cid.FDBytes != 0) {
		tcFatal(g, "FDBytes out-of-range");
	}

	FDInit(h, 0);
}

/* Save dictionary data in FDArray */
static void cidSaveFD(parseCtx h) {
	tcCtx g = h->g;
	FDInfo *fdInfo = &h->font->fdInfo[h->cid.fd];

	if (!SEEN_KEY(iStdVW)) {
		/* Lack of StdVW or Erode proc will cause bad rasterization */
		parseWarning(g, "no /StdVW in fd[%d]", h->cid.fd);
	}

	fdInfo->SDBytes = (short)getKeyInt(h, iSDBytes, 0);
	fdInfo->SubrMapOffset = getKeyInt(h, iSubrMapOffset, 0);
	fdInfo->SubrCount = getKeyInt(h, iSubrCount, 0);

	saveDict(h, &fdInfo->FD, TOP_DICT_FIRST, TOP_DICT_LAST);
	saveDict(h, &fdInfo->Private, PRIVATE_DICT_FIRST, PRIVATE_DICT_LAST);

	csNewPrivate(g, h->cid.fd, h->lenIV, h->decrypt);
	FDInit(h, h->cid.fd + 1);
}

/* Used to fetch 1-4 byte offsets out of CID subr and char offset tables. */
static long getOffset(parseCtx h, char *buf, int size) {
	long result = 0;
	switch (size) {
		case 4: result = *buf++ & 0xff;

		case 3: result = result << 8 | (*buf++ & 0xff);

		case 2: result = result << 8 | (*buf++ & 0xff);

		case 1: result = result << 8 | (*buf & 0xff);
			break;
	}
	return result;
}

/* There is no CID equivalent of font.c/readSubrs; they are directly added to
   the new.fds.private[fd].subrs data maintained by cs.c/csAddSubr. This is
   more comparable to the loop in font.c/addFont. */
static void cidReadSubrs(parseCtx h, int fd) {
	long i;
	FDInfo *fdInfo = &h->font->fdInfo[fd];
	char *pSD = &h->buf.array[fdInfo->SubrMapOffset];
	long thisSubrOffset = getOffset(h, pSD, fdInfo->SDBytes);

	for (i = 1; i <= fdInfo->SubrCount; i++) {
		long subrLength;
		long nextSubrOffset;

		pSD += fdInfo->SDBytes;
		nextSubrOffset = getOffset(h, pSD, fdInfo->SDBytes);

		subrLength = nextSubrOffset - thisSubrOffset;
		if (subrLength == 0 || subrLength > CS_MAX_SIZE) {
			tcFatal(h->g, "bad subr length fd[%d].subr[%u]", fd, i);
		}

		csAddSubr(h->g,
		          (unsigned)subrLength, &h->buf.array[thisSubrOffset], fd);
		thisSubrOffset = nextSubrOffset;
	}
}

/* Compare CIDs */
static int CDECL cmpCIDs(const void *first, const void *second) {
	unsigned a = *(unsigned short *)first;
	unsigned b = *(unsigned short *)second;
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

/* Read char offsets from the binary data, subset them, and accumulate da's
   full of offsets and lengths. */
static void cidReadChars(parseCtx h) {
	tcCtx g = h->g;
	unsigned icid;
	unsigned nChars = 0;
	int tableBytes = h->cid.GDBytes + h->cid.FDBytes;
	SID *sid = MEM_NEW(g, sizeof(SID) * h->cid.Count);
	unsigned short *subsetSID = NULL;
	unsigned short *pSubsetSID = NULL;


	/* xxx need to add CID 0 if not in spec when subsetting */
	h->chars.cnt = 0;
	for (icid = 0; icid < h->cid.Count; ++icid) {
		/* Use subset list or enumerate entire range. */
		Char *new;
		unsigned CID = icid;
		char *pOffset = &h->buf.array[h->cid.MapOffset + CID * tableBytes];
		long thisCSOffset =
		    getOffset(h, pOffset + h->cid.FDBytes, h->cid.GDBytes);
		long nextCSOffset =
		    getOffset(h, pOffset + tableBytes + h->cid.FDBytes, h->cid.GDBytes);
		long csLength = nextCSOffset - thisCSOffset;
		if (csLength > CS_MAX_SIZE) {
			parseFatal(g, "bad char length cid#%hd", icid);
		}

		/* If we subsetted an empty interval (glyph is not in font) it's an
		   error. If we don't have a subset spec, ignore it. */
		if (csLength == 0) {
			continue;
		}

		new = dnaNEXT(h->chars);
		new->index = thisCSOffset;
		new->length = (unsigned short)csLength;
		new->code = 0;
		new->id = CID;
		new->fdIndex = (h->cid.FDBytes == 0) ?
		    0 : (FDIndex)getOffset(h, pOffset, h->cid.FDBytes);
		new->encrypted = 1;
		if (new->fdIndex >= h->font->fdCount) {
			parseFatal(g, "fdIndex[%d] out-of-range cid#%hu",
			           new->fdIndex, CID);
		}
		h->font->fdInfo[new->fdIndex].seenChar = 1;
		sid[nChars++] = CID;
	}

	if (pSubsetSID) {
		MEM_FREE(g, pSubsetSID);
	}

	/* Add this charset (actually a list of CID's, not SID's) */
	h->font->iCharset = charsetAdd(g, nChars - 1, &sid[1], 1);
	MEM_FREE(g, sid);
}

/* Add chars to font */
static void cidAddChars(parseCtx h) {
#ifndef MIN
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#endif
	int i;
	FDIndex *fdIndex = MEM_NEW(h->g, sizeof(FDIndex) * h->chars.cnt);
	int fd;
	int newFDIndex = 0;

	h->font->fdIndex = MEM_NEW(h->g, sizeof(FDIndex) * h->chars.cnt);

	if (h->g->flags & TC_SMALLMEMORY) {
		/* Small memory read */
		long savecnt = h->buf.cnt;
		h->g->cb.tmpOpen(h->g->cb.ctx);
		for (i = 0; i < h->chars.cnt; i++) {
			Char *chr = &h->chars.array[i];
			long skipcnt = chr->index - h->cid.bytesRead;

			/* Skip over any data we are not interested in */
			psSkipBinary(h->ps, skipcnt);
			h->cid.bytesRead += skipcnt;

			/* Read the charstring into buf */
			h->buf.cnt = savecnt;
			psReadBinary(h->ps, chr->length);
			h->cid.bytesRead += chr->length;
			csAddChar(h->g, chr->length, &h->buf.array[savecnt],
			          chr->id, chr->fdIndex, chr->encrypted);
			h->font->fdIndex[i] = chr->fdIndex; /* Get raw fd number */
			chr->encrypted = 0;
		}
		h->g->cb.tmpRewind(h->g->cb.ctx);
	}
	else {
		for (i = 0; i < h->chars.cnt; i++) {
			Char *chr = &h->chars.array[i];
			csAddChar(h->g, chr->length, &h->buf.array[chr->index],
			          chr->id, chr->fdIndex, chr->encrypted);
			h->font->fdIndex[i] = chr->fdIndex; /* Get raw fd number */
			chr->encrypted = 0;
		}
	}

	/* Renumber fd indexes */
	for (fd = 0; fd < h->font->fdCount; fd++) {
		FDInfo *fdInfo = &h->font->fdInfo[fd];
		if (fdInfo->seenChar) {
			fdInfo->newFDIndex = newFDIndex++;
		}
	}

	/* Replace references with new values */
	for (i = 0; i < h->chars.cnt; i++) {
		fdIndex[i] = h->font->fdInfo[h->font->fdIndex[i]].newFDIndex;
	}

	/* Add to collection of indexes */
	h->font->iFDSelect = fdselectAdd(h->g, (unsigned)h->chars.cnt, fdIndex);

	MEM_FREE(h->g, fdIndex);
	csEndFont(h->g, h->chars.cnt, NULL);

#undef MIN
}
