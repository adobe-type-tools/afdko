/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. *//***********************************************************************/

/* Font conversion database manager */

#include "package.h"
#include DYNARR
#include HOTCONV

#include "lstdio.h"
#include "lstring.h"
#include "lctype.h"
#include "lstdlib.h"

#include "fcdb.h"
#include "file.h"
#include "ctutil.h"

/* Keyword ids */
enum {
	kFamily,				/* "f", "family" */
	kSubfamily,				/* "s", "subfamily" */
	kCompatible,			/* "l", "compatible" */
	kCompatibleFull,		/* "m", "Mac compatible full " */
	kOldCompatible,			/* "c", old " compatible", FDK 2.0 and earlier */
	kBold,					/* "b", "bold" */
	kItalic,				/* "i", "italic" */
	kBoldItalic,			/* "t", "bolditalic" */
	kMacEnc,				/* "macenc" */
	kItalicangle,
	kWeight,
	kWidth,
	kWPFFamily,
	kWPFStyle,
	kUnknown
};
			
/* Font reference */
typedef struct {
	long iFontName;			/* FontName string index */
	long location;			/* b31-24: file id, b23-0: file offset */
	long length;			/* Record length (excluding [FontName]) */
	long line;				/* Line number of record key */
} FontRef;

/* Module context */
struct fcdbCtx_ {
	dnaDCL(FontRef, refs);	/* Font record list */
	dnaDCL(char, FontNames);/* Concatenated null-terminated FontNames */
	dnaDCL(char, record);	/* Record buffer */
	char *MatchName;		/* For use with bsearch lookup */
	fcdbCallbacks cb;		/* Client callbacks */
	unsigned fileid;		/* Current file being parsed */
	long line;				/* Current line being parsed */
	unsigned short syntaxVersion; /* used by fcdb call backs. The cbCtx call backs need another copy of this */
};

/* Match FontName key */
static int CDECL matchFontName(const void *key, const void *value) {
	const struct fcdbCtx_ *h = key;
	return strcmp(h->MatchName, 
				  &h->FontNames.array[((FontRef *)value)->iFontName]);
}

/* sort FontNamse key */
static int CDECL cmpFontName(const void *first, const void *second, void *ctx) {
	const struct fcdbCtx_ *h = ctx;
	return strcmp(&h->FontNames.array[((FontRef *)first)->iFontName], 
				  &h->FontNames.array[((FontRef *)second)->iFontName]);
}

/* Report parsing error */
static void rptError(fcdbCtx h, int errid) {
	h->cb.error(h->cb.ctx, h->fileid, h->line, errid);
}

