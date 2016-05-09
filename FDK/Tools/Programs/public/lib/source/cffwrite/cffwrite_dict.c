/* Copyright 2014 Adobe Systems Incorporated (http://www.adobe.com/). All Rights Reserved.
   This software is licensed as OpenSource, under the Apache License, Version 2.0. This license is available at: http://opensource.org/licenses/Apache-2.0. */

/*
 * Dictionary support.
 */

#include "cffwrite_dict.h"
#include "cffwrite_sindex.h"
#include "dictops.h"
#include "ctutil.h"

#include <math.h>
#if PLAT_SUN4
#include "sun4/fixstring.h"
#else /* PLAT_SUN4 */
#include <string.h>
#endif /* PLAT_SUN4 */

#include <stdio.h>

/* ----------------------------- Module Context ---------------------------- */

struct dictCtx_ {
	dnaDCL(char, tmp);      /* Temporary string storage */
	cfwCtx g;               /* Package context */
};

/* Initialize module. */
void cfwDictNew(cfwCtx g) {
	dictCtx h = cfwMemNew(g, sizeof(struct dictCtx_));

	/* Link contexts */
	h->g = g;
	g->ctx.dict = h;

	dnaINIT(g->ctx.dnaSafe, h->tmp, 30, 10);
}

/* Prepare module for reuse. */
void cfwDictReuse(cfwCtx g) {
	dictCtx h = g->ctx.dict;
	h->tmp.cnt = 0;
}

/* Free resources */
void cfwDictFree(cfwCtx g) {
	dictCtx h = g->ctx.dict;

	if (h == NULL) {
		return;
	}

	dnaFREE(h->tmp);

	cfwMemFree(g, h);
	g->ctx.dict = NULL;
}

/* Add string to string index. */
static void addString(cfwCtx g, abfString *str) {
	str->impl = cfwSindexAddString(g, str->ptr);
}

/* Save FSType in PostScript string. */
static void saveFSType(cfwCtx g, abfTopDict *dst) {
	dictCtx h = g->ctx.dict;
	char buf[50];

	if (dst->PostScript.ptr != ABF_UNSET_PTR) {
		if (strstr(dst->PostScript.ptr, "/FSType") != NULL) {
			return; /* PostScript string already has /FSType */
		}
		/* Copy string to tmp buf */
		/* 64-bit warning fixed by cast here */
		strcpy(dnaEXTEND(h->tmp, (long)strlen(dst->PostScript.ptr)),
		       dst->PostScript.ptr);
	}

	/* Append /FSType to tmp buf */
	sprintf(buf, "/FSType %ld def", dst->FSType);
	/* 64-bit warning fixed by cast here */
	strcpy(dnaEXTEND(h->tmp, (long)strlen(buf)), buf);

	/* Set PostScript string to tmp buf */
	dst->PostScript.ptr = h->tmp.array;
}

/* Save OrigFontType in PostScript string. */
static void saveOrigFontType(cfwCtx g, abfTopDict *dst) {
	dictCtx h = g->ctx.dict;
	char buf[50];
	char *OrigFontType = NULL;  /* Suppress optimizer warning */

	switch (dst->OrigFontType) {
		case abfOrigFontTypeType1:
			OrigFontType = "Type1";
			break;

		case abfOrigFontTypeCID:
			OrigFontType = "CID";
			break;

		case abfOrigFontTypeTrueType:
			OrigFontType = "TrueType";
			break;

		case abfOrigFontTypeOCF:
			OrigFontType = "OCF";
			break;

		default:
			switch (dst->sup.srcFontType) {
				case abfSrcFontTypeType1Name:
				case abfSrcFontTypeCFFName:
					if (!(dst->sup.flags & ABF_CID_FONT)) {
						return; /* Nothing to save */
					}
					OrigFontType = "Type1"; /* Name-keyed to cid-keyed conversion */
					break;

				case abfSrcFontTypeType1CID:
				case abfSrcFontTypeCFFCID:
					if (dst->sup.flags & ABF_CID_FONT) {
						return; /* Nothing to save */
					}
					OrigFontType = "CID"; /* CID-keyed to name-keyed conversion */
					break;

				case abfSrcFontTypeSVGName:
					OrigFontType = "SVG";
					break;

				case abfSrcFontTypeUFOName:
					OrigFontType = "UFO";
					break;

				case abfSrcFontTypeTrueType:
					/* TrueType to name-keyed or cid-keyed conversion */
					OrigFontType = "TrueType";
					break;

				default:
					return;
			}
			break;
	}

	if (dst->PostScript.ptr != ABF_UNSET_PTR) {
		if (strstr(dst->PostScript.ptr, "/OrigFontType") != NULL) {
			return; /* PostScript string already has /OrigFontType */
		}
		/* Copy string to tmp buf */
		/* 64-bit warning fixed by cast here */
		strcpy(dnaEXTEND(h->tmp, (long)strlen(dst->PostScript.ptr)),
		       dst->PostScript.ptr);
	}

	/* Append /OrigFontType to tmp buf */
	sprintf(buf, "/OrigFontType /%s def", OrigFontType);
	/* 64-bit warning fixed by cast here */
	strcpy(dnaEXTEND(h->tmp, (long)strlen(buf)), buf);

	/* Set PostScript string to tmp buf */
	dst->PostScript.ptr = h->tmp.array;
}