/* Add new database file */
void fcdbAddFile(fcdbCtx h, unsigned fileid, void *callBackCtx) {
	unsigned short syntaxVersion = 0; /* 0 means not yet set. Inidcates which version is being used. */
	
	/* Next state table */
	static unsigned char next[10][9] = {
		/* 	blank	\n		\r		[		]		*		c		=		EOF	 	 index */
		/* -------- ------- ------- ------- ------- ------- -------- ----- */
		{	0,		0,		2,		3,		1,		1,		9,		1,		0},		/* [0] */
		{	1,		0,		2,		1,		1,		1,		1,		1,		0},		/* [1] */
		{	0,		0,		2,		3,		1,		1,		1,		1,		0},		/* [2] */
		{	4,		0,		2,		1,		1,		5,		5,		5,		0},		/* [3] */
		{	4,		0,		2,		1,		1,		5,		5,		5,		0},		/* [4] */
		{	6,		0,		2,		1,		7,		5,		5,		5,		0},		/* [5] */
		{	6,		0,		2,		1,		7,		1,		1,		1,		0},		/* [6] */
		{	8,		0,		2,		1,		1,		1,		1,		1,		0},		/* [7] */
		{	8,		0,		2,		1,		1,		1,		1,		1,		0},		/* [8] */
		{	1,		0,		2,		1,		1,		1,		1,		1,		0},		/* [9] */
	};

	/* Action table */

#define	I_	(1<<0)	/* Increment line counter */
#define	E_	(1<<1)	/* Report sytax error */
#define A_	(1<<2)	/* Accumulate key */
#define	S_	(1<<3)	/* Save key */
#define O_	(1<<4)	/* Open square bracket; save length & reset key index */
#define C_	(1<<5)	/* Close square bracket; record offset */
#define	Q_	(1<<6)	/* Quit on EOF */
#define	M_	(1<<7)	/* Mark as having seen an old "c=" sequence, so that we will build the name table as FDK's did up through 2.0. */

	static unsigned char action[10][9] = {
		/* 	blank	\n		\r		[		]		*		c		=		EOF	 	 index */
		/* -------- ------- ------- ------- ------- ------- -------- ----- */
		{	0,		I_,		I_,		O_,		0,		0,		0,		0,		Q_},	/* [0] */
		{	0,		I_,		I_,		0,		0,		0,		0,		0,		Q_},	/* [1] */
		{	0,		0,		I_,		O_,		0,		0,		0,		0,		Q_},	/* [2] */
		{	0,		I_|E_,	I_|E_,	E_,		E_,		A_,		A_,		A_,		E_|Q_},	/* [3] */
		{	0,		I_|E_,	I_|E_,	E_,		E_,		A_,		A_,		A_,		E_|Q_},	/* [4] */
		{	0,		I_|E_,	I_|E_,	E_,		C_,		A_,		A_,		A_,		E_|Q_},	/* [5] */
		{	0,		I_|E_,	I_|E_,	E_,		C_,		E_,		E_,		E_,		E_|Q_},	/* [6] */
		{	0,		I_|S_,	I_|S_,	E_,		E_,		E_,		E_,		E_,		E_|Q_},	/* [7] */
		{	0,		I_|S_,	I_|S_,	E_,		E_,		E_,		E_,		E_,		E_|Q_},	/* [8] */
		{	0,		I_,		I_,		0,		0,		0,		0,		M_,		Q_},	/* [9] */
	};

	char key[HOT_MAX_FONT_NAME];
	int iKey = 0;		/* Suppress optimizer warning */
	int state = 0;
	long offset = 0;
	long recoff = 0;	/* Suppress optimizer warning */
	FontRef *ref = NULL;

	/* Initialize match name to key for lookups */
	h->MatchName = key;

	h->fileid = fileid;
	h->line = 1;

	for (;;) {
		size_t count;
		char *pbeg = h->cb.refill(h->cb.ctx, h->fileid, &count);
		char *pend = pbeg + count;
		char *p = pbeg;

		while (p != pend) {
			int class;	/* Character class */
			int actn;	/* Action flags */
			int c = *p++;

			switch (c) {
				case '\t': case '\v': case '\f': case ' ':	/* Blank */
					class = 0;
					break;
				case '\n':
					class = 1;
					break;
				case '\r':
					class = 2;
					break;
				case '[':
					class = 3;
					break;
				case ']':
					class = 4;
					break;
				case 'c':
					class = 6;
					break;
				case '=':
					class = 7;
					break;
				default:
					class = 5;
					break;
				case '\0':	/* EOF */
					class = 8;
					break;
			}

			actn = action[state][class];
			state = next[state][class];

			/* Perform actions */
			if (actn == 0) {
				continue;
			}
			if (actn & E_) {
				/* Syntax error */
				rptError(h, fcdbSyntaxErr);
			}
			if (actn & S_) {

				/* Terminate section key */
				key[iKey] = '\0';

				ref = (FontRef *)bsearch(h, h->refs.array, h->refs.cnt, 
							 sizeof(FontRef), matchFontName);
				if (ref) {
					/* Already in list */
					rptError(h, fcdbDuplicateErr);
					state = 1;
				}
				else {
					size_t index;
					/* Not found in list */
					index = h->refs.cnt;
					ref = &dnaGROW(h->refs,index)[index];
					
					/* Make hole */
					memmove(ref + 1, ref,  
							sizeof(FontRef) * (h->refs.cnt++ - index));

					/* Fill record */
					ref->iFontName = h->FontNames.cnt;
					ref->location = h->fileid<<24 | recoff;
					ref->line = h->line;

					strcpy(dnaEXTEND(h->FontNames, iKey + 1), key);
				}
			}
			if (actn & A_) {
				if (iKey == HOT_MAX_FONT_NAME - 1) {
					/* Key length exceeded */
					rptError(h, fcdbKeyLengthErr);
					state = 1;
				}
				else {
					key[iKey++] = c;
				}
			}
			if (actn & O_) {
				iKey = 0;
				if (ref != NULL) {
					/* Set record length */
					ref->length = p - pbeg + offset - 1 - recoff;
				}
			}
			if (actn & C_) {
				recoff = p - pbeg + offset;
			}
			if (actn & Q_) {
				if (ref != NULL) {
					/* Set record length */
					ref->length = p - pbeg + offset - 1 - recoff;
				}
     			ctuQSort(h->refs.array, h->refs.cnt, sizeof(FontRef), cmpFontName, h);
				if (syntaxVersion == 0) {
					syntaxVersion = 2;
				}
				h->syntaxVersion = syntaxVersion;
				h->cb.setMenuVersion(callBackCtx, fileid, syntaxVersion);
				return;
			}
			if (actn & I_) {
				h->line++;
			}
			
			if  (actn & M_) {
				if (syntaxVersion == 0) {
					syntaxVersion = 1;
				}
			}
		}

		/* Update buffer offset */
		offset += count;
	}
#undef I_
#undef E_
#undef A_
#undef S_
#undef O_
#undef C_
#undef Q_
#undef M_

     ctuQSort(h->refs.array, h->refs.cnt, sizeof(FontRef), cmpFontName, h);
	if (syntaxVersion == 0) {
		syntaxVersion = 2;
	}
	h->syntaxVersion = syntaxVersion;
	h->cb.setMenuVersion(callBackCtx, fileid, syntaxVersion);

}

/* Get key identifer for keyword */
static int getKeywordId(char *keyword) {
	static struct {
		char *name;
		short id;
	} keywords[] = {
			{"f", 			kFamily},
			{"s",			kSubfamily},
			{"l",			kCompatible},
			{"m",			kCompatibleFull},
			{"c",			kOldCompatible},
			{"b",			kBold},
			{"i",			kItalic},
			{"t",			kBoldItalic},
			{"wpff",		kWPFFamily},
			{"wpfs",		kWPFStyle},
			{"macenc",		kMacEnc},
			{"family", 		kFamily},
			{"subfamily",	kSubfamily},
			{"style family",	kCompatible},
			{"mac full",	kCompatibleFull},
			{"compatible",	kOldCompatible},
			{"bold",		kBold},
			{"italic",		kItalic},
			{"bolditalic",	kBoldItalic},
			{"italicangle",	kItalicangle},
			{"width",		kWidth},
			{"weight",		kWeight},
			
		};
	unsigned int i;
	for (i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++) {
		if (strcmp(keyword, keywords[i].name) == 0) {
			return keywords[i].id;
		}
	}
	return kUnknown;
}

/* Parse comma-terminated number from head of string. If no number or number
   not comma-terminated return -1. If number converted but out of range return
   -2. If conversion suceeded a pointer following the terminating comma is
   return by the "q" argument. */
static long parseNumber(fcdbCtx h, char **p, int isNameId) {
	char *q;
	long value = strtol(*p, &q, 0);
	if (*p != q) {
		/* Number converted */
		while (isspace(*q)) {
			q++;
		}

		if (*q == ',') {
			if (isNameId) {
				if (value < 0 || value > 65535) {
					/* Name id range error */
					rptError(h, fcdbIdRangeErr);
					return -2;
				}
			}
			else if (value < 0 || value > 255) {
				/* Name id range error */
				rptError(h, fcdbCodeRangeErr);
				return -2;
			}

			/* Conversion suceeded */
			*p = q + 1;
			return value;
		}
		else {
			/* Unterminated number; no conversion */
			return -1;
		}
	}

	/* No conversion */
	return -1;
}