/* Copy top dict data from client specification. */
void cfwDictCopyTop(cfwCtx g, abfTopDict *dst, abfTopDict *src) {
	/* Copy all fields */
	*dst = *src;

	if (g->flags & CFW_EMBED_OPT) {
		/* When embedding save FSType or OrigFontType */
		if (dst->FSType != ABF_UNSET_INT) {
			saveFSType(g, dst);
		}
		else {
			saveOrigFontType(g, dst);
		}
	}

	/* Add strings to index */
	addString(g, &dst->version);
	addString(g, &dst->Notice);
	addString(g, &dst->Copyright);
	addString(g, &dst->FullName);
	addString(g, &dst->FamilyName);
	addString(g, &dst->Weight);
	addString(g, &dst->PostScript);
	addString(g, &dst->BaseFontName);
	addString(g, &dst->SynBaseFontName);
	addString(g, &dst->cid.CIDFontName);
	addString(g, &dst->cid.Registry);
	addString(g, &dst->cid.Ordering);
}

/* Copy font dict data from client specification. */
void cfwDictCopyFont(cfwCtx g, abfFontDict *dst, abfFontDict *src) {
	/* Copy all fields */
	*dst = *src;

	/* Add strings to index */
	addString(g, &dst->FontName);
}

/* Copy Private dict data from client specification. */
void cfwDictCopyPrivate(cfwCtx g, abfPrivateDict *dst, abfPrivateDict *src) {
	/* Copy all fields */
	*dst = *src;
}

/* Save integer number arg in DICT. */
void cfwDictSaveInt(DICT *dict, long i) {
	char *arg = dnaEXTEND(*dict, 5);
	dict->cnt -= 5 - cfwEncInt(i, (unsigned char *)arg);
}

/* Save real number arg in DICT. If not fractional save as integer. */
void cfwDictSaveReal(DICT *dict, float r) {
	char buf[50];
	int value;          /* Current nibble value */
	int last = 0;       /* Last nibble value */
	int odd = 0;        /* Flags odd nibble */
	long i = (long)r;

	if (i == r) {
		/* Value was integer */
		cfwDictSaveInt(dict, i);
		return;
	}

	ctuDtostr(buf, r, 0, 8); /* 8 places is as good as it gets when converting ASCII real numbers->float-> ASCII real numbers, as happens to all the  PrivateDict values.*/

	*dnaNEXT(*dict) = cff_BCD;
	for (i = buf[0] == '0';; i++) {
		switch (buf[i]) {
			case '\0':
				/* Terminate number */
				*dnaNEXT(*dict) = odd ? last << 4 | 0xf : 0xff;
				return;

			case '+':
				continue;

			case '-':
				value = 0xe;
				break;

			case '.':
				value = 0xa;
				break;

			case 'E':
			case 'e':
				value = (buf[++i] == '-') ? 0xc : 0xb;
				break;

			default:
				value = buf[i] - '0';
				break;
		}

		if (odd) {
			*dnaNEXT(*dict) = last << 4 | value;
		}
		else {
			last = value;
		}
		odd = !odd;
	}
}

/* Save dict operator. */
void cfwDictSaveOp(DICT *dict, int op) {
	if (op & 0xff00) {
		*dnaNEXT(*dict) = cff_escape;
	}
	*dnaNEXT(*dict) = (unsigned char)op;
}

/* Save integer operator. */
void cfwDictSaveIntOp(DICT *dict, long i, int op) {
	cfwDictSaveInt(dict, i);
	cfwDictSaveOp(dict, op);
}