/* Parse name key value */
static int parseName(fcdbCtx h, unsigned keywordId, char *p) {
	long id;
	int win;
	unsigned short platformId = HOT_NAME_MS_PLATFORM;
	unsigned short platspecId = HOT_NAME_MS_UGL;
	unsigned short languageId = HOT_NAME_MS_ENGLISH;
	unsigned short nameId = 0;	/* Suppress optimizer warning */

	if (h->cb.addname == NULL) {
	  return 0;
	}
	
	/* Try to parse platform id */
	id = parseNumber(h, &p, 1);
	if (id == -2) {
		return 1;
	}
	else if (id == -1) {
		;	/* Use MS defaults */
	}
	else {
		platformId = (unsigned short)id;
		if (platformId == HOT_NAME_MS_PLATFORM) {
			;	/* Use MS defaults */
		}
		else if (platformId == HOT_NAME_MAC_PLATFORM) {
			/* Set Macintosh defaults */
			platformId = HOT_NAME_MAC_PLATFORM;
			platspecId = HOT_NAME_MAC_ROMAN;
			languageId = HOT_NAME_MAC_ENGLISH;
		}
		else {
			rptError(h, fcdbSyntaxErr);
			return 1;
		}

		/* Try to parse platform-specific id */
		id = parseNumber(h, &p, 1);
		if (id == -2) {
			return 1;
		}
		else if (id == -1) {
			;	/* Use assigned defaults */
		}
		else {
			platspecId = (unsigned short)id;

			/* Try to parse language id */
			id = parseNumber(h, &p, 1);
			if (id == -2) {
				return 1;
			}
			else if (id == -1) {
				rptError(h, fcdbSyntaxErr);
				return 1;
			}
			else {
				languageId = (unsigned short)id;
			}
		}
	}
	if (*p == '\0') {
		rptError(h, fcdbEmptyNameErr);
		return 1;
	}

	/* Prepare name string */
	win = platformId == HOT_NAME_MS_PLATFORM;

	/* Convert keyword id to name id */
	switch (keywordId) {
		case kFamily:
			nameId = HOT_NAME_PREF_FAMILY;
			break;
		case kSubfamily:
			nameId = HOT_NAME_PREF_SUBFAMILY;
			break;
		case kCompatible:
			nameId = HOT_NAME_FAMILY;
			if (h->syntaxVersion != 2) {
				rptError(h, fcdbMixedSyntax);
			}
			break;
		case kCompatibleFull:
			if (h->syntaxVersion != 2) {
				rptError(h, fcdbMixedSyntax);
			}
			if (platformId != HOT_NAME_MAC_PLATFORM) {
				rptError(h, fcdbWinCompatibleFullError);
				return 1;
			}
			nameId = HOT_NAME_COMP_FULL;
			break;
		case kOldCompatible:
			if (h->syntaxVersion != 1) {
				rptError(h, fcdbMixedSyntax);
			}
			nameId = win? HOT_NAME_FAMILY: HOT_NAME_COMP_FULL;
			break;
		case kWPFFamily:
			nameId = HOT_NAME_WPF_FAMILY;
			break;
		case kWPFStyle:
			nameId = HOT_NAME_WPF_STYLE;
			break;
	}

	/* Add name string */
	if (h->cb.addname(h->cb.ctx, platformId, platspecId, languageId, nameId, p)) {
		rptError(h, fcdbSyntaxErr);
		return 1;
	}

	return 0;
}

/* Parse style-linked FontName */
static int parseLink(fcdbCtx h, unsigned keywordId, char *p) {
	if (h->cb.addlink == NULL) {
		return 0;
	}
	
	h->cb.addlink(h->cb.ctx, keywordId - kBold + 1, p);
	return 0;
}

/* Parse encoding key value */
static int parseEnc(fcdbCtx h, char *p) {
	char *q;
	long code = parseNumber(h, &p, 0);
	
	if (code == -2) {
		return 1;
	}
	else if (code == -1) {
		rptError(h, fcdbSyntaxErr);
		return 1;
	}

	/* Skip leading blanks */
	while (isspace(*p)) {
		p++;
	}

	/* Skip glyph name */
	for (q = p; *p != '\0'; p++) {
		if (isspace(*p)) {
			rptError(h, fcdbSyntaxErr);
			return 1;
		}
	}

	h->cb.addenc(h->cb.ctx, code, q);
	return 0;
}