/* Save real number operator in DICT. */
void cfwDictSaveRealOp(DICT *dict, float r, int op) {
	cfwDictSaveReal(dict, r);
	cfwDictSaveOp(dict, op);
}

/* Save simple int array operator in DICT. */
static void saveIntArrayOp(DICT *dict, int cnt, long *array, int op) {
	int i;
	for (i = 0; i < cnt; i++) {
		cfwDictSaveInt(dict, array[i]);
	}
	cfwDictSaveOp(dict, op);
}

/* Save simple real array operator in DICT. */
static void saveRealArrayOp(DICT *dict, int cnt, float *array, int op) {
	int i;
	for (i = 0; i < cnt; i++) {
		cfwDictSaveReal(dict, array[i]);
	}
	cfwDictSaveOp(dict, op);
}

/* Save real delta-encoded array operator in DICT. */
static void saveRealDeltaOp(DICT *dict, int cnt, float *array, int op) {
	int i;
	/* Compute deltas */
	for (i = cnt - 1; i > 0; i--) {
		array[i] -= array[i - 1];
	}
	saveRealArrayOp(dict, cnt, array, op);
}

/* Save integer delta-encoded array operator in DICT. */
static void saveIntDeltaOp(DICT *dict, int cnt, long *array, int op) {
	int i;
	/* Compute deltas */
	for (i = cnt - 1; i > 0; i--) {
		array[i] -= array[i - 1];
	}
	saveIntArrayOp(dict, cnt, array, op);
}

/* Save string in DICT. */
static void saveStringOp(cfwCtx g, DICT *dict, SRI sri, int op) {
	cfwDictSaveInt(dict, cfwSindexAssignSID(g, sri));
	cfwDictSaveOp(dict, op);
}

/* Save normal FontMatrix. */
static void saveFontMatrix(DICT *dict, abfFontMatrix *FontMatrix) {
	if (!abfIsDefaultFontMatrix(FontMatrix)) {
		saveRealArrayOp(dict, 6, FontMatrix->array, cff_FontMatrix);
	}
}