/* Parse font record */
static int parseRecord(fcdbCtx h) {
	/* Next state table */
	static unsigned char next[8][7] = {
		/* 	blank	\n		\r		;		=		*		EOR	 	 index */
		/* -------- ------- ------- ------- ------- ------- -------- ----- */
		{	0,		0,		2,		1,		1,		3,		0},		/* [0] */
		{	1,		0,		2,		1,		1,		1,		0},		/* [1] */
		{	0,		0,		2,		1,		1,		3,		0},		/* [2] */
		{	4,		0,		2,		1,		5,		3,		0},		/* [3] */
		{	4,		0,		2,		1,		5,		1,		0},		/* [4] */
		{	6,		0,		2,		7,		7,		7,		0},		/* [5] */
		{	6,		0,		2,		7,		7,		7,		0},		/* [6] */
		{	7,		0,		2,		7,		7,		7,		0},		/* [7] */
	};

	/* Action table */

#define I_	(1<<0)	/* Increment line counter */
#define	E_	(1<<1)	/* Report sytax error */
#define P_	(1<<2)	/* Save string pointer */
#define	K_	(1<<3)	/* Save keyword */
#define V_	(1<<4)	/* Save value */
#define	Q_	(1<<5)	/* Quit on end-of-record */

	static unsigned char action[8][7] = {
		/* 	blank	\n		\r		;		=		*		EOR	 	 index */
		/* -------- ------- ------- ------- ------- ------- -------- ----- */
		{	0,		I_,		I_,		0,		E_,		P_,		Q_},	/* [0] */
		{	0,		I_,		I_,		0,		0,		0,		Q_},	/* [1] */
		{	0,		0,		I_,		0,		E_,		P_,		Q_},	/* [2] */
		{	K_,		I_|E_,	I_|E_,	E_,		K_,		0,		E_|Q_},	/* [3] */
		{	0,		I_|E_,	I_|E_,	E_,		0,		E_,		E_|Q_},	/* [4] */
		{	0,		I_|E_,	I_|E_,	P_,		P_,		P_,		E_|Q_},	/* [5] */
		{	0,		I_|E_,	I_|E_,	P_,		P_,		P_,		E_|Q_},	/* [6] */
		{	0,		I_|V_,	I_|V_,	0,		0,		0,		V_|Q_},	/* [7] */
	};

	char *pbeg = NULL;		/* Suppress optimizer warning */
	int keywordId = 0;		/* Suppress optimizer warning */
	int state = 0;
	char *p = h->record.array;

	for (;;) {
		int class;				/* Character class */
		int actn;				/* Action flags */
		int c = *p++;

		switch (c) {
			case '\t': case '\v': case '\f': case ' ': /* Blank */
				class = 0;
				break;
			case '\n': 
				class = 1;
				break;
			case '\r':
				class = 2;
				break;
			case ';':
				class = 3;
				break;
			case '=':
				class = 4;
				break;
			case '#':
				class = 6;
				break;
			default:
				class = 5;
				break;
			case '\0':				/* End of record */
				class = 6;
				break;
		}

		actn = action[state][class];
		state = next[state][class];

		/* Perform actions */
		if (actn == 0) {
			continue;
		}
		if (actn & E_) {
			rptError(h, fcdbSyntaxErr);
			return 1;
		}
		if (actn & P_) {
			pbeg = p - 1;
		}
		if (actn & K_) {
			*(p - 1) = '\0';	/* Terminate keyword */
			keywordId = getKeywordId(pbeg);
			if (keywordId == kUnknown) {
				rptError(h, fcdbSyntaxErr);
				return 1;
			}
		}
		if (actn & V_) {
			char *q;

			/* Trim trailing blanks */
			for (q = p - 1; isspace(*q); q--) {}
			*(q + 1) = '\0';

			switch (keywordId) {
				case kFamily:
				case kSubfamily:
				case kCompatible:
				case kCompatibleFull:
				case kOldCompatible:
				case kWPFFamily:
				case kWPFStyle:
					if (parseName(h, keywordId, pbeg)) {
						return 1;
					}
					break;
				case kBold:
				case kItalic:
				case kBoldItalic:
					if (parseLink(h, keywordId, pbeg)) {
						return 1;
					}
					break;
				case kMacEnc:
					if (parseEnc(h, pbeg)) {
						return 1;
					}
					break;
				}
			}
		if (actn & Q_) {
			return 0;
		}
		if (actn & I_) {
			h->line++;
		}
	}
#undef I_
#undef E_
#undef P_
#undef K_
#undef V_
#undef Q_
}