/* Fill CFF top dict. */
void cfwDictFillTop(cfwCtx g, DICT *dst,
                    abfTopDict *top, abfFontDict *font0, long iSyntheticBase) {
	int embed = g->flags & (CFW_EMBED_OPT | CFW_ROM_OPT);

	dst->cnt = 0;
	if (iSyntheticBase != -1) {
		/* Make synthetic font dictionary */
		cfwDictSaveIntOp(dst, iSyntheticBase, cff_SyntheticBase);

		/* FullName */
		if (top->FullName.impl != SRI_UNDEF) {
			saveStringOp(g, dst, (SRI)top->FullName.impl, cff_FullName);
		}

		/* ItalicAngle */
		if (top->ItalicAngle != cff_DFLT_ItalicAngle) {
			cfwDictSaveRealOp(dst, top->ItalicAngle, cff_ItalicAngle);
		}

		/* FontMatrix */
		saveFontMatrix(dst, &font0->FontMatrix);

		return;
	}

	/* ROS */
	if (top->sup.flags & ABF_CID_FONT) {
		cfwDictSaveInt(dst,
		               cfwSindexAssignSID(g, (SRI)top->cid.Registry.impl));
		cfwDictSaveInt(dst,
		               cfwSindexAssignSID(g, (SRI)top->cid.Ordering.impl));
		cfwDictSaveInt(dst, top->cid.Supplement);
		cfwDictSaveOp(dst, cff_ROS);
	}

	/* version */
	if (top->version.impl != SRI_UNDEF &&
	    !embed) {
		saveStringOp(g, dst, (SRI)top->version.impl, cff_version);
	}

	/* Notice */
	if (top->Notice.impl != SRI_UNDEF) {
		saveStringOp(g, dst, (SRI)top->Notice.impl, cff_Notice);
	}

	if (top->Copyright.impl != SRI_UNDEF &&
	    (!embed || top->Notice.impl == SRI_UNDEF)) {
		saveStringOp(g, dst, (SRI)top->Copyright.impl, cff_Copyright);
	}

	/* FullName */
	if (top->FullName.impl != SRI_UNDEF &&
	    !embed) {
		saveStringOp(g, dst, (SRI)top->FullName.impl, cff_FullName);
	}

	/* FamilyName */
	if (top->FamilyName.impl != SRI_UNDEF &&
	    !embed) {
		saveStringOp(g, dst, (SRI)top->FamilyName.impl, cff_FamilyName);
	}

	/* Weight */
	if (top->Weight.impl != SRI_UNDEF) {
		saveStringOp(g, dst, (SRI)top->Weight.impl, cff_Weight);
	}

	/* isFixedPitch */
	if (top->isFixedPitch != cff_DFLT_isFixedPitch &&
	    !embed) {
		cfwDictSaveIntOp(dst, top->isFixedPitch, cff_isFixedPitch);
	}

	/* ItalicAngle */
	if (top->ItalicAngle != cff_DFLT_ItalicAngle) {
		cfwDictSaveRealOp(dst, top->ItalicAngle, cff_ItalicAngle);
	}

	/* UnderlinePosition */
	if (top->UnderlinePosition != cff_DFLT_UnderlinePosition &&
	    !embed) {
		cfwDictSaveRealOp(dst, top->UnderlinePosition, cff_UnderlinePosition);
	}

	/* UnderlineThickness */
	if (top->UnderlineThickness != cff_DFLT_UnderlineThickness &&
	    !embed) {
		cfwDictSaveRealOp(dst, top->UnderlineThickness, cff_UnderlineThickness);
	}

	/* PostScript */
	if (top->PostScript.impl != SRI_UNDEF) {
		saveStringOp(g, dst, (SRI)top->PostScript.impl, cff_PostScript);
	}

	/* BaseFontName */
	if (top->BaseFontName.impl != SRI_UNDEF) {
		saveStringOp(g, dst, (SRI)top->BaseFontName.impl, cff_BaseFontName);
	}

	/* BaseFontBlend */
	if (top->BaseFontBlend.cnt != ABF_EMPTY_ARRAY) {
		saveIntDeltaOp(dst, top->BaseFontBlend.cnt, top->BaseFontBlend.array,
		               cff_BaseFontBlend);
	}

	/* FontBBox */
	if (top->FontBBox[0] != 0 ||
	    top->FontBBox[1] != 0 ||
	    top->FontBBox[2] != 0 ||
	    top->FontBBox[3] != 0) {
        top->FontBBox[0] = roundf(top->FontBBox[0]);
        top->FontBBox[1] = roundf(top->FontBBox[1]);
        top->FontBBox[2] = roundf(top->FontBBox[2]);
        top->FontBBox[3] = roundf(top->FontBBox[3]);
		saveRealArrayOp(dst, 4, top->FontBBox, cff_FontBBox);
	}

	if (top->sup.flags & ABF_CID_FONT) {
		/* FontMatrix; note default values are different from font dict */
		if (top->cid.FontMatrix.cnt != ABF_EMPTY_ARRAY &&
		    (top->cid.FontMatrix.array[0] != 1.0 ||
		     top->cid.FontMatrix.array[1] != 0.0 ||
		     top->cid.FontMatrix.array[2] != 0.0 ||
		     top->cid.FontMatrix.array[3] != 1.0 ||
		     top->cid.FontMatrix.array[4] != 0.0 ||
		     top->cid.FontMatrix.array[5] != 0.0)) {
			saveRealArrayOp(dst, 6, top->cid.FontMatrix.array, cff_FontMatrix);
		}
	}
	else {
		/* PaintType */
		if (font0->PaintType != cff_DFLT_PaintType) {
			cfwDictSaveIntOp(dst, font0->PaintType, cff_PaintType);
		}

		/* FontMatrix */
		saveFontMatrix(dst, &font0->FontMatrix);
	}

	/* UniqueID */
	if (top->UniqueID != ABF_UNSET_INT) {
		cfwDictSaveIntOp(dst, top->UniqueID, cff_UniqueID);
	}

	/* StrokeWidth */
	if (top->StrokeWidth != cff_DFLT_StrokeWidth) {
		cfwDictSaveRealOp(dst, top->StrokeWidth, cff_StrokeWidth);
	}

	if (top->sup.flags & ABF_CID_FONT) {
		/* CIDFontVersion */
		if (top->cid.CIDFontVersion != cff_DFLT_CIDFontVersion) {
			cfwDictSaveRealOp(dst, top->cid.CIDFontVersion, cff_CIDFontVersion);
		}

		/* CIDFontRevision */
		if (top->cid.CIDFontRevision != cff_DFLT_CIDFontRevision) {
			cfwDictSaveIntOp(dst, top->cid.CIDFontRevision,
			                 cff_CIDFontRevision);
		}

		/* CIDCount */
		if (top->cid.CIDCount != cff_DFLT_CIDCount) {
			cfwDictSaveIntOp(dst, top->cid.CIDCount, cff_CIDCount);
		}

		/* UIDBase */
		if (top->cid.UIDBase != ABF_UNSET_INT) {
			cfwDictSaveIntOp(dst, top->cid.UIDBase, cff_UIDBase);
		}
	}

	/* XUID */
	if (top->XUID.cnt != ABF_EMPTY_ARRAY) {
		saveIntArrayOp(dst, top->XUID.cnt, top->XUID.array, cff_XUID);
	}
}

/* Fill CFF font dict. */
void cfwDictFillFont(cfwCtx g, DICT *dst, abfFontDict *src) {
	dst->cnt = 0;

	/* FontName */
	if (src->FontName.impl != SRI_UNDEF) {
		saveStringOp(g, dst, (SRI)src->FontName.impl, cff_FontName);
	}

	/* PaintType */
	if (src->PaintType != cff_DFLT_PaintType) {
		cfwDictSaveIntOp(dst, src->PaintType, cff_PaintType);
	}

	/* FontMatrix */
	saveFontMatrix(dst, &src->FontMatrix);
}

/* Delete duplicate family array. */
static void delFamilyDups(long bluescnt, float *blues,
                          long *familycnt, float *family, int other) {
	long i;
	long j;
	long iBeg;
	int matched;

	if (bluescnt == 0 || bluescnt != *familycnt) {
		return;
	}

	if (other) {
		iBeg = 0;
		matched = 0;
	}
	else {
		/* Match bottom zones */
		if (blues[0] != family[0] ||
		    blues[1] != family[1]) {
			return;
		}
		iBeg = 2;
		matched = 2;
	}

	for (i = iBeg; i < *familycnt; i += 2) {
		/* Get family pair for comparison */
		float lo = family[i + 0];
		float hi = family[i + 1];

		/* Search blues for a match */
		for (j = iBeg; j < bluescnt; j += 2) {
			if (blues[j + 0] == lo && blues[j + 1] == hi) {
				/* Match found; count it */
				matched += 2;
				break;
			}
		}
	}

	if (matched == bluescnt) {
		*familycnt = 0;
	}
}