static void buildDefaultRec(fcdbCtx h, char *FontName) {
	unsigned short platformId = HOT_NAME_MS_PLATFORM;
	unsigned short platspecId = HOT_NAME_MS_UGL;
	unsigned short languageId = HOT_NAME_MS_ENGLISH;
	char familyName[64];
	char *subFamily = NULL; /* This is set to point into the FontName*/
	int sepIndex = strcspn(FontName, "-");
	unsigned short nameId = 0;	/* Suppress optimizer warning */

	if ((sepIndex > -1) && (FontName[sepIndex] == '-')) { /* On Windows, strcspn returns the string length when the char is not found! */
		
		subFamily = &FontName[sepIndex+1];
		strncpy(familyName, FontName, sepIndex);
		familyName[sepIndex] = 0;
	}
	else {
		strcpy(familyName, FontName);
	}
	
	nameId = HOT_NAME_FAMILY; /* family */
	h->cb.addname(h->cb.ctx, platformId, platspecId, languageId, nameId, familyName);

	nameId = HOT_NAME_SUBFAMILY; /* style */
	if (subFamily == NULL) {
		h->cb.addname(h->cb.ctx, platformId, platspecId, languageId, nameId, "Regular");
	}
	else {
		h->cb.addname(h->cb.ctx, platformId, platspecId, languageId, nameId, subFamily);
	}

	platformId = HOT_NAME_MAC_PLATFORM;
	platspecId = HOT_NAME_MAC_ROMAN;
	languageId = HOT_NAME_MAC_ENGLISH;
	nameId = HOT_NAME_PREF_FAMILY; /* family */
	h->cb.addname(h->cb.ctx, platformId, platspecId, languageId, nameId, familyName);
	nameId = HOT_NAME_PREF_SUBFAMILY; /* style */
	if (subFamily == NULL) {
		h->cb.addname(h->cb.ctx, platformId, platspecId, languageId, nameId, "Regular");
	}
	else {
		h->cb.addname(h->cb.ctx, platformId, platspecId, languageId, nameId, subFamily);
	}
	
}


/* Get font record from database. Return 0 if record found and parsed OK else
   return 1. */ 
int fcdbGetRec(fcdbCtx h, char *FontName) {
	FontRef *ref;

	if (h->refs.cnt == 0) {
		buildDefaultRec(h, FontName);
		return 1;
	}

	h->MatchName = FontName;
	ref = (FontRef *)bsearch(h, h->refs.array, h->refs.cnt, sizeof(FontRef), matchFontName);
	if (ref == NULL) {
		return 1;
	}

	/* Get record */
	h->fileid = ref->location>>24 & 0xff;
	h->line = ref->line;
	h->cb.getbuf(h->cb.ctx, h->fileid, ref->location & 0xffffff, 
				 ref->length, dnaGROW(h->record, ref->length));
	h->record.array[ref->length] = '\0';

	return parseRecord(h);
}
	
/* Create new context */
fcdbCtx fcdbNew(fcdbCallbacks *cb, void* dna_ctx) {
	fcdbCtx h = cbMemNew(cb->ctx, sizeof(struct fcdbCtx_));
	dnaCtx local_dna_ctx = (dnaCtx)dna_ctx;
	
	h->cb = *cb;	/* Copy callbacks */

	dnaINIT(local_dna_ctx, h->refs, 2600, 500);
	dnaINIT(local_dna_ctx, h->FontNames, 50000, 20000);
	dnaINIT(local_dna_ctx, h->record, 1000, 5000);
	h->syntaxVersion = 0;
	return h;
}

/* Free callback context */
void fcdbFree(fcdbCtx h) {
	void *cxt = h->cb.ctx;
	dnaFREE(h->refs);
	dnaFREE(h->FontNames);
	dnaFREE(h->record);
	cbMemFree(cxt, h);
}

#if DEBUG

static void dbbyte(unsigned byte) {
	if (byte < 0x80) {
		printf("%c", byte);
	}
	else {
		static char hex[] = "0123456789abcdef";
		printf(":%c%c", hex[byte>>4 & 0xf], hex[byte & 0xf]);
	}
}

static void dbbytes(int msb, int lsb) {
	static char hex[] = "0123456789abcdef";
	printf("!%c%c%c%c", 
		   hex[msb>>4 & 0xf], hex[msb & 0xf],
		   hex[lsb>>4 & 0xf], hex[lsb & 0xf]);
}