/* Fill CFF FDInfo DICTs. */
void cfwDictFillPrivate(cfwCtx g, DICT *dst, abfPrivateDict *src) {
	dst->cnt = 0;

	if (!(g->flags & CFW_NO_FAMILY_OPT)) {
		/* Delete duplicates from the Family arrays */
		delFamilyDups(src->BlueValues.cnt, src->BlueValues.array,
		              &src->FamilyBlues.cnt, src->FamilyBlues.array, 0);
		delFamilyDups(src->OtherBlues.cnt, src->OtherBlues.array,
		              &src->FamilyOtherBlues.cnt,
		              src->FamilyOtherBlues.array, 1);
	}

	/* Delete StemSnap arrays that match corresponding Std value */
	if (src->StemSnapH.cnt == 1 && src->StemSnapH.array[0] == src->StdHW) {
		src->StemSnapH.cnt = ABF_EMPTY_ARRAY;
	}
	if (src->StemSnapV.cnt == 1 && src->StemSnapV.array[0] == src->StdVW) {
		src->StemSnapV.cnt = ABF_EMPTY_ARRAY;
	}

	/* BlueValues */
	if (src->BlueValues.cnt != ABF_EMPTY_ARRAY) {
		saveRealDeltaOp(dst, src->BlueValues.cnt, src->BlueValues.array,
		                cff_BlueValues);
	}

	/* OtherBlues */
	if (src->OtherBlues.cnt != ABF_EMPTY_ARRAY) {
		saveRealDeltaOp(dst, src->OtherBlues.cnt, src->OtherBlues.array,
		                cff_OtherBlues);
	}

	/* FamilyBlues */
	if (src->FamilyBlues.cnt != ABF_EMPTY_ARRAY) {
		saveRealDeltaOp(dst, src->FamilyBlues.cnt, src->FamilyBlues.array,
		                cff_FamilyBlues);
	}

	/* FamilyOtherBlues */
	if (src->FamilyOtherBlues.cnt != ABF_EMPTY_ARRAY) {
		saveRealDeltaOp(dst, src->FamilyOtherBlues.cnt,
		                src->FamilyOtherBlues.array, cff_FamilyOtherBlues);
	}

	/* BlueScale */
	if (src->BlueScale != (float)cff_DFLT_BlueScale) {
		cfwDictSaveRealOp(dst, src->BlueScale, cff_BlueScale);
	}

	/* BlueShift */
	if (src->BlueShift != cff_DFLT_BlueShift) {
		cfwDictSaveRealOp(dst, src->BlueShift, cff_BlueShift);
	}

	/* BlueFuzz */
	if (src->BlueFuzz != cff_DFLT_BlueFuzz) {
		cfwDictSaveRealOp(dst, src->BlueFuzz, cff_BlueFuzz);
	}

	/* StdHW */
	if (src->StdHW != ABF_UNSET_REAL) {
		cfwDictSaveRealOp(dst, src->StdHW, cff_StdHW);
	}

	/* StdVW */
	if (src->StdVW != ABF_UNSET_REAL) {
		cfwDictSaveRealOp(dst, src->StdVW, cff_StdVW);
	}

	/* StemSnapH */
	if (src->StemSnapH.cnt != ABF_EMPTY_ARRAY) {
		saveRealDeltaOp(dst, src->StemSnapH.cnt, src->StemSnapH.array,
		                cff_StemSnapH);
	}

	/* StemSnapV */
	if (src->StemSnapV.cnt != ABF_EMPTY_ARRAY) {
		saveRealDeltaOp(dst, src->StemSnapV.cnt, src->StemSnapV.array,
		                cff_StemSnapV);
	}
	/* ForceBold */
	if (src->ForceBold != cff_DFLT_ForceBold) {
		cfwDictSaveIntOp(dst, src->ForceBold, cff_ForceBold);
	}

	/* LanguageGroup */
	if (src->LanguageGroup != cff_DFLT_LanguageGroup) {
		cfwDictSaveIntOp(dst, src->LanguageGroup, cff_LanguageGroup);
	}

	/* ExpansionFactor */
	if (src->ExpansionFactor != (float)cff_DFLT_ExpansionFactor) {
		cfwDictSaveRealOp(dst, src->ExpansionFactor, cff_ExpansionFactor);
	}

	/* initialRandomSeed */
	if (src->initialRandomSeed != cff_DFLT_initialRandomSeed) {
		cfwDictSaveRealOp(dst, src->initialRandomSeed, cff_initialRandomSeed);
	}
}

/* ----------------------------- Debug Support ----------------------------- */

#if CFW_DEBUG

static void dbdict(DICT *dict) {
	static char *opname[32] = {
		/*  0 */ "version",
		/*  1 */ "Notice",
		/*  2 */ "FullName",
		/*  3 */ "FamilyName",
		/*  4 */ "Weight",
		/*  5 */ "FontBBox",
		/*  6 */ "BlueValues",
		/*  7 */ "OtherBlues",
		/*  8 */ "FamilyBlues",
		/*  9 */ "FamilyOtherBlues",
		/* 10 */ "StdHW",
		/* 11 */ "StdVW",
		/* 12 */ "escape",
		/* 13 */ "UniqueID",
		/* 14 */ "XUID",
		/* 15 */ "charset",
		/* 16 */ "Encoding",
		/* 17 */ "CharStrings",
		/* 18 */ "Private",
		/* 19 */ "Subrs",
		/* 20 */ "defaultWidthX",
		/* 21 */ "nominalWidthX",
		/* 22 */ "reserved22",
		/* 23 */ "reserved23",
		/* 24 */ "reserved24",
		/* 25 */ "reserved25",
		/* 26 */ "reserved26",
		/* 27 */ "reserved27",
		/* 28 */ "shortint",
		/* 29 */ "longint",
		/* 30 */ "BCD",
		/* 31 */ "reserved31",
	};
	static char *escopname[] = {
		/*  0 */ "Copyright",
		/*  1 */ "isFixedPitch",
		/*  2 */ "ItalicAngle",
		/*  3 */ "UnderlinePosition",
		/*  4 */ "UnderlineThickness",
		/*  5 */ "PaintType",
		/*  6 */ "CharstringType",
		/*  7 */ "FontMatrix",
		/*  8 */ "StrokeWidth",
		/*  9 */ "BlueScale",
		/* 10 */ "BlueShift",
		/* 11 */ "BlueFuzz",
		/* 12 */ "StemSnapH",
		/* 13 */ "StemSnapV",
		/* 14 */ "ForceBold",
		/* 15 */ "reservedESC15",
		/* 16 */ "lenIV",
		/* 17 */ "LanguageGroup",
		/* 18 */ "ExpansionFactor",
		/* 19 */ "initialRandomSeed",
		/* 20 */ "SyntheticBase",
		/* 21 */ "PostScript",
		/* 22 */ "BaseFontName",
		/* 23 */ "BaseFontBlend",
		/* 24 */ "reservedESC24",
		/* 25 */ "reservedESC25",
		/* 26 */ "reservedESC26",
		/* 27 */ "reservedESC27",
		/* 28 */ "reservedESC28",
		/* 29 */ "reservedESC29",
		/* 30 */ "ROS",
		/* 31 */ "CIDFontVersion",
		/* 32 */ "CIDFontRevision",
		/* 33 */ "CIDFontType",
		/* 34 */ "CIDCount",
		/* 35 */ "UIDBase",
		/* 36 */ "FDArray",
		/* 37 */ "FDIndex",
		/* 38 */ "FontName",
		/* 39 */ "Chameleon",
	};
	int i;
	unsigned char *buf = (unsigned char *)dict->array;

	printf("--- dict\n");
	i = 0;
	while (i < dict->cnt) {
		int op = buf[i];
		switch (op) {
			case cff_version:
			case cff_Notice:
			case cff_FullName:
			case cff_FamilyName:
			case cff_Weight:
			case cff_FontBBox:
			case cff_BlueValues:
			case cff_OtherBlues:
			case cff_FamilyBlues:
			case cff_FamilyOtherBlues:
			case cff_StdHW:
			case cff_StdVW:
			case cff_UniqueID:
			case cff_XUID:
			case cff_charset:
			case cff_Encoding:
			case cff_CharStrings:
			case cff_Private:
			case cff_Subrs:
			case cff_defaultWidthX:
			case cff_nominalWidthX:
			case cff_reserved22:
			case cff_reserved23:
			case cff_reserved24:
			case cff_reserved25:
			case cff_reserved26:
			case cff_reserved27:
			case cff_Blend:
				printf("%s ", opname[op]);
				i++;
				break;

			case cff_escape:
			{
				/* Process escaped operator */
				int escop = buf[i + 1];
				if (escop > (long)ARRAY_LEN(escopname) - 1) {
					printf("? ");
				}
				else {
					printf("%s ", escopname[escop]);
				}
				i += 2;
				break;
			}

			case cff_shortint:
				/* 2 byte number */
				printf("%d ", buf[i + 1] << 8 | buf[i + 2]);
				i += 3;
				break;

			case cff_longint:
				/* 5 byte number */
				printf("%ld ", ((long)buf[i + 1] << 24 | (long)buf[i + 2] << 16 |
				                (long)buf[i + 3] << 8 | (long)buf[i + 4]));
				i += 5;
				break;

			case cff_BCD:
			{
				int count = 0;
				int byte = 0; /* Suppress optimizer warning */
				for (;; ) {
					int nibble;

					if (count++ & 1) {
						nibble = byte & 0xf;
					}
					else {
						byte = buf[++i];
						nibble = byte >> 4;
					}
					if (nibble == 0xf) {
						break;
					}

					printf("%c", "0123456789.EE?-?"[nibble]);
					if (nibble == 0xc) {
						printf("-");
					}
				}
				printf(" ");
				i++;
			}
			break;

			case 247:
			case 248:
			case 249:
			case 250:
				/* +ve 2 byte number */
				printf("%d ", 108 + 256 * (buf[i] - 247) + buf[i + 1]);
				i += 2;
				break;

			case 251:
			case 252:
			case 253:
			case 254:
				/* -ve 2 byte number */
				printf("%d ", -108 - 256 * (buf[i] - 251) - buf[i + 1]);
				i += 2;
				break;

			case 255:
				printf("? ");
				i++;
				break;

			default:
				/* 1 byte number */
				printf("%d ", buf[i] - 139);
				i++;
				break;
		}
	}
	printf("\n");
}

/* This function just serves to suppress annoying "defined but not used"
   compiler messages when debugging */
static void CTL_CDECL dbuse(int arg, ...) {
	dbuse(0, dbdict);
}

#endif /* CFW_DEBUG */