static void dbwinname(char *str) {
	/* Convert UTF-8 to 16-bit */
	while (*str != '\0') {
		unsigned s0 = *str++;
		if (s0 < 0xc0) {
			/* 1-byte */
			if (s0 == 0x21) {
				printf("!0021"); /* Handle exclam */
			}
			else {
				printf("%c", (char)s0);
			}
		}
		else {
			unsigned s1 = *str++;
			if (s0 < 0xe0) {
				/* 2-byte */
				dbbytes(s0>>2 & 0x07,
						s0<<6 | (s1 & 0x3f));
			}
			else {
				/* 3-byte */
				unsigned s2 = *str++;
				dbbytes(s0<<4 | (s1>>2 & 0x0f),
						s1<<6 | (s2 & 0x3f));
			}
		}
	}
}

static void dbmacname(char *str) {
	while (*str != '\0') {
		unsigned s0 = *str++;
		if (s0 == 0x3a) {
			printf(":3a");		/* Handle colon */
		}
		else {
			dbbyte(s0);
		}
	}
}

static int dbAddName(void *ctx, 
					 unsigned short platformId, unsigned short platspecId,
					 unsigned short languageId, unsigned short nameId, 
					 char *str) {
	int win = platformId == HOT_NAME_MS_PLATFORM;

	/* Print keyword */
	switch (nameId) {
		case HOT_NAME_FAMILY:
			printf("\t%s=", win? "c": "f");
			break;
		case HOT_NAME_SUBFAMILY:
		case HOT_NAME_PREF_SUBFAMILY:
			printf("\ts=");
			break;
		case HOT_NAME_PREF_FAMILY:
			printf("\tf=");
			break;
		case HOT_NAME_COMP_FULL:
			printf("\tc=");
			break;
	}

	if (win) {
		if (platspecId != HOT_NAME_MS_UGL || 
			languageId != HOT_NAME_MS_ENGLISH) {
			printf("%hu,%hu,0x%04hx,", platformId, platspecId, languageId);
		}
		dbwinname(str);
	}
	else {
		if (platspecId != HOT_NAME_MAC_ROMAN ||
			languageId != HOT_NAME_MAC_ENGLISH) {
			printf("%hu,%hu,%hu,", platformId, platspecId, languageId);
		}
		else {
			printf("%hu,", platformId);
		}
		dbmacname(str);
	}
	printf("\n");
	return 0;
}

void dbAddEnc(void *ctx, int code, char *gname) {
	printf("\tmacenc=%d,%s\n", code, gname);
}

static void dbrec(fcdbCtx h, char *FontName) {
	fcdbCallbacks tmp;

	/* Swap callbacks with debug versions */
	tmp = h->cb;
	h->cb.addname = dbAddName;
	h->cb.addenc = dbAddEnc;

	printf("[%s]\n", FontName);
	if (fcdbGetRec(h, FontName)) {
		printf("fcdbGetRec failed for font [%s]\n", FontName);
	}
	else {
		printf("\n");
	}
	/* Restore original callbacks */
	h->cb = tmp;
}

static void dbrecs(fcdbCtx h) {
	int i;
	fcdbCallbacks tmp;

	/* Swap callbacks with debug versions */
	tmp = h->cb;
	h->cb.addname = dbAddName;
	h->cb.addenc = dbAddEnc;

	for (i = 0; i < h->refs.cnt; i++) {
		char *FontName = &h->FontNames.array[h->refs.array[i].iFontName];
		printf("[%s]\n", FontName);
		if (fcdbGetRec(h, FontName)) {
			printf("fcdbGetRec failed for font [%s]\n", FontName);
		}
		else {
			printf("\n");
		}
	}

	/* Restore original callbacks */
	h->cb = tmp;
}

static void dbrefs(fcdbCtx h) {
	int i;

	printf("--- refs[index]={location,length,FontName}\n");
	for (i = 0; i < h->refs.cnt; i++) {
		FontRef *ref = &h->refs.array[i];
		printf("[%d]={%08lx,%3ld,%s}\n", i, ref->location, ref->length,
			   &h->FontNames.array[ref->iFontName]);
	}
}

/* This function just serves to suppress annoying "defined but not used"
   compiler messages when debugging */
static void CDECL dbuse(int arg, ...) {
	dbuse(0, dbrec, dbrecs, dbrefs);
}
#endif
